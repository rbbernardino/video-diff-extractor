#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp> 
#include <opencv2/videoio.hpp>

#include <experimental/filesystem>

#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "args.hxx"
#include "SSIM.hpp"
#include "eta.hpp"
#include "alphanum.hpp"

using namespace std;
using namespace cv;
namespace fs = std::experimental::filesystem;

string const CUR_FRAME_WINNAME = "Current Frame";
int const RSZ_WIDTH = 640;
int const RSZ_HEIGHT = 480;
float DEFAULT_SIM_THRESH = 0.97;
int DEFAULT_UPDATE_PROGRESS_RATE = 100;
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
    stringStream << std::setfill('0') << std::setw(2) << hours << ":"
		 << std::setfill('0') << std::setw(2) << minutes << ":"
		 << std::setfill('0') << std::setw(2) << seconds << flush;
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
    args::ValueFlag<std::string> pInputPath(parser, "videoOrDirectory", "The input video file OR directory", {'i'});
    args::ValueFlag<std::string> pReferenceDirPath(parser, "directory", "The reference images dir path", {'r'});
    args::ValueFlag<std::string> pOutDirPath(parser, "directory", "The output directory path", {'o'});
    args::ValueFlag<int> pStartFrame(parser, "start_frame", "Ignores all frames before the specified one", {'s'});
    args::ValueFlag<int> pEndFrame(parser, "end_frame", "Ignores all frames after the specified one", {'e'});
    args::ValueFlag<float> pSimThresh(parser, "sim_thresh", "Similarity threshold", {'t'});
    args::ValueFlag<int> pUpdateProgressRate(parser, "N", "Show progress every N frames", {'u'});
    args::Flag pVerbose(parser, "verbose", "Show image if found object only", {'v'});
    args::Flag pVerbose2(parser, "verbose", "Show EVERY image being processed", {"verbose2"});
    
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

    if(!pInputPath || !pReferenceDirPath || !pOutDirPath) {
        std::cout << parser;
        return 1;
    }

    fs::path outPath = fs::path(args::get(pOutDirPath));
    fs::path inputPath = fs::path(args::get(pInputPath));
    fs::path refImagesDirPath = fs::path(args::get(pReferenceDirPath));

    long startFrame = 1;
    if(pStartFrame)
        startFrame = args::get(pStartFrame);

    long endFrame;
    if(pEndFrame)
        endFrame = args::get(pEndFrame);

    int visualRefreshRate;
    if(pUpdateProgressRate)
        visualRefreshRate = args::get(pUpdateProgressRate);
    else
        visualRefreshRate = DEFAULT_UPDATE_PROGRESS_RATE;
    
    float simThresh = DEFAULT_SIM_THRESH;
    if(pSimThresh)
        simThresh = args::get(pSimThresh);
    
    if(!fs::exists(inputPath))
    {
        std::cerr << "ERROR, input path '" << inputPath.string() << "' does not exist" << endl;
        return -1;
    } else if(!fs::is_directory(inputPath))
    {
        std::cerr << "ERROR, path '" << inputPath.string() << "' is not a directory" << endl;
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
    
    if(!fs::exists(outPath))
        fs::create_directories(outPath);
    
    // copy all paths to a vector and sort them
    cout << "Reading reference images and resizing..." << endl;
    typedef vector<fs::path> pvec;
    pvec refimg_paths;
    copy(fs::directory_iterator(refImagesDirPath), fs::directory_iterator(), back_inserter(refimg_paths));
    sort(refimg_paths.begin(), refimg_paths.end());
    fs::path fpath;
    vector<Mat> refImages(refimg_paths.size());
    for (int i = 0; i < refimg_paths.size(); i++) {
        cout << "Reference Image " << i << " (" << refimg_paths[i].filename() << ")\r" << flush;
        fpath = refimg_paths[i];
        Mat full_size = cv::imread(fpath.string()); 
        cv::resize(full_size, refImages[i], cv::Size(RSZ_WIDTH, RSZ_HEIGHT), 0, 0, cv::INTER_AREA);
    }
    cout << string(120, ' ') << '\r' << flush;

    // obtain frames paths
    cout << "Getting input frames paths..." << endl;
    pvec input_paths;
    copy(fs::directory_iterator(inputPath), fs::directory_iterator(), back_inserter(input_paths));
    sort(input_paths.begin(), input_paths.end(), doj::alphanum_less<std::string>());

    long frame_count = input_paths.size();
    if(frame_count > 0) {
        cout << "Frame count: " << frame_count << endl << endl;
    } else {
        cout << "No frames found!" << endl;
        exit(1);
    }

    if(!pEndFrame) {
        endFrame = frame_count;
    }

    if(pVerbose || pVerbose2) {
        cv::namedWindow(CUR_FRAME_WINNAME, cv::WINDOW_NORMAL);
        resizeWindow(CUR_FRAME_WINNAME, 640, 480);
    }

    // --------------------------------------
    Mat full_frame, cur_frame;
    full_frame = imread(input_paths[startFrame - 1]);
    cv::resize(full_frame, cur_frame, cv::Size(RSZ_WIDTH, RSZ_HEIGHT), 0, 0, cv::INTER_AREA);

    cout << "Started to process files." << endl;
    if(pVerbose || pVerbose2)
        cv::imshow(CUR_FRAME_WINNAME, cur_frame);
    waitKey(100);
    EtaEstimator eta(endFrame - startFrame + 1);
    long cur_frame_number;
    for(long i = startFrame-1; i < endFrame; i++)
    {
        full_frame = imread(input_paths[i]);
        cv::resize(full_frame, cur_frame, cv::Size(RSZ_WIDTH, RSZ_HEIGHT), 0, 0, cv::INTER_AREA);

        bool has_foreground = true;
        int back_img_index;
        float diff_score, max_score = 0.0;
        cur_frame_number = i+1;
        for(int i = 0; i < refImages.size(); i++) {
            // the strategy is to compare the input frame with each
            // background reference frame. If any of the background
            // frames is nearly equal to the input frame, than there
            // is no foreground.
            // -------------
            // this is necessary because the background varies along time
            diff_score = compareImages(refImages[i], cur_frame);
            if(diff_score >= max_score) {
                max_score = diff_score;
                back_img_index = i;
            }
            if(diff_score >= simThresh) {
                has_foreground = false;
                break;
            }
        }

        if(has_foreground) {
            if(pVerbose || pVerbose2) {
                cv::imshow(CUR_FRAME_WINNAME, cur_frame);
            }
            // string timestamp = millis_to_timestamp(cap.get(cv::CAP_PROP_POS_MSEC));
            cout << "Object detected! | max_sim=" << fixed << setprecision(4) << max_score
                 << " | " << "frame " << cur_frame_number // << " (" << timestamp << ")"
                 << " | " << "ETA " << eta
                 << endl;
            // std::ostringstream stringStream;
            // stringStream << inputPath.stem().string()
            //              << "_f" << cur_frame_number
            //              << "-t" << timestamp
            //              << "-ms" << fixed << setprecision(4) << max_score
            //              << OUT_EXT
            //              << flush;
            // std::string outName = stringStream.str();
            // cout << "Writing to " << outName << endl;
            // cv::imwrite((outPath / outName).string(), cur_frame);
            fs::path full_out_path = outPath / input_paths[i].filename();
            cv::imwrite(full_out_path.string(), cur_frame);
            // waitKey(100);
        } else if((cur_frame_number % visualRefreshRate) == 0) {
            cout << "frame " << std::setfill('0') << std::setw(6) << cur_frame_number
                 // << " (" << millis_to_timestamp(cap.get(cv::CAP_PROP_POS_MSEC)) << ")"
                 << " | " << "max sim = " << max_score
                 << " | " << "ETA " << eta
                 << endl;
            if(pVerbose || pVerbose2) {
                cv::imshow(CUR_FRAME_WINNAME, cur_frame);
                waitKey(100);
            }
        } else if(pVerbose2) {
            cv::imshow(CUR_FRAME_WINNAME, cur_frame);
            waitKey(100);
        }
        eta.update();
    }
    return 0;
}
