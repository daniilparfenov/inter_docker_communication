#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <amqpcpp/linux_tcp.h>
#include <ev.h>
#include <unistd.h>

#include <iostream>
#include <string>

int main() {
    // Создание цикла событий libev
    struct ev_loop *evloop = EV_DEFAULT;

    // Создание соединения с RabbitMQ
    AMQP::Address address("amqp://user52:pwd52@host.docker.internal:5672/");
    AMQP::LibEvHandler handler(evloop);
    AMQP::TcpConnection connection(&handler, address);
    AMQP::TcpChannel channel(&connection);

    // Отправка сообщений в очередь
    std::string msg = "Hello, world! (from producer)";

    // Создание очереди и отправка сообщений
    channel.declareQueue("task_queue", AMQP::durable)
        .onSuccess([&connection, &channel, &msg](const std::string &name, uint32_t, uint32_t) {
            /*
             * Отправка сообщений в очередь
             * В данном случае отправляется 10 сообщений с интервалом в 2 секунды
             */
            for (int i = 0; i < 10; i++) {
                channel.publish("", name, msg.c_str());
                std::cout << "Отправлено coобщение " << msg << " в очередь: " << name << std::endl;
                sleep(2);
            }
            connection.close();  // Закрытие соединения
        })
        .onError([](const char *message) {
            std::cout << "Ошибка при отправлении сообщения: " << message << std::endl;
        });
    ;

    // Запуск цикла событий libev
    ev_run(evloop, 0);
    return 0;
}
