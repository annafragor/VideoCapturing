#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <iostream>
#include <string>

using namespace std;
using namespace cv;

std::string filename = "/home/anne/programming/openCV/VideoCapturing/";

Mat src, img1, mask, final;

Point point;
vector<Point> pts;
int drag = 0;
int var = 0;
int flag = 0;

void mouseHandler(int, int, int, int, void*);

void mouseHandler(int event, int x, int y, int, void*)
{
    if (event == EVENT_LBUTTONDOWN && !drag)
    {
        if (flag == 0)
        {
            if (var == 0)
                img1 = src.clone();
            point = Point(x, y);
            circle(img1, point, 2, Scalar(0, 0, 255), -1, 8, 0);
            pts.push_back(point);
            var++;
            drag  = 1;

            if (var > 1)
                line(img1,pts[var-2], point, Scalar(0, 0, 255), 2, 8, 0);

            imshow("Source", img1);
        }
    }

    if (event == EVENT_LBUTTONUP && drag)
    {
        imshow("Source", img1);
        drag = 0;
    }

    if (event == EVENT_RBUTTONDOWN)
    {
        flag = 1;
        img1 = src.clone();

        if (var != 0)
        {
            polylines(img1, pts, 1, Scalar(0,0,0), 2, 8, 0);
        }

        imshow("Source", img1);
    }

    if (event == EVENT_RBUTTONUP)
    {
        flag = var;
        final = Mat::zeros(src.size(), CV_8UC3);
        mask = Mat::zeros(src.size(), CV_8U);

        vector<vector<Point> > vpts;
        vpts.push_back(pts);
        fillPoly(mask, vpts, Scalar(255, 255, 255), 8, 0);
        bitwise_and(src, src, final, mask);
        imshow("Mask", mask);
        imwrite(filename, mask);
        imshow("Result", final);
        imshow("Source", img1);
    }

    if (event == EVENT_MBUTTONDOWN)
    {
        pts.clear();
        var = 0;
        drag = 0;
        flag = 0;
        imshow("Source", src);
    }
}


int main(int argc, char **argv)
{
    if (argc == 2)
    {
//        std::cout << argv[1] << std::endl;
        filename += "mask" + std::string(argv[1]) + ".jpg";
        std::cout << "result mask is stored in: " << filename << std::endl << std::endl;

        cout << "left mouse button - set a point to create mask shape" << endl
             << "right mouse button - create mask from points" << endl
             << "middle mouse button - reset" << endl;

        int id = atoi(argv[1]);
        VideoCapture inputVideo(id);
        if (!inputVideo.isOpened())  // check if we succeeded
        {
            std::cout << "Couldn't connect to camera" << std::endl;
            return -1;
        }

        inputVideo >> src;
        if (src.empty()) return 0;

        namedWindow("Source", WINDOW_AUTOSIZE);
        setMouseCallback("Source", mouseHandler, NULL);
        imshow("Source", src);
        waitKey(0);
    }
    else
    {
        cout << "You should pass ONE argument: ID of your camera." << endl;
    }
    return 0;
}