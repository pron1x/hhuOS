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

#include "SoundBlasterPro.h"
#include "kernel/system/System.h"
#include "kernel/service/InterruptService.h"
#include "device/isa/Isa.h"

Device::Sound::SoundBlasterPro::SoundBlasterPro(uint16_t baseAddress, uint8_t irqNumber, uint8_t dmaChannel) :
    SoundBlaster(baseAddress), mixerAddressPort(baseAddress + 0x04), mixerDataPort(baseAddress + 0x05), irqNumber(irqNumber), dmaChannel(dmaChannel) {}

void Device::Sound::SoundBlasterPro::setSamplingRate(uint16_t samplingRate, uint8_t channels, uint8_t bits) {
    if (channels > 2) {
        Util::Exception::throwException(Util::Exception::INVALID_ARGUMENT, "SoundBlasterPro: Too many channels!");
    }
    if (bits != 8) {
        Util::Exception::throwException(Util::Exception::INVALID_ARGUMENT, "SoundBlasterPro: Illegal sample format!");
    }

    auto timeConstant = static_cast<uint16_t>(65536 - (256000000 / (samplingRate * channels)));
    writeToDSP(0x40);
    writeToDSP((timeConstant & 0xff00) >> 8);

    if (samplingRate <= 23000 && channels == 1) {
        enableLowPassFilter();
    } else {
        disableLowPassFilter();
    }

    if (channels > 1) {
        enableStereo();
    } else {
        disableStereo();
    }

    currentSamplingRate = samplingRate;
}

void Device::Sound::SoundBlasterPro::setBufferSize(uint32_t bufferSize) {
    writeToDSP(0x48);
    writeToDSP((bufferSize - 1) & 0x00ff);
    writeToDSP(((bufferSize - 1) & 0xff00) >> 8);
}

void Device::Sound::SoundBlasterPro::enableLowPassFilter() {
    mixerAddressPort.writeByte(0x0e);
    mixerDataPort.writeByte(mixerDataPort.readByte() & 0xdf);
}

void Device::Sound::SoundBlasterPro::disableLowPassFilter() {
    mixerAddressPort.writeByte(0x0e);
    mixerDataPort.writeByte(mixerDataPort.readByte() | 0x20);
}

void Device::Sound::SoundBlasterPro::enableStereo() {
    // First, we set the mixer to stereo mode
    mixerAddressPort.writeByte(0x0e);
    mixerDataPort.writeByte(mixerDataPort.readByte() | 0x02);

    // Now it is necessary to let the DSP output a single silent byte
    reinterpret_cast<char*>(dmaMemory)[0] = 0;
    prepareDma(0, 2, false);

    writeToDSP(0x14);
    writeToDSP(0x00);
    writeToDSP(0x00);

    // Now wait for an interrupt. The DSP should then be able to output stereo sound
    waitForInterrupt();
    ackInterrupt();
}

void Device::Sound::SoundBlasterPro::disableStereo() {
    mixerAddressPort.writeByte(0x0e);
    mixerDataPort.writeByte(mixerDataPort.readByte() & 0xfd);
}

void Device::Sound::SoundBlasterPro::stopAutoInitialize() {
    writeToDSP(0xda);
}

void Device::Sound::SoundBlasterPro::prepareDma(uint16_t addressOffset, uint32_t bufferSize, bool autoInitialize) {
    Isa::selectChannel(dmaChannel);
    Isa::setMode(dmaChannel, Isa::READ, autoInitialize, false, Isa::SINGLE_TRANSFER);
    Isa::setAddress(dmaChannel,reinterpret_cast<uint32_t>(Kernel::System::getService<Kernel::MemoryService>().getPhysicalAddress(dmaMemory)) + addressOffset);
    Isa::setCount(dmaChannel, bufferSize - 1);
    Isa::deselectChannel(dmaChannel);
}

void Device::Sound::SoundBlasterPro::playPcmData(const uint8_t *data, uint32_t size) {
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

void Device::Sound::SoundBlasterPro::plugin() {
    // Older DSPs (version < 4) don't support manual IRQ- and DMA-configuration.
    // They must be configured via jumpers and there is no real way to get the IRQ- and DMA-numbers in software.
    // We just assume the DSP to use IRQ 10 and DMA channel 1, if not specified else in the constructor.
    auto &interruptService = Kernel::System::getService<Kernel::InterruptService>();
    interruptService.assignInterrupt(static_cast<Kernel::InterruptDispatcher::Interrupt>(32 + irqNumber), *this);
    interruptService.allowHardwareInterrupt(static_cast<Pic::Interrupt>(irqNumber));
}
