#include <kernel/Kernel.h>
#include <kernel/services/SoundService.h>
#include <lib/sound/BeepFile.h>
#include "Sound.h"

Sound::Sound() : Thread("Sound") {
    speaker = Kernel::getService<SoundService>()->getSpeaker();
    timeService = Kernel::getService<TimeService>();
}

void Sound::run () {
    while(isRunning) {

        BeepFile::load("/music/tetris.beep")->play();

        speaker->off();
        timeService->msleep(1000);

        BeepFile::load("/music/mario.beep")->play();

        speaker->off();
        timeService->msleep(1000);

    }
}

Sound::~Sound() {
    speaker->off();
}
