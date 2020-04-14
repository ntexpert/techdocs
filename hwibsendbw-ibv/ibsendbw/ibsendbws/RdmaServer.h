//
// Created by cj on 1/7/20.
//

#ifndef SOFT_RDMA_RDMA_SERVER_H
#define SOFT_RDMA_RDMA_SERVER_H

#include <string>
#include <rdma/rdma_cma.h>



class RdmaServer {
public:
    RdmaServer();
    ~RdmaServer();
    void start();

private:
    std::string endpoint;
    std::string port;
    rdma_cm_id * listenId;
    int sendFlags;
};




#endif //SOFT_RDMA_RDMA_SERVER_H
