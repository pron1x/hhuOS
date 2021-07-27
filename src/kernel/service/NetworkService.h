/*
 * Copyright (C) 2018/19 Thiemo Urselmann
 * Heinrich-Heine University
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 * Note:
 * All references marked with [...] refer to the following developers manual.
 * Intel Corporation. PCI/PCI-X Family of Gigabit Ethernet Controllers Software Developer’s Manual.
 * 317453006EN.PDF Revision 4.0. 2009.
 */

#ifndef HHUOS_NETWORKSERVICE_H
#define HHUOS_NETWORKSERVICE_H

#include <kernel/network/PacketHandler.h>
#include <kernel/network/internet/IP4Module.h>
#include <kernel/network/ethernet/EthernetModule.h>
#include <kernel/network/internet/icmp/ICMP4Module.h>
#include <kernel/network/udp/UDP4Module.h>

namespace Kernel {

/**
 * This class implements the KernelService class.
 *
 * User of a network driver like those who want to send
 * packets and others as well are meant to use this class
 * to get access of a driver.
 */
    class NetworkService final : public KernelService {

    private:
        /**
         * Provide service information on the kernel log.
         */
        Logger &log = Logger::get("NetworkService");

        /**
         * A list where all drivers will be collected.
         */
        Util::ArrayList<NetworkDevice *> drivers;

        uint16_t deviceCounter = 0;
        uint8_t *loopbackBuffer = nullptr;
        uint8_t *buffers[MAX_DEVICE_COUNT]{};

        Management *management = nullptr;
        EventBus *eventBus = nullptr;

        PacketHandler *packetHandler = nullptr;
        IP4Module *ip4Module = nullptr;
        EthernetModule *ethernetModule = nullptr;
        ICMP4Module *icmp4Module = nullptr;
        UDP4Module *udp4Module = nullptr;

        Spinlock *accessLock = nullptr;

    public:
        /**
         * Constructor.
         * It registers a Loopback interface at startup.
         */
        NetworkService();

        /**
         * Destructor.
         */
        ~NetworkService();

        /**
         * The ID to identify this service among other in the kernel
         * registered services.
         */
        static constexpr const char *SERVICE_NAME = "NetworkService";

        /**
         * @return The number of registered divers.
         */
        uint32_t getDeviceCount();

        /**
         * @param index Index of the driver to fetch.
         * @return The corresponding driver.
         */
        NetworkDevice &getDriver(uint8_t index);

        /**
         * @param index Index of the driver to remove.
         */
        void removeDevice(uint8_t index);

        /**
         * Adds the driver to the list.
         * @param driver The driver to add.
         */
        void registerDevice(NetworkDevice &driver);

        UDP4SocketController *createSocketController(uint16_t bufferSize);

        uint8_t collectLinkAttributes(Util::ArrayList<String> *strings);

        uint8_t collectInterfaceAttributes(Util::ArrayList<String> *strings);

        uint8_t collectRouteAttributes(Util::ArrayList<String> *strings);

        uint8_t collectARPTables(Util::ArrayList<String> *strings);

        uint8_t assignIP4Address(const String &identifier, IP4Address *ip4Address, IP4Netmask *ip4Netmask);

        uint8_t unAssignIP4Address(const String &identifier);

        uint8_t registerSocketController(uint16_t listeningPort, UDP4SocketController *controller);

        uint8_t registerSocketController(uint16_t *listeningPortTarget, UDP4SocketController *controller);

        uint8_t unregisterSocketController(uint16_t destinationPort);

        uint8_t setDefaultRoute(IP4Address *gatewayAddress, const String &outDevice);

        uint8_t removeDefaultRoute();
    };

}

#endif
