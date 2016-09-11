#ifndef CLASSIFIER_H
#define CLASSIFIER_H

#include <opencv2/opencv.hpp>
#include <caffe/caffe.hpp>
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

using std::cout;
using std::cerr;
using std::endl;
using std::pair;
using std::vector;
using std::string;
using cv::Mat;
using caffe::Caffe;

/* Pair (label, confidence) representing a prediction. */
typedef std::pair<uint, float> Prediction;

class Classifier {
public:
    Classifier(const string& modelFile, const string& weightFile, const string& meanFile, uint labelCount);
    Prediction classify(const Mat& img);

private:
    void loadNet(const string& modelFile, const string& weightFile, uint labelCount);
    void loadMeanFile(const string& meanFile);
    vector<float> predict(const Mat& img);
    void wrapInputLayer(vector<Mat>* inputChannels);
    void preprocess(const Mat& img, vector<Mat>* inputChannels);
    void cropImage(const Mat& sourceImage, Mat &croppedImage, uint croppedSize);
private:
    static const cv::Size IMAGE_SIZE;
    caffe::shared_ptr<caffe::Net<float> > m_net;
    cv::Size m_inputGeometry;
    uint m_numChannels;
    Mat m_mean;
};

#endif

