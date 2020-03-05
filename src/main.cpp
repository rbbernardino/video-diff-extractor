#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp> 
#include <opencv2/imgproc/imgproc.hpp>

#include <experimental/filesystem>
#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp> // to_upper, to_lower, split

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <array>
#include <map>
#include <fstream> // read file streams
#include <sstream> // read string stream from file

#include "args.hxx"

using namespace std;
using namespace cv;
namespace fs = std::experimental::filesystem;

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
	cout << parser;
	return 1;
    }

    fs::path outPath = fs::path(args::get(pOutDirPath));
    return 0;
}