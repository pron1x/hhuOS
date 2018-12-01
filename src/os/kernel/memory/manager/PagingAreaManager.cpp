/*
 * Copyright (C) 2018 Burak Akguel, Christian Gesse, Fabian Ruhland, Filip Krakowski, Michael Schoettner
 * Heinrich-Heine University
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

#include <kernel/memory/MemLayout.h>
#include "PagingAreaManager.h"
#include "kernel/memory/Paging.h"

PagingAreaManager::PagingAreaManager()
        : BitmapMemoryManager(VIRT_PAGE_MEM_START, VIRT_PAGE_MEM_END, false, PAGESIZE, true) {

    managerType = PAGING_AREA_MANAGER;

    // We use already 256 Page Tables for Kernel mappings and one Page Directory
    // as the Kernel´s PD
    for(uint32_t i = 0; i < 8; i++){
        freeBitmap[i] = 0xFFFFFFFF;
    }
    freeBitmap[8] = 0xC0000000;

    bmpSearchOffset = 8;

    // subtract already reserved memory from free memory
    freeMemory -= (8 * 32 * blockSize + 2 * blockSize);
}
