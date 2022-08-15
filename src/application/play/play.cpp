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

#include <cstdint>
#include "lib/util/system/System.h"
#include "lib/util/file/wav/File.h"

int32_t main(int32_t argc, char *argv[]) {
    if (argc < 2) {
        Util::System::error << "play: No arguments provided!" << Util::Stream::PrintWriter::endl << Util::Stream::PrintWriter::flush;
        return -1;
    }

    auto wavFile = Util::File::File(argv[1]);
    if (!wavFile.exists() || wavFile.isDirectory()) {
        Util::System::error << "play: '" << argv[1] << "' could not be opened!" << Util::Stream::PrintWriter::endl << Util::Stream::PrintWriter::flush;
        return -1;
    }

    auto fileInputStream = Util::Stream::FileInputStream(wavFile);
    auto bufferedInputStream = Util::Stream::BufferedInputStream(fileInputStream);
    auto buffer = new uint8_t[wavFile.getLength()];
    fileInputStream.read(buffer, 0, wavFile.getLength());
    auto wav = Util::File::Wav::File(buffer);

    auto soundBlasterFile = Util::File::File("/device/soundblaster");
    auto outputStream = Util::Stream::FileOutputStream(soundBlasterFile);
    outputStream.write(wav.getData(), 0, wav.getDataSize());

    return 0;
}