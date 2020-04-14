//
// Created by cj on 1/7/20.
//

#include "RdmaServer.h"
#include <memory>
#include <iostream>
#include <functional>
#include <netdb.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>




RdmaServer::RdmaServer()
    : endpoint("0.0.0.0")
    , port("9090")
    , listenId(nullptr)
    , sendFlags(0)
{
}

RdmaServer::~RdmaServer() {
    if (listenId) {
        rdma_destroy_ep(listenId);
        listenId = nullptr;
    }
    std::cout << "server stopped\n";
}

void RdmaServer::start() {
    std::cout << "server start listen\n";

	int buffer_size = 4096;//4096

    rdma_addrinfo hints, *res;
    memset(&hints, 0, sizeof(rdma_addrinfo));
    hints.ai_flags = RAI_PASSIVE;
    hints.ai_port_space = RDMA_PS_TCP;
    auto ret = rdma_getaddrinfo((char*)endpoint.c_str(), (char*)port.c_str(), &hints, &res);
    if (ret) {
        std::cerr << "server rdma_getaddrinfo failed: " << ret << " " << gai_strerror(ret) << "\n";
        return;
    }
    std::unique_ptr<rdma_addrinfo, std::function<void (rdma_addrinfo*)>> addrInfos(res, [](rdma_addrinfo * res) {
        rdma_freeaddrinfo(res);
    });

    ibv_qp_init_attr qpInitAttr;
    memset(&qpInitAttr, 0, sizeof(ibv_qp_init_attr));
	qpInitAttr.cap.max_send_wr = 1;
	qpInitAttr.cap.max_recv_wr = 128;
	qpInitAttr.cap.max_send_sge = 1;
	qpInitAttr.cap.max_recv_sge = 1;
    qpInitAttr.cap.max_inline_data = 64;
    qpInitAttr.sq_sig_all = 1;
    ret = rdma_create_ep(&listenId, res, nullptr, &qpInitAttr);
    if (ret) {
        std::cerr << "server rdma_create_ep failed: " << ret << " " << strerror(ret) << "\n";
        return;
    }

    ret = rdma_listen(listenId, 0);
    if (ret) {
        std::cerr << "server rdma_listen failed: " << ret << " " << strerror(ret) << "\n";
        return;
    }
	else
	{
		std::cout << "server wait client to connect ..." << std::endl;
	}

    rdma_cm_id * inId;
    ret = rdma_get_request(listenId, &inId);
    if (ret) {
        std::cerr << "server rdma_get_request failed: " << ret << " " << strerror(ret) 
		          << "event: " << listenId->event->event << "status: " << listenId->event->status << "\n";
        return;
    }
	else
	{
		std::cout << "server connected ...inId: " << inId << std::endl;
	}
    std::unique_ptr<rdma_cm_id, std::function<void (rdma_cm_id*)>> incomingId(inId, [](rdma_cm_id * inId) {
        rdma_destroy_ep(inId);
    });

    ibv_qp_attr qpAttr;
    memset(&qpAttr, 0, sizeof(ibv_qp_attr));
    memset(&qpInitAttr, 0, sizeof(ibv_qp_init_attr));
    ret = ibv_query_qp(inId->qp, &qpAttr, IBV_QP_CAP, &qpInitAttr);
    if (ret) {
        std::cerr << "server ibv_query_qp failed: " << ret << " " << strerror(ret) << "\n";
        return;
    }
    if (qpInitAttr.cap.max_inline_data >= 256) {
        sendFlags = IBV_SEND_INLINE;
        std::cout << "server qp max inline data " << qpInitAttr.cap.max_inline_data << "\n";
    } else {
        std::cout << "server rdma device did not support IBV_SEND_INLINE, using seg sends\n";
    }

    uint8_t *sendMessageBuffer    = nullptr;
    uint8_t *receiveMessageBuffer = nullptr;

	receiveMessageBuffer = (uint8_t*)calloc(128, buffer_size);
	if (!receiveMessageBuffer) {
		fprintf(stderr, "Failed to allocate RDMA receiveMessageBuffer\n");
		return;
	}

	sendMessageBuffer = (uint8_t*)calloc(1, buffer_size);
	if (!sendMessageBuffer) {
		fprintf(stderr, "Failed to allocate RDMA sendMessageBuffer\n");
		return;
	}

	ibv_mr* pReceiveMessage[128];
	int*    pContext[128];
	for (int l = 0; l < 128; l++)
	{
		pContext[l] = (int*)malloc(sizeof(int));
		*pContext[l] = l;
	}
	

	for (int i = 0; i < 128; i++)
	{
		pReceiveMessage[i] = rdma_reg_msgs(inId, receiveMessageBuffer +i* buffer_size, buffer_size);
		if (!pReceiveMessage[i]) {
			std::cerr << "server rdma_reg_msgs for receive buffer failed \n";
			return;
		}
	}

	//
	//need free finally
	//


    auto pSendMessage = rdma_reg_msgs(inId, sendMessageBuffer, buffer_size);
    if (!pSendMessage) {
        std::cerr << "server rdma_reg_msgs for send buffer failed \n";
        return;
    }
    std::unique_ptr<ibv_mr, std::function<void (ibv_mr*)>> sendMsg(pSendMessage, [](ibv_mr * mr) {
        auto r = rdma_dereg_mr(mr);
        if (r) {
            std::cerr << "server rdma_dereg_mr for send buffer failed: " << r << " " << strerror(r) << "\n";
        }
    });

	for (int k = 0; k < 128; k++)
	{
		ret = rdma_post_recv(inId, pContext[k], receiveMessageBuffer + k* buffer_size, buffer_size, pReceiveMessage[k]);
		if (ret) {
			std::cerr << "server rdma_post_recv failed: " << ret << " " << strerror(ret) << "\n";
			return;
		}
	}

    ret = rdma_accept(inId, nullptr);
    if (ret) {
        std::cerr << "server rdma_accept failed: " << ret << " " << strerror(ret) << "\n";
        return;
    }
    std::unique_ptr<rdma_cm_id, std::function<void(rdma_cm_id*)>> releaseInId(inId, [](rdma_cm_id * inId){
        rdma_disconnect(inId);
    });

	while (1)
	{
		ibv_wc wc;
		do {
			ret = rdma_get_recv_comp(inId, &wc);
		} while (ret == 0);
		if (ret < 0) {
			std::cerr << "server rdma_get_recv_comp failed: " << ret << " " << strerror(ret) << "\n";
			return;
		}
		else {
			int index = *((int*)wc.wr_id);
			//std::cout << "server receive message length :" << wc.byte_len
			//	<< " wrid: " << wc.wr_id  
			//	<< " index: "<< index << "\n";


			//
			//do some real work here
			//

			auto ret = rdma_post_recv(inId, (int*)wc.wr_id, receiveMessageBuffer + index * buffer_size, buffer_size, pReceiveMessage[index]);
			if (ret) {
				std::cerr << "server post buffer back rdma_post_recv failed: " << ret << " " << strerror(ret) << "\n";
				return;
			}
			else
			{
				//std::cout << "wayne server post back recv buffer again:  " << pReceiveMessage[index] << "\n";
			}
		}
	}

}

