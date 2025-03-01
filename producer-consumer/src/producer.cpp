#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <amqpcpp/linux_tcp.h>
#include <ev.h>
#include <unistd.h>

#include <iostream>
#include <string>

int main() {
    struct ev_loop *evloop = EV_DEFAULT;

    AMQP::Address address("amqp://user52:pwd52@host.docker.internal:5672/");
    AMQP::LibEvHandler handler(evloop);
    AMQP::TcpConnection connection(&handler, address);
    AMQP::TcpChannel channel(&connection);

    std::string msg = "Hello, world! (from producer)";

    channel.declareQueue("task_queue", AMQP::durable)
        .onSuccess([&connection, &channel, &msg](const std::string &name, uint32_t, uint32_t) {
            for (int i = 0; i < 10; i++) {
                channel.publish("", name, msg.c_str());
                std::cout << "Отправлено coобщение " << msg << " в очередь: " << name << std::endl;
                sleep(2);
            }
            connection.close();
        })
        .onError([](const char *message) {
            std::cout << "Ошибка при отправлении сообщения: " << message << std::endl;
        });
    ;

    ev_run(evloop, 0);
    return 0;
}
