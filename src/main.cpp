#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp> 
#include <opencv2/videoio.hpp>

#include <experimental/filesystem>

#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "args.hxx"
#include "SSIM.hpp"

using namespace std;
using namespace cv;
namespace fs = std::experimental::filesystem;

string const CUR_FRAME_WINNAME = "Current Frame";
int const RSZ_WIDTH = 640;
int const RSZ_HEIGHT = 480;

bool read_resized(VideoCapture &cap, Mat &full_size, Mat &dest_img)
{
    if(!cap.read(full_size))
	return false;
    cv::resize(full_size, dest_img, cv::Size(RSZ_WIDTH, RSZ_HEIGHT), 0, 0, cv::INTER_AREA);
    return true;
}

string millis_to_timestamp(long millis)
{
    int seconds = (millis/1000) % 60;
    int minutes = (millis/(1000*60))%60;
    int hours = (millis/(1000*60*60)) % 24;
    std::ostringstream stringStream;
    stringStream << hours << ":" << minutes << ":" << seconds << flush;
    std::string copyOfStr = stringStream.str();
    return copyOfStr;
}


int main(int argc, char *argv[])
{
    args::ArgumentParser parser("A tool for extracting frames from a video file "
				"that differs from some reference frames. The "
				"difference is calculated with a configurable threshold.");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<std::string> pVideoPath(parser, "video", "The input video file", {'v'});
    args::ValueFlag<std::string> pReferenceDirPath(parser, "directory", "The reference images dir path", {'r'});
    args::ValueFlag<std::string> pOutDirPath(parser, "directory", "The output directory path", {'o'});

    try {
	parser.ParseCLI(argc, argv);
    }
    catch (args::Help)
    {
	std::cout << parser;
	return 0;
    }
    catch (args::ParseError e)
    {
	std::cerr << e.what() << std::endl;
	std::cerr << parser;
	return 1;
    }

    if(!pVideoPath || !pReferenceDirPath || !pOutDirPath) {
	std::cout << parser;
	return 1;
    }

    fs::path outPath = fs::path(args::get(pOutDirPath));
    fs::path videoPath = fs::path(args::get(pVideoPath));
    fs::path refImagesDirPath = fs::path(args::get(pReferenceDirPath));

    if(!fs::exists(videoPath))
    {
	std::cerr << "ERROR, video file '"
		  << videoPath.string()
		  << "' does not exist" << endl;
	return -1;
    } else if(!fs::is_regular_file(videoPath) && !fs::is_symlink(videoPath))
    {
	std::cerr << "ERROR, path '"
		  << videoPath.string()
		  << "' is not a file" << endl;
	return -1;
    }

    if(!fs::exists(refImagesDirPath))
    {
	std::cerr << "ERROR, reference images directory '"
		  << refImagesDirPath.string()
		  << "' does not exist" << endl;
	return -1;
    } else if(!fs::is_directory(refImagesDirPath))
    {
	std::cerr << "ERROR, path '"
		  << refImagesDirPath.string()
		  << "' is not a directory" << endl;
	return -1;
    }

    // copy all paths to a vector and sort them
    cout << "Reading reference images and resizing..." << endl;
    typedef vector<fs::path> pvec;
    pvec all_paths;
    copy(fs::directory_iterator(refImagesDirPath), fs::directory_iterator(), back_inserter(all_paths));
    sort(all_paths.begin(), all_paths.end());
    fs::path fpath;
    vector<Mat> refImages(all_paths.size());
    for (int i = 0; i < all_paths.size(); i++) {
	cout << "Reference Image " << i << " (" << all_paths[i].filename() << ")\r" << flush;
	fpath = all_paths[i];
	Mat full_size = cv::imread(fpath.string()); 
	cv::resize(full_size, refImages[i], cv::Size(RSZ_WIDTH, RSZ_HEIGHT), 0, 0, cv::INTER_AREA);
    }
    cout << string(120, ' ') << '\r' << flush;
    
    VideoCapture cap;
    cap.open(videoPath.string());
    if(!cap.isOpened())
    {
	cerr << "ERROR! Unable to open file." << endl;
	return -1;
    }

    long frame_count = cap.get(cv::CAP_PROP_FRAME_COUNT);
    if(frame_count > 0) {
	cout << "Frame count: " << frame_count << endl;
    } else {
	cout << "Couldn't get frame count from metadata..." << endl;
    }
    
    Mat full_frame, cur_frame;
    cv::namedWindow(CUR_FRAME_WINNAME, cv::WINDOW_NORMAL);
    resizeWindow(CUR_FRAME_WINNAME, 640, 480);

    cout << "Started to process video." << endl;
    read_resized(cap, full_frame, cur_frame);
    cv::imshow("Current Frame", cur_frame);
    waitKey(500);
    do {
	VQMT::SSIM comparator = VQMT::SSIM(cur_frame.cols, cur_frame.rows);
	bool has_foreground;
	float ssim;
	int back_img_index;
	for(int i = 0; i < refImages.size(); i++) {
	    ssim = comparator.compute(refImages[i], cur_frame);
	    if(ssim < 1) {
		has_foreground = true;
		back_img_index = i;
		break;
	    }
	}
	if(((long) cap.get(cv::CAP_PROP_POS_FRAMES) % 10) == 0) {
	    cout << "frame " << cap.get(cv::CAP_PROP_POS_FRAMES)
		 << " (" << millis_to_timestamp(cap.get(cv::CAP_PROP_POS_MSEC)) << ")"
		 << "\r" << flush;
	    // TODO change to show image only with verbose option, as waitKey makes the code slower
	    if(false) {
		cv::imshow("Current Frame", cur_frame);
		waitKey(100);
	    }
	}
	if(has_foreground) {
	    cv::imshow("Current Frame", cur_frame);
	    cout << "diff (1-ssmi): " << 1-ssim << endl;
	    cout << "frame " << cap.get(cv::CAP_PROP_POS_FRAMES)
		 << " (" << millis_to_timestamp(cap.get(cv::CAP_PROP_POS_MSEC)) << "ms)"
		 << endl;
	    waitKey(500);
	}
    } while(read_resized(cap, full_frame, cur_frame));
    return 0;
}
