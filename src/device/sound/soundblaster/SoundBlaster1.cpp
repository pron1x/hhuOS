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

#include "SoundBlaster1.h"
#include "device/isa/Isa.h"
#include "kernel/service/MemoryService.h"
#include "kernel/system/System.h"
#include "kernel/service/InterruptService.h"

namespace Device::Sound {

SoundBlaster1::SoundBlaster1(uint16_t baseAddress, uint8_t irqNumber, uint8_t dmaChannel) :
    SoundBlaster(baseAddress), irqNumber(irqNumber), dmaChannel(dmaChannel) {}

void SoundBlaster1::setSamplingRate(uint16_t samplingRate, uint8_t channels, uint8_t bits) {
    if (channels > 1) {
        Util::Exception::throwException(Util::Exception::INVALID_ARGUMENT, "SoundBlaster1: Too many channels!");
    }
    if (bits != 8) {
        Util::Exception::throwException(Util::Exception::INVALID_ARGUMENT, "SoundBlaster1: Illegal sample format!");
    }

    auto timeConstant = static_cast<uint16_t>(65536 - (256000000 / samplingRate));
    writeToDSP(0x40);
    writeToDSP(static_cast<uint8_t>((timeConstant & 0xff00) >> 8));
}

void SoundBlaster1::setBufferSize(uint32_t bufferSize) {
    writeToDSP(0x14);
    writeToDSP(static_cast<uint8_t>((bufferSize - 1) & 0x00ff));
    writeToDSP(static_cast<uint8_t>(((bufferSize - 1) & 0xff00) >> 8));
}

void SoundBlaster1::prepareDma(uint16_t addressOffset, uint32_t bufferSize) {
    Isa::selectChannel(dmaChannel);
    Isa::setMode(dmaChannel, Isa::READ, false, false, Isa::SINGLE_TRANSFER);
    Isa::setAddress(dmaChannel, reinterpret_cast<uint32_t>(Kernel::System::getService<Kernel::MemoryService>().getPhysicalAddress(dmaMemory)) + addressOffset);
    Isa::setCount(dmaChannel, static_cast<uint16_t>(bufferSize - 1));
    Isa::deselectChannel(dmaChannel);
}

void SoundBlaster1::playPcmData(const uint8_t *data, uint32_t size) {
    bool firstBlock = true;
    uint32_t count = size >= 0x8000 ? 0x8000 : size;
    uint16_t addressOffset = 0;

    auto dmaAddress = Util::Memory::Address<uint32_t>(dmaMemory);
    auto dataAddress = Util::Memory::Address<uint32_t>(data);

    soundLock.acquire();
    turnSpeakerOn();

    dmaAddress.copyRange(dataAddress, count);

    for(uint32_t i = 0x8000; i < size; i += 0x8000) {
        firstBlock = !firstBlock;
        prepareDma(addressOffset, count);
        setBufferSize(count);

        count = (size - i) >= 0x8000 ? 0x8000 : (size - i);
        addressOffset = static_cast<uint16_t>(firstBlock ? 0 : 0x8000);

        dmaAddress.add(addressOffset).copyRange(dataAddress.add(i), count);
        dmaAddress.add(addressOffset).add(count).setRange(0, 0x8000 - count);

        waitForInterrupt();
        ackInterrupt();
    }

    turnSpeakerOff();
    soundLock.release();
}

void SoundBlaster1::plugin() {
    // Older DSPs (version < 4) don't support manual IRQ- and DMA-configuration.
    // They must be configured via jumpers and there is no real way to get the IRQ- and DMA-numbers in software.
    // We just assume the DSP to use IRQ 10 and DMA channel 1, if not specified else in the constructor.
    auto &interruptService = Kernel::System::getService<Kernel::InterruptService>();
    interruptService.assignInterrupt(static_cast<Kernel::InterruptDispatcher::Interrupt>(32 + irqNumber), *this);
    interruptService.allowHardwareInterrupt(static_cast<Pic::Interrupt>(irqNumber));
}

}