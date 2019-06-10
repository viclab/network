/*
 * co_local.cpp
 *
 *  Created on: Oct 19, 2018
 *      Author: frank
 */
#include <iostream>
#include "asio.hpp"
#include "asio/experimental.hpp"
#include "config.h"
#include "encrypt.h"

using namespace std;
using asio::ip::tcp;
using asio::ip::address;
//using asio::experimental::awaitable;
using asio::experimental::co_spawn;
using asio::experimental::detached;
namespace this_coro = asio::experimental::this_coro;

template <typename T>
using awaitable = asio::experimental::awaitable<T, asio::io_context::executor_type>;

//clang++ -std=c++14 -DASIO_STANDALONE -fcoroutines-ts -lstdc++  -I../asio/asio/include co_local.cpp config.cpp encrypt.cpp -o co_local

static const int buffer_size = 512;

awaitable<void> RecvEncryptSend(tcp::socket& sock1, tcp::socket& sock2)
{
	try{
	auto token = co_await this_coro::token();
	char buff[buffer_size];
	for(;;)
	{
		size_t n = co_await sock1.async_receive(asio::buffer(buff, buffer_size), token);
		Cypher::Instance().Encrypt(buff, n);
		co_await asio::async_write(sock2, asio::buffer(buff, n), token);
	}
	}catch(std::exception& e)
	{
		cout<<"Exception on RecvAndSend: "<<e.what()<<"\n";
	}
}

awaitable<void> Work(tcp::socket local_socket)
{
	try {
	auto executor = co_await this_coro::executor();
	auto token = co_await this_coro::token();
	//recv 05 01 00
	char buff[buffer_size];
	size_t n = co_await local_socket.async_receive(asio::buffer(buff, buffer_size), token);
	if(n != 3 || buff[0] != 0x05 || buff[1] != 0x01 || buff[2] != 0x00)
	{
		std::cout<<"recv 050100 err\n";
		co_return;
	}

	//send 05 00 to client
	buff[1] = 0x00;
	co_await asio::async_write(local_socket, asio::buffer(buff, 2), token);

	//recv 05 01 00 01/03
	n = co_await local_socket.async_receive(asio::buffer(buff, buffer_size), token);

	//connect to remote server
	tcp::socket remote_socket(executor.context());
	co_await remote_socket.async_connect(SSConfig::Instance().ServerEndpoint(), token);

	//encrypt data and send
	Cypher::Instance().Encrypt(buff, n);
	co_await asio::async_write(remote_socket, asio::buffer(buff, n), token);

	//recv encrypted 050000... from remote
	n = co_await remote_socket.async_read_some(asio::buffer(buff, buffer_size), token);

	//send 050000... to client
	Cypher::Instance().Decrypt(buff, n);
	co_await asio::async_write(local_socket, asio::buffer(buff, n), token);

	// now handshake is done, begin to transfer data
	// recv from client --> encrypt --> send to remote
	co_spawn(executor, [&local_socket, &remote_socket]() {
			return RecvEncryptSend(local_socket, remote_socket);
		}, detached);

	// recv from remote --> decrypt --> send to client
	for(;;)
	{
		size_t n = co_await remote_socket.async_receive(asio::buffer(buff, buffer_size), token);
		Cypher::Instance().Decrypt(buff, n);
		co_await asio::async_write(local_socket, asio::buffer(buff, n), token);
	}
	} catch(std::exception& e)
	{
		std::cout<<"Exception: "<<e.what()<<"\n";
	}
}

awaitable<void> listener()
{
  auto executor = co_await this_coro::executor();
  auto token = co_await this_coro::token();

  tcp::acceptor acceptor(executor.context(), tcp::endpoint(tcp::v4(), SSConfig::Instance().LocalPort()));
  for (;;)
  {
    tcp::socket socket = co_await acceptor.async_accept(token);
    co_spawn(executor,
        [socket = std::move(socket), executor]() mutable
        {
          return Work(std::move(socket));
        },
        detached);
  }
}

int main(int argc, const char* argv[])
{
	if(argc != 3 || std::string(argv[1]) != "-c")
	{
		std::cerr<<"usage: co_local -c proxy.conf"<<"\n";
		return 1;
	}

	if(! SSConfig::Instance().ReadConfig(argv[2]))
	{
		return 1;
	}

	asio::io_context io_context;
	co_spawn(io_context, listener, detached);
	io_context.run();
	return 0;
}



