#include <iostream>
#include <thread>
#include <mutex>

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
// N_OF_BUTTONS below to the new quantity of buttons. To add it's functionablily, 
// just add the function to the switch statement in onMouseClickled function
#define N_OF_BUTTONS 9

const utils::stringvec buttonsNames = {
	"VIEW MASK",
	"APPLY THRESHOLD",
	"THRESHOLD INV",
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
	cv::Mat threshold_mask;
	utils::stringvec Images;
	std::vector<cv::Mat> channels;
	int mask_id;
	int actual_channel;
	int th_value;
	int n_of_images;
	int current_image;
	rectanglesButtons* buttons;
	bool mask_view_on;
	bool add_on;
	bool th_on;
	bool th_inv;
};

cv::Mat globalImageToDisplayThread;
std::mutex globalImageToDisplayThread_mutex;
bool killThread = false;
const std::string path = "/home/inaki/Desktop/cv/cropedSticks/";
const std::string _extension = ".tif";
std::string extension;

void readImage(const std::string& _path, data& globalData);
void display_img(data& globalData, bool displayMask);
void displayImage();
void setup(data& globalData);
void setupButtons(data& globalData);
void displayButton(rectanglesButtons rectangle, cv::Mat& imagePlusControls);

void onButtonViewMaskClicked(data& globalData);
void onButtonApllyThresholdClicked(data& globalData);
void onButtonThresholdInvClicked(data& globalData);
void onButtonDeleteClicked(data& globalData);
void onButtonAddClicked(data& globalData);
void onButtonGoBackClicked(data& globalData);
void onButtonSaveMaskClicked(data& globalData);

void nMaskChanged(int pos, void* param);
void channelChanged(int pos, void* param);
void thresholdValueChanged(int pos, void* param);
void onMouseClickled(int event, int x, int y, int flags, void* userdata);

int main()
{

	data globalData;

	setup(globalData);
	setupButtons(globalData);

	extension = _extension;
	utils::read_directory(path, globalData.Images);

	if(globalData.Images.empty())
		return 2;

	globalData.n_of_images = globalData.Images.size();
	globalData.current_image = 0;

	/*utils::stringvec::iterator it;

	for(it = globalData.Images.begin(); it != globalData.Images.end(); it++)
	{
		std::cout << *it << std::endl;
	}*/

	readImage(path + globalData.Images.at(globalData.current_image), globalData);
	display_img(globalData, false);

	std::thread display_thread(displayImage);
	display_thread.join();
	
	return 0;
}

void readImage(const std::string& _path, data& globalData)
{
	cv::Mat image = cv::imread(_path);
	if(image.cols > 0)
	{
		cv::Mat mask = cv::Mat::zeros(image.rows, image.cols, CV_8UC1);
		std::vector<cv::Mat> rgbChannels, cmykChannels;
		cv::Mat aux;
		cv::cvtColor(image, aux, cv::COLOR_BGR2RGB);
		cv::split(aux, rgbChannels);

		globalData.read_image = image;
		globalData.mask = mask;
		globalData.channels = rgbChannels;
	}
	// CMYK
}

void displayImage()
{
	while(!killThread)
	{
		globalImageToDisplayThread_mutex.lock();
		if(globalImageToDisplayThread.rows == DISPLAY_SIZE_H)
			cv::imshow(W_NAME, globalImageToDisplayThread);
		globalImageToDisplayThread_mutex.unlock();
		char c = cv::waitKey(100);
		if((int)c == 27)
			break;
	}
}

void display_img(data& globalData, bool displayMask)
{
	cv::Mat display_image;
	cv::Mat img;

	std::cout << "Actual channel = " << globalData.actual_channel << std::endl;
	if(globalData.actual_channel == 0)
		globalData.read_image.copyTo(img);
	else
	{
		globalData.channels.at(globalData.actual_channel-1).copyTo(img);
		cv::cvtColor(img, img, cv::COLOR_GRAY2BGR);
	}

	if(displayMask)
	{
		img.setTo(cv::Scalar(255,0,0), globalData.mask==0);
		img.setTo(cv::Scalar(0,255,0), globalData.mask==1);
		img.setTo(cv::Scalar(0,0,255), globalData.mask==2);
	}
	else if(globalData.th_on)
		img.setTo(cv::Scalar(0,255,255), globalData.threshold_mask==0);

	cv::resize(img, display_image, cv::Size(DISPLAY_SIZE_H, DISPLAY_SIZE_W));

	display_image.copyTo(globalData.imagePlusControls.colRange(0, DISPLAY_SIZE_W));
	globalImageToDisplayThread_mutex.lock();
	globalData.imagePlusControls.copyTo(globalImageToDisplayThread);
	globalImageToDisplayThread_mutex.unlock();
}

void setup(data& globalData)
{
	cv::namedWindow(W_NAME, cv::WINDOW_AUTOSIZE);

	globalData.imagePlusControls = cv::Mat::zeros(DISPLAY_SIZE_H, static_cast<int>(1.5f*DISPLAY_SIZE_W), CV_8UC3);

	int slider1pos = 0;
	globalData.mask_id = slider1pos;
	int nOfMasks = 3;
	cv::createTrackbar(
		"Mask label",
		W_NAME,
		&slider1pos,
		nOfMasks-1,
		nMaskChanged,
		(void*) &globalData);

	int slider2pos = 0;
	globalData.actual_channel = slider2pos;
	int nOfChannels = 4; // Color R G B C M Y BK
	cv::createTrackbar(
		"Channel",
		W_NAME,
		&slider2pos,
		nOfChannels-1,
		channelChanged,
		(void*) &globalData);

	int slider3pos = 0;
	globalData.th_value = slider3pos;
	cv::createTrackbar(
		"Threshold",
		W_NAME,
		&slider2pos,
		255,
		thresholdValueChanged,
		(void*) &globalData);

	cv::setMouseCallback(W_NAME, onMouseClickled, (void*) &globalData);
}

void setupButtons(data& globalData)
{
	globalData.buttons = new rectanglesButtons[N_OF_BUTTONS];

	globalData.mask_view_on = false;
	globalData.add_on = true;
	globalData.th_on = false;
	globalData.th_inv = false;

	for(int i=0; i<N_OF_BUTTONS; i++)
	{
		globalData.buttons[i]._rect = cv::Rect(DISPLAY_SIZE_W, i * DISPLAY_SIZE_H/N_OF_BUTTONS,
									0.5f * DISPLAY_SIZE_W, DISPLAY_SIZE_H/N_OF_BUTTONS);
		globalData.buttons[i]._name = buttonsNames.at(i);
		if(i != 2 && i != 3 && i != 0 && i != 4)
			globalData.buttons[i]._color = cv::Scalar(255);
		else if(i == 2 || i == 3 || i == 0)
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
	erodedRect.y = button._rect.y + ero/2;
	erodedRect.height = button._rect.height - ero;
	erodedRect.width = button._rect.width - 2*ero;

	cv::rectangle(imagePlusControls, erodedRect, button._color, 6);

	cv::Size sz = cv::getTextSize(button._name, cv::FONT_HERSHEY_COMPLEX,1 , 2, NULL);
	int deltaX = (erodedRect.width - sz.width)/2;
	int deltaY = (erodedRect.height - sz.height)/2 + sz.height;
	cv::putText(imagePlusControls, button._name,
				cv::Point2d(erodedRect.x + deltaX, erodedRect.y + deltaY),
				cv::FONT_HERSHEY_COMPLEX, 1, button._color, 2);
}

void onButtonViewMaskClicked(data& globalData)
{
	if(globalData.mask_view_on)
	{
		globalData.mask_view_on = false;
		globalData.buttons[0]._color = cv::Scalar(0,0,255);
		displayButton(globalData.buttons[0], globalData.imagePlusControls);
	} else
	{
		globalData.mask_view_on = true;
		globalData.th_on = false;
		globalData.buttons[0]._color = cv::Scalar(0,255,0);
		displayButton(globalData.buttons[0], globalData.imagePlusControls);
		
	}
	display_img(globalData, globalData.mask_view_on);
}

void onButtonApllyThresholdClicked(data& globalData)
{
	if(globalData.threshold_mask.cols > 0)
	{
		globalData.mask.copyTo(globalData.previous_mask);
		globalData.mask.setTo(cv::Scalar(globalData.mask_id), globalData.threshold_mask==0);
		globalData.th_on = false;
		display_img(globalData, globalData.mask_view_on);
	}
	
}

void onButtonThresholdInvClicked(data& globalData)
{
	if(globalData.th_inv)
	{
		globalData.buttons[2]._color = cv::Scalar(0,0,255);
		displayButton(globalData.buttons[2], globalData.imagePlusControls);
		globalData.th_inv = false;
	}
	else
	{
		globalData.buttons[2]._color = cv::Scalar(0,255,0);
		displayButton(globalData.buttons[2], globalData.imagePlusControls);
		globalData.th_inv = true;
	}
	thresholdValueChanged(globalData.th_value, (void*) &globalData);
	//display_img(globalData, globalData.mask_view_on);
}

void onButtonDeleteClicked(data& globalData)
{
	if(globalData.add_on)
	{
		globalData.add_on = false;
		globalData.buttons[3]._color = cv::Scalar(0,255,0);
		globalData.buttons[4]._color = cv::Scalar(0,0,255);
		displayButton(globalData.buttons[3], globalData.imagePlusControls);
		displayButton(globalData.buttons[4], globalData.imagePlusControls);
		display_img(globalData, globalData.mask_view_on);
	}
}

void onButtonAddClicked(data& globalData)
{
	if(!globalData.add_on)
	{
		globalData.add_on = true;
		globalData.buttons[4]._color = cv::Scalar(0,255,0);
		globalData.buttons[3]._color = cv::Scalar(0,0,255);
		displayButton(globalData.buttons[4], globalData.imagePlusControls);
		displayButton(globalData.buttons[3], globalData.imagePlusControls);
		display_img(globalData, globalData.mask_view_on);
	}
}

void onButtonGoBackClicked(data& globalData)
{
	if(globalData.previous_mask.cols > 0)
	{
		globalData.previous_mask.copyTo(globalData.mask);
		display_img(globalData, globalData.mask_view_on);
	}
}

void onButtonNextImageClicked(data& globalData)
{
	globalData.current_image++;
	if(globalData.current_image < globalData.n_of_images)
	{
		readImage(path + globalData.Images.at(globalData.current_image), globalData);
		display_img(globalData, false);
	}
}

void onButtonSaveMaskClicked(data& globalData)
{
	std::string maskName = "mask_" + globalData.Images.at(globalData.current_image);
	bool exists_mask_folder = std::filesystem::exists(std::filesystem::path(path + "/masks"));

	if(!exists_mask_folder)
		std::filesystem::create_directory(path + "/masks");

	cv::imwrite(path + "masks/" + maskName, globalData.mask);
}

void nMaskChanged(int pos, void* param)
{
	data *globalData = (data*)param;
	globalData->mask_id = pos;
}

void channelChanged(int pos, void* param)
{
	data *globalData = (data*)param;
	globalData->actual_channel = pos;
	std::cout << "channel changed" << std::endl;
	display_img(*globalData, globalData->mask_view_on);
}

void thresholdValueChanged(int pos, void* param)
{
	data *globalData = (data*)param;

	if(globalData->actual_channel > 0)
	{

		globalData->th_value = pos;
		globalData->mask_view_on = false;
		globalData->buttons[0]._color = cv::Scalar(0,0,255);
		displayButton(globalData->buttons[0], globalData->imagePlusControls);
		globalData->th_on = true;

		int thType;
		if(globalData->th_inv)
			thType = cv::THRESH_BINARY_INV;
		else
			thType = cv::THRESH_BINARY;

		cv::threshold(globalData->channels.at(globalData->actual_channel-1),
					  globalData->threshold_mask, pos, 255, thType);
	}

	display_img(*globalData, globalData->mask_view_on);
}

void onMouseClickled(int event, int x, int y, int flags, void* userdata)
{

	data *globalData = (data*)userdata;

	if  ( event == cv::EVENT_LBUTTONDOWN )
     {
          if(x < DISPLAY_SIZE_W)
          {
          	std::cout << "Mask id = " << globalData->mask_id << std::endl;
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

          	switch(button_idx)
          	{
          		case 0:
	          		onButtonViewMaskClicked(*globalData);
	          		break;
          		case 1:
          			onButtonApllyThresholdClicked(*globalData);
          			break;
          		case 2:
	          		onButtonThresholdInvClicked(*globalData);
	          		break;
          		case 3:
	          		onButtonDeleteClicked(*globalData);
	          		break;
          		case 4:
	          		onButtonAddClicked(*globalData);
	          		break;
          		case 5:
	          		onButtonGoBackClicked(*globalData);
	          		break;
          		case 6:
	          		onButtonNextImageClicked(*globalData);
	          		break;
          		case 7:
          			onButtonSaveMaskClicked(*globalData);
          			break;
          		case 8:
          			// To quit the program just kill the displaying thread
          			killThread = true;
	          		break;
          		default:
          			break;
          	}

          }
     }
}