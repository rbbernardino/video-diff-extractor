#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp> 
// #include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <experimental/filesystem>
// #include <boost/range/iterator_range.hpp>
// #include <boost/algorithm/string.hpp> // to_upper, to_lower, split

#include <stdio.h>
#include <stdlib.h>
#include <string>
// #include <array>
// #include <map>
// #include <fstream> // read file streams
// #include <sstream> // read string stream from file

#include "args.hxx"
#include "SSIM.hpp"

using namespace std;
using namespace cv;
namespace fs = std::experimental::filesystem;

string CUR_FRAME_WINNAME = "Current Frame";

bool read_resized(VideoCapture &cap, Mat &full_size, Mat &dest_img)
{
    if(!cap.read(full_size))
	return false;
    cv::resize(full_size, dest_img, cv::Size(640, 480), 0, 0, cv::INTER_AREA);
    return true;
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
    
    Mat full_frame, frame0, frame1;
    cv::namedWindow(CUR_FRAME_WINNAME, cv::WINDOW_NORMAL);
    resizeWindow(CUR_FRAME_WINNAME, 640, 480);

    // cap.read(frame0);
    read_resized(cap, full_frame, frame0);
    read_resized(cap, full_frame, frame1);
    // waitKey(5000);
    cv::imshow("Current Frame", frame0);
    waitKey(1000);

    long cur_frame_num = 1;
    do {
	VQMT::SSIM comparator = VQMT::SSIM(frame0.cols, frame0.rows);
	float diff = comparator.compute(frame0, frame1);
	if((cur_frame_num % 100) == 0) {
	    cout << "frame " << cur_frame_num << "\r" << flush;
	    cv::imshow("Current Frame", frame1);
	    waitKey(1000);
	}
	if(diff < 1) {
	    cv::imshow("Current Frame", frame1);
	    cout << "diff: " << 1-diff << endl;
	    waitKey();
	}
	frame1.copyTo(frame0);
	cur_frame_num++;
    } while(read_resized(cap, full_frame, frame1));
    return 0;
}
