#include <unistd.h>      // for close()
#include <sys/socket.h>  // for socket() etc.
#include <netinet/in.h>  // for sockaddr_in
#include <arpa/inet.h>   // for inet_addr()
#include <print>
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <filesystem>
#include <ranges>
#include <span>
#include <boost/asio.hpp>


using namespace std;

constexpr int PORT = 5678;

template<typename BufferType>
void readFile(filesystem::path file_name, BufferType& buffer)
{
    ifstream file_stream{ file_name , ios::binary };
    if (!file_stream.is_open())
    {
        string_view error_msg = "Faild to open file";
        buffer.assign(error_msg.begin(), error_msg.end());
    }
    else
    {
        buffer.assign(istreambuf_iterator<char>(file_stream), {});
    }
}

class Session : public enable_shared_from_this<Session>
{
    using SocketType = boost::asio::ip::tcp::socket;

public:
    Session(SocketType&& socket) : socket_(move(socket)) {}

    void start()
    {
        boost::asio::async_read_until(socket_, receive_buffer_, '\n',
            [self = shared_from_this()](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                self->handleRead(ec, bytes_transferred);
            });
    }

    void handleRead(const boost::system::error_code& ec, std::size_t bytes_transferred)
    {
        if (ec)
        {
            std::cerr << "Read error: " << ec.message() << std::endl;
            return;
        }

        istream is(&receive_buffer_);
        string file_path;
        getline(is, file_path);
        print("receive message({}B):\n{}\n", bytes_transferred, file_path);
        readFile(file_path, send_buffer_);
        boost::asio::async_write(socket_, boost::asio::buffer(send_buffer_),
            [self = shared_from_this()](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                self->handleWrite(ec, bytes_transferred);
            });
    }

    void handleWrite(const boost::system::error_code& ec, std::size_t bytes_transferred)
    {
        if (ec)
        {
            std::cerr << "Write error: " << ec.message() << std::endl;
        }
        // print("send message({}B):\n{}\n",
        //     bytes_transferred,
        //     string_view(send_buffer_.begin(), send_buffer_.begin() + bytes_transferred));


        // 无论读写成功与否，处理完后关闭连接
        boost::system::error_code ignored_ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
        socket_.close(ignored_ec);
    }

private:
    SocketType socket_;
    boost::asio::streambuf receive_buffer_;
    string send_buffer_;
};

class Listener
{
public:
    Listener(boost::asio::io_context& io_context, boost::asio::ip::tcp::endpoint end_point)
        : acceptor_(io_context)
    {
        acceptor_.open(end_point.protocol());
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
        acceptor_.bind(end_point);
        acceptor_.listen(boost::asio::socket_base::max_listen_connections);
    }

    void start()
    {
        do_accept();
    }

    void do_accept()
    {
        acceptor_.async_accept(
            [self = this](const boost::system::error_code& ec, boost::asio::ip::tcp::socket socket)
            {
                if (!ec)
                {
                    std::print("New connection from {}:{}\n",
                        socket.remote_endpoint().address().to_string(),
                        socket.remote_endpoint().port());

                    std::make_shared<Session>(std::move(socket))->start();
                }
                else
                {
                    std::cerr << "Accept error: " << ec.message() << std::endl;
                }

                // 继续监听下一个连接
                self->do_accept();
            });
    }

private:
    boost::asio::ip::tcp::acceptor acceptor_;
};


int main()
{
    boost::asio::io_context io_context;
    Listener listener{ io_context, {boost::asio::ip::tcp::v4(), PORT} };
    print("listen on {}\n", PORT);

    listener.start();

    io_context.run();
}

