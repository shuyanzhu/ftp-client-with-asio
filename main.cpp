#include "std_include.hpp"

#include <boost/asio.hpp>

#include "ftp_client.hpp"

// std::string output;
static int cnt = 1;
struct Debug{
    int i;
    int count;
    Debug(){
        count = cnt++;
        std::cout << "Debug construct of " << count << std::endl;
    }
    Debug(const Debug &from){
        i = from.i;
        count = cnt++;
        std::cout << "I am coping from " << from.count << std::endl;
        std::cout << "Debug copy of " << count << std::endl;
    }
    ~Debug(){
        std::cout << "Debug destroy of " << count << std::endl;
    }
};

void debugWrapper(std::function<void()> hdl){
    std::cout << "I'm in Wraper" << std::endl;
    hdl();
}
void debugMain(){
    using DebugLambda = std::function<void()>;
    Debug debug2;
    static DebugLambda debugLambda2;
    debugLambda2 = [debug2]{
        std::cout << "I do nothing" << std::endl;
    };
    debugLambda2();
    DebugLambda hdl;
    hdl = debugLambda2;
    hdl();
    // debugWrapper(debugLambda);
}
int main(){
    FtpClient ftpClient;
    ftpClient.run();
    // auto coutBuf = std::cout.rdbuf();
    
    // std::ostringstream ss;
    // std::stringbuf *stringStream = ss.rdbuf();

    // std::ostream *o = new std::ofstream("xxx");
    // auto oldBuf = o->rdbuf();
    // ss << "SHBBB";
    // std::cout << ss.str() << stringStream-> << std::endl;
    // std::cout.rdbuf(stringStream);
    // std::cout.flush();
    // std::cout.rdbuf(oldBuf);
    // std::cout << "debug" << std::endl;
    // std::ofstream stream;
    // stream.open("xxx");
    // while(true)
    //     debugMain();

    // std::function<void(int)> debug = [&debug](int num) {
    //     boost::asio::streambuf buf;
    //     buf.prepare(num);
    //     buf.commit(num);
    //     std::cout << buf.size() << std::endl;
    // };
    // debug(512);
    // char *debugPtr;
    // debugPtr = (char *)malloc(512);
    // debugPtr = (char *)malloc(512);
    // debugPtr = (char *)malloc(512);
    // debugPtr = (char *)malloc(512);

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