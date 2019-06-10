/*session.cpp
				frank May 30, 2018
*/

#include "session.h"
#include "encrypt.h"
#include "config.h"

#define log(fmt, ...) do { fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, __VA_ARGS__); } while (0)

#define ERROR_RETURN  if(ec) { Close(ec); return; }

#define EC_LEN_RETURN (const asio::error_code& ec, size_t len) { \
        ERROR_RETURN

#define PSELF_EC_LEN_RETURN [self, this] EC_LEN_RETURN

void SSSession::RecvEncryptSend(tcp::socket& sock1,
		tcp::socket& sock2, buffer_type& buff)
{
	auto self = shared_from_this();
	UpdateNexttimeoutTime();
	sock1.async_receive(asio::buffer(buff), [self, this, &sock1, &sock2, &buff] EC_LEN_RETURN
		Cypher::Instance().Encrypt(buff, len);
	    UpdateNexttimeoutTime();
		asio::async_write(sock2, asio::buffer(buff, len), [self, this, &sock1, &sock2, &buff] EC_LEN_RETURN
	        local_sended_ += len;
	        LogPkg(false, true);
		    RecvEncryptSend(sock1, sock2, buff);
		});
	});
}

void SSSession::RecvDecryptSend(tcp::socket& sock1,
		tcp::socket& sock2, buffer_type& buff)
{
	auto self = shared_from_this();
	UpdateNexttimeoutTime();
	sock1.async_receive(asio::buffer(buff), [self, this, &sock1, &sock2, &buff] EC_LEN_RETURN
		Cypher::Instance().Decrypt(buff, len);
	    UpdateNexttimeoutTime();
		asio::async_write(sock2, asio::buffer(buff, len), [self, this, &sock1, &sock2, &buff] EC_LEN_RETURN
            local_recved_ += len;
            LogPkg(false, true);
			RecvDecryptSend(sock1, sock2, buff);
		});
	});
}

bool GetIpPort(const char* buff, const size_t len, std::string& ip_or_domain, int& port)
{
    if(len < 4)
    {
        std::cout<<"buff too short, must > 4, now is:"<<len<<"\n";
        return false;
    }

    if(buff[3] == 0x01)
    {
        if(len != 10)
        {
            std::cout<<"ip port must be 10, now is:"<<len<<"\n";
            return false;
        }
        short* p = (short*)(buff+8);
        port = *p;
        port = ntohs(port);
        ip_or_domain = std::to_string(int((unsigned char)buff[4]));
        for(int i = 5; i <= 7; ++i)
        {
            ip_or_domain.append(".");
            ip_or_domain.append(std::to_string(int((unsigned char)buff[i])));
        }
    }
    else if(buff[3] == 0x03)
    {
        if(len < 7)
        {
            std::cout<<"ip port must be 7, now is:"<<len<<"\n";
            return false;
        }
        int url_len = int(buff[4]);
        if(len != 7 + url_len)
        {
            std::cout<<"length err, not "<<7+url_len<<", but "<<len<<"\n";
            return false;
        }
        ip_or_domain = std::string(&buff[5], url_len);
        short* p = (short*)(buff+len-2);
        port = *p;
        port = ntohs(port);
    }
    else
    {
        std::cout<<"unsupported type "<<int(buff[3])<<"\n";
        return false;
    }
    return true;
}

void SSSession::LocalStart()
{
	UpdateNexttimeoutTime();
	auto self = shared_from_this();
	// recv 05 01 00
	local_socket_.async_receive(asio::buffer(local_buffer_), [self, this] EC_LEN_RETURN
		//check the value
		if(len < 3 || local_buffer_[0] != 0x05 || local_buffer_[1] != 0x01 || local_buffer_[2] != 0x00)
		{
			log("fd=%d, %s", local_socket_.native_handle(), "recv 05 01 00 err");
			return;
		}

		// send 05 00 to client
		remote_buffer_[0] = 0x05;
		remote_buffer_[1] = 0x00;
		UpdateNexttimeoutTime();
		asio::async_write(local_socket_, asio::buffer(remote_buffer_, 2), [self, this] EC_LEN_RETURN
			// receive 05 01 00 01/03 ... from client
			UpdateNexttimeoutTime();
			local_socket_.async_receive(asio::buffer(local_buffer_), [self, this] EC_LEN_RETURN
			    Package p(local_fd_);
		        if(! GetIpPort(local_buffer_, len, request_domain_, request_port_))
		        {
		            ERROR_RETURN
		        }
		        LogPkg();

				// connect to server
		        UpdateNexttimeoutTime();
				remote_socket_.async_connect(SSConfig::Instance().ServerEndpoint(), [self, this, len](const asio::error_code& ec) {
		            ERROR_RETURN
					// encrypt and send to remote
					Cypher::Instance().Encrypt(local_buffer_, len);
		            UpdateNexttimeoutTime();
					asio::async_write(remote_socket_, asio::buffer(local_buffer_, len), [self, this] EC_LEN_RETURN
						// receive encrypted 05 00 00 ... from remote
						UpdateNexttimeoutTime();
						remote_socket_.async_receive(asio::buffer(remote_buffer_), [self, this] EC_LEN_RETURN
							Cypher::Instance().Decrypt(remote_buffer_, len);
					        if(! GetIpPort(remote_buffer_, len, request_ip_, request_port_))
					        {
					            ERROR_RETURN
					        }
					        LogPkg();
							// reply to client
					        UpdateNexttimeoutTime();
							asio::async_write(local_socket_, asio::buffer(remote_buffer_, len), [self, this] EC_LEN_RETURN
								// now handshake is over, begin to transfer data
								RecvEncryptSend(local_socket_, remote_socket_, local_buffer_);
								RecvDecryptSend(remote_socket_, local_socket_, remote_buffer_);
							});
						});
					});
				});
			});
		});
	});
}

void SSSession::Close(const asio::error_code& ec)
{
    LogPkg(true);
	local_socket_.close();
	remote_socket_.close();
}

//============================================================
void Socks5Server::LocalStart()
{
	auto p = std::make_shared<SSSession>(io_context_, *this);
	acceptor_.async_accept(p->local_socket_, [p, this](const asio::error_code& err)
	{
		if(! err)
		{
		    p->local_fd_ = p->local_socket_.native_handle();
		    TimeoutManager::Instance()->AddSocket(p.get());
		    UdpSend(Package(p->local_fd_));
			p->LocalStart();
			LocalStart();
		}
		else
		{
			log("accept error, msg: %s", err.message().c_str());
		}
	});
}

std::string& StringFormat(std::string& buff, const std::string fmt_str, ...)
{
    size_t n = 256;

    if(buff.size() < n)
    {
        buff.resize(n);
    }
    else
    {
        n = buff.size();
    }

    while(1)
    {
        va_list ap;
        va_start(ap, fmt_str);
        const int final_n = vsnprintf(&buff[0], n, fmt_str.c_str(), ap);
        va_end(ap);
        if(final_n < 0) // 文档说了返回负值表示encoding error
        {
            //n += size_t(-final_n);
            buff = "encoding error";
            break;
        }

        if(static_cast<size_t>(final_n) >= n)
        {
            n += static_cast<size_t>(final_n) - n + 1;
            if(n > 4096) // 免得分配的内存太大不能控制
            {
                buff = "string too long, larger then 4096...";
                break;
            }
            buff.resize(n);
        }
        else
        {
            buff[final_n] = '\0';
            buff.resize(final_n);
            break;
        }
    }
    return buff;
}

void Socks5Server::UdpSend(const Package& p)
{
    std::string str = p.ToString();
    udp_socket_.send_to(asio::buffer(str), udp_dest_);
    //std::cout<<str<<"\n";
}

void SSSession::LogPkg(bool close, bool period)
{
    Package p(local_fd_);
    if(close)
    {
        p.fd = - p.fd;
    }
    else
    {
        if(period)
        {
            auto now = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
            int t = ms.count() / 100; // 每隔100ms发一次
            if(t == last_send_time_)
            {
                return;
            }
            last_send_time_ = t;
        }
        p.start_time = std::chrono::duration_cast<std::chrono::seconds>(
                start_time_.time_since_epoch()).count();
        p.domain = request_domain_;
        p.ip = request_ip_;
        p.port = request_port_;
        p.recved = local_recved_;
        p.sended = local_sended_;
    }
    socks5_server_.UdpSend(p);
}

void TimeoutManager::AddSocket(const SSSession* s)
{
	int fd = s->local_fd_;
	if(fd >= sessions_.size())
	{
		log("fd=%d too large\n", fd);
		return;
	}

	if(sessions_[fd] != nullptr)
	{
		log("fd=%d exists already\n", fd);
		return;
	}

	sessions_[fd] = const_cast<SSSession*>(s);
}

void TimeoutManager::RemoveSocket(const SSSession* s)
{
	int fd = s->local_fd_;
	if(sessions_[fd] == nullptr)
	{
		log("fd=%d nothing to remove\n", fd);
		return;
	}

	sessions_[fd] = nullptr;
}

void TimeoutManager::Scan()
{
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	for(SSSession* s : sessions_)
	{
		if(!s)
			continue;
		if(s->next_timeout_time_ < now)
		{
			log("fd=%d close due to timeout\n", s->local_fd_);
			asio::error_code ec;
			s->Close(ec);
		}
	}
}

SSSession::~SSSession()
{
	TimeoutManager::Instance()->RemoveSocket(this);
}
