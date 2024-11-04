#include <iostream>
#include "include/TcpServer.h"
int main()
{

    std::unique_ptr<TcpServer> tpcserver (new TcpServer);
    tpcserver->start("127.0.0.1",4321);
    return 0;
}

