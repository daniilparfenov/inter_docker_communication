#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <amqpcpp/linux_tcp.h>
#include <ev.h>

#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

int main() {
    struct ev_loop *evloop = EV_DEFAULT;
    AMQP::Address address("amqp://user52:pwd52@host.docker.internal:5672/");
    AMQP::LibEvHandler handler(evloop);
    AMQP::TcpConnection connection(&handler, address);
    AMQP::TcpChannel channel(&connection);

    channel.declareQueue("image_queue", AMQP::durable)
        .onSuccess([&channel](const std::string &name, uint32_t, uint32_t) {
            channel.consume(name, AMQP::noack)
                .onReceived([&channel](const AMQP::Message &message, uint64_t, bool) {
                    std::vector<uchar> img_data(message.body(),
                                                message.body() + message.bodySize());
                    cv::Mat img = cv::imdecode(img_data, cv::IMREAD_COLOR);

                    if (img.empty()) {
                        std::cerr << "Ошибка: не удалось декодировать изображение" << std::endl;
                        return;
                    }

                    cv::Mat inverted;
                    cv::bitwise_not(img, inverted);

                    std::vector<uchar> encoded_img;
                    cv::imencode(".jpg", inverted, encoded_img);

                    channel.publish("", "processed_image_queue",
                                    reinterpret_cast<const char *>(encoded_img.data()),
                                    encoded_img.size());
                    std::cout << "Изображение обработано и отправлено обратно." << std::endl;
                });
        })
        .onError([](const char *message) {
            std::cerr << "Ошибка при получении: " << message << std::endl;
        });

    ev_run(evloop, 0);
    return 0;
}
