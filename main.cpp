#include "std_include.hpp"

#include <boost/asio.hpp>

#include "ftp_client.hpp"

int main(){
    FtpClient ftpClient;
    ftpClient.run();
    // Runner runner;
    // runner.runPrompt();
    // std::string testInput;
    // std::getline(std::cin, testInput);
    // boost::asio::io_service ioService;
    // boost::asio::ip::tcp::socket socket(ioService);
    // auto socket = std::make_shared<boost::asio::ip::tcp::socket>(ioService);
    // boost::asio::ip::tcp::endpoint end1(boost::asio::ip::address_v4::from_string("127.0.0.1"), 21);
    // boost::asio::ip::tcp::endpoint end2(boost::asio::ip::address_v4::from_string("192.168.93.130"), 21);
    // boost::system::error_code ec;

    // socket.connect(end1, ec);
    // socket.close();
    // socket.connect(end2, ec);
    // if(ec) std::cout << ec << std::endl;
    // std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    // socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    // boost::asio::streambuf response;
    // std::cout << response.size() << std::endl;
    // response.prepare(5);
    // std::cout << response.size() << std::endl;
    // response.commit(5);
    // std::cout << response.size() << std::endl;
    // response.prepare(5);
    // std::cout << response.size() << std::endl;
    // response.commit(5);
    // std::cout << response.size() << std::endl;
    return 0;
}