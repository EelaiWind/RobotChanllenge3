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

static const uint MODEL_1_AUTOMOBILE_LABEL = 1;
static const uint MODEL_1_TRUCK_LABEL = 3;

static const uint MODEL_2_CAT_LABEL = 1;


typedef pair<Point2d, float> TargetAndProbability;

bool compareCadidates(TargetAndProbability object1, TargetAndProbability object2)
{
    return (object1.second > object2.second);
}

int main(int argc, char * argv[])
{
    if (argc < 7){
        cerr << "Please set:"<< endl;
        cerr << "VEHICLE network prototxt, VEHICLE caffemodel and VEHICLE image mean file, and" << endl;
        cerr << "CAT network prototxt, CAT affemodel and CAT image mean file" << endl;
        exit(-1);
    }

    TcpConnection tcpConnection;
    const string modelFile = argv[1];
    const string weightFile = argv[2];
    const string meanFile = argv[3];
    const string catModelFile = argv[4];
    const string catWeightFile = argv[5];
    const string catMeanFile = argv[6];

    cout << "Load Caffe model...";
    Classifier vehicleClassifier(modelFile, weightFile, meanFile, 4);
    Classifier catClassifier(catModelFile, catWeightFile, catMeanFile, 4);
    cout << "[finish]" << endl;

    double laserAngels[8];

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

Mat snapshot = readImageFromCamera();
Mat squareImage = clipSquareImage(snapshot, cornerPoints);
cv::namedWindow("Snapshot", cv::WINDOW_NORMAL);
cv::imshow("Snapshot", squareImage);
cv::waitKey(500);
cv::destroyWindow("Snapshot");

vector<Mat> smallImages = splitIntoSmallImage(squareImage);
vector<TargetAndProbability> automobileTargets;
vector<TargetAndProbability> catTargets;
vector<TargetAndProbability> truckTargets;
for (uint i = 0; i < smallImages.size(); ++i){
    std::ostringstream oss;
    oss << "cut_result/image_" << i << ".jpg";
    cv::imwrite(oss.str(), smallImages.at(i));
    Prediction vehiclePrediction = vehicleClassifier.classify(smallImages.at(i));
    Prediction catPrediction = catClassifier.classify(smallImages.at(i));
    if (vehiclePrediction.first == MODEL_1_AUTOMOBILE_LABEL){
        Point2d targetIndex(i/16, i%16);
        automobileTargets.push_back(std::make_pair(targetIndex, vehiclePrediction.second));
        cout << "Image(" << targetIndex.x << ", " << targetIndex.y << ") is a automobile (probability = " << vehiclePrediction.second << ")" << endl;
    }
    else if (vehiclePrediction.first == MODEL_1_TRUCK_LABEL){
     Point2d targetIndex(i/16, i%16);
     truckTargets.push_back(std::make_pair(targetIndex, vehiclePrediction.second));
     cout << "Image(" << targetIndex.x << ", " << targetIndex.y << ") is a truck (probability = " << vehiclePrediction.second << ")" << endl;
 }

 if(catPrediction.first == MODEL_2_CAT_LABEL){
     Point2d targetIndex(i/16, i%16);
     catTargets.push_back(std::make_pair(targetIndex, catPrediction.second));
     cout << "Image(" << targetIndex.x << ", " << targetIndex.y << ") is a cat (probability = " << catPrediction.second << ")" << endl;
 }
}

std::sort(automobileTargets.begin(), automobileTargets.end(), compareCadidates);
std::sort(catTargets.begin(), catTargets.end(), compareCadidates);
std::sort(truckTargets.begin(), truckTargets.end(), compareCadidates);

const uint CAT_PER_PATCH = 4;
const uint AUTOBOBILE_PER_PATCH = 3;
const uint TRUCK_PER_PATCH = 3;

for (uint i =0; i < automobileTargets.size() && i < AUTOBOBILE_PER_PATCH; ++i){
    Point2d targetIndex = automobileTargets.at(i).first;
    Point2d targetPoint = calculatePerspectivePoint(perspectiveMatrix, targetIndex);
    std::pair<double, double> targetAngle = calculateAngleFromPosition(LASER_VERTICAL_DISTANCE, targetPoint);
    tcpConnection.sendAngle(targetAngle.first, targetAngle.second);
    cout << "  Target(" << targetIndex.x << ", " << targetIndex.y << ") is a automobile (probability = " << automobileTargets.at(i).second << ")" << endl;
}

for (uint i =0; i < catTargets.size() && i < CAT_PER_PATCH; ++i){
    if ( i == (CAT_PER_PATCH-1) && catTargets.at(i).second < 0.9){
        break;
    }
    Point2d targetIndex = catTargets.at(i).first;
    Point2d targetPoint = calculatePerspectivePoint(perspectiveMatrix, targetIndex);
    std::pair<double, double> targetAngle = calculateAngleFromPosition(LASER_VERTICAL_DISTANCE, targetPoint);
    tcpConnection.sendAngle(targetAngle.first, targetAngle.second);
    cout << "  Target(" << targetIndex.x << ", " << targetIndex.y << ") is a cat (probability = " << catTargets.at(i).second << ")" << endl;
}

for (uint i =0; i < truckTargets.size() && i < TRUCK_PER_PATCH; ++i){
    Point2d targetIndex = truckTargets.at(i).first;
    Point2d targetPoint = calculatePerspectivePoint(perspectiveMatrix, targetIndex);
    std::pair<double, double> targetAngle = calculateAngleFromPosition(LASER_VERTICAL_DISTANCE, targetPoint);
    tcpConnection.sendAngle(targetAngle.first, targetAngle.second);
    cout << "  Target(" << targetIndex.x << ", " << targetIndex.y << ") is a truck (probability = " << truckTargets.at(i).second << ")" << endl;
}

return 0;
}
