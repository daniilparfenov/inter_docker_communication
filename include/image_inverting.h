#ifndef INCLUDE_IMAGE_INVERTING_H
#define INCLUDE_IMAGE_INVERTING_H

#include <opencv2/opencv.hpp>
#include <vector>

// Структура задачи - куска изображения для обработки
struct Task {
    cv::Mat imagePart;
    int rowOffset;
};

// Делит image на строки размера taskSize и записывает в tasks
void divideImage(cv::Mat& image, size_t taskSize, std::vector<Task>& tasks);

// Инвертирует изображение
void invertImagePart(cv::Mat& imagePart);  // Consumer

#endif  // INCLUDE_IMAGE_INVERTING_H
