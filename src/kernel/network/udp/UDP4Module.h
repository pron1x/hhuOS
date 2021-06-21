//
// Created by hannes on 17.05.21.
//

#ifndef HHUOS_UDP4MODULE_H
#define HHUOS_UDP4MODULE_H


#include <kernel/network/NetworkEventBus.h>
#include <kernel/network/udp/sockets/UDP4SocketController.h>

namespace Kernel {
    class UDP4Module : public Receiver {
    private:
        NetworkEventBus *eventBus = nullptr;
        Util::HashMap<uint16_t, UDP4SocketController *> *sockets = nullptr;
        Spinlock *socketAccessLock = nullptr;

    public:
        explicit UDP4Module(NetworkEventBus *eventBus);

        Logger &log = Logger::get("UDP4Module");

        /**
         * Inherited method from Receiver.
         * This method is meant to be overridden and
         * implemented by this class.
         */
        void onEvent(const Event &event) override;

        uint8_t registerControllerFor(uint16_t destinationPort, UDP4SocketController *controller);

        uint8_t unregisterControllerFor(uint16_t destinationPort);

        virtual ~UDP4Module();
    };

}

#endif //HHUOS_UDP4MODULE_H
