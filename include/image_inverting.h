#ifndef INCLUDE_IMAGE_INVERTING_H
#define INCLUDE_IMAGE_INVERTING_H

#include <opencv2/opencv.hpp>
#include <vector>

// Структура задачи для потребителей
struct Task {
    cv::Mat imagePart;
    int rowOffset;
};

void divideImage(cv::Mat& image, size_t taskSize, std::vector<Task>& taskQueue);  // Producer

void invertImagePart(cv::Mat& imagePart);  // Consumer

#endif  // INCLUDE_IMAGE_INVERTING_H
