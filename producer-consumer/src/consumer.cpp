#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <amqpcpp/linux_tcp.h>
#include <ev.h>

#include <iostream>

int main() {
    struct ev_loop *evloop = EV_DEFAULT;

    AMQP::Address address("amqp://user52:pwd52@host.docker.internal:5672/");
    AMQP::LibEvHandler handler(evloop);
    AMQP::TcpConnection connection(&handler, address);
    AMQP::TcpChannel channel(&connection);

    channel.declareQueue("task_queue", AMQP::durable)
        .onSuccess([&channel](const std::string &name, uint32_t, uint32_t) {
            channel.consume(name, AMQP::noack)
                .onReceived([](const AMQP::Message &message, uint64_t, bool) {
                    std::cout << "Получено сообщение из очереди: "
                              << std::string(message.body(), message.body() + message.bodySize())
                              << std::endl;
                });
        })
        .onError([](const char *message) {
            std::cout << "Ошибка при получении сообщения: " << message << std::endl;
        });

    ev_run(evloop, 0);
    return 0;
}
