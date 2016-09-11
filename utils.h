#ifndef UTILS_H_
#define UTILS_H_

#include "opencv2/opencv.hpp"
#include <vector>
#include <iostream>

using cv::Point2d;
using cv::Point2f;
using cv::Mat;

using std::vector;
using std::string;
using std::cout;
using std::cerr;
using std::endl;

static const uint IMAGE_PATCH_WIDTH = 512;
static const uint IMAGE_PATCH_HEIGHT = 512;
static const uint CAMERA_WIDTH = 2304;
static const uint CAMERA_HEIGHT = 1536;

void onMouseCallback(int event, int x, int y, int flags, void* parameter);
vector<Point2f> selectCornerPointsFromCamera();
Mat clipSquareImage(const Mat &snapshot, const vector<Point2f> &cornerPoints);
vector<Mat> splitIntoSmallImage(const Mat &squareImage);

#endif