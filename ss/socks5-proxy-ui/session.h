/*
	session.h
				frank May 30, 2018
*/

#ifndef MYSHADOWSOCKS_ASIO_SESSION_H_
#define MYSHADOWSOCKS_ASIO_SESSION_H_

#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <vector>
#include "asio.hpp"

using asio::ip::tcp;

std::string& StringFormat(std::string& buff, const std::string fmt_str, ...);

struct Package
{
    explicit Package(int f):fd(f){}
    int fd;
    int start_time = 0; // time_t
    std::string domain;
    std::string ip;
    int port = 0;
    int sended = 0;
    int recved = 0;
    inline std::string ToString() const
    {
        std::string ret;
        StringFormat(ret, R"({"fd":%d, "start_time":%d, "domain":"%s", "ip":"%s", "port":%d, "sended":%d, "recved":%d})",
                fd, start_time, domain.c_str(), ip.c_str(), port, sended, recved);
        return ret;
    }
};

class Socks5Server;
class SSSession;

// 用扫描法来管理超时时间
class TimeoutManager
{
public:
	static TimeoutManager* Instance()
	{
		static TimeoutManager* p = new TimeoutManager;
		return p;
	}

	void AddSocket(const SSSession* s);
	void RemoveSocket(const SSSession* s);
	void Scan();
private:
	TimeoutManager()
	{
		sessions_.resize(1000);
		std::fill(sessions_.begin(), sessions_.end(), nullptr);
	}

	TimeoutManager(const TimeoutManager&) = delete;
	TimeoutManager& operator=(const TimeoutManager&) = delete;
private:
	std::vector<SSSession*> sessions_;
};

// client ---data---> local ---encrypted data---> server ---decrypted data---> remote
class SSSession : public std::enable_shared_from_this<SSSession>
{
	friend class TimeoutManager;
	friend class Socks5Server;
	static const int BUFFER_SIZE = 512;
public:
	SSSession(asio::io_context& ioc, Socks5Server& ss5s)
        :local_socket_(ioc), remote_socket_(ioc), resolver_(ioc), socks5_server_(ss5s),
         local_fd_(-1), local_sended_(0), local_recved_(0), last_send_time_(0)
    {
	    start_time_ = std::chrono::steady_clock::now();
	    UpdateNexttimeoutTime();
    }
	~SSSession();
	void LocalStart(); // only called in the "local" module
private:
	using buffer_type = char [BUFFER_SIZE];
	//recv data from sock1, into buff, then encrypt, then send to sock2
	void RecvEncryptSend(tcp::socket& sock1, tcp::socket& sock2, buffer_type& buff);
	//recv data from sock1, into buff, then decrypt, then send to sock2
	void RecvDecryptSend(tcp::socket& sock1, tcp::socket& sock2, buffer_type& buff);
	void Close(const asio::error_code& ec);
	void LogPkg(bool close = false, bool period = false); // 收到数据的包太多了，每隔一段时间发一次,period为true表示每隔一段时间发送
	void UpdateNexttimeoutTime() { next_timeout_time_ = std::chrono::steady_clock::now() + std::chrono::seconds(360); }
private:
	tcp::socket local_socket_; // listen as server for client, or for local
	tcp::socket remote_socket_;
    tcp::resolver resolver_;
    Socks5Server& socks5_server_;
private:
	char local_buffer_[BUFFER_SIZE]; // receive from local_socket_
	char remote_buffer_[BUFFER_SIZE]; // receive from remote_socket_
	std::chrono::steady_clock::time_point start_time_;
	std::string request_domain_; // 请求解析的域名，也可能直接请求ip
	std::string request_ip_; // 解析出来的ip
	int request_port_;
	int local_fd_;
	int local_sended_;
	int local_recved_;
	int last_send_time_; // 收取数据的消息太频繁，每隔一段时间发一次，记住上次发送的时间
private: // 超时管理，每个session记一个时间，如果超过此时间未收到数据或发送数据未完成，则认为超时
	std::chrono::steady_clock::time_point next_timeout_time_; // 下一次超时的时间
};

class Socks5Server
{
public:
	Socks5Server(asio::io_context& ioc, const tcp::endpoint& ep) :
	    io_context_(ioc), acceptor_(ioc, ep),
	    udp_socket_(ioc, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0))
    {
	    udp_dest_ = asio::ip::udp::endpoint(asio::ip::address::from_string("127.0.0.1"), 12345);
    }
	void LocalStart();
	void UdpSend(const Package& p);
private:
	asio::io_context& io_context_;
	asio::ip::tcp::acceptor acceptor_;
	asio::ip::udp::socket udp_socket_;
	asio::ip::udp::endpoint udp_dest_;
};

#endif /* MYSHADOWSOCKS_ASIO_SESSION_H_ */
