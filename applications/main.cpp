#include <iostream>
#include <typeinfo>
#include <vector>
#include <string>
#include <time.h>

#include "opencv2/video/background_segm.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/opencv.hpp"

#include "../include/CircularQueue.h"

using namespace cv;

/* ----------------------------------------------------------------------------------------
                  ВВОДИМ НЕОБХОДИМЫЕ ПЕРЕМЕННЫЕ ДЛЯ РАЗНЫХ КАМЕР
------------------------------------------------------------------------------------------- */
std::vector<Mat> mask(2);  // маски для разных камер
std::vector<Mat> prevFrame(2), thisFrame(2), maskedFrame(2);
std::vector<Mat> fgimg(2), bckImage(2), contouredFrame(2);
std::vector<Mat> savingFrame(2);  // сохраняемый кадр
std::vector<Mat> prevRes(2);
std::vector<double> prevCorr(2, -1.0), corr(2, -1.0), delta(2);
std::vector<double> empiricalCoeff{5, 5};   // эмпирически(экспериментально) определямый коэффициент
// по преодолении которого, мы засекаем движение в кадре
std::vector<CircularQueue<Mat>> queue(2, 5*CAP_PROP_FPS);

std::vector<std::string> prev_timeinfo_str(2), timeinfo_str(2), name(2);

int i = 0;

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

    // Compute covariance
    double covar = (im_float_1 - im1_Mean).dot(im_float_2 - im2_Mean) / n_pixels;

    return covar;
}


bool processFrame(
        int camIndex,  // индекс камеры в системе
        std::vector<VideoCapture> inputVideo,  // входной видеоканал
        std::vector<Ptr<BackgroundSubtractorMOG2>> bg,  // background
        std::vector<std::vector<std::vector<Point>>> contours,  // контуры для обозначения движения
        std::vector<std::string> path,  // путь для сохранения кадров
        VideoWriter outputVideoWithMask
)
{
    inputVideo[camIndex] >> thisFrame[camIndex]; // get a new frame from camera
    if (thisFrame[camIndex].empty()) return false; // check if videostream ended, then return

    // накладываем рисование контуров на кадре contouredFrame (thisFrame - чистый кадр)
    thisFrame[camIndex].copyTo(contouredFrame[camIndex]);
    bg[camIndex]->apply(contouredFrame[camIndex], fgimg[camIndex]);
    bg[camIndex]->getBackgroundImage(bckImage[camIndex]);
    erode(fgimg[camIndex], fgimg[camIndex], Mat());
    dilate(fgimg[camIndex], fgimg[camIndex], Mat());
    findContours(fgimg[camIndex], contours[camIndex], RETR_EXTERNAL, CHAIN_APPROX_NONE);
    drawContours(contouredFrame[camIndex], contours[camIndex], -1, Scalar(0, 0, 255), 2);

    // формируем название для картинки в виде "дата-время"
    time_t rawtime;
    time(&rawtime);
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    timeinfo_str[camIndex] = std::string(asctime(timeinfo));
    name[camIndex] = timeinfo_str[camIndex].substr(0, timeinfo_str[camIndex].size()-1);

    if (prev_timeinfo_str[camIndex] == "")
        prev_timeinfo_str[camIndex] = timeinfo_str[camIndex];

    if (prev_timeinfo_str[camIndex] == timeinfo_str[camIndex])
        i++;
    else
        i = 0;
    name[camIndex] += "-" + std::to_string(i) + ".jpg"; // Tue Nov 27 12:36:53 2018-1.jpg

    // показываем кадр с контурами движущихся объектов
//    imshow("frame" + std::to_string(camIndex), contouredFrame[camIndex]);

    // создаем кадр, который потенциально нужно сохранять в виде картинки
    // т.е. на него наложена маска
    thisFrame[camIndex].copyTo(savingFrame[camIndex]);
    if (prevFrame[camIndex].empty())
        thisFrame[camIndex].copyTo(prevFrame[camIndex]);
    prevFrame[camIndex].copyTo(thisFrame[camIndex], mask[camIndex]);  // накладывание маски на thisFrame

    prevFrame[camIndex].copyTo(contouredFrame[camIndex], mask[camIndex]);  // накладывание маски на thisFrame
    imshow("frame" + std::to_string(camIndex), contouredFrame[camIndex]);

    try  // проверка возникновения движения
    {
        bool result = false;
        Mat res;
        absdiff(thisFrame[camIndex], prevFrame[camIndex], res);  // res = разница между двумя кадрами
//        imshow("with_movement" + std::to_string(camIndex), res);
        threshold(res, res, THRESH_OTSU, THRESH_TRIANGLE, THRESH_BINARY);  // res - чб (отфильтрованная??) разница между кадрами
//        imshow("with_movement_3" + std::to_string(camIndex), res);

        if (prevRes[camIndex].empty())
            res.copyTo(prevRes[camIndex]);
        if (prevCorr[camIndex] < 0)
            prevCorr[camIndex] = corr[camIndex];

        queue[camIndex].push(savingFrame[camIndex], name[camIndex]); // pushback savingFrame в очередь

        corr[camIndex] = correlation(res, prevRes[camIndex]);
        std::cout << corr[camIndex] << std::endl;

        if ((prevCorr[camIndex] >= 0) && (corr[camIndex] >= 0))  // если как минимум три кадра уже прошло (т.е. есть 2 корреляции)
        {
            putText(savingFrame[camIndex], // frame
                    timeinfo_str[camIndex].substr(0, timeinfo_str[camIndex].size()-1), // string
                    Point(100,100),
                    FONT_HERSHEY_DUPLEX,
                    0.8, // thickness
                    Scalar(0,145,145) // color
            );
            // смотрим дельту между результатами корреляций
            delta[camIndex] = abs(corr[camIndex] - prevCorr[camIndex]);  // delta = |corr - prevCorr|
            if (delta[camIndex] > empiricalCoeff[camIndex])  // если delta большая, то засекли движение (!)
            {
                std::cout << "move" << std::endl;
                queue[camIndex].save(path[camIndex]);  // запись кадров из очереди на диск
                std::cout << queue[camIndex] << std::endl;
            }
        }
        prevCorr[camIndex] = corr[camIndex];
        prevRes[camIndex] = res;
    }
    catch (const cv::Exception& ex)
    {
        fprintf(stderr, "Exception converting image to PNG format: %s\n", ex.what());
    }
    thisFrame[camIndex].copyTo(prevFrame[camIndex]);

    putText(thisFrame[camIndex], // frame
            timeinfo_str[camIndex].substr(0, timeinfo_str[camIndex].size()-1), // string
            Point(100,100),
            FONT_HERSHEY_DUPLEX,
            0.8, // thickness
            Scalar(0,145,145) // color
    );
    prev_timeinfo_str[camIndex] = timeinfo_str[camIndex];

    imshow("maskedFrame" + std::to_string(camIndex), thisFrame[camIndex]);
    outputVideoWithMask << thisFrame[camIndex];
    return true;
}


int main(int, char**)
{
/* ----------------------------------------------------------------------------------------
        НАСТРАИВАЕМ ПОДКЛЮЧЕНИЕ К ДВУМ КАМЕРАМ (аналогично можно добавлять и другие)
------------------------------------------------------------------------------------------- */
    std::vector<std::string> outputFileWithMask(2);
    std::vector<std::string> path(2);
    // прописываем пути для 1 камеры (индекс 0)
    outputFileWithMask[0] = "/home/anne/programming/openCV/VideoCapturing/photos1/cap_withmask.avi";
    path[0] = "/home/anne/programming/openCV/VideoCapturing/photos1/";
    // прописываем пути для 2 камеры (индекс 1)
    outputFileWithMask[1] = "/home/anne/programming/openCV/VideoCapturing/photos2/cap_withmask.avi";
    path[1] = "/home/anne/programming/openCV/VideoCapturing/photos2/";

    std::vector<VideoCapture> inputVideo(2);
    // подключаемся к первой камере (индекс 0)
    inputVideo[0] = VideoCapture(0); // open camera
    if (!inputVideo[0].isOpened())  // check if we succeeded
    {
        std::cout << "Couldn't connect to camera 0" << std::endl;
        return -1;
    }
    // подключаемся ко второй камере (индекс 1)
    inputVideo[1] = VideoCapture(1); // open camera
    if (!inputVideo[1].isOpened())  // check if we succeeded
    {
        std::cout << "Couldn't connect to camera 1" << std::endl;
        return -1;
    }

/* ----------------------------------------------------------------------------------------
        НАСТРАИВАЕМ ОБРАБОТКУ ЗАДНЕГО ПЛАНА ИЗОБРАЖЕНИЯ
------------------------------------------------------------------------------------------- */
    std::vector<Ptr<BackgroundSubtractorMOG2>> bg(2);
    bg[0] = createBackgroundSubtractorMOG2();
    bg[0]->setNMixtures(3);
    bg[1] = createBackgroundSubtractorMOG2();
    bg[1]->setNMixtures(3);

/* ----------------------------------------------------------------------------------------
                                ВЫЧИСЛЯЕМ ПАРАМЕТРЫ КАДРОВ
------------------------------------------------------------------------------------------- */
    std::vector<std::vector<std::vector<Point>>> contours(2);
    std::vector<int> ex(2);
    ex[0] = static_cast<int>(inputVideo[0].get(CAP_PROP_FOURCC)); // Get Codec Type - Int form
    ex[1] = static_cast<int>(inputVideo[1].get(CAP_PROP_FOURCC)); // Get Codec Type - Int form

    // вычисляем размер кадра для первой камеры
    int width1 = (int) inputVideo[0].get(CAP_PROP_FRAME_WIDTH);
    int height1 = (int) inputVideo[0].get(CAP_PROP_FRAME_HEIGHT);
    Size S1 = Size(width1, height1); // set size of input frames
    // вычисляем размер кадра для второй камеры
    int width2 = (int) inputVideo[1].get(CAP_PROP_FRAME_WIDTH);
    int height2 = (int) inputVideo[1].get(CAP_PROP_FRAME_HEIGHT);
    Size S2 = Size(width2, height2); // set size of input frames

    VideoWriter outputVideoWithMask1; // output stream to file (masked video)
    outputVideoWithMask1.open(outputFileWithMask[0], ex[0], inputVideo[0].get(CAP_PROP_FPS), S1, true);
    if (!outputVideoWithMask1.isOpened())
    {
        std::cout  << "Could not open the output videofile for write with mask: " << outputFileWithMask[0] << std::endl;
        return -1;
    }
    VideoWriter outputVideoWithMask2; // output stream to file (masked video)
    outputVideoWithMask2.open(outputFileWithMask[1], ex[1], inputVideo[1].get(CAP_PROP_FPS), S2, true);
    if (!outputVideoWithMask2.isOpened())
    {
        std::cout  << "Could not open the output videofile for write with mask: " << outputFileWithMask[1] << std::endl;
        return -1;
    }

    mask[0] = imread("/home/anne/programming/openCV/VideoCapturing/mask0.jpg", IMREAD_COLOR);
    mask[1] = imread("/home/anne/programming/openCV/VideoCapturing/mask1.jpg", IMREAD_COLOR);

    namedWindow("frame0", WINDOW_AUTOSIZE);
    namedWindow("maskedFrame0", WINDOW_AUTOSIZE);
//    namedWindow("with_movement0", WINDOW_AUTOSIZE);
//    namedWindow("with_movement20", WINDOW_AUTOSIZE);
//    namedWindow("with_movement30", WINDOW_AUTOSIZE);

    namedWindow("frame1", WINDOW_AUTOSIZE);
    namedWindow("maskedFrame1", WINDOW_AUTOSIZE);
//    namedWindow("with_movement1", WINDOW_AUTOSIZE);
//    namedWindow("with_movement21", WINDOW_AUTOSIZE);
//    namedWindow("with_movement31", WINDOW_AUTOSIZE);

    // надо добавить проверку свободного места на диске
    for (;;)
    {

        processFrame(0, inputVideo, bg, contours, path, outputVideoWithMask1);
        processFrame(1, inputVideo, bg, contours, path, outputVideoWithMask2);

        if (waitKey(30) == 27) { // нажата ESC => finish program
            break;
        }
    }
    destroyWindow("frame");
    destroyWindow("maskedFrame");

    return 0;
}