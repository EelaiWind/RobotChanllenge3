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
