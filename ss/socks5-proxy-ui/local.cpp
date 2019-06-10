/*local.cpp
				frank May 30, 2018
*/
#include <iostream>
#include "encrypt.h"
#include "config.h"
#include "session.h"

void TimeOut(const asio::error_code& err, asio::steady_timer* t)
{
	TimeoutManager::Instance()->Scan();
	t->expires_after(std::chrono::seconds(60));
	t->async_wait(std::bind(TimeOut, std::placeholders::_1, t));
}

// usage: local -c proxy.conf
int main(int argc, const char* argv[])
{
	if(argc != 3 || std::string(argv[1]) != "-c")
	{
		std::cerr<<"usage: local -c proxy.conf"<<"\n";
		return 1;
	}

	if(! SSConfig::Instance().ReadConfig(argv[2]))
	{
		return 1;
	}

	std::cout<<"local listen at port:"<<SSConfig::Instance().LocalPort()<<"\n";
	asio::ip::tcp::endpoint ep(asio::ip::tcp::v4(), SSConfig::Instance().LocalPort());
	asio::io_context io_context(1); // only one thread mode
	Socks5Server ss(io_context, ep);
	ss.LocalStart();

	asio::steady_timer timer(io_context);
	timer.expires_after(std::chrono::seconds(60));
	timer.async_wait(std::bind(TimeOut, std::placeholders::_1, &timer));

	io_context.run();
	return 0;
}


