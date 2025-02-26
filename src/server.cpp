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

// Файловые дескрипторы для welcoming сокета и connection сокета
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
    // Обработчик сигнала SIGINT, чтобы корректно закрывать сокет при остановке процесса через
    // терминал
    signal(SIGINT, handle_signal);

    // Номер порта для подключения и переменная для кол-ва полученных/отправленных байт
    int portno, n;

    socklen_t clilen;  // Размер адресса клиента

    // Структуры для хранения интернет адресов сервера и клиента
    sockaddr_in serv_addr, cli_addr;

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    // Создаем welcome-socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));  // Инициализация адреса нулями
    portno = atoi(argv[1]);                        // Считываем номер порта из консольныхаргументов

    // Заполнение данных о адресе сервера
    serv_addr.sin_family = AF_INET;          // Тип домена
    serv_addr.sin_addr.s_addr = INADDR_ANY;  // IP хоста
    serv_addr.sin_port = htons(portno);  // Порт, перевод из host byte order в network byte order!

    // Связываем сокет с адресом
    if (bind(sockfd, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) error("ERROR on binding");

    // Ожидание подключения клиента
    listen(sockfd, 5);  // Сокет будет использоваться как пассивный (ожидать подключения)
    clilen = sizeof(cli_addr);
    // Блокирует процесс, пока клиент не подлючится, создается connection сокет
    newsockfd = accept(sockfd, (sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0) error("ERROR on accept");

    // Общение с клиентом
    while (1) {
        // Получение размера данных для обработки
        size_t dataSize;
        n = recv(newsockfd, &dataSize, sizeof(dataSize), 0);
        if (n == 0) break;
        if (n < 0) error("ERROR reading from socket");

        // Получение данных для обработки
        std::vector<uchar> buffer(dataSize);
        n = recv(newsockfd, buffer.data(), dataSize, 0);
        if (n < 0) error("ERROR reading from socket");

        // Десериализация полученного изображения
        cv::Mat imagePart = cv::imdecode(buffer, cv::IMREAD_COLOR);
        if (imagePart.empty()) {
            error("ERROR deserialization image");
        }

        // Инвертируем изображение
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
