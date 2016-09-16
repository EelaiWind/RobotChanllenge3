#include "utils.h"


void onMouseCallback(int event, int x, int y, int flags, void* parameter)
{
    if(event == CV_EVENT_LBUTTONDOWN && parameter != NULL){
        vector<Point2f> *cornerPoints = (vector<Point2f> *)parameter;
        if ( cornerPoints->size() < 4){
            cornerPoints->push_back(Point2d(x, y));
            cout << "Select point " << cornerPoints->size() << " = (" << x << ", " << y << ")" << endl;
        }

    }
}

vector<Point2f> selectCornerPointsFromCamera()
{
    static const string WINDOW_NAME = "Camera Output";
    vector<Point2f> cornerPoints;
    cv::VideoCapture cameraHandle(0);
    if (!cameraHandle.isOpened()){
        cerr << "Cannot open camera" << endl;
        exit(1);
    }
    cameraHandle.set(CV_CAP_PROP_FRAME_WIDTH, CAMERA_WIDTH);
    cameraHandle.set(CV_CAP_PROP_FRAME_HEIGHT, CAMERA_HEIGHT);

    cv::namedWindow(WINDOW_NAME, 1);
    //cv::resizeWindow(WINDOW_NAME, CAMERA_WIDTH*0.5, CAMERA_HEIGHT*0.5);
    cv::setMouseCallback(WINDOW_NAME, onMouseCallback, &cornerPoints);

    while(true){
        Mat snapshot;
        cameraHandle >> snapshot;
        for (uint i = 0; i < cornerPoints.size(); ++i){
            cv::circle(snapshot, cornerPoints.at(i), 3, cv::Scalar(0, 0, 255), -1);
        }
        imshow(WINDOW_NAME, snapshot);
        char c = cv::waitKey(100);
        if (c == 'r' && !cornerPoints.empty()){
            cornerPoints.pop_back();
            cout << cornerPoints.size() << " points left" << endl;
        }
        else if ( c == 'q' && cornerPoints.size() == 4){
            cv::setMouseCallback(WINDOW_NAME, NULL, NULL);
            cout << "Finish setting corner points" << endl;
            break;
        }
    }
    cv::destroyWindow(WINDOW_NAME);
    cameraHandle.release();
    return cornerPoints;
}

Mat clipSquareImage(const Mat &snapshot, const vector<Point2f> &cornerPoints)
{
    const Point2f cameraCoordinate[] = { cornerPoints.at(0), cornerPoints.at(1), cornerPoints.at(2), cornerPoints.at(3) };
    const Point2f patchCoordinate[] = { Point2f(0,0), Point2f(IMAGE_PATCH_WIDTH-1, 0), Point2f(IMAGE_PATCH_WIDTH-1, IMAGE_PATCH_HEIGHT-1), Point2f(0, IMAGE_PATCH_HEIGHT-1) };


    Mat squareImage = Mat(IMAGE_PATCH_HEIGHT, IMAGE_PATCH_WIDTH, snapshot.type());
    const Mat perspectiveTransformMatrix = cv::getPerspectiveTransform(cameraCoordinate, patchCoordinate);
    warpPerspective(snapshot, squareImage, perspectiveTransformMatrix, squareImage.size(), cv::INTER_LINEAR);

    return squareImage;
}

vector<Mat> splitIntoSmallImage(const Mat &squareImage)
{
    static const uint SMALL_IMAGE_SIZE = 32;
    vector<Mat> smallImage;
    smallImage.reserve(256);
    for(uint i = 0; i < IMAGE_PATCH_HEIGHT; i+=SMALL_IMAGE_SIZE){
        for(uint j = 0; j < IMAGE_PATCH_WIDTH; j+=SMALL_IMAGE_SIZE){
            smallImage.push_back( Mat(squareImage, cv::Rect(i, j, SMALL_IMAGE_SIZE, SMALL_IMAGE_SIZE)));
        }
    }
    return smallImage;
}

TcpConnection::TcpConnection(const string ip, const short int port)
{
    m_clientSocket = socket(AF_INET,SOCK_STREAM,0);
    if(m_clientSocket < 0){
        cerr << "socket creation failed : " << strerror(errno) << endl;
        exit(-1);
    }
    cout << "socket create successfully" << endl;

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family=AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(ip.c_str());

    if(connect(m_clientSocket,(struct sockaddr *) &serverAddress, sizeof(struct sockaddr_in))<0){
        cerr << "Connect failed : " << strerror(errno) << endl;
        exit(-1);
    }
    cout << "Connect successfully" << endl;
}

TcpConnection::~TcpConnection()
{
    close(m_clientSocket);
}

void TcpConnection::readAngles(double *angles)
{
    char readBuffer[BUFFER_SIZE];
    recv(m_clientSocket, readBuffer , BUFFER_SIZE, 0);
    char* points = strtok(readBuffer, ",");
    for(int i = 0; i < 8; i++) {
        angles[i] = atof(points);
        points = strtok(NULL, ",");
    }
}

void TcpConnection::sendAngle(double x, double y)
{
    std::ostringstream ss;
    ss<< x << "," << y << ",";
    char sendBuffer[BUFFER_SIZE];
    strncpy(sendBuffer, ss.str().c_str(), sizeof(sendBuffer));
    sendBuffer[BUFFER_SIZE-1]=0;

    if(write(m_clientSocket, sendBuffer, strlen(sendBuffer))==-1){
        cerr << "send angle error! : " << strerror(errno) << endl;
        exit(-1);
    }
    cout << "[Client] send: " << sendBuffer << endl;
}

Point2d calculatePerspectivePoint(const Mat &perspectivTransformMatrix, const Point2d &sourcePoint)
{
    Mat sourcePointMatrix(3,1, CV_64FC1);
    sourcePointMatrix.at<double>(0,0) = sourcePoint.x;
    sourcePointMatrix.at<double>(1,0) = sourcePoint.y;
    sourcePointMatrix.at<double>(2,0) = 1;

    const Mat targetPointMatrix = perspectivTransformMatrix * sourcePointMatrix;
    const double targetX = targetPointMatrix.at<double>(0,0) / targetPointMatrix.at<double>(2,0);
    const double targetY = targetPointMatrix.at<double>(1,0) / targetPointMatrix.at<double>(2,0);
    return Point2d(targetX, targetY);
}

Point2d calculatePositionFromAngle(const double verticalDistance, const double angleX, const double angleY)
{
    return Point2d(verticalDistance*tan(angleX * PI / 180), verticalDistance / cos(angleX * PI / 180) * tan(angleY * PI / 180));
}

std::pair<double, double> calculateAngleFromPosition(const double verticalDistance, const Point2d position){
    const double angleX = atan2(position.x, verticalDistance) * 180 / PI;
    const double angleY = atan2(position.y, verticalDistance / cos(angleX * PI / 180)) * 180 / PI;
    return std::make_pair(angleX, angleY);
}
