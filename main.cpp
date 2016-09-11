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
static const uint CAT_LABEL = 3;
static const uint TRUCK_LABEL = 9;
static const string labels[] = {"airplane", "automobile", "bird", "cat", "dog", "deer", "frog", "hourse", "ship", "truck"};

int main(int argc, char * argv[])
{
    if (argc < 4){
        cerr << "Please set network prototxt, caffemodel h5 and image mean file" << endl;
        exit(0);
    }
    const string modelFile = argv[1];
    const string weightFile = argv[2];
    const string meanFile = argv[3];

	Classifier classifier(modelFile, weightFile, meanFile, 10);
	
    vector<Point2f> cornerPoints = selectCornerPointsFromCamera();
	cornerPoints.reserve(4);
    
	cv::namedWindow("Camera Output", 1);
    while(true){
		Mat snapshot;
		cv::VideoCapture cameraHandle(0);
		if (!cameraHandle.isOpened()){
			cerr << "Cannot open camera" << endl;;
			exit(1);
		}
		cameraHandle.set(CV_CAP_PROP_FRAME_WIDTH, CAMERA_WIDTH);
		cameraHandle.set(CV_CAP_PROP_FRAME_HEIGHT, CAMERA_HEIGHT);
		cameraHandle.read(snapshot);
		cameraHandle.release();
		Mat squareImage = clipSquareImage(snapshot, cornerPoints);
		cv::imshow("Camera Output", squareImage);
		cv::waitKey(500);
		cv::destroyAllWindows();

        vector<Mat> smallImages = splitIntoSmallImage(squareImage);
        for (uint i = 0; i < smallImages.size(); ++i){
			std::ostringstream oss;
			oss << "image_" << i << ".jpg";
			cv::imwrite(oss.str(), smallImages.at(i));
            Prediction prediction = classifier.classify(smallImages.at(i));
            if (prediction.first == TRUCK_LABEL || prediction.first == CAT_LABEL || prediction.first == TRUCK_LABEL){
				// === TCP ===


				// ===========
                cout << "image #" << i << " is a " << labels[prediction.first] << " (probability = " << prediction.second << ")" << endl;
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
