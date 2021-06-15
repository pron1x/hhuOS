//
// Created by hannes on 15.05.21.
//

#include "EthernetModule.h"

namespace Kernel {
    EthernetModule::EthernetModule(NetworkEventBus *eventBus) {
        this->eventBus = eventBus;
        ethernetDevices = new Util::HashMap<String *, EthernetDevice *>();
    }

    void EthernetModule::registerNetworkDevice(NetworkDevice *networkDevice) {
        if (networkDevice == nullptr) {
            log.error("Given network device was null, not registering it");
            return;
        }
        this->registerNetworkDevice(
                new String(String::format("eth%d", deviceCounter)),
                networkDevice
        );
        deviceCounter++;
    }

    void EthernetModule::registerNetworkDevice(String *identifier, NetworkDevice *networkDevice) {
        if (identifier == nullptr) {
            log.error("Given identifier was null, not registering it");
            return;
        }
        if (networkDevice == nullptr) {
            log.error("Given network device was null, not registering it");
            return;
        }
        if (ethernetDevices == nullptr) {
            log.error("Internal list of ethernet devices was null, not registering network device");
            return;
        }
        //Return if an ethernet device connected to the same network device could be found
        if (ethernetDevices->containsKey(identifier)) {
            log.error("Given identifier already exists, ignoring it");
            return;
        }
        //Add a new connected ethernet device if no duplicate found
        this->ethernetDevices->put(identifier, new EthernetDevice(identifier, networkDevice));
    }

    void EthernetModule::unregisterNetworkDevice(NetworkDevice *networkDevice) {
        EthernetDevice *connectedDevice = getEthernetDevice(networkDevice);
        if (connectedDevice == nullptr) {
            log.error("No connected ethernet device could be found, not unregistering network device");
            return;
        }
        if (ethernetDevices == nullptr) {
            log.error("Internal list of ethernet devices was null, not unregistering network device");
            return;
        }
        ethernetDevices->remove(connectedDevice->getIdentifier());
    }

    void EthernetModule::collectEthernetDeviceAttributes(Util::ArrayList<String> *strings) {
        if (ethernetDevices == nullptr ||
            strings == nullptr
                ) {
            return;
        }
        for (String *currentKey:ethernetDevices->keySet()) {
            strings->add(getEthernetDevice(currentKey)->asString());
        }
    }

//Get ethernet device via identifier
    EthernetDevice *EthernetModule::getEthernetDevice(String *identifier) {
        if (ethernetDevices == nullptr) {
            log.error("Internal list of ethernet devices was null, not searching ethernet device");
            return nullptr;
        }
        if (ethernetDevices->containsKey(identifier)) {
            return ethernetDevices->get(identifier);
        }
        return nullptr;
    }

//Get ethernet device via network device it's connected to
    EthernetDevice *EthernetModule::getEthernetDevice(NetworkDevice *networkDevice) {
        if (ethernetDevices == nullptr) {
            log.error("Internal list of ethernet devices was null, not searching ethernet device");
            return nullptr;
        }
        for (String *current:ethernetDevices->keySet()) {
            if (getEthernetDevice(current)->connectedTo(networkDevice)) {
                return getEthernetDevice(current);
            }
        }
        return nullptr;
    }

    void EthernetModule::onEvent(const Event &event) {
        if ((event.getType() == EthernetSendEvent::TYPE)) {
            EthernetDevice *outDevice = ((EthernetSendEvent &) event).getOutDevice();
            EthernetFrame *outFrame = ((EthernetSendEvent &) event).getEthernetFrame();

            if (outDevice == nullptr) {
                log.error("Outgoing device was null, discarding frame");
                //delete on NULL objects simply does nothing
                delete outFrame;
                return;
            }
            if (outFrame == nullptr) {
                log.error("Outgoing frame was null, ignoring");
                return;
            }
            if (outFrame->getLengthInBytes() == 0) {
                log.error("Outgoing frame was empty, discarding frame");
                delete outFrame;
                return;
            }
            switch (outDevice->sendEthernetFrame(outFrame)) {
                case ETH_DELIVER_SUCCESS: {
                    //Frame will be deleted at the end of this method
                    break;
                }
                case ETH_FRAME_NULL: {
                    log.error("Outgoing frame was null, ignoring");
                    return;
                }
                case ETH_DEVICE_NULL: {
                    log.error("Outgoing device was null, discarding frame");
                    break;
                }
                case ETH_COPY_BYTEBLOCK_FAILED: {
                    log.error("Copy to byteBlock failed, discarding frame");
                    break;
                }
                case ETH_COPY_BYTEBLOCK_INCOMPLETE: {
                    log.error("Copy to byteBlock incomplete, discarding frame");
                    break;
                }
                case BYTEBLOCK_NETWORK_DEVICE_NULL: {
                    log.error("Network device in byteBlock was null, discarding frame");
                    break;
                }
                case BYTEBLOCK_BYTES_NULL: {
                    log.error("Internal bytes in byteBlock were null, discarding frame");
                    break;
                }
                default:
                    log.error("Sending failed with unknown error, discarding frame");
                    break;
            }
            //We are done here, delete outFrame no matter if sending worked or not
            //-> we still have our log entry if sending failed
            //NOTE: Any embedded data (like an IP4Datagram) will be deleted here
            delete outFrame;
            return;
        }
        if ((event.getType() == EthernetReceiveEvent::TYPE)) {
            auto *ethernetHeader = ((EthernetReceiveEvent &) event).getEthernetHeader();
            auto *input = ((EthernetReceiveEvent &) event).getInput();
            if (ethernetHeader == nullptr) {
                log.error("Incoming EthernetHeader was null, discarding input");
                delete input;
                return;
            }
            if (input == nullptr) {
                log.error("Incoming input was null, discarding EthernetHeader");
                delete ethernetHeader;
                return;
            }
            //TODO: Check frame's Source-MAC if it's for us or at least a BROADCAST message
            switch (ethernetHeader->getEtherType()) {
                case EthernetDataPart::EtherType::IP4: {
                    auto *ip4Header = new IP4Header();
                    if (ip4Header->parse(input)) {
                        log.error("Could not assemble IP4 header, discarding data");
                        //ip4Header is not part of inFrame here
                        //-> we need to delete it separately!
                        delete ip4Header;
                        delete input;
                        break;
                    }
                    //send input to next module via EventBus
                    eventBus->publish(new IP4ReceiveEvent(ip4Header, input));
                    break;
                }
                case EthernetDataPart::EtherType::ARP: {
                    auto *arpMessage = new ARPMessage();
                    if (arpMessage->parse(input)) {
                        log.error("Could not assemble ARP message, discarding data");
                        //arpMessage is not part of inFrame here
                        //-> we need to delete it separately!
                        delete arpMessage;
                        delete input;
                        break;
                    }
                    //Input has been parsed completely here, can be deleted
                    delete input;
                    //send input to next module via EventBus
                    eventBus->publish(new ARPReceiveEvent(arpMessage));
                    break;
                }
                default: {
                    log.error("EtherType of incoming EthernetFrame not supported, discarding data");
                    delete input;
                    break;
                }
            }
            //We are done here, cleanup memory
            //Input will be used in next module, so no delete here
            delete ethernetHeader;
        }
    }
}