课程《C++网络编程》的例子

编译：
git clone --recursive http://git.code.oa.com/franktang/cpp_network_example.git;
cd cpp_network_example;
mkdir -p build;
cd build;
cmake ..;
make;
编译得到的可执行文件位于bin目录中。


socks5-proxy-ui目录里面的session演示了定时器扫描法，同时也用udp实现了一个ui，需要单独编译。
编译local：
make clean all
编译ui：用qt-creater进行编译。