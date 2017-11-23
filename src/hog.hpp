// Standard libraries
#include <sys/stat.h>
#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <cmath>

// OpenCV headers
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

// Namespaces
using namespace std;
using namespace cv;

Mat get_hogdescriptor_visu(const Mat& color_origImg, vector<float>& descriptorValues, const Size & size );