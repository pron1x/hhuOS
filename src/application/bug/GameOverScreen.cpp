/*
 * Copyright (C) 2018-2023 Heinrich-Heine-Universitaet Duesseldorf,
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

#include "GameOverScreen.h"

#include <cstdint>

#include "lib/util/graphic/Fonts.h"
#include "lib/util/game/Game.h"
#include "lib/util/game/GameManager.h"
#include "BugDefender.h"
#include "lib/util/base/Address.h"
#include "lib/util/game/Graphics2D.h"
#include "lib/util/graphic/Colors.h"
#include "lib/util/graphic/Font.h"
#include "lib/util/io/key/Key.h"
#include "lib/util/math/Vector2D.h"

GameOverScreen::GameOverScreen(bool won) : won(won) {
    setKeyListener(*this);
}

void GameOverScreen::update(double delta) {}

void GameOverScreen::initializeBackground(Util::Game::Graphics2D &graphics) {
    auto charWidth = Util::Graphic::Fonts::TERMINAL_FONT.getCharWidth() / static_cast<double>(Util::Game::GameManager::getTransformation());
    graphics.clear();
    graphics.setColor(Util::Graphic::Colors::WHITE);

    if (won) {
        graphics.drawString(Util::Math::Vector2D((Util::Address<uint32_t>(CONGRATULATIONS).stringLength() * -charWidth) / 2, 0.1), CONGRATULATIONS);
        graphics.drawString(Util::Math::Vector2D((Util::Address<uint32_t>(INVASION_STOPPED).stringLength() * -charWidth) / 2, 0), INVASION_STOPPED);
    } else {
        graphics.drawString(Util::Math::Vector2D((Util::Address<uint32_t>(LOST).stringLength() * -charWidth) / 2, 0.1), LOST);
        graphics.drawString(Util::Math::Vector2D((Util::Address<uint32_t>(PLANET_INVADED).stringLength() * -charWidth) / 2, 0), PLANET_INVADED);
    }

    graphics.drawString(Util::Math::Vector2D((Util::Address<uint32_t>(NEW_GAME).stringLength() * -charWidth) / 2, -0.75), NEW_GAME);
}

void GameOverScreen::keyPressed(Util::Io::Key key) {
    switch (key.getScancode()) {
        case (Util::Io::Key::ESC) :
            Util::Game::GameManager::getGame().stop();
            break;
        case (Util::Io::Key::SPACE) :
            auto &game = Util::Game::GameManager::getGame();
            game.pushScene(new BugDefender());
            game.switchToNextScene();
    }
}

void GameOverScreen::keyReleased(Util::Io::Key key) {}
