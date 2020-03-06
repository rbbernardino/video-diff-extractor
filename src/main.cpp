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
float DIFF_SCORE_THRESH = 0.97;
string OUT_EXT = ".png";

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

float compareImages(Mat &imgA, Mat &imgB)
{
    Mat scoreImage;
    double maxScore;
    cv::matchTemplate(imgA, imgB, scoreImage, TM_CCOEFF_NORMED);
    minMaxLoc(scoreImage, 0, &maxScore);
    return maxScore;
    // VQMT::SSIM comparator = VQMT::SSIM(1280, 720);
    // float ssim = comparator.compute(a, b);
    // cout<< ssim << endl;
}

int main(int argc, char *argv[])
{
    // Mat a = cv::imread(argv[1], IMREAD_COLOR);
    // Mat b = cv::imread(argv[2], IMREAD_COLOR);
    // cout << "score " << compareImages(a, b) << endl;
    // exit(0);
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
    cv::imshow(CUR_FRAME_WINNAME, cur_frame);
    waitKey(500);
    do {
	bool has_foreground = true;
	int back_img_index;
	float diff_score;
	long cur_frame_number = cap.get(cv::CAP_PROP_POS_FRAMES);
	for(int i = 0; i < refImages.size(); i++) {
	    // the strategy is to compare the input frame with each
	    // background reference frame. If any of the background
	    // frames is nearly equal to the input frame, than there
	    // is no foreground.
	    // -------------
	    // this is necessary because the background varies along time
	    diff_score = compareImages(refImages[i], cur_frame);
	    if(diff_score >= DIFF_SCORE_THRESH) {
		has_foreground = false;
		back_img_index = i;
		break;
	    }
	}

	if((cur_frame_number % 10) == 0) {
	    cout << "frame " << cur_frame_number
		 << " (" << millis_to_timestamp(cap.get(cv::CAP_PROP_POS_MSEC)) << ")"
		 << endl;
	    // TODO change to show image only with verbose option, as waitKey makes the code slower
	    if(false) {
		cv::imshow(CUR_FRAME_WINNAME, cur_frame);
		waitKey(100);
	    }
	}
	if(has_foreground) {
	    cv::imshow(CUR_FRAME_WINNAME, cur_frame);
	    string timestamp = millis_to_timestamp(cap.get(cv::CAP_PROP_POS_MSEC));
	    cout << "Found Object! diff: " << 100 * (1 - diff_score) << "%" << endl;
	    cout << "frame " << cur_frame_number << " (" << timestamp << "ms)" << endl;
	    std::ostringstream stringStream;
	    stringStream << videoPath.stem().string()
			 << "_f" << cur_frame_number
			 << "-t" << timestamp
			 << "-d" << diff_score
			 << OUT_EXT
			 << flush;
	    std::string outName = stringStream.str();
	    cout << "Writing to " << outName << endl;
	    cv::imwrite((outPath / outName).string(), cur_frame);
	    waitKey(500);
	}
    } while(read_resized(cap, full_frame, cur_frame));
    return 0;
}
