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
int welcomeSocket_fd = -1, connectionSocket_fd = -1;

// Отображает ошибку, связанную с подключением клиента, и закрывает сокет для соединения с ним
void connectionError(const char *msg) {
    perror(msg);
    if (connectionSocket_fd != -1) close(connectionSocket_fd);
}

// Обработчик сигнала
void handle_signal(int sig) {
    if (sig == SIGINT) {
        printf("\n[Error] Received SIGINT. Exiting...\n");
        if (welcomeSocket_fd != -1) close(welcomeSocket_fd);
        if (connectionSocket_fd != -1) close(connectionSocket_fd);
        exit(0);
    }
}

// Функция для гарантированной отправки всех данных
int sendAll(int socket, const void *buffer, size_t length) {
    const char *data = static_cast<const char *>(buffer);
    size_t totalSent = 0;
    while (totalSent < length) {
        int sent = send(socket, data + totalSent, length - totalSent, 0);
        if (sent < 0) {
            return -1;
        }
        totalSent += sent;
    }
    std::cout << "Total bytes sent: " << totalSent << std::endl;
    return totalSent;
}

// Функция для гарантированного получения всех данных
int recvAll(int socket, void *buffer, size_t length) {
    char *data = static_cast<char *>(buffer);
    size_t totalReceived = 0;
    while (totalReceived < length) {
        int received = recv(socket, data + totalReceived, length - totalReceived, 0);
        if (received == 0) {
            break;
        }
        if (received < 0) {
            return -1;
        }
        totalReceived += received;
    }
    std::cout << "Total bytes received: " << totalReceived << std::endl;
    return totalReceived;
}

// Обрабатывает клиента
void handleClient() {
    int n = 1;
    while (n > 0) {
        // Получение размера данных для обработки
        size_t dataSize;
        std::cout << "Image data size receiving..." << std::endl;
        n = recvAll(connectionSocket_fd, &dataSize, sizeof(dataSize));
        if (n == 0) {
            std::cout << "Client finished data sending" << std::endl;
            break;
        }
        if (n < 0) {
            connectionError("ERROR image data size receiving");
            break;
        }

        // Получение данных для обработки
        std::vector<uchar> buffer(dataSize);
        std::cout << "Image data receiving..." << std::endl;
        n = recvAll(connectionSocket_fd, buffer.data(), dataSize);
        if (n == 0) {
            std::cout << "Client finished data sending" << std::endl;
            break;
        }
        if (n < 0) {
            connectionError("ERROR image data receiving");
            break;
        }

        // Десериализация полученного изображения
        std::cout << "Image deserialization..." << std::endl;
        cv::Mat imagePart = cv::imdecode(buffer, cv::IMREAD_COLOR);
        if (imagePart.empty()) {
            connectionError("ERROR deserialization image");
        }

        // Инвертируем изображение
        std::cout << "Image inverting..." << std::endl;
        invertImagePart(imagePart);

        // Сериализация и отправка клиенту
        cv::imencode(".jpg", imagePart, buffer);
        dataSize = buffer.size();
        std::cout << "Inverted image data size sending..." << std::endl;
        n = sendAll(connectionSocket_fd, &dataSize, sizeof(dataSize));
        if (n < 0) {
            connectionError("ERROR inverted image data size sending");
            break;
        }
        std::cout << "Inverted image data sending..." << std::endl;
        n = sendAll(connectionSocket_fd, buffer.data(), buffer.size());
        if (n < 0) {
            connectionError("ERROR inverted image data sending");
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    std::cout << "SERVER" << std::endl;
    // Обработчик сигнала SIGINT, чтобы корректно закрывать сокет при остановке процесса через
    // терминал
    signal(SIGINT, handle_signal);

    // Номер порта для подключения и переменная для кол-ва полученных/отправленных байт
    int portno;

    socklen_t clilen;  // Размер адресса клиента

    // Структуры для хранения интернет адресов сервера и клиента
    sockaddr_in serv_addr, cli_addr;

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port to listen provided\n");
        exit(1);
    }

    // Создаем welcome-socket
    welcomeSocket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (welcomeSocket_fd < 0) {
        perror("ERROR opening welcome socket");
        exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));  // Инициализация адреса нулями
    portno = atoi(argv[1]);                        // Считываем номер порта из консольныхаргументов

    // Заполнение данных о адресе сервера
    serv_addr.sin_family = AF_INET;          // Тип домена
    serv_addr.sin_addr.s_addr = INADDR_ANY;  // IP хоста
    serv_addr.sin_port = htons(portno);  // Порт, перевод из host byte order в network byte order!

    // Связываем сокет с адресом
    if (bind(welcomeSocket_fd, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding welcome socket");
        if (welcomeSocket_fd != -1) close(welcomeSocket_fd);
        exit(1);
    }
    // Ожидание подключения клиента
    listen(welcomeSocket_fd, 5);  // Сокет будет использоваться как пассивный (ожидать подключения)

    // Основной цикл сервера
    while (1) {
        clilen = sizeof(cli_addr);
        // Блокирует процесс, пока клиент не подлючится, создается connection сокет
        connectionSocket_fd = accept(welcomeSocket_fd, (sockaddr *)&cli_addr, &clilen);
        if (connectionSocket_fd < 0) {
            perror("ERROR accepting new client");
            continue;
        }
        std::cout << "New client connected" << std::endl;

        handleClient();
        if (connectionSocket_fd != -1) close(connectionSocket_fd);
    }

    if (connectionSocket_fd != -1) close(connectionSocket_fd);
    if (welcomeSocket_fd != -1) close(welcomeSocket_fd);
    return 0;
}
