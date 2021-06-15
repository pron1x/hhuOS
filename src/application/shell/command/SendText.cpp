//
// Created by hannes on 13.06.21.
//

#include <kernel/network/internet/addressing/IP4Address.h>
#include <kernel/network/applications/TextPrintServer.h>
#include "SendText.h"

SendText::SendText(Shell &shell) : Command(shell) {

}

void SendText::execute(Util::Array<String> &args) {
    Util::ArgumentParser parser(getHelpText(), 1);

    if (!parser.parse(args)) {
        stderr << args[0] << ": " << parser.getErrorString() << endl;
        return;
    }

    auto *server = new TextPrintServer(serverPort);
    if(server->start()){
        stderr << "Starting server failed!" << endl;
        delete server;
        return;
    }
//
//    auto *localhost = new IP4Address(127, 0, 0, 1);
//
//    auto *testString = new String("Hello world! Now it works...\0");
//    stdout << "CLIENT: Sending text '" << *testString << "'" << endl;
//
//    auto *sendSocket = new Kernel::UDP4Socket(localhost, serverPort);
//    sendSocket->send((char *)*testString,testString->length());
//    delete sendSocket;
//    delete testString;

//    auto *response = new char [testString->length()];
//    sendSocket->receive(response);
//
//    stdout << "CLIENT: Response was '" << response << "'" << endl;
//    if(server->stop()){
//        stderr << "Stopping server failed!" << endl;
//        return;
//    }
//
//    delete server;
}

const String SendText::getHelpText() {
    return "Utility for testing our UDP/IP protocol stack by sending and receiving a given text\n\n"
           "Usage: sendtext [TEXT]\n"
           "Options:\n"
           "   -h, --help: Show this help-message";
}