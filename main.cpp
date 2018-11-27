#include <iostream>
#include <string>
//#include <vector>
#include <time.h>
//#include <cv.h>

#include "opencv2/highgui/highgui.hpp"
//#include <opencv2/core/core.hpp>

//#include "opencv2/videoio.hpp"
#include "opencv2/opencv.hpp"
//#include <opencv2/videoio.hpp>
//#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;


int main(int, char**)
{
    const std::string outputFile = "/home/anne/programming/openCV/VideoCapturing/photos/cap.avi";
    const std::string outputFileWithMask = "/home/anne/programming/openCV/VideoCapturing/photos/cap_withmask.avi";

    VideoCapture inputVideo(0); // open camera
    if (!inputVideo.isOpened())  // check if we succeeded
    {
        std::cout << "Couldn't connect to camera" << std::endl;
        return -1;
    }

    int ex = static_cast<int>(inputVideo.get(CAP_PROP_FOURCC)); // Get Codec Type - Int form
    int width = (int) inputVideo.get(CAP_PROP_FRAME_WIDTH);
    int height = (int) inputVideo.get(CAP_PROP_FRAME_HEIGHT);
    Size S = Size(width, height); // set size of input frames

    VideoWriter outputVideo; // output stream to file (captured video)
    outputVideo.open(outputFile, ex, inputVideo.get(CAP_PROP_FPS), S, true);
    if (!outputVideo.isOpened())
    {
        std::cout  << "Could not open the output videofile for write: " << outputFile << std::endl;
        return -1;
    }

    VideoWriter outputVideoWithMask; // output stream to file (masked video)
    outputVideoWithMask.open(outputFileWithMask, ex, inputVideo.get(CAP_PROP_FPS), S, true);
    if (!outputVideoWithMask.isOpened())
    {
        std::cout  << "Could not open the output videofile for write with mask: " << outputFile << std::endl;
        return -1;
    }

    Mat mask;
    mask = imread("/home/anne/programming/openCV/VideoCapturing/mask.jpg", IMREAD_COLOR);

    Mat prevFrame;
    Mat thisFrame;
    Mat maskedFrame;

    std::string name = "";

    time_t rawtime0;
    time(&rawtime0);
    struct tm * prev_timeinfo = localtime(&rawtime0);
    std::string timeinfo_str = "";
    std::string prev_timeinfo_str = std::string(asctime(prev_timeinfo));;
    int i = 0;

    // надо добавить проверку свободного места на диске
    for (;;)
    {
        inputVideo >> thisFrame; // get a new frame from camera
        if (thisFrame.empty()) break; // check if videostream ended, then break

        namedWindow("frame", WINDOW_AUTOSIZE);
        namedWindow("maskedFrame", WINDOW_AUTOSIZE);

        time_t rawtime;
        time(&rawtime);
        struct tm * timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        timeinfo_str = std::string(asctime(timeinfo));
        name = timeinfo_str.substr(0, timeinfo_str.size()-1);
        if (prev_timeinfo_str == timeinfo_str)
            i++;
        else
            i = 0;
        name = name + "-" + std::to_string(i) + ".jpg"; // Tue Nov 27 12:36:53 2018-1.jpg

        imshow("frame", thisFrame);
        outputVideo << thisFrame;

        Mat savingFrame;
        thisFrame.copyTo(savingFrame);

        if (prevFrame.empty())
            thisFrame.copyTo(prevFrame);

        prevFrame.copyTo(thisFrame, mask);
        try
        {
            bool result = false;
            Mat res;
            absdiff(thisFrame, prevFrame, res);
            if (!res.empty())
            {
                putText(savingFrame, // frame
                        timeinfo_str.substr(0, timeinfo_str.size()-1), // string
                        Point(100,100),
                        FONT_HERSHEY_DUPLEX,
                        0.8, // thickness
                        Scalar(0,145,145) // color
                );
                result = imwrite("/home/anne/programming/openCV/VideoCapturing/photos/" + name, savingFrame);
            }
        }
        catch (const cv::Exception& ex)
        {
            fprintf(stderr, "Exception converting image to PNG format: %s\n", ex.what());
        }
        thisFrame.copyTo(prevFrame);

        putText(thisFrame, // frame
                timeinfo_str.substr(0, timeinfo_str.size()-1), // string
                Point(100,100),
                FONT_HERSHEY_DUPLEX,
                0.8, // thickness
                Scalar(0,145,145) // color
        );
        prev_timeinfo_str = timeinfo_str;

        imshow("maskedFrame", thisFrame);
        outputVideoWithMask << thisFrame;

        waitKey(30);
    }
    return 0;
}