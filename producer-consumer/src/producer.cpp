#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <amqpcpp/linux_tcp.h>
#include <ev.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

namespace fs = std::filesystem;

int main() {
    // Создание цикла событий libev
    struct ev_loop *evloop = EV_DEFAULT;

    // Создание соединения с RabbitMQ
    AMQP::Address address("amqp://user52:pwd52@host.docker.internal:5672/");
    AMQP::LibEvHandler handler(evloop);
    AMQP::TcpConnection connection(&handler, address);
    AMQP::TcpChannel channel(&connection);

    // Отправка изображения в очередь
    std::vector<std::string> img_paths;
    std::vector<uchar> img_data;

    // Парсинг путей к изображениям
    try {
        for (const auto &file : fs::directory_iterator(INPUT_DIR)) {
            if (fs::is_regular_file(file.path())) {  // Проверяем, что это именно файл
                img_paths.push_back(file.path());
            }
        }
    } catch (const fs::filesystem_error &e) {
        std::cerr << "Ошибка: " << e.what() << '\n';
    }

    // Создание очереди и отправка сообщений
    channel.declareQueue("image_queue", AMQP::durable)
        .onSuccess([&channel, &img_paths, &img_data](const std::string &name, uint32_t, uint32_t) {
            for (const auto &img_path : img_paths) {
                cv::Mat image = cv::imread(img_path);
                if (image.empty()) {
                    std::cerr << "Ошибка: не удалось загрузить изображение " << img_path
                              << std::endl;
                    return 1;
                }
                cv::imencode(".jpg", image, img_data);

                // Отправка сообщений в очередь

                channel.publish("", name, reinterpret_cast<const char *>(img_data.data()),
                                img_data.size());
                std::cout << "Изображение " << fs::path(img_path).filename()
                          << " отправлено в очередь: " << name << std::endl;
            }
        })
        .onError(
            [](const char *message) { std::cerr << "Ошибка отправки: " << message << std::endl; });

    // Получение обработанного изображения
    channel.declareQueue("processed_image_queue", AMQP::durable)
        .onSuccess(
            [&connection, &channel, &img_paths](const std::string &name, uint32_t, uint32_t) {
                channel.consume(name, AMQP::noack)
                    .onReceived([&connection, &img_paths](const AMQP::Message &message, uint64_t,
                                                          bool) {
                        static int receivedImgCount = 0;
                        /*
                         * Обработка полученного изображения
                         * В данном случае полученное изображение сохраняется в файл output.jpg
                         */

                        // Декодирование изображения
                        std::vector<uchar> img_data(message.body(),
                                                    message.body() + message.bodySize());
                        cv::Mat img = cv::imdecode(img_data, cv::IMREAD_COLOR);
                        if (img.empty()) {
                            std::cerr << "Ошибка: не удалось декодировать полученное изображение"
                                      << std::endl;
                            return 1;
                        }

                        // Сохранение изображения
                        std::string outputPath =
                            OUTPUT_DIR "/output" + std::to_string(receivedImgCount) + ".jpg";
                        cv::imwrite(outputPath, img);
                        std::cout << "Обработанное изображение сохранено как " << outputPath
                                  << std::endl;
                        receivedImgCount++;

                        if (receivedImgCount == img_paths.size())
                            connection.close();  // Закрытие соединения
                    });
            })
        .onError([](const char *message) {
            std::cerr << "Ошибка при получении обработанного изображения: " << message << std::endl;
        });

    // Запуск цикла событий libev
    ev_run(evloop, 0);
    return 0;
}
