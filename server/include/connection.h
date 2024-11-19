#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

#include <string>

//Запуск сервера
int start_server();
//Выключение сервера
void shutdown_server(int server_socket);

//Обработка подключения
int handle_connection(int server_socket, const std::string& exit_message);

//Подключить клиента, если есть запрос на подключение к серверу
int connect_client(int server_socket);
//Отключить клиента
void disconnect_client(int client_socket);

//Отправка сообщения клиенту
int send_message(int client_socket, const std::string& message);
//Приём сообщения от клиента
int recieve_message(int client_socket, std::string& message);

//Создание сокета
int create_server_socket();

#endif //SERVER_CONNECTION_H