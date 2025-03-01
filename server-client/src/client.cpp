#include <netdb.h>
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
#include <string>

#include "image_inverting.h"

// Файловый дескриптор сокета клиента
int sockfd;

void error(const char *msg) {
    perror(msg);
    exit(0);
}

// Обработчик сигнала
void handle_signal(int sig) {
    if (sig == SIGINT) {
        printf("\n[Error] Received SIGINT. Exiting...\n");
        close(sockfd);
        exit(0);
    }
}

// Функция для гарантированной отправки данных
void sendAll(int socket, const void *buffer, size_t length) {
    const char *data = static_cast<const char *>(buffer);
    size_t totalSent = 0;
    while (totalSent < length) {
        int sent = send(socket, data + totalSent, length - totalSent, 0);
        if (sent < 0) {
            error("ERROR writing to socket");
        }
        totalSent += sent;
    }
    std::cout << "Total bytes sent: " << totalSent << std::endl;
}

// Функция для гарантированного получения данных
void recvAll(int socket, void *buffer, size_t length) {
    char *data = static_cast<char *>(buffer);
    size_t totalReceived = 0;
    while (totalReceived < length) {
        int received = recv(socket, data + totalReceived, length - totalReceived, 0);
        if (received == 0) {
            break;
        }
        if (received < 0) {
            error("ERROR reading from socket");
        }
        totalReceived += received;
    }
    std::cout << "Total bytes received: " << totalReceived << std::endl;
}

int main(int argc, char *argv[]) {
    // Обработчик сигнала SIGINT, чтобы корректно закрывать сокет при остановке процесса через
    // терминал
    signal(SIGINT, handle_signal);

    int portno;             // Номер порта
    sockaddr_in serv_addr;  // Структура для хранения адреса сервера, к которому подключимся
    hostent *server;        // Структура для хранения информации о хосте

    if (argc < 3) {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }

    portno = atoi(argv[2]);  // Номер порта для подключения берется из консольных аргументов
    sockfd = socket(AF_INET, SOCK_STREAM, 0);  // Создание сокета клиента
    if (sockfd < 0) error("ERROR opening socket");

    server = gethostbyname(argv[1]);  // По имени хоста в интернете получаем информацию о нем
    if (server == NULL) error("ERROR, no provided host");

    // Заполняем информацию о адресе сервера полученными данными
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);  // Перевод из host byte order в network byte order!

    // Соединяемся с сервером
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    std::cout << "Server connection success" << std::endl;

    // Загрузка изображения для обработки
    std::string path = RESOURCES_DIR "/cute_dog.jpg";
    cv::Mat image = cv::imread(path);
    if (image.empty()) error("ERROR loading image");
    std::cout << "Image loading success" << std::endl;

    // Делим изображение
    std::vector<Task> dividedImage;
    divideImage(image, 16, dividedImage);
    std::cout << "Image dividing success" << std::endl;

    // Отправка кусков изображения серверу для обработки и запись результата в image
    for (Task &task : dividedImage) {
        // Сериализация куска изображения в формат JPEG
        std::vector<uchar> buffer;
        std::cout << "Image serialization..." << std::endl;
        cv::imencode(".jpg", task.imagePart, buffer);

        // Отправка размера данных
        size_t dataSize = buffer.size();
        std::cout << "Image part data size sending..." << std::endl;
        sendAll(sockfd, &dataSize, sizeof(dataSize));

        // Отправка данных
        std::cout << "Image part data sending..." << std::endl;
        sendAll(sockfd, buffer.data(), buffer.size());

        // Получение инвертированного куска изображения
        std::cout << "Inverted image part data size receiving..." << std::endl;
        recvAll(sockfd, &dataSize, sizeof(dataSize));
        buffer.resize(dataSize);
        std::cout << "Inverted image part data receiving..." << std::endl;
        recvAll(sockfd, buffer.data(), dataSize);

        // Десериализация куска изображения
        std::cout << "Image deserialization..." << std::endl;
        task.imagePart = cv::imdecode(buffer, cv::IMREAD_COLOR);
        if (task.imagePart.empty()) error("ERROR deserialization image");

        // Запись обработанного куска в результат
        task.imagePart.copyTo(image(cv::Range(task.rowOffset, task.rowOffset + task.imagePart.rows),
                                    cv::Range::all()));
    }

    // Сохранение изображения
    std::string filename = "output.jpg";
    if (cv::imwrite(filename, image)) {
        std::cout << "The image saved in" << filename << std::endl;
    } else {
        std::cerr << "ERROR image saving" << std::endl;
    }

    close(sockfd);
    return 0;
}
