//
// Created by hannes on 14.05.21.
//

#include <kernel/event/network/IP4SendEvent.h>
#include <kernel/network/NetworkDataPart.h>
#include <kernel/network/internet/icmp/ICMPEchoRequest.h>
#include "Ping.h"

Ping::Ping(Shell &shell) : Command(shell){

}

void Ping::execute(Util::Array<String> &args) {
//    Util::ArgumentParser parser(getHelpText(), 1);
//
//    if (!parser.parse(args)) {
//        stderr << args[0] << ": " << parser.getErrorString() << endl;
//        return;
//    }

    IP4Address *localhost = new IP4Address(127,0,0,1);
    ICMPEchoRequest *pingRequest = new ICMPEchoRequest();

    IP4Datagram *toBeSent = new IP4Datagram(localhost,pingRequest);

    Kernel::EventBus *eventBus = Kernel::System::getService<Kernel::EventBus>();
    eventBus->publish(
            Util::SmartPointer<Kernel::Event>(
                    new Kernel::IP4SendEvent(toBeSent)
            )
    );

}

const String Ping::getHelpText() {
    return "Utility for testing our IP protocol stack via ICMP Echo and ICMP Echo Reply\n\n"
           "Usage: ping [OPTION]\n"
           "Options:\n"
           "   -h, --help: Show this help-message";
}