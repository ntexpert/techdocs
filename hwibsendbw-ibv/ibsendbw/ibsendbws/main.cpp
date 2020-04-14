

#include <signal.h>
#include <iostream>
#include "RdmaServer.h"


using namespace std;




//static void *signal_handle_thread(void *arg) {
//    return nullptr;
//}



int main(int argc, char **argv) {
	int p = 0;

  	printf("Enter Wayne 3 inside attach cb int: ");
	scanf("%d", &p);

    try {
        RdmaServer server;
        server.start();
    } catch (const std::exception &ex) {
        std::cerr << "RdmaServer exit with exception " << ex.what() << "\n";
    } catch (...) {
        std::cerr << "RdmaServer exit with unknown error\n";
    }

    std::cout << "DRPC server"<<std::endl;

    return 0;
}
