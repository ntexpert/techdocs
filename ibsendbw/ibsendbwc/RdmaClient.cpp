//
// Created by cj on 1/7/20.
//
#include <unistd.h>
#include "RdmaClient.h"
#include <memory>
#include <functional>
#include <netdb.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <iostream>
#include <thread>

static void newThreadCallback(rdma_cm_id * connectId)
{	
	while (1)
	{
		try 
		{

		}
		catch (const std::exception &ex) {
			std::cerr << "cqThread exit with exception " << ex.what() << "\n";
		}
		catch (...) {
			std::cerr << "cqThread exit with unknown error\n";
		}
	}

}


//192.168.102.139
RdmaClient::RdmaClient()
    : endpoint("192.168.219.8")
    , port("9090")
    , connectId(nullptr)
    , sendFlags(0)
{
}

RdmaClient::~RdmaClient() {
    if (connectId) {
        rdma_destroy_ep(connectId);
        connectId = nullptr;
    }
    std::cout << "client stopped\n";
}

void RdmaClient::connect() {
    std::cout << "client start connect\n";
	int iGo = 0;
	while (iGo == 0)
	{
		sleep(1);
		iGo++;
		std::cout << "sleep 1 sec\n";
		
	}

	int buffer_size = 4096;//4096

    rdma_addrinfo hints, *res;
    memset(&hints, 0, sizeof(rdma_addrinfo));
    hints.ai_port_space = RDMA_PS_TCP;
    auto ret = rdma_getaddrinfo((char*)endpoint.c_str(), (char*)port.c_str(), &hints, &res);
    if (ret) {
        std::cerr << "client rdma_getaddrinfo failed: " << ret << " " << gai_strerror(ret) << "\n";
        return;
    }
    std::unique_ptr<rdma_addrinfo, std::function<void (rdma_addrinfo*)>> addrInfos(res, [](rdma_addrinfo * res) {
        rdma_freeaddrinfo(res);
    });

    ibv_qp_init_attr qpInitAttr;
    memset(&qpInitAttr, 0, sizeof(ibv_qp_init_attr));
	qpInitAttr.cap.max_send_wr = 128;
	qpInitAttr.cap.max_recv_wr = 1;
	qpInitAttr.cap.max_send_sge = 1;
	qpInitAttr.cap.max_recv_sge = 1;
    qpInitAttr.cap.max_inline_data = 64;
    qpInitAttr.qp_context = connectId;
    qpInitAttr.sq_sig_all = 1;
    ret = rdma_create_ep(&connectId, res, nullptr, &qpInitAttr);
    if (ret) {
        std::cerr << "client rdma_create_ep failed: " << ret << " " << strerror(ret) << "\n";
        return;
    }
    if (qpInitAttr.cap.max_inline_data >= 256) {
        sendFlags = IBV_SEND_INLINE;
        std::cout << "client qp max inline data " << qpInitAttr.cap.max_inline_data << "\n";
    } else {
        std::cout << "client rdma device did not support IBV_SEND_INLINE, using seg sends\n";
    }


	uint8_t *sendMessageBuffer = nullptr;
	uint8_t *receiveMessageBuffer = nullptr;
	

	receiveMessageBuffer = (uint8_t*)calloc(1, buffer_size);
	if (!receiveMessageBuffer) {
		fprintf(stderr, "Failed to allocate RDMA receiveMessageBuffer\n");
		return;
	}

	sendMessageBuffer = (uint8_t*)calloc(128, buffer_size);
	if (!sendMessageBuffer) {
		fprintf(stderr, "Failed to allocate RDMA sendMessageBuffer\n");
		return;
	}

    auto pReceiveMessage = rdma_reg_msgs(connectId, receiveMessageBuffer, buffer_size);
    if (!pReceiveMessage) {
        std::cerr << "client rdma_reg_msgs for receive buffer failed \n";
        return;
    }
    std::unique_ptr<ibv_mr, std::function<void (ibv_mr*)>> receiveMsg(pReceiveMessage, [](ibv_mr * mr) {
        auto r = rdma_dereg_mr(mr);
        if (r) {
            std::cerr << "client rdma_dereg_mr for receive buffer failed: " << r << " " << strerror(r) << "\n";
        }
    });

	ibv_mr* pSendMessage[128];
	int*    pContext[128];
	for (int l = 0; l < 128; l++)
	{
		pContext[l] = (int*)malloc(sizeof(int));
		*pContext[l] = l;
	}


	for (int i = 0; i < 128; i++)
	{

		pSendMessage[i] = rdma_reg_msgs(connectId, sendMessageBuffer + i* buffer_size, buffer_size);
		if (!pSendMessage[i]) {
			std::cerr << "client rdma_reg_msgs for send buffer failed \n";
			return;
		}
	}

	//
	//need free finally
	//


    ret = rdma_post_recv(connectId, nullptr, receiveMessageBuffer, buffer_size, pReceiveMessage);
    if (ret) {
        std::cerr << "client rdma_post_recv failed: " << ret << " " << strerror(ret) << "\n";
        return;
    }

    ret = rdma_connect(connectId, nullptr);
    if (ret) {
        std::cerr << "client rdma_connect failed: " << ret << " " << strerror(ret) 
			      << "event: " << connectId->event->event << "status: " << connectId->event->status <<"\n";
        return;
    }

    std::unique_ptr<rdma_cm_id, std::function<void(rdma_cm_id*)>> releaseInId(connectId, [](rdma_cm_id * inId){
        rdma_disconnect(inId);
    });


	//std::thread cqThread(newThreadCallback, connectId);


	while (1)
	{

		for (int k = 0; k < 128; k++)
		{

			ret = rdma_post_send(connectId, pContext[k], sendMessageBuffer + k * buffer_size, buffer_size, pSendMessage[k], sendFlags);
			if (ret) {
				std::cerr << "client rdma_post_send failed: " << ret << " " << strerror(ret)
					<< "event: " << connectId->event->event << "status: " << connectId->event->status
					<< "error: " << errno << "\n";
				return;
			}

		}

		for (int j = 0; j < 128; j++)
		{
			ibv_wc wc;
			auto ret = 0;
			do {
				ret = rdma_get_send_comp(connectId, &wc);
			} while (ret == 0);

			if (ret < 0) {
				std::cerr << "client rdma_get_send_comp failed: " << ret << " " << strerror(ret) << "\n";
				return;
			}
			else {
				int index = *((int*)wc.wr_id);
				std::cout << "client send message success length :" << wc.byte_len
					<< " wrid: " << wc.wr_id
					<< " index: " << index << "\n";
			}
		}
	}
}

