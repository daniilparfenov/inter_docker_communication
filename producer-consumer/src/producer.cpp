#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <amqpcpp/linux_tcp.h>
#include <ev.h>

#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

int main() {
    // Создание цикла событий libev
    struct ev_loop *evloop = EV_DEFAULT;

    // Создание соединения с RabbitMQ
    AMQP::Address address("amqp://user52:pwd52@host.docker.internal:5672/");
    AMQP::LibEvHandler handler(evloop);
    AMQP::TcpConnection connection(&handler, address);
    AMQP::TcpChannel channel(&connection);

    // Отправка изображения в очередь
    std::string path = RESOURCES_DIR "/cute_dog.jpg";
    std::vector<uchar> img_data;

    cv::Mat image = cv::imread(path);
    if (image.empty()) {
        std::cerr << "Ошибка: не удалось загрузить изображение " << path << std::endl;
        return 1;
    }

    cv::imencode(".jpg", image, img_data);

    // Создание очереди и отправка сообщений
    channel.declareQueue("image_queue", AMQP::durable)
        .onSuccess([&channel, &img_data](const std::string &name, uint32_t, uint32_t) {
            /*
             * Отправка сообщений в очередь
             * В данном случае отправляется 1 изображение
             */
            channel.publish("", name, reinterpret_cast<const char *>(img_data.data()),
                            img_data.size());
            std::cout << "Изображение отправлено в очередь: " << name << std::endl;
        })
        .onError(
            [](const char *message) { std::cerr << "Ошибка отправки: " << message << std::endl; });

    // Получение обработанного изображения
    channel.declareQueue("processed_image_queue", AMQP::durable)
        .onSuccess([&connection, &channel](const std::string &name, uint32_t, uint32_t) {
            channel.consume(name, AMQP::noack)
                .onReceived([&connection](const AMQP::Message &message, uint64_t, bool) {
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
                        return;
                    }

                    // Сохранение изображения
                    cv::imwrite("output.jpg", img);
                    std::cout << "Обработанное изображение сохранено как output.jpg" << std::endl;
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
