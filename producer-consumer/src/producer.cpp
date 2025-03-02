#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <amqpcpp/linux_tcp.h>
#include <ev.h>

#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

int main() {
    struct ev_loop *evloop = EV_DEFAULT;
    AMQP::Address address("amqp://user52:pwd52@host.docker.internal:5672/");
    AMQP::LibEvHandler handler(evloop);
    AMQP::TcpConnection connection(&handler, address);
    AMQP::TcpChannel channel(&connection);

    std::string path = RESOURCES_DIR "/cute_dog.jpg";
    std::vector<uchar> img_data;

    cv::Mat image = cv::imread(path);
    if (image.empty()) {
        std::cerr << "Ошибка: не удалось загрузить изображение " << path << std::endl;
        return 1;
    }

    cv::imencode(".jpg", image, img_data);

    channel.declareQueue("image_queue", AMQP::durable)
        .onSuccess([&connection, &channel, &img_data](const std::string &name, uint32_t, uint32_t) {
            channel.publish("", name, reinterpret_cast<const char *>(img_data.data()),
                            img_data.size());
            std::cout << "Изображение отправлено в очередь: " << name << std::endl;
            // connection.close();
        })
        .onError(
            [](const char *message) { std::cerr << "Ошибка отправки: " << message << std::endl; });

    channel.declareQueue("processed_image_queue", AMQP::durable)
        .onSuccess([&channel](const std::string &name, uint32_t, uint32_t) {
            channel.consume(name, AMQP::noack)
                .onReceived([](const AMQP::Message &message, uint64_t, bool) {
                    std::vector<uchar> img_data(message.body(),
                                                message.body() + message.bodySize());
                    cv::Mat img = cv::imdecode(img_data, cv::IMREAD_COLOR);
                    if (img.empty()) {
                        std::cerr << "Ошибка: не удалось декодировать полученное изображение"
                                  << std::endl;
                        return;
                    }
                    cv::imwrite("output.jpg", img);
                    std::cout << "Обработанное изображение сохранено как output.jpg" << std::endl;
                });
        })
        .onError([](const char *message) {
            std::cerr << "Ошибка при получении обработанного изображения: " << message << std::endl;
        });

    ev_run(evloop, 0);
    return 0;
}
