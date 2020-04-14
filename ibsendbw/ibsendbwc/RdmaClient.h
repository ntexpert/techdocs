//
// Created by cj on 1/7/20.
//

#ifndef SOFT_RDMA_RDMA_CLIENT_H
#define SOFT_RDMA_RDMA_CLIENT_H

#include <string>
#include <rdma/rdma_cma.h>



class RdmaClient {
public:
    RdmaClient();
    ~RdmaClient();

    void connect();

private:
    std::string endpoint;
    std::string port;
    rdma_cm_id * connectId;
    int sendFlags;
};



#endif //SOFT_RDMA_RDMA_CLIENT_H
