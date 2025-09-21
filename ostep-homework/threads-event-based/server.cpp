#include <unistd.h>      // for close()
#include <sys/socket.h>  // for socket() etc.
#include <netinet/in.h>  // for sockaddr_in
#include <arpa/inet.h>   // for inet_addr()
#include <print>
#include <array>
#include <chrono>
#include <fstream>
#include <string>
#include <filesystem>
#include <ranges>
#include <span>


using namespace std;

constexpr int PORT = 5678;

class Client
{
    using fd_type = int;
public:
    Client() : fd_(0) {}
    Client(fd_type fd) : fd_(fd) {}
    Client(const Client& another) = delete;
    Client(Client&& another)
    {
        fd_ = another.fd_;
        another.fd_ = -1;
    }

    Client& operator=(Client&& another)
    {
        if (this != &another)
        {
            fd_ = another.fd_;
            another.fd_ = -1;
        }

        return *this;
    }


    ~Client() { if (isAlive()) close(); }

    int close()
    {
        if (!isAlive()) return 0;

        fd_type old_fd = -1;
        swap(old_fd, fd_);
        print("{} disconnected\n", old_fd);
        return ::close(old_fd);
    }

    bool isAlive() const
    {
        return fd_ != -1;
    }

    fd_type getFd() const
    {
        return fd_;
    }

private:
    fd_type fd_;
};

string readFile(filesystem::path file_name)
{
    ifstream file_stream{ file_name };
    if (!file_stream.is_open())
    {
        return "Faild to open file";
    }
    else
    {
        return { istreambuf_iterator<char>{file_stream}, {} };
    }
}

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        print("Can't open server socket\n");
        return -1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        print("Can't bind\n");
        return -1;
    }

    if (listen(server_fd, 1024) == -1)
    {
        print("Listen failed\n");
        return -1;
    }

    print("listen on {}\n", PORT);

    int max_fd = server_fd;
    vector<Client> clients;

    while (true)
    {
        // clear dead clients
        if (clients.size() > 1e5)
        {
            erase_if(clients, [](const auto& client) {return !client.isAlive();});
        }

        fd_set read_set, write_set;
        FD_ZERO(&read_set);
        FD_ZERO(&write_set);
        FD_SET(server_fd, &read_set);

        for (const auto& client : clients)
        {
            if (client.isAlive())
            {
                FD_SET(client.getFd(), &read_set);
                FD_SET(client.getFd(), &write_set);
            }
        }

        auto ready_count = select(max_fd + 1, &read_set, &write_set, nullptr, nullptr);
        if (ready_count < 0)
        {
            perror("select");
            break;
        }

        if (FD_ISSET(server_fd, &read_set))
        {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
            if (client_fd == -1)
            {
                perror("accept failed");
                continue; // 继续等待下一个连接
            }
            clients.emplace_back(client_fd);
            max_fd = max(max_fd, client_fd);
            print("connected {}:{}\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            --ready_count;
        }

        if (ready_count == 0) continue;

        for (auto& client : clients)
        {
            if (!client.isAlive()) continue;

            if (FD_ISSET(client.getFd(), &read_set))
            {
                array<byte, 1024> buffer;
                ssize_t bytes_read = read(client.getFd(), buffer.data(), buffer.size());
                if (bytes_read <= 0)
                {
                    client.close();
                }
                else
                {
                    print("receive message: ");
                    for_each_n(buffer.begin(), bytes_read, [](auto b) {
                        print("{}", static_cast<char>(b));});
                    print("\n");

                    auto file_contents = readFile(
                        string_view{ reinterpret_cast<const char*>(buffer.data()), static_cast<size_t>(bytes_read) });
                    write(client.getFd(), file_contents.data(), file_contents.size());
                    print("send back:\n{}\n", file_contents);

                    client.close();
                }
            }
        }
    }

    // 7. 关闭服务器套接字 (通常在无限循环中不会执行到这里)
    close(server_fd);

    return 0;

}