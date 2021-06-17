//
// Created by hannes on 17.06.21.
//

#include "UDP4ServerSocket.h"
namespace Kernel{
    UDP4ServerSocket::UDP4ServerSocket(uint16_t listeningPort) {
        this->listeningPort=listeningPort;
        networkService = System::getService<NetworkService>();
        controller = networkService->createSocketController();
    }

    UDP4ServerSocket::~UDP4ServerSocket() {
        close();
        delete controller;
    }

    uint8_t UDP4ServerSocket::bind() {
        return networkService->registerSocketController(listeningPort, controller);
    }

    uint8_t UDP4ServerSocket::close() {
        return networkService->unregisterSocketController(listeningPort);
    }

    //Server send()
    uint8_t UDP4ServerSocket::send(IP4Address *givenDestination, uint16_t givenRemotePort, void *dataBytes, size_t length) {
        if (
                dataBytes == nullptr ||
                givenDestination == nullptr ||
                length == 0 ||
                givenRemotePort == 0
                ) {
            return 1;
        }
        auto *byteBlock = new NetworkByteBlock(length);
        if(byteBlock->append(dataBytes,length)){
            delete byteBlock;
            return 1;
        }

        controller->publishSendEvent(
                new IP4Address(givenDestination),
                new UDP4Datagram(this->listeningPort, givenRemotePort, byteBlock)
        );
        return 0;
    }

    //Extended receive() for server and clients who need to know IP4 or UDP4 headers
    int UDP4ServerSocket::receive(void *targetBuffer, size_t length, IP4Header **ip4Header, UDP4Header **udp4Header) {
        return controller->receive(targetBuffer, length, ip4Header, udp4Header);
    }
}