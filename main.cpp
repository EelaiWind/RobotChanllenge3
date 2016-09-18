#include "classifier.h"
#include "utils.h"
#include <iostream>
#include <sstream>

using std::cin;
using std::cerr;
using std::cout;
using std::endl;
using std::vector;
using std::string;
using cv::Point2f;
using cv::Mat;

static const uint AUTOMOBILE_LABEL = 1;
static const uint CAT_LABEL = 2;
static const uint TRUCK_LABEL = 3;
static const string labels[] = {"other", "automobile", "cat", "truck"};

int main(int argc, char * argv[])
{
    if (argc < 4){
        cerr << "Please set network prototxt, caffemodel h5 and image mean file" << endl;
        exit(-1);
    }
    const string modelFile = argv[1];
    const string weightFile = argv[2];
    const string meanFile = argv[3];

    double laserAngels[8];
    TcpConnection tcpConnection;

    Classifier classifier(modelFile, weightFile, meanFile, 4);

    vector<Point2f> cornerPoints = selectCornerPointsFromCamera();
    cout << "Reading Angles ..." << endl;
    tcpConnection.readAngles(laserAngels);
    Point2f laserCornerPoints[4];
    for (uint i = 0; i < 4; ++i){
        laserCornerPoints[i] = calculatePositionFromAngle(LASER_VERTICAL_DISTANCE, laserAngels[i*2], laserAngels[i*2+1]);
    }
    Mat perspectiveMatrix = cv::getPerspectiveTransform(originalCornerPoints, laserCornerPoints);
    cv::namedWindow("Snapshot", cv::WINDOW_NORMAL);
    cout << "Start Challenge 3" << endl;
    while(true){
        Mat snapshot = readImageFromCamera();
        Mat squareImage = clipSquareImage(snapshot, cornerPoints);
        cv::imshow("Snapshot", squareImage);
        cv::waitKey(500);
        cv::destroyWindow("Snapshot");

        vector<Mat> smallImages = splitIntoSmallImage(squareImage);
        for (uint i = 0; i < smallImages.size(); ++i){
            std::ostringstream oss;
            oss << "cut_result/image_" << i << ".jpg";
            cv::imwrite(oss.str(), smallImages.at(i));
            Prediction prediction = classifier.classify(smallImages.at(i));
            if (prediction.first == TRUCK_LABEL || prediction.first == CAT_LABEL || prediction.first == TRUCK_LABEL){
                Point2d targetIndex((i/16)+0.5, (i%16)+0.5);
                Point2d targetPoint = calculatePerspectivePoint(perspectiveMatrix, targetIndex);
                std::pair<double, double> targetAngle = calculateAngleFromPosition(LASER_VERTICAL_DISTANCE, targetPoint);
                tcpConnection.sendAngle(targetAngle.first, targetAngle.second);
                cout << "image(" << targetIndex.x << ", " << targetIndex.y << ") is a " << labels[prediction.first] << " (probability = " << prediction.second << ")" << endl;
            }
        }

        cout << "Classification Finish" << endl;
        cout << "Enter 'q' to quit or other caharater to start again = ";
        char command;
        cin >> command;
        if (command == 'q'){
            break;
        }
    }

    return 0;
}
