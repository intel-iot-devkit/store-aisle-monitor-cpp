/*
* Copyright (c) 2018 Intel Corporation.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
* LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
* OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
* WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


// INCLUDING ALL THE HEADERS

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/video.hpp>
#include <iostream>
#include <fstream>
#include <ctime>
#include "opencv2/video/background_segm.hpp"
#include <stdlib.h>
#include <string>
#include "stdafx.h"
#include <was/storage_account.h>
#include <was/blob.h>
#include <was/common.h>
#include <cpprest/filestream.h>
#include <cpprest/containerstream.h>
#include "samples_common.h"
#include <nlohmann/json.hpp>

using namespace std;
using namespace cv;
using namespace azure;
using json = nlohmann::json;

json jsonobj;
string account_name, account_key;
int flag_name, flag_key;


// Upload the image to cloud
void upload_data(string file_path, string file_name)
{
	try
	{
		// Define the connection-string with your values
		string connection_string = "DefaultEndpointsProtocol=https;AccountName="+account_name+";AccountKey="+account_key;
		const utility::string_t storage_connection_string(_XPLATSTR(connection_string));

		// Retrieve storage account from connection string
		azure::storage::cloud_storage_account storage_account = azure::storage::cloud_storage_account::parse(storage_connection_string);

		// Create the blob client
		azure::storage::cloud_blob_client blob_client = storage_account.create_cloud_blob_client();

		// Retrieve a reference to a previously created container or create a new container
		azure::storage::cloud_blob_container container = blob_client.get_container_reference(_XPLATSTR("store-aisle-monitor"));
		container.create_if_not_exists();

		// Make the blob container publicly accessible
		azure::storage::blob_container_permissions permissions;
		permissions.set_public_access(azure::storage::blob_container_public_access_type::blob);
		container.upload_permissions(permissions);

		// Creates a local object
		azure::storage::cloud_block_blob blockBlob = container.get_block_blob_reference(_XPLATSTR(file_name));

		// Transfers data in the file to the blob on the service
		blockBlob.upload_from_file(file_path);
		cout<<"uploaded "<<file_path<<endl;
	}
	catch (const std::exception& e)
	{
		std::wcout << _XPLATSTR("Error: ") << e.what() << std::endl;
	}
}


// Typecast the integer to string
string tostr(int x)
{
	stringstream str;
	str << x;
	return str.str();
}


// Returns the string which contains current date and time
string currentDateTime()
{
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];
	string char_to_string;
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
	for(int i = 0; i < 19; i++)
	{
		char_to_string += buf[i];
	}
	return char_to_string;
}


// Images are saved locally and upload it to cloud
void time(Mat final_img, int path, int count)
{
	string image_name, image_path, extension;
	extension=".png";
	image_path = currentDateTime() + "_PC_" + tostr(count) + extension;
        image_name = image_path;
	if(path == 1)
	{
		image_path = "HEATMAP_" + image_path;
		imwrite(image_path, final_img);
	}
	if(path == 2)
	{
		image_path = "PEOPLE_COUNT_" + image_path;
		imwrite(image_path, final_img);
	}
	if(path == 3)
	{
		image_path = "INTEGRATED_" + image_path;
		imwrite(image_path, final_img);
		if(flag_key && flag_name)
		{
			upload_data(image_path, image_name);
		}
	}
	final_img.release();
}


// Detects the people and adjusts the bounding boxes accordingly
int detectAndDraw(const HOGDescriptor &hog, Mat &image)
{
	vector<Rect> found, found_filtered;
	char message[200];

	/*
	 Run the detector with default parameters. to get a higher hit-rate
	 (and more false alarms, respectively), decrease the hitThreshold and
	 groupThreshold (set groupThreshold to 0 to turn off the grouping completely).
	*/
	hog.detectMultiScale(image, found, 0, Size(8, 8), Size(16, 16), 1.07, 2);

	for(size_t i = 0; i < found.size(); i++ )
	{
		Rect r = found[i];
		size_t j;
		// Do not add small detections inside a bigger detection
		for (j = 0; j < found.size(); j++)
		{
			if (j != i && (r & found[j]) == r)
			{
				break;
			}
		}
		if (j == found.size())
		{
			found_filtered.push_back(r);
		}
	}
	for (size_t i = 0; i < found_filtered.size(); i++)
	{
		Rect r = found_filtered[i];
		// The HOG detector returns slightly larger rectangles than the real objects.
		// so, we slightly shrink the rectangles to get a nicer output.
		r.x += cvRound(r.width * 0.1);
		r.width = cvRound(r.width * 0.8);
		r.y += cvRound(r.height * 0.07);
		r.height = cvRound(r.height * 0.8);
		rectangle(image, r.tl(), r.br(), Scalar(0, 255, 0), 3);
	}
	sprintf(message, "People Count : %lu", found_filtered.size());
	putText(image, message, Point2f(15, 65), FONT_HERSHEY_COMPLEX, 1, Scalar(255, 255, 255), 2, true);
	return found_filtered.size();
}


// Set the HOG Descriptor attributes
int setup(Mat &frame)
{
	int count;
	count = 0;
	HOGDescriptor hog;
	hog.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());
	count = detectAndDraw(hog, frame);
	return count;
}


// Generates heatmap, number of people in each frame & integrated video
int main(int argc, char *argv[])
{
	VideoCapture cap;
	int count, fps, frame_count;
	int frame_width, option, frame_height;
	int thres, maxValue, key;	
	maxValue = 2;
	option = 0;
	thres = 2;
	count = 0;
	frame_count = 1;
	double alpha, beta;
	alpha = 0.5;
	String input;
	std::string conf_file2 = "resources/config.json";
	std::string conf_file = "../resources/config.json";
	std::ifstream confFile(conf_file);
	if (!confFile.is_open())
	{
		std::ifstream confFile2(conf_file2);
		if (!confFile2.is_open())
		{
		    cout << "Could not open config file" << endl;
		    return 2;
		}
		confFile2>>jsonobj;
	}
	else
		confFile>>jsonobj;

	while ((option = getopt(argc, argv, ":n:k:")) != -1)
	{
	        switch (option)
	        {
	         	case 'n': account_name = optarg;
				  flag_name = 1;
	        	  break;

	        	case 'k': account_key = optarg;
				  flag_key = 1;
				  break;
	        }
	}
	// parsing input source from config.json
	auto obj = jsonobj["inputs"];
	input = obj[0]["video"];

	if (input.size() == 1 && *(input.c_str()) >= '0' && *(input.c_str()) <= '9')
		cap.open(std::stoi(input));
	else
		cap.open(input);

	if(!cap.isOpened())
	{
		cout << "Error opening video stream or file" << endl;
		return -1;
	}

	// Parsing frame properties
	frame_width = cap.get(CAP_PROP_FRAME_WIDTH);
	frame_height = cap.get(CAP_PROP_FRAME_HEIGHT);
	fps = cap.get(CAP_PROP_FPS);

	// Initializing background segmentation pointer
	Ptr<BackgroundSubtractorMOG2> background_segmentor_object_file = createBackgroundSubtractorMOG2();

	Mat gray, accum_image, first_frame, frame;
         
	Mat result_overlay_video, final_img;
	cap.read(first_frame);
	accum_image = Mat::zeros(Size(first_frame.cols,first_frame.rows),CV_8UC1);


	while(cap.isOpened())
	{
		cap.read(frame);
		if (frame.empty())
		{
			cout << "Video stream ended" << endl;
			return -1;
		}

		// Converting to Grayscale
		cvtColor(frame, gray, COLOR_BGR2GRAY);

		// Remove the background
		background_segmentor_object_file ->apply(gray, gray);

		// Thresholding the image
		threshold(gray, gray, thres, maxValue, THRESH_BINARY);

		// Adding to the accumulated image
		accum_image = gray + accum_image;

		// Saving the accumulated image
		applyColorMap(accum_image, result_overlay_video, COLORMAP_HOT);
		addWeighted(frame, 0.5, result_overlay_video, 0.5, 0.0, result_overlay_video);

		count = setup(frame);

		// Integrated video generation
		addWeighted(frame, 0.45, result_overlay_video, 0.55, 0.0, final_img);

		imshow("store-aisle-monitor", final_img);
		if(frame_count%(5*fps) == 0)
		{
			time(result_overlay_video, 1, count);
			time(frame, 2, count);
			time(final_img, 3, count);
			frame_count = 1;
		}
		frame_count++;

		key = (waitKey(1) & 0xFF);
		if(key == 'q')
			break;

		frame.release();
	}

	// Adding all accumulated frames to the first frame
	applyColorMap(accum_image, accum_image, COLORMAP_HOT);

	beta = (1.0 - alpha);
	addWeighted( first_frame, alpha, accum_image, beta, 0.0, first_frame);
	imwrite( "result_overlay_final.jpg", first_frame);

	// When everything done, release the video capture
	cap.release();

	// Closes all the windows
	destroyAllWindows();
	return 0;
}


