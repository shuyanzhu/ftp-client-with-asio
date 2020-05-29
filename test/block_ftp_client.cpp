
#include <iostream>
#include <istream>
#include <ostream>
#include <fstream>
#include <string>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
 
using boost::asio::ip::tcp;
using namespace std;
 
string strHost, strPort;
 
//得到文件长度
long GetFileLength(const string& stringLine)
{
	long ldFileLength = 0 ;
	size_t nBeginPos = stringLine.find_first_of(" ");
	size_t nEndPos = stringLine.find_first_of("\r\n");
	std::stringstream stream;
	string strLength = stringLine.substr(nBeginPos,nEndPos-nBeginPos+1) ;
	stream << strLength;
	stream >> ldFileLength;
	return ldFileLength;
}
 
//得到ftp返回命令的数值
int GetRetCode(const string& stringLine)
{
	if (stringLine.size() < 3)
	{
		return -1;
	}
	size_t nPos = stringLine.find_first_of(" ");
	std::stringstream stream;
	string strCode = stringLine.substr(0,nPos) ;
	int nRetCode = 0;
	stream << strCode;
	stream >> nRetCode;
	return nRetCode;
}
//
bool SendDataCommand(tcp::socket& socket, string strCommand, string& strResult)
{
	boost::asio::streambuf request;
	boost::asio::streambuf response;
	std::string strLine ;
	std::ostream request_stream(&request);
	std::istream response_stream(&response);
 
	strCommand += ("\r\n");
	cout<<endl<<"command is :"<<strCommand<<endl;
	request_stream <<strCommand; 
	boost::asio::write(socket, request);
	return true;
}
// 发送命令到FTP服务器
 
bool SendCommand(tcp::socket& socket, string strCommand, string& strResult)
{
	boost::asio::streambuf request;
	boost::asio::streambuf response;
	std::string strLine ;
	std::ostream request_stream(&request);
	std::istream response_stream(&response);
 
	strCommand += ("\r\n");
	cout<<endl<<"command is :"<<strCommand<<endl;
	request_stream <<strCommand; 
	boost::asio::write(socket, request);
	boost::asio::read_until(socket, response, "\r\n");
	std::getline(response_stream, strResult);
	return true;
}
 
bool GetHostAndPort(string& strResult)
{
	size_t nBegin = strResult.find("(");
	size_t nEnd = strResult.find(")");
 
	if (nBegin==-1 || nEnd==-1)
		return false ;
 
	strHost = strResult.substr(nBegin+1, nEnd-nBegin-1);
	nBegin = strHost.find_last_of(",");
	strPort = strHost.substr(nBegin+1, nEnd-nBegin+1);
	int nPort = 0, tmp=0;
 
	tmp = boost::lexical_cast<int>(strPort);
	strHost = strHost.substr(0,nBegin);
 
	nBegin = strHost.find_last_of(",");
	strPort = strHost.substr(nBegin+1, nEnd-nBegin+1);
	nPort = boost::lexical_cast<int>(strPort);
	nPort = 256*nPort + tmp;
	strPort = boost::lexical_cast<std::string>(nPort);
 
	return true;
}
 
 
bool OpenControl(tcp::socket& socket)
{
	bool bRet = false ;
	string strResult;
 
	SendCommand(socket,"", strResult);
	std::cout<<strResult<<endl;
	cout<<"Ret Code is:"<<GetRetCode(strResult)<<endl;
 
	/// send USER 
	//SendCommand(socket,"USER anonymous", strResult);
	SendCommand(socket,"USER zhm", strResult);
	std::cout<<strResult<<endl;
	cout<<"User Ret Code is:"<<GetRetCode(strResult)<<endl;
 
	/// send PASSWORD
	//SendCommand(socket,"PASS anonymous", strResult);
	SendCommand(socket,"PASS 137152", strResult);
	std::cout<<strResult<<endl;
	cout<<"User Ret Code is:"<<GetRetCode(strResult)<<endl;
 
	//send TYPE 1
	SendCommand(socket,"TYPE I", strResult);
	std::cout<<strResult<<endl;
	cout<<"User Ret Code is:"<<GetRetCode(strResult)<<endl;
 
	//send SIZE
	//string strObject = "incoming/ProcessExplorer/procexp.exe";
	string strObject = "upload/ReadMe.txt";
	//string strObject = "upload/计算机程序设计艺术第三版第一卷：基本算法.pdf";
	string strCommand("SIZE ");
	strCommand += strObject;
 
	SendCommand(socket, strCommand, strResult);
	std::cout<<strResult<<endl;
	cout<<"User Ret Code is:"<<GetRetCode(strResult)<<endl;
 
	long ldFileSize = GetFileLength(strResult);
	cout<<"FileSize is:"<<ldFileSize<<endl;
	if (ldFileSize>0)
	{
		bRet = true;
	}
 
	////send TYPE 1
	//SendCommand(socket,"TYPE I", strResult);
	//std::cout<<strResult<<endl;
	//cout<<"TYPE 1 Ret Code is:"<<GetRetCode(strResult)<<endl;
 
	//send PASV
	SendCommand(socket,"PASV", strResult);
	std::cout<<strResult<<endl;
	cout<<"PASV Ret Code is:"<<GetRetCode(strResult)<<endl;
 
	bRet = GetHostAndPort(strResult);
 
 
	strCommand="RETR " ;
	strCommand += strObject;
	SendDataCommand(socket,strCommand, strResult);
 
	return bRet;
}
 
bool OpenDataChannel(tcp::socket& data_socket)
{
	bool bRet = false;
 
 
	boost::asio::streambuf response;
	//asio::error_code error;
	boost::system::error_code error;
 
	if (response.size() > 0)
		std::cout << &response;
 
	ofstream file;
	file.open("file.txt",ios_base::binary);
 
	// Read until EOF, writing data to output as we go.
	while (boost::asio::read(data_socket, response,
		boost::asio::transfer_at_least(1), error))
	{
		std::cout << &response;
		//file << &response;
	}
	file.close();
	if (error != boost::asio::error::eof)
		//throw boost::asio::system_error(error);
		throw boost::system::error_code(error);
 
	return bRet;
}
 
 
int main(int argc, char* argv[])
{
 
	////用asio进行网络连接至少需要一个boost::asio::io_service对象 
	boost::asio::io_service io_service;
 
	// 我们需要把在命令行参数中指定的服务器转换为TCP上的节点.
	// 完成这项工作需要boost::asio::ip::tcp::resolver对象 
	tcp::resolver time_server_resolver(io_service);
 
	//一个resolver对象查询一个参数,并将其转换为TCP上节点的列表.
	//tcp::resolver::query query("192.168.1.111","ftp");
	tcp::resolver::query query("ftp.5xue.com","ftp");
 
	//节点列表可以用 boost::asio::ip::tcp::resolver::iterator 来进行迭代. 
	// iterator默认的构造函数生成一个end iterator
 
	tcp::resolver::iterator endpoint_iterator = time_server_resolver.resolve(query); 
	tcp::resolver::iterator end;
 
	//现在我们建立一个连接的sockert,由于获得节点既有IPv4也有IPv6的.   
	// 所以,我们需要依次尝试他们直到找到一个可以正常工作的.
	// 这步使得我们的程序独立于IP版本
 
	tcp::socket socket(io_service);
	//boost::asio::error_code error = boost::asio::error::host_not_found;
	boost::system::error_code error = boost::asio::error::host_not_found;
	while (error && endpoint_iterator != end)
	{    
		socket.close();     
		socket.connect(*endpoint_iterator++, error);;
	}
 
	if (error)
	{
		//throw boost::asio::system_error(error);
		throw boost::system::system_error(error);
	}
 
	//commu with ftp server
	bool bState = OpenControl(socket);
 
	if (!bState)
		return -1;
 
	tcp::socket data_socket(io_service);
	//tcp::resolver::query data_query("192.168.1.111",strPort);
	tcp::resolver::query data_query("ftp.5xue.com",strPort);
 
	tcp::resolver::iterator data_endpoint_iterator = time_server_resolver.resolve(data_query); 
	error = boost::asio::error::host_not_found;
	
	if(error && data_endpoint_iterator != end)
	{    
		data_socket.close();     
		data_socket.connect(*data_endpoint_iterator++, error);
		cout << "data_socket.close(): "<< error.value() << " \t" << error.message() << endl;
	}
 
	if (error)
	{
		//throw boost::asio::system_error(error);
		throw boost::system::system_error(error);
	}    
	OpenDataChannel(data_socket);
 
	return 0;
}