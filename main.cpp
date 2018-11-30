#include <iostream>
#include <vector>
#include <string>
//#include <vector>
#include <time.h>
//#include <cv.h>

#include <typeinfo>

#include "opencv2/highgui/highgui.hpp"
//#include <opencv2/core/core.hpp>

//#include "opencv2/videoio.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/video/background_segm.hpp"
//#include <opencv2/videoio.hpp>
//#include <opencv2/imgproc/imgproc.hpp>
#include "include/CircularQueue.h"

using namespace cv;

double correlation(cv::Mat& image_1, cv::Mat& image_2)
{
    std::cout << ".";

// convert data-type to "float"
    cv::Mat im_float_1;
    image_1.convertTo(im_float_1, CV_32F);
    std::cout << ".";
    cv::Mat im_float_2;
    image_2.convertTo(im_float_2, CV_32F);
    std::cout << ".";

    int n_pixels = im_float_1.rows * im_float_1.cols;

// Compute mean and standard deviation of both images
    cv::Scalar im1_Mean, im1_Std, im2_Mean, im2_Std;
    meanStdDev(im_float_1, im1_Mean, im1_Std);
    std::cout << ".";
    meanStdDev(im_float_2, im2_Mean, im2_Std);
    std::cout << ".";

// Compute covariance and correlation coefficient
    double covar = (im_float_1 - im1_Mean).dot(im_float_2 - im2_Mean) / n_pixels;
//    double correl = covar / (im1_Std[0] * im2_Std[0]);

    return covar;
}

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

    Ptr<BackgroundSubtractorMOG2> bg = createBackgroundSubtractorMOG2();
    bg->setNMixtures(3);
    std::vector<std::vector<Point>> contours;

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

    Mat mask = imread("/home/anne/programming/openCV/VideoCapturing/mask.jpg", IMREAD_COLOR);
    Mat prevFrame, thisFrame, maskedFrame;
    Mat fgimg, bckImage, contouredFrame;
    Mat savingFrame;  // сохраняемый кадр
    Mat prevRes;
    double prevCorr = -1.0;
    double corr = -1.0;
    double delta;
    double empiricalCoeff = 5;  // эмпирически(экспериментально) определямый коэффициент
                                 // по преодолении которого, мы засекаем движение в кадре
    bool movementDetected = false;

    std::string name = "";
    time_t rawtime0;
    time(&rawtime0);
    struct tm * prev_timeinfo = localtime(&rawtime0);
    std::string timeinfo_str = "";
    std::string prev_timeinfo_str = std::string(asctime(prev_timeinfo));;
    int i = 0;

    namedWindow("frame", WINDOW_AUTOSIZE);
    namedWindow("maskedFrame", WINDOW_AUTOSIZE);
    namedWindow("with_movement", WINDOW_AUTOSIZE);
    namedWindow("with_movement_2", WINDOW_AUTOSIZE);
    namedWindow("with_movement_3", WINDOW_AUTOSIZE);

    // надо добавить проверку свободного места на диске
    for (;;)
    {
        inputVideo >> thisFrame; // get a new frame from camera
        if (thisFrame.empty()) break; // check if videostream ended, then break

        // накладываем рисование контуров на кадре contouredFrame (thisFrame - чистый кадр)
        thisFrame.copyTo(contouredFrame);
        bg->apply(contouredFrame, fgimg);
        bg->getBackgroundImage(bckImage);
        erode(fgimg, fgimg, Mat());
        dilate(fgimg, fgimg, Mat());
        findContours(fgimg, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);
        drawContours(contouredFrame, contours, -1, Scalar(0, 0, 255), 2);

        // формируем название для картинки в виде "дата-время"
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
        name += "-" + std::to_string(i) + ".jpg"; // Tue Nov 27 12:36:53 2018-1.jpg

        // показываем кадр с контурами движущихся объектов
        imshow("frame", contouredFrame);
        // соханяем чистый кадр в cap.avi
        // outputVideo << thisFrame;

        // сохдаем кадр, который потенциально нужно сохранять в виде картинки
        // т.е. на него наложена маска
        thisFrame.copyTo(savingFrame);
        if (prevFrame.empty())
            thisFrame.copyTo(prevFrame);
        prevFrame.copyTo(thisFrame, mask);  // накладывание маски на thisFrame

        try  // проверка возникновения движения
        {
            bool result = false;
            Mat res;
            absdiff(thisFrame, prevFrame, res);  // res = разница между двумя кадрами
            imshow("with_movement", res);
            threshold(res, res, THRESH_OTSU, THRESH_TRIANGLE, THRESH_BINARY);  // res - чб (отфильтрованная??) разница между кадрами
            imshow("with_movement_3", res);

            if (prevRes.empty())
                res.copyTo(prevRes);
            if (prevCorr < 0)
                prevCorr = corr;

            corr = correlation(res, prevRes);
            std::cout << corr << std::endl;

            if ((prevCorr >= 0) && (corr >= 0))  // если как минимум три кадра уже прошло (т.е. есть 2 корреляции)
            {
                // смотрим дельту между результатами корреляций
                delta = abs(corr - prevCorr);  // delta = |corr - prevCorr|
                if (delta > empiricalCoeff)  // если delta большая, то засекли движение (!)
                {
                    movementDetected = true;  // установили флаг
                    putText(savingFrame, // frame
                            timeinfo_str.substr(0, timeinfo_str.size()-1), // string
                            Point(100,100),
                            FONT_HERSHEY_DUPLEX,
                            0.8, // thickness
                            Scalar(0,145,145) // color
                    );
                    std::cout << "move" << std::endl;
                    // --------------------------------------------------------------------------
                    // запись кадров из очереди на диск  // imwrite("/home/anne/programming/openCV/VideoCapturing/photos/" + name, savingFrame);
                    // pushback savingFrame в очередь
                    // --------------------------------------------------------------------------
                }
            }

            prevCorr = corr;
            prevRes = res;
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

        if (waitKey(30) == 27) { // нажата ESC => finish program
            break;
        }
    }
    destroyWindow("frame");
    destroyWindow("maskedFrame");

    return 0;
}