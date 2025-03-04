# Приложение для инвертирования изображения с использованием Клиент-Сервер архитектуры на сокетах

## Описание проекта
Приложение из исходного изображения генерирует новое, каждый пиксель которого инвертирован. 
Проект разбит на две программы: client-producer и server-consumer. Взаимодействие между процессами 
происходит с помощью сокетов.

## Функционал проекта
- **Клиент**
  - Выступает в роли producer'а
  - Делит входное изображение на строки фиксированной ширины, сериализует данные и отправляет серверу для обработки
  - Постепенно получает обработанные данные, десериализует их и собирает готовое инвертированное изображение
- **Сервер**
  - Выступает в роли consumer'а
  - Ожидает подключения клиента
  - Получает блок изображения для обработки и десериализует его
  - Обрабатывает полученную часть изображения
  - Отправляет обратно клиенту, предварительно сериализуя

## Запуск
1. Для работы проекта необходима библиотека OpenCV.
2. Для запуска необходимо знать имя хоста, где запускается сервер, а также порт, подключение к которому сервер будет ожидать.
   - Приложение-сервер ожидает, что вы передадите в качестве консольного аргумента номер порта:
     ```bash
     ./server portno
     ```
   - Приложение клиента ожидает ввода имени хоста и порта:
     ```bash
     ./client hostname portno
     ```
   - В случае если и сервер, и клиент — один компьютер, в качестве имени хоста укажите `localhost`, а порт — 2000 или больше (обычно они свободны).
3. Для сборки проекта использовалась билдовая система CMake. Для запуска следуйте следующей инструкции:
   ```bash
   mkdir build
   ```
   - **1-ый терминал:**
     ``` bash
     cd build
     cmake .. && cmake --build .
     ./server 2000
     ```

   - **2-ой терминал:**
     ```bash
     cd build
     ./client localhost 2000
     ```
## Системные требования
- Linux/Unix-подобная ОС
