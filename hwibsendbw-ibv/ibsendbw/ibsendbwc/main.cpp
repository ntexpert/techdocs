

#include <signal.h>
#include <iostream>
#include "RdmaClient.h"



using namespace std;




//static void *signal_handle_thread(void *arg) {
//    return nullptr;
//}



int main(int argc, char **argv) {
	int p = 0;

  	printf("Enter Wayne 3 inside attach cb int: ");
	scanf("%d", &p);

    try {
        RdmaClient client;
        client.connect();
    } catch (const std::exception &ex) {
        std::cerr << "RdmaClient exit with exception " << ex.what() << "\n";
    } catch (...) {
        std::cerr << "RdmaClient exit with unknown error\n";
    }

    std::cout << "DRPC client"<<std::endl;

    return 0;
}
