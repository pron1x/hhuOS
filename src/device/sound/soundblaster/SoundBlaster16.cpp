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

#include "SoundBlaster16.h"
#include "device/isa/Isa.h"
#include "kernel/system/System.h"
#include "kernel/service/InterruptService.h"

namespace Device::Sound {

SoundBlaster16::SoundBlaster16(uint16_t baseAddress, uint8_t irqNumber, uint8_t dmaChannel8, uint8_t dmaChannel16) :
    SoundBlaster(baseAddress), mixerAddressPort(baseAddress + 0x04), mixerDataPort(baseAddress + 0x05),
    irqNumber(irqNumber), dmaChannel8(dmaChannel8), dmaChannel16(dmaChannel16) {}

void SoundBlaster16::setSamplingRate(uint16_t samplingRate, uint8_t channels, uint8_t bits) {
    if (channels > 2) {
        Util::Exception::throwException(Util::Exception::INVALID_ARGUMENT, "SoundBlaster16: Too many channels!");
    }
    if (bits != 8 && bits != 16) {
        Util::Exception::throwException(Util::Exception::INVALID_ARGUMENT, "SoundBlaster16: Illegal sample format!");
    }

    writeToDSP(0x41);
    writeToDSP(static_cast<uint8_t>((samplingRate & 0xff00) >> 8));
    writeToDSP(static_cast<uint8_t>(samplingRate & 0x00ff));

    currentSamplingRate = samplingRate;
    currentChannels = channels;
    currentBits = bits;
}

void SoundBlaster16::setBufferSize(uint32_t bufferSize) {
    uint8_t mode;
    if (currentChannels == 1) {
        mode = currentBits == 8 ? 0x00 : 0x10;
    } else {
        mode = currentBits == 8 ? 0x20 : 0x30;
    }

    if (currentBits == 16) {
        bufferSize /= 2;
    }

    writeToDSP(mode);
    writeToDSP((bufferSize - 1) & 0x00ff);
    writeToDSP(((bufferSize - 1) & 0xff00) >> 8);
}

void SoundBlaster16::stopAutoInitialize() {
    writeToDSP(currentBits == 16 ? 0xd9 : 0xda);
}

void SoundBlaster16::prepareDma(uint16_t addressOffset, uint32_t bufferSize, bool autoInitialize) {
    uint8_t dmaChannel = currentBits == 8 ? dmaChannel8 : dmaChannel16;

    if (currentBits == 16) {
        bufferSize /= 2;
    }

    Isa::selectChannel(dmaChannel);
    Isa::setMode(dmaChannel, Isa::READ, autoInitialize, false, Isa::SINGLE_TRANSFER);
    Isa::setAddress(dmaChannel,reinterpret_cast<uint32_t>(Kernel::System::getService<Kernel::MemoryService>().getPhysicalAddress(dmaMemory)) + addressOffset);
    Isa::setCount(dmaChannel, bufferSize - 1);
    Isa::deselectChannel(dmaChannel);
}

void SoundBlaster16::playPcmData(const uint8_t *data, uint32_t size) {
    uint8_t commandByte = currentBits == 8 ? 0xc6 : 0xb6;
    uint32_t count = ((size) >= 0x10000) ? 0x10000 : size;
    uint16_t addressOffset = 0;
    bool firstBlock = true;
    auto dmaAddress = Util::Memory::Address<uint32_t>(dmaMemory);
    auto dataAddress = Util::Memory::Address<uint32_t>(data);

    soundLock.acquire();

    dmaAddress.copyRange(dataAddress, count);

    prepareDma(addressOffset, size < 0x10000 ? size : 0x10000);
    writeToDSP(commandByte);
    setBufferSize(size < 0x10000 ? size : 0x8000);

    for (uint32_t i = 0x10000; i < size; i += 0x8000) {
        if (i + 0x8000 >= size) {
            stopAutoInitialize();
        }

        waitForInterrupt();

        count = ((size - i) >= 0x8000) ? 0x8000 : size - i;
        addressOffset = firstBlock ? 0 : 0x8000;
        dmaAddress.add(addressOffset).copyRange(dataAddress.add(i), count);
        dmaAddress.add(addressOffset).add(count).setRange(0, 0x8000 - count);

        firstBlock = !firstBlock;
        ackInterrupt();
    }

    stopAutoInitialize();
    soundLock.release();
}

void SoundBlaster16::plugin() {
    // Manually configure the DSP to use the specified DMA channels.
    if(dmaChannel8 > 3 || dmaChannel8 == 2) {
        dmaChannel8 = 1;
    }

    if(dmaChannel16 > 7 || dmaChannel16 < 5) {
        dmaChannel16 = 5;
    }

    mixerAddressPort.writeByte(0x81);
    mixerDataPort.writeByte((1 << dmaChannel8) | (1 << dmaChannel16));

    // Manually configure the DSP to use the specified IRQ number.
    mixerAddressPort.writeByte(0x80);

    switch(irqNumber) {
        case 2 :
            mixerDataPort.writeByte(0x01);
            break;
        case 5 :
            mixerDataPort.writeByte(0x02);
            break;
        case 7 :
            mixerDataPort.writeByte(0x04);
            break;
        case 10 :
            mixerDataPort.writeByte(0x08);
            break;
        default :
            irqNumber = 10;
            mixerDataPort.writeByte(0x08);
            break;

    }

    auto &interruptService = Kernel::System::getService<Kernel::InterruptService>();
    interruptService.assignInterrupt(static_cast<Kernel::InterruptDispatcher::Interrupt>(32 + irqNumber), *this);
    interruptService.allowHardwareInterrupt(static_cast<Pic::Interrupt>(irqNumber));
}

}