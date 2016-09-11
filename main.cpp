#include "classifier.h"
#include "utils.h"
#include <iostream>

using std::cin;
using std::vector;
using std::string;
using cv::Point2f;
using cv::Mat;

static const uint AUTOMOBILE_LABEL = 1;
static const uint CAT_LABEL = 3;
static const uint TRUCK_LABEL = 9;

int main(int argc, char * argv[])
{
    if (argc < 4){
        LOG(INFO) << "Please set network prototxt, caffemodel h5 and image mean file";
        exit(0);
    }
    const string modelFile = argv[1];
    const string weightFile = argv[2];
    const string meanFile = argv[3];

    vector<Point2f> cornerPoints = selectCornerPointsFromCamera();

    cv::VideoCapture cameraHandle(0);
    if (!cameraHandle.isOpened()){
        LOG(ERROR) << "Cannot open camera";
        exit(1);
    }
    cameraHandle.set(CV_CAP_PROP_FRAME_WIDTH, CAMERA_WIDTH);
    cameraHandle.set(CV_CAP_PROP_FRAME_HEIGHT, CAMERA_HEIGHT);
    Classifier classifier(modelFile, weightFile, meanFile, 10);

    while(true){
        Mat snapshot;
        cameraHandle >> snapshot;
        Mat squareImage = clipSquareImage(snapshot, cornerPoints);
        vector<Mat> smallImages = splitIntoSmallImage(squareImage);
        for (uint i = 0; i < smallImages.size(); ++i){
            Prediction predition = classifier.classify(smallImages.at(i));
            if (predition.first == TRUCK_LABEL || predition.first == CAT_LABEL || predition.first == TRUCK_LABEL){
                LOG(INFO) << "image #" << i << " is a target (probability = " << predition.second << ")";
            }
        }
        // === TCP ===


        // ===========
        char command;
        cin >> command;
        if (command == 'q'){
            break;
        }
    }
    cameraHandle.release();

    return 0;
}
