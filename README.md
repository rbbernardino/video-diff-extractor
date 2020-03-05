# README #

A tool for extracting frames from a video file that differs from some reference frames. The difference is calculated with a configurable threshold.

### How do I get set up? ###

* **C++14**
* **boost 1.58** (on linux: `libboost-all-dev`)
* **OpenCV 3.2**

### How to Run ###

- `./diff-video -v <FILE> -r <DIRECTORY> -o <DIRECTORY>`
	- `-v video_file`, input video to be processed
	- `-r reference_images`, directory with reference images (background)
    - `-o out_dir`, where the extracted images will be written to
