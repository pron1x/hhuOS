//
// Created by hannes on 14.05.21.
//

#ifndef HHUOS_ICMP4ECHO_H
#define HHUOS_ICMP4ECHO_H

#include <kernel/network/internet/icmp/ICMP4Message.h>

class ICMP4Echo : public ICMP4Message {
private:
    typedef struct icmp4echo {
        uint8_t type;
        uint8_t code;
        uint16_t checksum;
        uint16_t identifier;
        uint16_t sequenceNumber;
    } echo_t;

    echo_t myMessage;

public:
    //Sending constructor
    ICMP4Echo(uint16_t identifier, uint16_t sequenceNumber);

    //Receiving constructor
    ICMP4Echo(IP4DataPart *dataPart);

    uint8_t copyDataTo(NetworkByteBlock *byteBlock) override;

    uint16_t getLengthInBytes() override;

    uint16_t getIdentifier();

    uint16_t getSequenceNumber();

    ICMP4MessageType getICMP4MessageType() override;
};


#endif //HHUOS_ICMP4ECHO_H
