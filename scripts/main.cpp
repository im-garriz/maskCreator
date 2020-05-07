#include <iostream>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "utils.h"

#define DISPLAY_SIZE 700
#define W_NAME "Mask Creator"


void display_img(cv::Mat img, cv::Mat& display_image, cv::Mat& imagePlusControls);
void setup(cv::Mat& imagePlusControls, cv::Mat& display_image);

void nMaskChanged(int pos, void* param);
void channelChanged(int pos, void* param);
void thresholdValueChanged(int pos, void* param);

int main()
{
	cv::Mat image;
	cv::Mat display_image;

	cv::Mat imagePlusControls(DISPLAY_SIZE, 0.3f*DISPLAY_SIZE, CV_8UC3);
	

	setup(imagePlusControls, display_image);
	image = cv::imread("/home/inaki/Desktop/cv/cropedSticks/I_2_20200430134218_0001-701260480-25x15x361-1.0.12-P0-0-0.tif");

	
	display_img(image, display_image, imagePlusControls);

	return 0;
}

void display_img(cv::Mat img, cv::Mat& display_image, cv::Mat& imagePlusControls)
{
	cv::resize(img, display_image, cv::Size(DISPLAY_SIZE, DISPLAY_SIZE));
	display_image.copyTo(imagePlusControls.colRange(0, DISPLAY_SIZE));
	cv::imshow(W_NAME, imagePlusControls);
	cv::waitKey();
}

void setup(cv::Mat& imagePlusControls, cv::Mat& display_image)
{
	cv::namedWindow(W_NAME, cv::WINDOW_AUTOSIZE);

	imagePlusControls = cv::Mat::zeros(DISPLAY_SIZE, static_cast<int>(1.5f*DISPLAY_SIZE), CV_8UC3);
	//imagePlusControls.colRange(0, DISPLAY_SIZE) = display_img;

	int slider1pos = 0;
	int nOfMasks = 3;
	cv::createTrackbar(
		"Mask label",
		W_NAME,
		&slider1pos,
		nOfMasks-1,
		nMaskChanged);

	int slider2pos = 0;
	int nOfChannels = 7;
	cv::createTrackbar(
		"Channel",
		W_NAME,
		&slider2pos,
		nOfChannels-1,
		channelChanged);

	int slider3pos = 0;
	cv::createTrackbar(
		"Threshold",
		W_NAME,
		&slider2pos,
		255,
		thresholdValueChanged);
}

void nMaskChanged(int pos, void* param)
{
	std::cout << "N masks changes\n";
}

void channelChanged(int pos, void* param)
{
	std::cout << "Channel changes\n";
}

void thresholdValueChanged(int pos, void* param)
{
	std::cout << "Therhold value changes\n";
}