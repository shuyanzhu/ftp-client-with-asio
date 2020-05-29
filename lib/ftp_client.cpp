#include "ftp_client.hpp"

#ifdef WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

void SetStdinEcho(bool enable = true)
{
#ifdef WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 
    DWORD mode;
    GetConsoleMode(hStdin, &mode);

    if( !enable )
        mode &= ~ENABLE_ECHO_INPUT;
    else
        mode |= ENABLE_ECHO_INPUT;

    SetConsoleMode(hStdin, mode );

#else
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    if( !enable )
        tty.c_lflag &= ~ECHO;
    else
        tty.c_lflag |= ECHO;

    (void) tcsetattr(STDIN_FILENO, TCSANOW, &tty);
#endif
}

using namespace boost::asio::ip;

FtpClient::FtpClient(std::string url):
    ioServiceWork_(ioService_),
    cmdConn_(ioService_),
    dataConn_(ioService_),
    state_(State::Unconnected),
    serverAddr_(address::from_string(url), 21){
    
    cmdConn_.async_connect(serverAddr_, std::bind(&FtpClient::connectedHandler, this, std::placeholders::_1));
}
void FtpClient::connectedHandler(const boost::system::error_code &ec){
    if(ec) return;

    std::cout << "ftp client connected" << std::endl;
    // boost::asio::const_buffer
    boost::asio::streambuf request;
    boost::asio::streambuf response;
    std::string requestString, responseString;
    std::ostream requestStream(&request);
    std::istream responseStream(&response);

    boost::asio::read_until(cmdConn_, response, "\r\n");
    std::getline(responseStream, responseString);
    responseStream.clear();
    std::cout << "Connect response: " << responseString << std::endl;

    auto blockIO = [&](const std::string &cmd){
        requestString = cmd + " " + requestString + "\r\n";
        requestStream << requestString;
        boost::asio::write(cmdConn_, request);
        boost::asio::read_until(cmdConn_, response, "\r\n");
        std::getline(responseStream, responseString);
        responseStream.clear();
        std::cout << "Response of " << cmd << ": "<< responseString << std::endl;
    };

    std::cout << "Please input user name: ";
    std::cout.flush();
    std::cin >> requestString;
    blockIO("USER");

    std::cout << "Please input password:" << std::endl;
    std::cout.flush();
    SetStdinEcho(false);
    std::cin >> requestString;
    blockIO("PASS");
    SetStdinEcho(true);

    // blockIO("PASV");
    // int posEnd = responseString.find(")");
    // int pos1 = responseString.find_last_of(",");
    // int pos0 = responseString.substr(0, pos1).find_last_of(",");
    // auto portStr = responseString.substr(pos0 + 1, pos1 - pos0 - 1);
    // unsigned short port = 256 * std::stoi(portStr);
    // portStr = responseString.substr(pos1 + 1, posEnd - pos1 - 1);
    // port += std::stoi(portStr);
    // std::cout << serverAddr_.address().to_string() << ":" << port << std::endl;

    // boost::system::error_code connectDataEc;
    // tcp::endpoint dataEndpoint(serverAddr_.address(), port);
    // std::cout << dataEndpoint.port() << std::endl;
    // dataConn_.connect(dataEndpoint, connectDataEc);
    runPrompt();
}

// lambda递归 + 异步，很坑
static std::function<void(const boost::system::error_code &ec, size_t)> afterData;
void FtpClient::cmdInPassiveMode(const std::string &cmd, std::shared_ptr<std::string> result){
    auto request = std::make_shared<boost::asio::streambuf>();
    auto response = std::make_shared<boost::asio::streambuf>();
    auto requestStream = std::make_shared<std::ostream>(request.get());
    auto responseStream = std::make_shared<std::istream>(response.get());
    auto dataConn = std::make_shared<tcp::socket>(ioService_); // not used
    afterData = [this, dataConn, request, response, requestStream, responseStream, result](const boost::system::error_code &ec, size_t readNum){
            if(ec == boost::asio::error::eof){
                auto responseBuff = response->data();
                // std::cout << response->size() << std::endl;
                result->assign(boost::asio::buffers_begin(responseBuff), boost::asio::buffers_end(responseBuff));
                response->consume(result->size());
                // std::cout << response->size() << std::endl;
                dataConn_.close(); // next connect, 可能行为: 有数据RST，无数据FIN
                boost::asio::async_read_until(cmdConn_, *response, "\r\n", [this, reponse](const boost::system::error_code &ec, size_t){
                    std::string responseString;
                    if(ec); // FIXME
                    std::getline(*responseStream, responseString);
                    std::cout << responseString << std::endl;
                    runPrompt();
                });
            } else if(!ec){
                // try{
                    response->commit(readNum);
                    dataConn_.async_read_some(response->prepare(512), afterData);
                // } catch(const std::exception &e){
                    // std::cout << e.what() << std::endl;
                // }
            } else {
                // FIXME
            }
    };
    auto afterConnect = [cmd, this, dataConn, request, response, requestStream, responseStream](const boost::system::error_code &ec){
        if(ec) return; // FIXME
        std::cout << "Passive connect success" << std::endl;
        // std::cout << dataConn_.remote_endpoint().port() << std::endl;
        // std::cout << dataConn.use_count() << std::endl;

        *requestStream << cmd;
        boost::asio::async_write(cmdConn_, *request, [this, dataConn, responseStream, response](const boost::system::error_code &ec, size_t){
            std::string responseString;
            std::getline(*responseStream, responseString);
            std::cout << responseString << std::endl;
            // std::cout << "debug:" << dataConn.use_count() << std::endl; // capture dataConn in every level
            // size_t num = dataConn_.read_some(response->prepare(5120));
            dataConn_.async_read_some(response->prepare(512), afterData);
        });
    };
    auto afterPASV = [this, afterConnect, dataConn, request, response, requestStream, responseStream](const boost::system::error_code &ec, size_t){
        std::string responseString;
        std::getline(*responseStream, responseString);
        // std::cout << "after getline" << response->size() << std::endl;

        int posEnd = responseString.find(")");
        int pos1 = responseString.find_last_of(",");
        int pos0 = responseString.substr(0, pos1).find_last_of(",");
        auto portStr = responseString.substr(pos0 + 1, pos1 - pos0 - 1);
        int port = 256 * std::stoi(portStr);
        portStr = responseString.substr(pos1 + 1, posEnd - pos1 - 1);
        port += std::stoi(portStr);

        tcp::endpoint dataEndpoint(serverAddr_.address(), port);
        // std::cout << dataConn.use_count() << std::endl;
        dataConn_.async_connect(dataEndpoint, afterConnect);
    };

    
    *requestStream << "PASV\r\n";
    boost::asio::async_write(cmdConn_, *request, [this, response, afterPASV](const boost::system::error_code &ec, size_t){
        boost::asio::async_read_until(cmdConn_, *response, "\r\n", afterPASV);
    });
}
void FtpClient::runPrompt(){
    auto tmp = std::make_shared<std::string>();
        while(true){
            std::cout << ">";
            std::cout.flush();
            std::string input;
            std::getline(std::cin, input);
            if(input.find("ls") == 0){
                // LIST(input.substr(3), std::bind(&FtpClient::runPrompt, this));
                cmdInPassiveMode("LIST\r\n", tmp);
                std::cout << tmp << std::endl;
                break;
            } else if(input.find("get") == 0){

                break;
            } else if(input.find("put") == 0){
                break;
            } else {
                std::cout << "Not supported command: " << input << std::endl;
            }
        }
    }