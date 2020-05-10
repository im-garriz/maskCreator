#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

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
// Macro to change whether we want the 0 (background) label to be seen
// when view mask button gets pressed
#define PRINT_BACKGROUND_LABEL false
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
	"EXIT"};

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
	cv::Mat read_image;	// The image that has been read from the specified directory
	cv::Mat imagePlusControls; // a image containing the resized read image + buttons
	cv::Mat mask; // Te current created mask
	cv::Mat previous_mask; // Memory of the previous mask so as to going back
	cv::Mat threshold_mask; // A cv::Mat that holds the threshold
	utils::stringvec Images; // vector of strings that contain the image files in the directory
	std::vector<cv::Mat> channels; // vector of cv::Mat that contains the different channels
	int mask_id; // The id of the mask with which we're working (0 (background) to n_of_masks-1)
	int actual_channel; // The idx of the channel which is being displayed (0 means color image)
	int th_value; // Current value of the threshold (trackbar)
	int n_of_images; // Number of images found at the directory
	int current_image; // idx of the current image
	int radiusClick; // Radius of the circle displayed when doubleclicked the image
	rectanglesButtons* buttons; // Array of rectanglesButtons
	bool mask_view_on; // boolean that holds whether we want to see the mask or not
	bool add_on; // true when add is ON, false when delete is ON
	bool th_on;   // boolean that enables the threshold mode (so as to not trying to do a threshold to a 3-channel image)
	bool th_inv; // boolean that holds whether the threshold mode is inverted or not
	cv::Point2d rect_p1; // Coordinates of the point 1 (when displaying the rectangle)
	cv::Point2d rect_p2; // Coordinates of the point 2 (when displaying the rectangle)
	std::chrono::time_point<std::chrono::system_clock> m_StartTime; // Start time of the rectangle display is clicked
	std::chrono::time_point<std::chrono::system_clock> m_EndTime;  // End time when rectangle is finished

};

// Global cv::Mat to which the image we want to display will be copied
// so as to the display_thread reads it
cv::Mat globalImageToDisplayThread;
// Mutex variable for reading the matrix above in both threads
std::mutex globalImageToDisplayThread_mutex;
// Boolean used to kill the display_thread and as a result, to exit
bool killThread = false;
// Path that contains the images we want to label
const std::string path = "/home/inaki/Desktop/cv/cropedSticks/";
// Extension of the images
const std::string _extension = ".tif";
// Declaration of the extern extension variable (utils.h)
std::string extension;

// Function that reads all the images with the specified extension in the specifies path
void readImage(const std::string& _path, data& globalData);
// Functions that copies the corresponding image (masks, thresholds and so on to the global image)
void display_img(data& globalData, bool displayMask);
// Inf. loop that prints the global image continously that is called in the display_thread
void displayImage();
// Function that setups all trackbars and mouse events
void setup(data& globalData);
// Funtion that setups the buttons
void setupButtons(data& globalData);
// Function that displays a button on the screen
void displayButton(rectanglesButtons rectangle, cv::Mat& imagePlusControls);

// Button events
void onButtonViewMaskClicked(data& globalData);
void onButtonApllyThresholdClicked(data& globalData);
void onButtonThresholdInvClicked(data& globalData);
void onButtonDeleteClicked(data& globalData);
void onButtonAddClicked(data& globalData);
void onButtonGoBackClicked(data& globalData);
void onButtonSaveMaskClicked(data& globalData);

// Slider events
void nMaskChanged(int pos, void* param);
void channelChanged(int pos, void* param);
void thresholdValueChanged(int pos, void* param);
void radiousValueChanged(int pos, void* param);

// Mouse event
void onMouseClickled(int event, int x, int y, int flags, void* userdata);

int main()
{
	// I create a instance of data struct that will hold all the information
	data globalData;

	// I call both setups
	setup(globalData);
	setupButtons(globalData);

	// I read images from the specified path with the specified extension
	extension = _extension;
	utils::read_directory(path, globalData.Images);

	// I the program has not found any image quits
	if(globalData.Images.empty())
		return 2;

	// I initialize some variables explained above
	globalData.n_of_images = globalData.Images.size();
	globalData.current_image = 0;

	// I read the first image and I display it
	readImage(path + globalData.Images.at(globalData.current_image), globalData);
	display_img(globalData, false);

	// I create and start the display_thread
	std::thread display_thread(displayImage);
	display_thread.join();
	
	return 0;
}

void readImage(const std::string& _path, data& globalData)
{
	cv::Mat image = cv::imread(_path);
	// If I have read an image
	if(image.cols > 0)
	{
		cv::Mat mask = cv::Mat::zeros(image.rows, image.cols, CV_8UC1); // Mask initialization to 0
		// I get the RGB channels
		std::vector<cv::Mat> rgbChannels, cmykChannels;
		cv::Mat aux;
		cv::cvtColor(image, aux, cv::COLOR_BGR2RGB);
		cv::split(aux, rgbChannels);

		// I store the info in my globalData struct
		globalData.read_image = image;
		globalData.mask = mask;
		globalData.channels = rgbChannels;
	}
	// CMYK
}

void displayImage()
{
	// While the thread is running
	while(!killThread)
	{
		globalImageToDisplayThread_mutex.lock();
		// If the image is not empty
		if(globalImageToDisplayThread.rows == DISPLAY_SIZE_H)
			// I plot it
			cv::imshow(W_NAME, globalImageToDisplayThread);
		globalImageToDisplayThread_mutex.unlock();
		// Refresh rate of 100ms
		char c = cv::waitKey(100);
		// I also can quit the program by pressing the ESC key
		if((int)c == 27)
			break;
	}
}

void display_img(data& globalData, bool displayMask)
{
	cv::Mat display_image;
	cv::Mat img;

	// If I wanna plot the color image
	if(globalData.actual_channel == 0)
		globalData.read_image.copyTo(img);
	else
	{
		// Else I want to plot a certain channel
		globalData.channels.at(globalData.actual_channel-1).copyTo(img);
		cv::cvtColor(img, img, cv::COLOR_GRAY2BGR);
	}

	// If I want to display the mask
	if(displayMask)
	{
		// I only print the 0 label if the macro is set to true above (blue)
		if(PRINT_BACKGROUND_LABEL)
			img.setTo(cv::Scalar(255,0,0), globalData.mask==0);
		// Label 1 green 
		img.setTo(cv::Scalar(0,255,0), globalData.mask==1);
		// Label 2 red
		img.setTo(cv::Scalar(0,0,255), globalData.mask==2);
	}
	else if(globalData.th_on)
		// Else I don't want to display the mask
		img.setTo(cv::Scalar(0,255,255), globalData.threshold_mask==0);

	// I resize the image to de desired size
	cv::resize(img, display_image, cv::Size(DISPLAY_SIZE_H, DISPLAY_SIZE_W));

	// I join it to the buttons image
	display_image.copyTo(globalData.imagePlusControls.colRange(0, DISPLAY_SIZE_W));
	// I copy it to the global cv::Mat (the display_thread will display it)
	globalImageToDisplayThread_mutex.lock();
	globalData.imagePlusControls.copyTo(globalImageToDisplayThread);
	globalImageToDisplayThread_mutex.unlock();
}

void setup(data& globalData)
{
	// I create the window 
	cv::namedWindow(W_NAME, cv::WINDOW_AUTOSIZE);

	// I create the image where the buttons will be displayed
	globalData.imagePlusControls = cv::Mat::zeros(DISPLAY_SIZE_H,
								   static_cast<int>(1.5f*DISPLAY_SIZE_W), CV_8UC3);

	// I create each slider with its event function initialized at 0
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
		&slider3pos,
		255,
		thresholdValueChanged,
		(void*) &globalData);

	int slider4pos = 0;
	globalData.radiusClick = slider4pos;
	cv::createTrackbar(
		"Radius of clicked point",
		W_NAME,
		&slider4pos,
		100,
		radiousValueChanged,
		(void*) &globalData);

	// I set mouse callbacks to the onMouseClickled function
	cv::setMouseCallback(W_NAME, onMouseClickled, (void*) &globalData);
}

void setupButtons(data& globalData)
{
	// I create the buttons
	globalData.buttons = new rectanglesButtons[N_OF_BUTTONS];

	// The state of the buttons when the program is launched
	globalData.mask_view_on = false;
	globalData.add_on = true;
	globalData.th_on = false;
	globalData.th_inv = false;

	for(int i=0; i<N_OF_BUTTONS; i++)
	{
		// For each button I do what is below so as to they are all equal and equidistant
		globalData.buttons[i]._rect = cv::Rect(DISPLAY_SIZE_W, i * DISPLAY_SIZE_H/N_OF_BUTTONS,
									0.5f * DISPLAY_SIZE_W, DISPLAY_SIZE_H/N_OF_BUTTONS);
		globalData.buttons[i]._name = buttonsNames.at(i);
		if(i != 2 && i != 3 && i != 0 && i != 4)
			globalData.buttons[i]._color = cv::Scalar(255);
		else if(i == 2 || i == 3 || i == 0)
			globalData.buttons[i]._color = cv::Scalar(0,0,255);
		else
			globalData.buttons[i]._color = cv::Scalar(0,255,0);

		// I display the button
		displayButton(globalData.buttons[i], globalData.imagePlusControls);
	}
}

void displayButton(rectanglesButtons button, cv::Mat& imagePlusControls)
{
	// I erode the real reactangle just for appearence
	cv::Rect erodedRect;
	const int ero = 15;
	erodedRect.x = button._rect.x + ero;
	erodedRect.y = button._rect.y + ero/2;
	erodedRect.height = button._rect.height - ero;
	erodedRect.width = button._rect.width - 2*ero;

	// I display it on the image
	cv::rectangle(imagePlusControls, erodedRect, button._color, 6);

	// I create the text inside the button
	cv::Size sz = cv::getTextSize(button._name, cv::FONT_HERSHEY_COMPLEX,1 , 2, NULL);
	int deltaX = (erodedRect.width - sz.width)/2;
	int deltaY = (erodedRect.height - sz.height)/2 + sz.height;
	cv::putText(imagePlusControls, button._name,
				cv::Point2d(erodedRect.x + deltaX, erodedRect.y + deltaY),
				cv::FONT_HERSHEY_COMPLEX, 1, button._color, 2);
}

void onButtonViewMaskClicked(data& globalData)
{
	// I set the mask_view_member to the correct value and I display
	// the button VIEW MASK with its new color (red if it has been disabled
	// and green if enabled)
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
	// If the threshold mask is created
	if(globalData.threshold_mask.cols > 0)
	{
		// I apply the threshold to the mask (with the mask_id label)
		globalData.mask.copyTo(globalData.previous_mask);
		globalData.mask.setTo(cv::Scalar(globalData.mask_id), globalData.threshold_mask==0);
		globalData.th_on = false;
		display_img(globalData, globalData.mask_view_on);
	}
	
}

void onButtonThresholdInvClicked(data& globalData)
{
	// I set the th_inv member to the correct value and I display
	// the button INV THRESHOLD with its new color (red if it has been disabled
	// and green if enabled)
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
	// I calculate the new threshold mask with the new inversion mode
	thresholdValueChanged(globalData.th_value, (void*) &globalData);
}

void onButtonDeleteClicked(data& globalData)
{
	// If I click delete and it was disabled, I enable it and I change its color
	// to green and ADD's color to red
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
	// If I click add and it was disabled, I enable it and I change its color
	// to green and DELETE's color to red
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
	// When go back is pressed, I just copy the previous mask to the
	// mask and I display the result
	if(globalData.previous_mask.cols > 0)
	{
		globalData.previous_mask.copyTo(globalData.mask);
		display_img(globalData, globalData.mask_view_on);
	}
}

void onButtonNextImageClicked(data& globalData)
{
	// When next image is pressed, the current image counter gets increased and
	// if it was not the last image, I read the next one and I display it
	globalData.current_image++;
	if(globalData.current_image < globalData.n_of_images)
	{
		readImage(path + globalData.Images.at(globalData.current_image), globalData);
		display_img(globalData, false);
	}
}

void onButtonSaveMaskClicked(data& globalData)
{
	// When save mask is pressed, I save it to the masks folder in the above given path
	std::string maskName = "mask_" + globalData.Images.at(globalData.current_image);
	bool exists_mask_folder = std::filesystem::exists(std::filesystem::path(path + "/masks"));

	// If masks do not exist, the program creates it
	if(!exists_mask_folder)
		std::filesystem::create_directory(path + "/masks");

	cv::imwrite(path + "masks/" + maskName, globalData.mask);
}

void nMaskChanged(int pos, void* param)
{
	// I update the mask_id value when the correspondent slider gets moved
	data *globalData = (data*)param;
	globalData->mask_id = pos;
}

void channelChanged(int pos, void* param)
{
	// I update the actual_channel value when the correspondent slider gets moved
	// And I display it
	data *globalData = (data*)param;
	globalData->actual_channel = pos;
	display_img(*globalData, globalData->mask_view_on);
}

void thresholdValueChanged(int pos, void* param)
{

	data *globalData = (data*)param;

	// When I change the threshold value and I'm not working with the color image
	if(globalData->actual_channel > 0)
	{
		// I save the slider's pos at th_value
		globalData->th_value = pos;
		// I set mask_view_on to false because I wanna see the threshold and I set
		// its buttons color to red
		globalData->mask_view_on = false;
		globalData->buttons[0]._color = cv::Scalar(0,0,255);
		displayButton(globalData->buttons[0], globalData->imagePlusControls);
		// I activate th_on becasue I'm working with the threshold
		globalData->th_on = true;

		// I set the threshold time according to the value of th_inv
		int thType;
		if(globalData->th_inv)
			thType = cv::THRESH_BINARY_INV;
		else
			thType = cv::THRESH_BINARY;

		// I do the threshold and I store it on threshold_mask
		cv::threshold(globalData->channels.at(globalData->actual_channel-1),
					  globalData->threshold_mask, pos, 255, thType);
	}

	// Finally, I display it
	display_img(*globalData, globalData->mask_view_on);
}

void radiousValueChanged(int pos, void* param)
{
	// I update the radius value when the slider gets moved
	data *globalData = (data*)param;
	globalData->radiusClick = pos;
}

void onMouseClickled(int event, int x, int y, int flags, void* userdata)
{

	// Mouse events
	data *globalData = (data*)userdata;

	// I store the clicked point
	cv::Point2d p(x, y);

	// I scale it to it's corresponding point in the original image
	int h_scaling_factor = globalData->read_image.cols/DISPLAY_SIZE_H;
	int w_scaling_factor = globalData->read_image.rows/DISPLAY_SIZE_W;

	cv::Point2d p_real(h_scaling_factor*x, w_scaling_factor*y);

	if  ( event == cv::EVENT_LBUTTONDBLCLK && x < DISPLAY_SIZE_W )
	{
		// If double click -> I create a circle of radius radiusClick at the clicked pos
		globalData->mask.copyTo(globalData->previous_mask);

		cv::Scalar c;

		// I delete is enabled im deleting -> mask 0 (background), else I use mask_id
		if(globalData->add_on)
			c = cv::Scalar(globalData->mask_id);
		else
			c = cv::Scalar(0);

		// I create the circle
		cv::circle(globalData->mask, p_real, globalData->radiusClick, c, cv::FILLED);
		display_img(*globalData, globalData->mask_view_on);
	}

	if  ( event == cv::EVENT_LBUTTONUP && x < DISPLAY_SIZE_W )
	{
		//  Press down + press up -> rectangle

		// I store the time that has needed the user to create the rectangle
		globalData->m_EndTime = std::chrono::system_clock::now();

		// I only display the rectangle if it has been more than 250ms, if not, it has been a double click so I don't wanna print a rectangle
		if(std::chrono::duration_cast<std::chrono::milliseconds>(globalData->m_EndTime - globalData->m_StartTime).count() > 250)
		{
			// The same as explained with circles
			globalData->rect_p2 = p_real;

			globalData->mask.copyTo(globalData->previous_mask);

			cv::Scalar c;

			if(globalData->add_on)
				c = cv::Scalar(globalData->mask_id);
			else
				c = cv::Scalar(0);

			cv::rectangle(globalData->mask, globalData->rect_p1, globalData->rect_p2, c, cv::FILLED);
			display_img(*globalData, globalData->mask_view_on);
		}
	}

	if  ( event == cv::EVENT_LBUTTONDOWN )
 	{
 		// If it's not a button
		if(x < DISPLAY_SIZE_W)
		{
			// When I do a single click, I store system time (as explained above) and I store
			// the clicked point so as to create the rectangle
			globalData->m_StartTime = std::chrono::system_clock::now();
			globalData->rect_p1 = p_real;
		}
		else
		{
			// If I click the buttons side
			int button_idx = -1;

			for(int i=0; i<N_OF_BUTTONS; i++)
			{
				// I search for the clicked one
				if(globalData->buttons[i]._rect.contains(p))
				{
					button_idx = i;
					break;
				}
			}

			// And I call its function
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