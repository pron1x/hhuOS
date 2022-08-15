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

#ifndef HHUOS_SOUNDBLASTER16_H
#define HHUOS_SOUNDBLASTER16_H

#include "SoundBlaster.h"

namespace Device::Sound {

/**
 * Driver for SoundBlaster cards, which use a DSP with version number 4.xx.
 * The IRQ-number and DMA-channel, which the card uses, are set by the driver.
 * If not specified else in the constructor, this driver uses IRQ 10, DMA-channel 1 for 8-bit transfers and
 * DMA-channel 5 for 16-bit transfers.
 *
 * SoundBlaster cards with DSP versions >= 2.xx support the "auto-initialize" DMA-mode, which means, that after
 * a buffer has been transferred, the DMA-controller restarts transferring the same region again.
 * This way, there is no delay between two buffers, as it is the case when using "single-cycle" DMA-mode. To make use
 * of the "auto-initialize" feature, this driver uses a double buffering mechanism, where a 64KB DMA-region is split up
 * into two 32K regions. While the card is receiving data from one of the regions, the next 32KB of data are already
 * being copied to the other region.
 *
 * The SoundBlaster16 cards provide a more advanced mixer than it's predecessors. However, this driver does not make
 * use of the mixer's functionality. Stereo mode and the low-pass filter, which need to be enabled manually on
 * SoundBlaster Pro cards, are enable automatically by the hardware on SoundBlaster16 cards.
 *
 * Supported audio formats:
 *      -Mono PCM, 5000-44100 Hz, 8-bit samples
 *      -Mono PCM, 5000-44100 Hz, 16-bit samples
 *      -Stereo PCM, 5000-44100 Hz, 8-bit samples
 *      -Stereo PCM, 5000-44100 Hz, 16-bit samples
 *
 * TODO: 16-bit audio does not work correctly -> The card won't issue any interrupts after the first transfer
 */
class SoundBlaster16 : public SoundBlaster {

public:
    /**
     * Constructor.
     *
     * Valid values for irqNumber are: 2, 5, 7, 10 (Default: 10).
     * Valid values for dmaChannel8 are: 0, 1, 3 (Default: 1).
     * Valid values for dmaChannel16 are: 5, 6, 7 (Default: 5).
     *
     * CAUTION: If an invalid value is given to the constructor, the respective default-value will be used instead!
     *
     * @param baseAddress The IO-port's start address
     * @param irqNumber The IRQ number to use
     * @param dmaChannel8 The DMA channel to use for 8-bit transfers
     * @param dmaChannel16 The DMA channel to use for 16-bit transfers
     */
    explicit SoundBlaster16(uint16_t baseAddress, uint8_t irqNumber = 10, uint8_t dmaChannel8 = 1, uint8_t dmaChannel16 = 5);

    /**
     * Copy Constructor.
     */
    SoundBlaster16(const SoundBlaster16 &other) = delete;

    /**
     * Assignment operator.
     */
    SoundBlaster16 &operator=(const SoundBlaster16 &other) = delete;

    /**
     * Destructor.
     */
    ~SoundBlaster16() override = default;

    /**
     * Overriding function from SoundBlaster.
     */
    void setSamplingRate(uint16_t samplingRate, uint8_t channels, uint8_t bits) override;

    /**
     * Overriding function from SoundBlaster.
     */
    void playPcmData(const uint8_t *data, uint32_t size) override;

    /**
     * Overriding function from InterruptHandler.
     */
    void plugin() override;

private:
    /**
     * Prepare the DMA-controller for a data transfer to the sound card.
     *
     * @param addressOffset Offset to add to the base address, that is used for DMA-transfer (SoundBlaster::dmaMemory)
     * @param bufferSize The size of the buffer to be transferred
     * @param autoInitialize Whether or not to use the "auto-initialize" DMA-mode
     */
    void prepareDma(uint16_t addressOffset, uint32_t bufferSize, bool autoInitialize = true);

    /**
     * Set the size of the buffer, that the DSP awaits to play.
     *
     * @param bufferSize The size
     */
    void setBufferSize(uint32_t bufferSize);

    /**
     * Stop the "auto-initialize" mode after the DMA-controller has stopped transferring the current buffer.
     */
    void stopAutoInitialize();

private:

    IoPort mixerAddressPort;
    IoPort mixerDataPort;

    uint32_t currentSamplingRate = 0;
    uint8_t currentChannels = 0;
    uint8_t currentBits = 0;
    uint8_t irqNumber;
    uint8_t dmaChannel8;
    uint8_t dmaChannel16;
};

}

#endif
