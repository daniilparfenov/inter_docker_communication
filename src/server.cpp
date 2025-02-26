#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

#include "image_inverting.h"

int sockfd, newsockfd;

void error(const char *msg) {
    perror(msg);
    exit(1);
}

// Обработчик сигнала
void handle_signal(int sig) {
    if (sig == SIGINT) {
        printf("\n[Error] Received SIGINT. Exiting...\n");
        close(sockfd);
        close(newsockfd);
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handle_signal);
    int portno;
    socklen_t clilen;
    size_t buffSize = sizeof(Task) * 16;
    std::vector<Task> buffer(buffSize);
    struct sockaddr_in serv_addr, cli_addr;
    int n = 1;

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    // Создаем welcome-socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    // Связываем сокет с адресом
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    // Ждем клиента
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0) error("ERROR on accept");

    while (1) {
        // Получение размера данных
        size_t dataSize;
        n = recv(newsockfd, &dataSize, sizeof(dataSize), 0);
        if (n == 0) break;
        if (n < 0) error("ERROR reading from socket");

        // Получение данных
        std::vector<uchar> buffer(dataSize);
        n = recv(newsockfd, buffer.data(), dataSize, 0);
        if (n < 0) error("ERROR reading from socket");

        // Десериализация изображения
        cv::Mat imagePart = cv::imdecode(buffer, cv::IMREAD_COLOR);
        if (imagePart.empty()) {
            error("ERROR deserialization image");
        }

        // Инвертируем
        invertImagePart(imagePart);

        // Сериализация и отправка клиенту
        cv::imencode(".jpg", imagePart, buffer);
        dataSize = buffer.size();
        n = send(newsockfd, &dataSize, sizeof(dataSize), 0);
        if (n < 0) error("ERROR writing to socket");
        n = send(newsockfd, buffer.data(), buffer.size(), 0);
        if (n < 0) error("ERROR writing to socket");
    }

    close(newsockfd);
    close(sockfd);
    return 0;
}
