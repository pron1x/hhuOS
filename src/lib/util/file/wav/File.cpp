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

#include "File.h"

namespace Util::File::Wav {

File::File(uint8_t *buffer) : buffer(buffer) {
    auto &riffChunk = *((RiffChunk*) buffer);

    audioFormat = riffChunk.formatChunk.audioFormat;
    bitsPerSample = riffChunk.formatChunk.bitsPerSample;
    numChannels = riffChunk.formatChunk.numChannels;
    samplesPerSecond = riffChunk.formatChunk.samplesPerSecond;
    bytesPerSecond = riffChunk.formatChunk.bytesPerSecond;
    bitsPerSample = riffChunk.formatChunk.bitsPerSample;
    frameSize = static_cast<uint16_t>(riffChunk.formatChunk.numChannels * ((bitsPerSample + 7) / 8));
    dataSize = riffChunk.dataChunk.chunkSize;
    sampleCount = dataSize / frameSize;
}

uint8_t* File::getData() const {
    return &buffer[sizeof(RiffChunk)];
}

uint32_t File::getDataSize() const {
    return dataSize;
}

File::~File() {
    delete buffer;
}

File::AudioFormat File::getAudioFormat() const {
    return audioFormat;
}

uint16_t File::getNumChannels() const {
    return numChannels;
}

uint32_t File::getSamplesPerSecond() const {
    return samplesPerSecond;
}

uint32_t File::getBytesPerSecond() const {
    return bytesPerSecond;
}

uint16_t File::getBitsPerSample() const {
    return bitsPerSample;
}

uint16_t File::getFrameSize() const {
    return frameSize;
}

uint32_t File::getSampleCount() const {
    return sampleCount;
}

}