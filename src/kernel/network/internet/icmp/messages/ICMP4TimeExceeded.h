//
// Created by hannes on 18.05.21.
//

#ifndef HHUOS_ICMP4TIMEEXCEEDED_H
#define HHUOS_ICMP4TIMEEXCEEDED_H


#include <kernel/network/internet/icmp/ICMP4Message.h>

class ICMP4TimeExceeded : public ICMP4Message {
    //Sending constructor
    ICMP4TimeExceeded();

    //Receiveing constructor
    ICMP4TimeExceeded(IP4DataPart *dataPart);

    uint8_t copyDataTo(NetworkByteBlock *byteBlock) override;

    uint16_t getLengthInBytes() override;

public:
    ICMP4MessageType getICMP4MessageType() override;
};


#endif //HHUOS_ICMP4TIMEEXCEEDED_H
