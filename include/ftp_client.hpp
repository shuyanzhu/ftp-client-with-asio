#pragma once
#include "std_include.hpp"
#include <boost/asio.hpp>

class FtpClient{
public:
    enum class State{ // 未使用
        Unconnected, // 未连接（初始化、连接中）
        Connected,   // 已连接（连接完成、发送USER）
        Login,       // 登录中（USER完成、发送PASS）
        Running      // 完成登录
    };
    FtpClient(std::string url = "127.0.0.1");
    bool setEndpoint(std::string ip, int port);
    void connectedHandler(const boost::system::error_code &ec);
    using TransDataHandle = std::function<void(const boost::system::error_code &ec, size_t)>;
    void cmdInPassiveMode(const std::string &cmd, TransDataHandle hdl);
    void LIST(const std::string &path, std::function<void()>);
    void RETR(const std::string &path, std::function<void()>);
    void STOR(const std::string &path, std::function<void()>);
    void runPrompt();
    
    void run(){ ioService_.run(); }
    
private:
    boost::asio::io_service ioService_; // const io_service &sv 的用法会报错
    boost::asio::io_service::work ioServiceWork_; // after ioService
    State state_;
    boost::asio::ip::tcp::socket cmdConn_;
    boost::asio::ip::tcp::socket dataConn_;
    boost::asio::ip::tcp::endpoint serverAddr_;

    char buf1024_[1024];
};