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

bool compareCadidates(std::pair<uint, float> object1,std::pair<uint, float> object2)
{
    if (object1.second >= object2.second){
        return true;
    }
    else{
        return false;
    }
}

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

    cout << "Load Caffe model...";
    Classifier classifier(modelFile, weightFile, meanFile, 4);
    cout << "[finish]" << endl;
    cout << "Select corner points..." << endl;
    vector<Point2f> cornerPoints = selectCornerPointsFromCamera();
    cout << "[Finish]" << endl;
    cout << "Reading Angles ...";
    tcpConnection.readAngles(laserAngels);
    cout << "[Finish]" << endl;
    Point2f laserCornerPoints[4];
    for (uint i = 0; i < 4; ++i){
        laserCornerPoints[i] = calculatePositionFromAngle(LASER_VERTICAL_DISTANCE, laserAngels[i*2], laserAngels[i*2+1]);
    }
    Mat perspectiveMatrix = cv::getPerspectiveTransform(originalCornerPoints, laserCornerPoints);


    cout << "Enter 'y' to start challenge : ";
    while(true){
		string reply;
        cin >> reply;
        if (reply == "y" || reply == "Y"){
            break;
        }
    }
    cout << "Start Challenge 3..." << endl;

    vector<std::pair<uint, float> > targetCandidates;
    Mat snapshot = readImageFromCamera();
    Mat squareImage = clipSquareImage(snapshot, cornerPoints);
    cv::namedWindow("Snapshot", cv::WINDOW_NORMAL);
    cv::imshow("Snapshot", squareImage);
    cv::waitKey(500);
    cv::destroyWindow("Snapshot");

    vector<Mat> smallImages = splitIntoSmallImage(squareImage);
    for (uint i = 0; i < smallImages.size(); ++i){
        std::ostringstream oss;
        oss << "cut_result/image_" << i << ".jpg";
        cv::imwrite(oss.str(), smallImages.at(i));
        Prediction prediction = classifier.classify(smallImages.at(i));
        if (prediction.first == AUTOMOBILE_LABEL || prediction.first == CAT_LABEL || prediction.first == TRUCK_LABEL){
            Point2d targetIndex((i/16)+0.5, (i%16)+0.5);
            cout << "Image(" << targetIndex.x << ", " << targetIndex.y << ") is a " << labels[prediction.first] << " (probability = " << prediction.second << ")" << endl;
            targetCandidates.push_back(std::make_pair(i, prediction.second));
        }
    }

    std::sort(targetCandidates.begin(), targetCandidates.end(), compareCadidates);
    uint minIndex = std::min((uint)10, targetCandidates.size());
    for (uint i =0; i < minIndex; ++i){
        uint index = targetCandidates.at(i).first;
        Point2d targetIndex((index/16)+0.5, (index%16)+0.5);
        Point2d targetPoint = calculatePerspectivePoint(perspectiveMatrix, targetIndex);
        std::pair<double, double> targetAngle = calculateAngleFromPosition(LASER_VERTICAL_DISTANCE, targetPoint);
        cout << "Target(" << targetIndex.x << ", " << targetIndex.y << ") wth probability = " << targetCandidates.at(i).second << endl;
        tcpConnection.sendAngle(targetAngle.first, targetAngle.second);
    }

    return 0;
}
