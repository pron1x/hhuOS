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

#include "SoundBlaster2.h"
#include "device/isa/Isa.h"
#include "kernel/system/System.h"
#include "kernel/service/InterruptService.h"

namespace Device::Sound {

SoundBlaster2::SoundBlaster2(uint16_t baseAddress, uint8_t irqNumber, uint8_t dmaChannel) :
    SoundBlaster( baseAddress), irqNumber(irqNumber), dmaChannel(dmaChannel) {}

void SoundBlaster2::setSamplingRate(uint16_t samplingRate, uint8_t channels, uint8_t bits) {
    if (channels > 1) {
        Util::Exception::throwException(Util::Exception::INVALID_ARGUMENT, "SoundBlaster2: Too many channels!");
    }
    if (bits != 8) {
        Util::Exception::throwException(Util::Exception::INVALID_ARGUMENT, "SoundBlaster2: Illegal sample format!");
    }

    auto timeConstant = static_cast<uint16_t>(65536 - (256000000 / samplingRate));
    writeToDSP(0x40);
    writeToDSP((timeConstant & 0xff00) >> 8);

    currentSamplingRate = samplingRate;
}

void SoundBlaster2::setBufferSize(uint32_t bufferSize) {
    writeToDSP(0x48);
    writeToDSP((bufferSize - 1) & 0x00ff);
    writeToDSP(((bufferSize - 1) & 0xff00) >> 8);
}

void SoundBlaster2::prepareDma(uint16_t addressOffset, uint32_t bufferSize, bool autoInitialize) {
    Isa::selectChannel(dmaChannel);
    Isa::setMode(dmaChannel, Isa::READ, autoInitialize, false, Isa::SINGLE_TRANSFER);
    Isa::setAddress(dmaChannel,reinterpret_cast<uint32_t>(Kernel::System::getService<Kernel::MemoryService>().getPhysicalAddress(dmaMemory)) + addressOffset);
    Isa::setCount(dmaChannel, bufferSize - 1);
    Isa::deselectChannel(dmaChannel);
}

void SoundBlaster2::stopAutoInitialize() {
    writeToDSP(0xda);
}

void SoundBlaster2::playPcmData(const uint8_t *data, uint32_t size) {
    uint8_t commandByte = currentSamplingRate > 23000 ? 0x90 : 0x1c;
    uint32_t count = ((size) >= 0x10000) ? 0x10000 : size;
    uint16_t addressOffset = 0;
    bool firstBlock = true;
    auto dmaAddress = Util::Memory::Address<uint32_t>(dmaMemory);
    auto dataAddress = Util::Memory::Address<uint32_t>(data);

    soundLock.acquire();
    turnSpeakerOn();

    dmaAddress.copyRange(dataAddress, count);

    prepareDma(addressOffset, size < 0x10000 ? size : 0x10000);
    setBufferSize(size < 0x10000 ? size : 0x8000);
    writeToDSP(commandByte);

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
    turnSpeakerOff();
    soundLock.release();
}

void SoundBlaster2::plugin() {
    // Older DSPs (version < 4) don't support manual IRQ- and DMA-configuration.
    // They must be configured via jumpers and there is no real way to get the IRQ- and DMA-numbers in software.
    // We just assume the DSP to use IRQ 10 and DMA channel 1, if not specified else in the constructor.
    auto &interruptService = Kernel::System::getService<Kernel::InterruptService>();
    interruptService.assignInterrupt(static_cast<Kernel::InterruptDispatcher::Interrupt>(32 + irqNumber), *this);
    interruptService.allowHardwareInterrupt(static_cast<Pic::Interrupt>(irqNumber));
}

}