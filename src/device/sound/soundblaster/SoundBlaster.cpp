/*
 * Copyright (C) 2018-2022 Heinrich-Heine-Universitaet Duesseldorf,
 * Institute of Computer Science, Department Operating Systems
 * Burak Akguel, Christian Gesse, Fabian Ruhland, Filip Krakowski, Michael Schoettner
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
 */

#include "SoundBlaster.h"
#include "kernel/system/System.h"
#include "lib/util/async/Thread.h"
#include "SoundBlaster1.h"
#include "SoundBlaster2.h"
#include "SoundBlasterPro.h"
#include "SoundBlaster16.h"
#include "kernel/service/FilesystemService.h"
#include "SoundBlasterNode.h"

namespace Device::Sound {

Kernel::Logger SoundBlaster::log = Kernel::Logger::get("Soundblaster");

SoundBlaster::SoundBlaster(uint16_t baseAddress) :
        resetPort(baseAddress + 0x06),
        readDataPort(baseAddress + 0x0a),
        writeDataPort(baseAddress + 0x0c),
        readBufferStatusPort(baseAddress + 0x0e),
        dmaMemory(Kernel::System::getService<Kernel::MemoryService>().allocateLowerMemory(64 * 1024, Util::Memory::PAGESIZE)) {}

void SoundBlaster::initialize() {
    uint16_t baseAddress = getBasePort();
    if (baseAddress == 0) {
        return;
    }

    log.info("Found base port at address [%x]", baseAddress);
    auto readDataPort = IoPort(baseAddress + 0x0a);
    auto readBufferStatusPort = IoPort(baseAddress + 0x0e);
    auto writeDataPort = IoPort(baseAddress + 0x0c);

    // Issue version command
    while((readBufferStatusPort.readByte() & 0x80) == 0x80);
    writeDataPort.writeByte(0xe1);

    // Get version
    while((readBufferStatusPort.readByte() & 0x80) != 0x80);
    uint8_t majorVersion = readDataPort.readByte();
    while((readBufferStatusPort.readByte() & 0x80) != 0x80);
    uint8_t minorVersion = readDataPort.readByte();
    log.info("Major version: [%x], Minor version: [%x]", majorVersion, minorVersion);

    SoundBlaster *soundBlaster;
    if (majorVersion == 1) {
        soundBlaster = new SoundBlaster1(baseAddress);
    } else if(majorVersion == 2) {
        soundBlaster = new SoundBlaster2(baseAddress);
    } else if(majorVersion == 3) {
        soundBlaster = new SoundBlasterPro(baseAddress);
    } else if(majorVersion >= 4) {
        soundBlaster = new SoundBlaster16(baseAddress);
    } else {
        Util::Exception::throwException(Util::Exception::ILLEGAL_STATE, "SoundBlaster: Unsupported DSP version!");
    }

    // TODO: Use ioctl for setting parameters, once it is implemented
    soundBlaster->setSamplingRate(8000, 1, 8);

    // Create filesystem node
    auto &filesystem = Kernel::System::getService<Kernel::FilesystemService>().getFilesystem();
    auto &driver = filesystem.getVirtualDriver("/device");
    auto *lfbNode = new SoundBlasterNode(soundBlaster);

    if (!driver.addNode("/", lfbNode)) {
        Util::Exception::throwException(Util::Exception::ILLEGAL_STATE, "SoundBlaster: Failed to add node!");
    }
}

bool SoundBlaster::isAvailable() {
    return getBasePort() != 0;
}

bool SoundBlaster::checkPort(uint16_t baseAddress) {
    auto resetPort = IoPort(static_cast<uint16_t>(baseAddress + 0x06));
    auto readDataPort = IoPort(static_cast<uint16_t>(baseAddress + 0x0a));
    auto readBufferStatusPort = IoPort(static_cast<uint16_t>(baseAddress + 0x0e));

    // Issue reset command
    resetPort.writeByte(0x01);
    Util::Async::Thread::sleep(Util::Time::Timestamp::ofMilliseconds(5));
    resetPort.writeByte(0x00);

    bool timeout = true;
    uint32_t timeoutTime = Util::Time::getSystemTime().toMilliseconds() + TIMEOUT;

    // Wait for read buffer to become ready
    do {
        uint8_t status = readBufferStatusPort.readByte();

        if ((status & 0x80) == 0x80) {
            timeout = false;
            break;
        }
    } while (Util::Time::getSystemTime().toMilliseconds() < timeoutTime);

    if (timeout) {
        return false;
    }

    timeoutTime = Util::Time::getSystemTime().toMilliseconds() + TIMEOUT;

    // Wait for ready code (represented by 0xaa) to appear in the read buffer
    do {
        uint8_t status = readDataPort.readByte();

        if(status == 0xaa) {
            return true;
        }
    } while (Util::Time::getSystemTime().toMilliseconds() < timeoutTime);

    return false;
}

uint16_t SoundBlaster::getBasePort() {
    for (uint16_t i = FIRST_BASE_ADDRESS; i <= LAST_BASE_ADDRESS; i += 0x10) {
        if (checkPort(i)) {
            return i;
        }
    }

    return 0;
}

uint8_t SoundBlaster::readFromDSP() {
    while ((readBufferStatusPort.readByte() & 0x80) != 0x80);
    return readDataPort.readByte();
}

void SoundBlaster::writeToDSP(uint8_t value) {
    while ((readBufferStatusPort.readByte() & 0x80) == 0x80);
    writeDataPort.writeByte(value);
}

uint8_t SoundBlaster::readFromADC() {
    writeToDSP(0x20);
    return readFromDSP();
}

void SoundBlaster::writeToDAC(uint8_t value) {
    writeToDSP(0x10);
    writeToDSP(value);
}

void SoundBlaster::turnSpeakerOn() {
    writeToDSP(0xd1);
}

void SoundBlaster::turnSpeakerOff() {
    writeToDSP(0xd3);
}

bool SoundBlaster::reset() {
    // Issue reset command
    resetPort.writeByte(0x01);
    Util::Async::Thread::sleep(Util::Time::Timestamp::ofMilliseconds(5));
    resetPort.writeByte(0x00);

    bool timeout = true;
    uint32_t timeoutTime = Util::Time::getSystemTime().toMilliseconds() + TIMEOUT;

    do {
        uint8_t status = readBufferStatusPort.readByte();

        if ((status & 0x80) == 0x80) {
            timeout = false;
            break;
        }
    } while(Util::Time::getSystemTime().toMilliseconds() < timeoutTime);

    if (timeout) {
        return false;
    }

    timeoutTime = Util::Time::getSystemTime().toMilliseconds() + TIMEOUT;

    // Wait for ready code (represented by 0xaa) to appear in the read buffer
    do {
        uint8_t status = readDataPort.readByte();

        if(status == 0xaa) {
            return true;
        }
    } while(Util::Time::getSystemTime().toMilliseconds() < timeoutTime);

    return false;
}

void SoundBlaster::ackInterrupt() {
    readBufferStatusPort.readByte();
}

void SoundBlaster::waitForInterrupt() {
    while (!receivedInterrupt);
    receivedInterrupt = false;
}

void SoundBlaster::trigger(const Kernel::InterruptFrame &frame) {
    receivedInterrupt = true;
}

}