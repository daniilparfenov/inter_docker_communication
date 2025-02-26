#include "image_inverting.h"

#include <algorithm>
#include <opencv2/opencv.hpp>
#include <vector>

// Функция-производитель. Делит изображение на строки размера taskSize
void divideImage(cv::Mat& image, size_t taskSize, std::vector<Task>& taskQueue) {
    for (int row = 0; row < image.rows; row += taskSize) {
        int rowsToProcess = std::min((int)taskSize, image.rows - row);
        cv::Mat imageBlock = image(cv::Range(row, row + rowsToProcess), cv::Range::all());
        taskQueue.push_back(Task{imageBlock, row});
    }
}

// Функция-потребитель. Инвертирует в строке изображения каждый пиксель
void invertImagePart(cv::Mat& imagePart) {
    // Инверсия каждого пикселя в imagePart
    for (int i = 0; i < imagePart.rows; ++i) {
        for (int j = 0; j < imagePart.cols; ++j) {
            cv::Vec3b& pixel = imagePart.at<cv::Vec3b>(i, j);
            pixel = cv::Vec3b(255 - pixel[0], 255 - pixel[1], 255 - pixel[2]);
        }
    }
}
