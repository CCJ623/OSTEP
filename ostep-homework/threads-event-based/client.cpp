#include <print>
#include <string>
#include <iostream>
#include <array>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <chrono>
#include <string_view>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h> 

using namespace std;

constexpr string_view SERVER_ADDRESS = "127.0.0.1";
constexpr int SERVER_PORT = 5678;
constexpr auto MIN_DELAY_TIME = 0ms;
constexpr auto MAX_DELAY_TIME = 2000ms;
constexpr string_view SEND_MESSAGE = "Horn Pub.txt";

static size_t CONNECTION_NUM;

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        perror("bad arguments");
        return -1;
    }

    string_view argument{ argv[1] };
    from_chars(argument.data(), argument.data() + argument.size(), CONNECTION_NUM);

    int max_client_fd = 0;
    vector<int> client_fds;
    client_fds.reserve(CONNECTION_NUM);
    unordered_map<decltype(client_fds)::value_type, bool> message_sent,
        message_read;
    for (size_t i = 0;i != CONNECTION_NUM; ++i)
    {
        int client_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (client_fd == -1)
        {
            perror("socket failed");
            return 1;
        }

        // 设置为非阻塞模式
        if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1)
        {
            perror("fcntl O_NONBLOCK failed");
            close(client_fd);
            return 1;
        }

        max_client_fd = max(max_client_fd, client_fd);
        client_fds.push_back(client_fd);
        message_sent.emplace(client_fd, false);
        message_read.emplace(client_fd, false);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    // 使用inet_pton将IP字符串转换为二进制格式
    if (inet_pton(AF_INET, SERVER_ADDRESS.data(), &server_addr.sin_addr) <= 0)
    {
        perror("invalid address");
        return 1;
    }

    // 2. 发起非阻塞连接
    for_each(client_fds.begin(), client_fds.end(),
        [&server_addr](const auto& fd) {
            int conn_status = connect(fd, (sockaddr*)&server_addr, sizeof(server_addr));
            if (conn_status == -1 && errno != EINPROGRESS)
            {
                perror("connect failed");
                close(fd);
            }
        });

    auto start_time = chrono::steady_clock::now();

    while (true)
    {
        fd_set read_fds, write_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        for_each(client_fds.begin(), client_fds.end(),
            [&read_fds, &write_fds, &message_read, &message_sent, &max_client_fd](const auto& fd) {
                if (!(message_sent.at(fd)))
                {
                    FD_SET(fd, &write_fds);
                }
                else if (!(message_read.at(fd)))
                {
                    FD_SET(fd, &read_fds);
                }
            });

        int activity = select(max_client_fd + 1, &read_fds, &write_fds, NULL, 0);
        if (activity < 0)
        {
            perror("select error");
            return -1;
        }

        for_each(client_fds.begin(), client_fds.end(),
            [&read_fds, &write_fds, &message_read, &message_sent, &max_client_fd, &start_time](const auto& fd)
            {
                if (!(message_sent.at(fd)) &&
                    FD_ISSET(fd, &write_fds))
                {
                    auto bytes_sent = write(fd, SEND_MESSAGE.data(), SEND_MESSAGE.size());
                    if (bytes_sent == -1)
                    {
                        perror("send failed");
                    }
                    if (bytes_sent > 0)
                    {
                        print("fd({}) send: {}\n", fd, SEND_MESSAGE);
                        message_sent[fd] = true;
                    }
                }
                else if (!(message_read.at(fd)) &&
                    FD_ISSET(fd, &read_fds))
                {
                    static array<byte, 1024> buffer;
                    auto bytes_read = read(fd, buffer.data(), buffer.size());
                    message_read[fd] = true;
                    close(fd);

                    print("fd({}) received:\n", fd);
                    for_each_n(buffer.begin(), bytes_read, [](const auto& b) {
                        print("{}", static_cast<char>(b));
                        });
                    print("\n");
                }
            });

        // exit if all done
        bool all_done = true;
        for (const auto& fd : client_fds)
        {
            if (!(message_read.at(fd)))
            {
                all_done = false;
                break;
            }
        }
        if (all_done)
        {
            break;
        }
    }

    auto end_time = chrono::steady_clock::now();
    print("connection num: {}\n", CONNECTION_NUM);
    print("cost: {}\n", chrono::duration_cast<chrono::milliseconds>(end_time - start_time));

    return 0;
}