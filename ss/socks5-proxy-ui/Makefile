all:
	clang++ -std=c++2a -DASIO_STANDALONE local.cpp session.cpp \
../socks5-proxy/config.cpp \
../socks5-proxy/encrypt.cpp \
-o local \
-I../asio/asio/include \
-I../socks5-proxy \
-lstdc++

clean:
	rm -f local
