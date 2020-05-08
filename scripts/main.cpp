#include <iostream>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "utils.h"

// Size to which the image will be resized so as to be displayed. This size
// should be multiple of the real image dimmentions, so as to reconstruct the
// mask from the resized image with more precision
#define DISPLAY_SIZE_H 800
#define DISPLAY_SIZE_W 800
// Name of the window
#define W_NAME "Mask Creator"
// To add a new button just add its label to buttonsNames below and set the macro
// N_OF_BUTTONS below to the new quantity of buttons
#define N_OF_BUTTONS 8

const utils::stringvec buttonsNames = {
	"VIEW MASK",
	"APPLY THRESHOLD",
	"DELETE",
	"ADD",
	"GO BACK",
	"NEXT IMAGE",
	"SAVE MASK",
	"EXIT",};

// Struct that holds all the features we need to
// create a button
struct rectanglesButtons
{
	cv::Rect _rect;
	std::string _name;
	cv::Scalar _color;
};

// Struct that holds data that we might want to use in mouse and 
// slider callbacks
struct data
{
	cv::Mat read_image;
	cv::Mat imagePlusControls;
	cv::Mat mask;
	cv::Mat previous_mask;
	std::vector<cv::Mat> channels;
	int mask_id;
	rectanglesButtons* buttons;
};

void readImage(const std::string& path, data& globalData);
void display_img(cv::Mat img, cv::Mat& imagePlusControls);
void setup(data& globalData);
void setupButtons(data& globalData);
void displayButton(rectanglesButtons rectangle, cv::Mat& imagePlusControls);

void nMaskChanged(int pos, void* param);
void channelChanged(int pos, void* param);
void thresholdValueChanged(int pos, void* param);
void onMouseClickled(int event, int x, int y, int flags, void* userdata);

int main()
{
	//cv::Mat image;
	//cv::Mat display_image, imagePlusControls;
	data globalData;

	
	//rectanglesButtons *buttons;
	setup(globalData);
	setupButtons(globalData);
	
	////////////////////////////////
	const std::string path = "/home/inaki/Desktop/cv/cropedSticks/I_2_20200430134218_0001-701260480-25x15x361-1.0.12-P0-0-0.tif";

	readImage(path, globalData);

	display_img(globalData.read_image, globalData.imagePlusControls);

	return 0;
}

void readImage(const std::string& path, data& globalData)
{
	cv::Mat image = cv::imread(path);
	cv::Mat mask = cv::Mat::zeros(image.rows, image.cols, CV_8UC1);
	std::vector<cv::Mat> rgbChannels, cmybkChannels;
	cv::Mat aux;
	cv::cvtColor(image, aux, cv::COLOR_BGR2RGB);
	cv::split(aux, rgbChannels);

	globalData.read_image = image;
	globalData.mask = mask;
	globalData.channels = rgbChannels;
	// CMYK
}

void display_img(cv::Mat img, cv::Mat& imagePlusControls)
{
	cv::Mat display_image;
	cv::resize(img, display_image, cv::Size(DISPLAY_SIZE_H, DISPLAY_SIZE_W));
	display_image.copyTo(imagePlusControls.colRange(0, DISPLAY_SIZE_W));
	cv::imshow(W_NAME, imagePlusControls);
	cv::waitKey();
}

void setup(data& globalData)
{
	cv::namedWindow(W_NAME, cv::WINDOW_AUTOSIZE);

	globalData.imagePlusControls = cv::Mat::zeros(DISPLAY_SIZE_H, static_cast<int>(1.5f*DISPLAY_SIZE_W), CV_8UC3);
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
	int nOfChannels = 4; // Color R G B C M Y BK
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

	cv::setMouseCallback(W_NAME, onMouseClickled, (void*) &globalData);
}

void setupButtons(data& globalData)
{
	globalData.buttons = new rectanglesButtons[N_OF_BUTTONS];

	for(int i=0; i<N_OF_BUTTONS; i++)
	{
		globalData.buttons[i]._rect = cv::Rect(DISPLAY_SIZE_W, i * DISPLAY_SIZE_H/N_OF_BUTTONS,
									0.5f * DISPLAY_SIZE_W, DISPLAY_SIZE_H/N_OF_BUTTONS);
		globalData.buttons[i]._name = buttonsNames.at(i);
		if(i != 2 && i != 3)
			globalData.buttons[i]._color = cv::Scalar(255);
		else if(i == 2)
			globalData.buttons[i]._color = cv::Scalar(0,0,255);
		else
			globalData.buttons[i]._color = cv::Scalar(0,255,0);

		displayButton(globalData.buttons[i], globalData.imagePlusControls);
	}
}

void displayButton(rectanglesButtons button, cv::Mat& imagePlusControls)
{
	cv::Rect erodedRect;
	const int ero = 15;
	erodedRect.x = button._rect.x + ero;
	erodedRect.y = button._rect.y + ero;
	erodedRect.height = button._rect.height - ero;
	erodedRect.width = button._rect.width - 2*ero;

	cv::rectangle(imagePlusControls, erodedRect, button._color, 8, 2);

	cv::Size sz = cv::getTextSize(button._name, cv::FONT_HERSHEY_COMPLEX,1 , 2, NULL);
	int deltaX = (erodedRect.width - sz.width)/2;
	int deltaY = (erodedRect.height - sz.height)/2 + sz.height;
	cv::putText(imagePlusControls, button._name,
				cv::Point2d(erodedRect.x + deltaX, erodedRect.y + deltaY),
				cv::FONT_HERSHEY_COMPLEX, 1, button._color, 2);
}

void nMaskChanged(int pos, void* param)
{
	std::cout << "N masks changes" << std::endl;
}

void channelChanged(int pos, void* param)
{
	std::cout << "Channel changes" << std::endl;
}

void thresholdValueChanged(int pos, void* param)
{
	std::cout << "Therhold value changes" << std::endl;
}

void onMouseClickled(int event, int x, int y, int flags, void* userdata)
{

	data *globalData = (data*)userdata;

	if  ( event == cv::EVENT_LBUTTONDOWN )
     {
          if(x < DISPLAY_SIZE_W)
          {
          	std::cout << "Foto" << std::endl;
          }
          else
          {
          	std::cout << "Boton" << std::endl;
          	cv::Point2d p(x, y);
          	int button_idx = -1;

          	for(int i=0; i<N_OF_BUTTONS; i++)
          	{
          		if(globalData->buttons[i]._rect.contains(p))
          		{
          			button_idx = i;
          			std::cout << "Button: " << globalData->buttons[i]._name << std::endl;
          			break;
          		}
          	}

          }
     }
}