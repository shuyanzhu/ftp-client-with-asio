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

    // runPrompt();
    ioService_.post(std::bind(&FtpClient::runPrompt, this));
}

void FtpClient::cmdInPassiveMode(const std::string &cmd, TransDataHandle hdl){
    auto request = std::make_shared<boost::asio::streambuf>();
    auto response = std::make_shared<boost::asio::streambuf>();
    auto dataConn = std::make_shared<tcp::socket>(ioService_); // not used
    auto afterCmd = [=](const boost::system::error_code &ec, size_t){
        std::ostream requestStream(request.get());
        std::istream responseStream(response.get());
        if(ec){
            ioService_.post(std::bind(&FtpClient::runPrompt, this));
            return;
        }
        std::string responseString;
        std::getline(responseStream, responseString);
        std::cout << responseString << std::endl;
        hdl(ec, 0);
    };
    auto afterConnect = [=](const boost::system::error_code &ec){
        std::ostream requestStream(request.get());
        std::istream responseStream(response.get());
        if(ec) {
            return; // FIXME
        }

        requestStream << cmd;
        boost::asio::async_write(cmdConn_, *request, [=](const boost::system::error_code &ec, size_t){
            boost::asio::async_read_until(cmdConn_, *response, "\r\n", afterCmd);
        });
    };
    auto afterPASV = [=](const boost::system::error_code &ec, size_t){
        std::ostream requestStream(request.get());
        std::istream responseStream(response.get());
        std::string responseString;
        std::getline(responseStream, responseString);
        std::cout << responseString << std::endl;

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

    
    std::ostream requestStream(request.get());
    requestStream << "PASV\r\n";
    boost::asio::async_write(cmdConn_, *request, [=](const boost::system::error_code &ec, size_t){
        boost::asio::async_read_until(cmdConn_, *response, "\r\n", afterPASV);
    });
}
void FtpClient::LIST(const std::string &path, std::function<void()>){
    std::string tmpPath;
    if(path.size() < 3) 
        tmpPath = "";
    else 
        tmpPath = path.substr(3);

    auto response = std::make_shared<boost::asio::streambuf>();
    // lambda递归 + 异步，很坑
    // lambda表达式无需捕获静态变量，可以直接使用
    // 不可以直接赋值，否则闭包捕获的永远都是第一次的智能指针，在这里虽然无伤大雅，但是别的地方可能有问题
    static std::function<void(const boost::system::error_code &ec, size_t)> transDataHdl;
    transDataHdl = [this, response](const boost::system::error_code &ec, size_t readNum){
            std::istream responseStream(response.get());
            if(ec == boost::asio::error::eof){
                dataConn_.shutdown(boost::asio::ip::tcp::socket::shutdown_both); // send FIN
                // std::cout << response->size() << std::endl;
                boost::asio::async_read_until(cmdConn_, *response, "\r\n", [this, response](const boost::system::error_code &ec, size_t){
                    std::istream responseStream(response.get());
                    dataConn_.close(); // 应用层表示接受结束
                    std::string responseString;
                    if(ec); // FIXME
                    std::getline(responseStream, responseString);
                    std::cout << responseString << std::endl;
                    // runPrompt();
                    ioService_.post(std::bind(&FtpClient::runPrompt, this));
                });
            } else if(!ec){
                    response->commit(readNum);
                    auto oldBuf = std::cout.rdbuf();
                    std::cout << response.get(); // osstringstream的stringbuf不能这样用，至于为什么，不知道
                    std::cout.flush();
                    std::cout.rdbuf(oldBuf);
                    dataConn_.async_read_some(response->prepare(512), transDataHdl);
            } else {
                // FIXME, 其他错误暂未处理
            }
    };

    cmdInPassiveMode("LIST " + tmpPath + "\r\n", transDataHdl);
}
void FtpClient::RETR(const std::string &path, std::function<void()>){
    std::string remotePath, localPath;
    if(path.size() == 3){
        // FIXME
    } else{
        std::istringstream is(path.substr(3));
        is >> remotePath;
        if(remotePath.size() == 0){
            // FIXME
        } else{
            is >> localPath;
            localPath = (localPath.size() != 0 ? localPath : remotePath);
        }
    }
    std::shared_ptr<std::ofstream> of = std::make_shared<std::ofstream>(localPath);
    if(!(*of)){
        std::cout << "不能创建本地文件: " << localPath << std::endl;
        ioService_.post(std::bind(&FtpClient::runPrompt, this));
        return;
    }

    // 闭包需要的东西
    auto response = std::make_shared<boost::asio::streambuf>();
    // lambda递归 + 异步，很坑
    // lambda表达式无需捕获静态变量，可以直接使用
    // 不可以直接赋值，否则闭包捕获的永远都是第一次的智能指针，在这里虽然无伤大雅，但是别的地方可能有问题
    static std::function<void(const boost::system::error_code &ec, size_t)> transDataHdl;
    transDataHdl = [this, response, of](const boost::system::error_code &ec, size_t readNum){
            std::istream responseStream(response.get());
            if(ec == boost::asio::error::eof){
                dataConn_.shutdown(tcp::socket::shutdown_both); // send FIN
                of->close();
                // auto num = boost::asio::read_until(cmdConn_, *response, "\r\n");
                // response->consume(num);
                // auto num2 = boost::asio::read_until(cmdConn_, *response, "\r\n");
                boost::asio::async_read_until(cmdConn_, *response, "\r\n", [this, response](const boost::system::error_code &ec, size_t readNum){
                    std::istream responseStream(response.get());
                    dataConn_.close(); // 应用层表示接收结束
                    std::string responseString;
                    if(ec); // FIXME
                    std::getline(responseStream, responseString);
                    std::cout << responseString << std::endl;
                    // runPrompt();
                    ioService_.post(std::bind(&FtpClient::runPrompt, this));
                });
            } else if(!ec){
                    response->commit(readNum);
                    auto buf = of->rdbuf();
                    auto baseOf = static_cast<std::shared_ptr<std::ostream>>(of);
                    (*baseOf) << response.get();
                    baseOf->flush();
                    baseOf->rdbuf(buf); // 必须读会，至于为什么，不知道

                    // std::string output;
                    // std::ostringstream ss;
                    // ss << response.get();
                    // output = ss.str();
                    // (*of) << output;

                    // auto buf = response->data();
                    // std::string output;
                    // output.assign(boost::asio::buffers_begin(buf), boost::asio::buffers_end(buf));
                    // (*of) << output; 
                    // response->consume(readNum);
                    dataConn_.async_read_some(response->prepare(500), transDataHdl);
            } else {
                // FIXME, 其他错误暂未处理
            }
    };

    cmdInPassiveMode("RETR " + remotePath + "\r\n", transDataHdl);
}
void FtpClient::STOR(const std::string &path, std::function<void()>){
    std::string remotePath, localPath;
    if(path.size() == 3){
        // FIXME
    } else{
        std::istringstream is(path.substr(3));
        is >> localPath;
        if(localPath.size() == 0){
            // FIXME
        } else{
            is >> remotePath;
            remotePath = (remotePath.size() != 0 ? remotePath : localPath);
        }
    }
    auto inputStream = std::make_shared<std::ifstream>(localPath);
    if(!(*inputStream)){
        ioService_.post(std::bind(&FtpClient::runPrompt, this));
        return;
    }

    auto response = std::make_shared<boost::asio::streambuf>();
    static std::function<void(const boost::system::error_code &ec, size_t)> transDataHdl;
    transDataHdl = [this, response, inputStream](const boost::system::error_code &ec, size_t writeNum){
            std::istream responseStream(response.get());
            if(ec){
                ioService_.post(std::bind(&FtpClient::runPrompt, this));
                return;
            } else {
                if(inputStream->eof()){
                    dataConn_.shutdown(boost::asio::ip::tcp::socket::shutdown_both); // send FIN
                    boost::asio::async_read_until(cmdConn_, *response, "\r\n", [this, response](const boost::system::error_code &ec, size_t readNum){
                        std::istream responseStream(response.get());
                        dataConn_.close(); // 应用层得知对方接收结束
                        std::string responseString;
                        if(ec); // FIXME
                        std::getline(responseStream, responseString);
                        std::cout << responseString << std::endl;
                        // runPrompt();
                        ioService_.post(std::bind(&FtpClient::runPrompt, this));
                    });
                } else{
                    inputStream->read(buf1024_, 1024);
                    boost::asio::async_write(dataConn_, boost::asio::buffer(buf1024_, inputStream->gcount()), transDataHdl);
                }
            }
    };

    cmdInPassiveMode("STOR " + remotePath + "\r\n", transDataHdl);
}
void FtpClient::runPrompt(){
        while(true){
            std::cout << ">";
            std::cout.flush();
            std::string input;
            std::getline(std::cin, input);
            if(input.find("ls") == 0){
                LIST(input, std::bind(&FtpClient::runPrompt, this));
                break;
            } else if(input.find("get") == 0){
                RETR(input, std::bind(&FtpClient::runPrompt, this));
                break;
            } else if(input.find("put") == 0){
                STOR(input, std::bind(&FtpClient::runPrompt, this));
                break;
            } else {
                std::cout << "Not supported command: " << input << std::endl;
            }
        }
}