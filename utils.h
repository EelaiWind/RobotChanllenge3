#ifndef UTILS_H_
#define UTILS_H_

#include "opencv2/opencv.hpp"
#include <vector>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

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

static const Point2f UPPER_LEFT_POINT(0, 0);
static const Point2f UPPER_RIGHT_POINT(16, 0);
static const Point2f BOTTOM_RIGHT_POINT(16, 16);
static const Point2f BOTTOM_LEFT_UPPER_POINT(0, 16);
static const Point2f originalCornerPoints[] = {UPPER_LEFT_POINT, UPPER_RIGHT_POINT, BOTTOM_RIGHT_POINT, BOTTOM_LEFT_UPPER_POINT};
static const double LASER_VERTICAL_DISTANCE = 100;
static const double PI = 3.141592653589793;

void onMouseCallback(int event, int x, int y, int flags, void* parameter);
vector<Point2f> selectCornerPointsFromCamera();
Mat clipSquareImage(const Mat &snapshot, const vector<Point2f> &cornerPoints);
vector<Mat> splitIntoSmallImage(const Mat &squareImage);

class TcpConnection{
public:
    TcpConnection(string ip = "192.168.0.100", short int port = 8000);
    ~TcpConnection();
    void readAngles(double *angles);
    void sendAngle(double x, double y);
private:
    static const  uint BUFFER_SIZE = 1024;
    int m_clientSocket;
};

Point2d calculatePerspectivePoint(const Mat &perspectivTransformMatrix, const Point2d &sourcePoint);
Point2d calculatePositionFromAngle(const double verticalDistance = LASER_VERTICAL_DISTANCE, const double angleX, const double angleY);
std::pair<double, double> calculateAngleFromPositio(const double LASER_VERTICAL_DISTANCE, const Point2d position);

#endif