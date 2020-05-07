#include <iostream>
#include <iterator>
#include <algorithm>
#include <string>
#include <vector>
#include <filesystem>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

int main()
{
	cv::Mat image;
	image = cv::imread("/home/inaki/Desktop/cv/cropedSticks/I_2_20200430134218_0001-701260480-25x15x361-1.0.12-P0-0-0.tif");
	cv::namedWindow("Image", cv::WINDOW_AUTOSIZE);
	cv::pyrDown(image, image);
	cv::imshow("Image", image);

	cv::waitKey();
	return 0;
}