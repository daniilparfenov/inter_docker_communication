#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <amqpcpp/linux_tcp.h>
#include <ev.h>

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

    // Получение изображения из очереди
    channel.declareQueue("image_queue", AMQP::durable)
        .onSuccess([&channel](const std::string &name, uint32_t, uint32_t) {
            channel.consume(name, AMQP::noack)
                .onReceived([&channel](const AMQP::Message &message, uint64_t, bool) {
                    /*
                     * Обработка полученного изображения
                     * В данном случае полученное изображение инвертируется
                     * и отправляется обратно в очередь
                     */
                    std::vector<uchar> img_data(message.body(),
                                                message.body() + message.bodySize());
                    cv::Mat img = cv::imdecode(img_data, cv::IMREAD_COLOR);

                    if (img.empty()) {
                        std::cerr << "Ошибка: не удалось декодировать изображение" << std::endl;
                        return;
                    }

                    cv::Mat inverted;                // Инвертированное изображение
                    cv::bitwise_not(img, inverted);  // Инвертирование изображения

                    std::vector<uchar> encoded_img;               // Кодированное изображение
                    cv::imencode(".jpg", inverted, encoded_img);  // Кодирование изображения

                    // Отправка обработанного изображения в очередь
                    channel.publish("", "processed_image_queue",
                                    reinterpret_cast<const char *>(encoded_img.data()),
                                    encoded_img.size());
                    std::cout << "Изображение обработано и отправлено обратно." << std::endl;
                });
        })
        .onError([](const char *message) {
            std::cerr << "Ошибка при получении: " << message << std::endl;
        });

    // Запуск цикла событий libev
    ev_run(evloop, 0);
    return 0;
}
