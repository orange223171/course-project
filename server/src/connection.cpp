#include "connection.h"

#ifdef UNIX

//Подключаем заголовочные файлы, которые необходимы для взаимодействия с сокетами на Unix-системах
#include <sys/socket.h> 
#include <netdb.h>
#include <unistd.h>

#include <iostream>
//Файл, содержащий функцию memset()
#include <cstring>

//IP-адрес и номер порта
const char* ip_address = "127.0.0.1";
const char* port = "8080";

//Размер очереди для полностью установленных соединений, ожидающих, пока процесс примет их
const int backlog = 8;

int start_server()
{
    std::cout << "[Info] Starting the server...\n";

    //Создаём сокет сервера
    int server_socket = create_server_socket();
    //Проверяем созданный сокет сервера
    if (server_socket < 0)
    {
        return -1;
    }

    std::cout << "[Info] The server is start\n";

    return server_socket;
}
void shutdown_server(int server_socket)
{
    std::cout << "[Info] Shutting down the server...\n";
    //Закрываем сокет
    close(server_socket);
    std::cout << "[Info] The server is shut down\n";
}

int handle_connection(int server_socket, fd_set* socket_polling_list, int& max_socket, const std::string& exit_message)
{
    //Проходим все сокеты от единицы до максимального сокета, который был создан
    for (int socket = 0; socket <= max_socket + 1; socket++)
    {
        //Проверяем на наличие в списке обрабатываемых сокетов
        if (FD_ISSET(socket, socket_polling_list))
        {
            //Создаём новый список сокетов
            fd_set fd_socket;
            //Очищаем список
            FD_ZERO(&fd_socket);
            //Добавляем в этот список только обрабатываемый в текущий момент сокет
            FD_SET(socket, &fd_socket);
            //Устанавливаем нулевое время ожидания
            timeval time_for_select = {0, 0}; 

            //Функция select возвращает количество сокетов из списка, доступных для чтения
            //Так как в списке только один сокет, select вернёт либо 0, либо 1, либо отрицательное значение, обозначающее ошибку
            int socket_data = select(socket + 1, &fd_socket, nullptr, nullptr, &time_for_select);
            
            //Проверяем, доступен ли текущий сокет для чтения, если нет, то переходим к обработке следующего сокета
            if (socket_data == 0)
            {
                continue;
            }
            //Проверяем, не завершилась ли фунция select с ошибкой
            if (socket_data < 0)
            {
                std::cout << "[Error] Failed to fetch data on the server from socket: " << socket << "\n";
                return -1;
            }

            //Если обрабатываемый сокет - сокет сервера, то подключаем клиента, если есть запрос на подключение к серверу
            if (socket == server_socket)
            {
                if (connect_client(server_socket, socket_polling_list, max_socket) < 0)
                {
                    return -1;
                }
            }
            //Иначе читаем поступившие сообщения от клиента по этому сокету
            else
            {
                std::string message;

                if (recieve_message(socket, message) < 0)
                {
                    disconnect_client(socket, socket_polling_list);
                    continue;
                }

                std::cout << "[Client: " << socket << "] " << message << "\n";

                send_message(socket, message);

                if (message == exit_message)
                {
                    std::cout << "[Info] Recieve exit message from client " << socket << "\n";
                    return 1;
                }
            }
        }
    }

    return 0;
}

int connect_client(int server_socket, fd_set* socket_polling_list, int& max_socket)
{
    sockaddr_storage address;
    socklen_t address_len = sizeof(address);

    //Создаём сокет клиента
    int client_socket = accept(server_socket, reinterpret_cast<sockaddr*>(&address), &address_len);

    //Проверяем созданный сокет клиента
    if (client_socket < 0)
    {
        std::cout << "[Error] Failed to connect client\n";
        return -1;
    }

    std::cout << "[Info] Connect new client: " << client_socket << "\n";

    //Если значение сокета клиента больше максимального, то устанавливаем его максимальным
    if (client_socket > max_socket)
    {
        max_socket = client_socket;
    }

    //Добавляем сокет клиента в список опрашиваемых сокетов
    FD_SET(client_socket, socket_polling_list);

    return 0;
}
void disconnect_client(int client_socket, fd_set* socket_polling_list)
{
    //Закрываем сокет клиента
    close(client_socket);
    //Удаляем сокет из списка опрашиваемых сокетов
    FD_CLR(client_socket, socket_polling_list);

    std::cout << "[Info] Client " << client_socket << " has been disconnected\n";

    return;
}

int send_message(int client_socket, const std::string& message)
{
    if (send(client_socket, message.data(), message.size(), 0) < 0)
    {
        std::cout << "[Error] Failed to send data to client " << client_socket << "\n";
        return -1;
    }

    return 0;
}
int recieve_message(int client_socket, std::string& message)
{
    const int length = 128;
    char* buffer = new char[length];

    int recieve = recv(client_socket, buffer, length, 0);

    if (recieve == 0)
    {
        delete[] buffer;
        return -1;
    }
    if (recieve < 0)
    {
        std::cout << "[Error] Failed recieve data from client " << client_socket << "\n";
        delete[] buffer;
        return -1;
    }

    message = std::string(buffer);

    delete[] buffer;
    return 0;
}

int create_server_socket()
{
    //addrinfo хранит в себе информацию об адресе, по которому будет происходить подключение
    //В hints записываются данные о типе подключения, которые потом будут перезаписаны в address
    //adress будет использоваться для записи итогового адреса
    addrinfo hints, *address;
    
    //Очищаем переменную hints c помощью функции memset, чтобы в adress не попало никаких лишних данных
    std::memset(&hints, 0x00, sizeof(addrinfo));
    //Используем IPv4
    hints.ai_family = AF_INET;
    //Используем тип сокета TCP
    hints.ai_socktype = SOCK_STREAM;
    //Используем протокол TCP
    hints.ai_protocol = IPPROTO_TCP;

    //Записываем адрес по указателю adress, если функция возвращает не ноль, значит произошла ошибка    
    if (getaddrinfo(ip_address, port, &hints, &address) != 0)
    {
        std::cout << "[Error] Failed to resolve address\n";
        return -1;
    }

    //Создаём сокет
    int server_socket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);

    //Проверяем созданный сокет
    if (server_socket < 0)
    {
        std::cout << "[Error] Failed to create server_socket\n";

        //Освобождаем память от address
        freeaddrinfo(address);
        return -1;
    }

    //Привязываем созданный сокет к адресу
    if (bind(server_socket, address->ai_addr, address->ai_addrlen) == -1)
    {
        std::cout << "[Error] Failed to bind server_socket to address\n";

        //Освобождаем память от address
        freeaddrinfo(address);
        return -1;
    }

    //Освобождаем память от address
    freeaddrinfo(address);

    //Настраиваем сокет
    int yes = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        std::cout << "[Error] Failed to set server_socket options\n";
        return -1;
    }

    //Настраиваем сокет на прослушивание
    if (listen(server_socket, backlog) == -1)
    {
        std::cout << "[Error] Failed to activate soket listenner\n";
        return -1;
    }

    //Возвращаем созданный сокет
    return server_socket;
}

#endif /*UNIX*/