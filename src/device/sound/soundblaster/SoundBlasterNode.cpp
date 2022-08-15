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

#include "SoundBlasterNode.h"

namespace Device::Sound {

SoundBlasterNode::SoundBlasterNode(SoundBlaster *soundBlaster) : MemoryNode("soundblaster"), soundBlaster(soundBlaster) {}

Util::File::Type SoundBlasterNode::getFileType() {
    return Util::File::CHARACTER;
}

SoundBlasterNode::~SoundBlasterNode() {
    delete soundBlaster;
}

uint64_t SoundBlasterNode::writeData(const uint8_t *sourceBuffer, uint64_t pos, uint64_t numBytes) {
    soundBlaster->playPcmData(sourceBuffer, numBytes);
    return numBytes;
}

}