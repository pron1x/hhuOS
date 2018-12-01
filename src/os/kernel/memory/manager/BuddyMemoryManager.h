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

#ifndef __BUDDY_MANAGER_H__
#define __BUDDY_MANAGER_H__

#include <cstdint>
#include "kernel/memory/manager/MemoryManager.h"

struct BuddyNode {
    void *addr;
    BuddyNode *next;
};

/**
 * Memory manager, that implements the buddy algorithm.
 *
 * This memory manager does not allow allocation with an alignment.
 *
 * TODO: Implement realloc.
 *
 * @author Burak Akguel, Christian Gesse, Fabian Ruhland, Filip Krakowski, Michael Schoettner
 * @date 2018
 */
class BuddyMemoryManager : public MemoryManager {

private:
	uint8_t minOrder = 0;
	uint8_t maxOrder = 0;

	struct BuddyNode **freeList = nullptr;

public:

    /**
     * Constructor.
     *
     * @param memoryStartAddress Start address of the memory area to manage
     * @param memoryEndAddress End address of the memory area to manage
     * @param doUnmap Indicates, whether or not the manager should unmap freed memory by itself
     * @param minOrder Minimum order (power of two), that the chunks of memory have
     */
	BuddyMemoryManager(uint32_t memoryStartAddress, uint32_t memoryEndAddress, bool doUnmap, uint8_t minOrder = 4);

	/**
	 * Copy-constructor.
	 */
	BuddyMemoryManager(const BuddyMemoryManager &copy) = delete;

	/**
	 * Destructor,
	 */
	~BuddyMemoryManager() override;

	/**
	 * Overriding function from MemoryManager.
	 */
	void *alloc(uint32_t size) override;

    /**
     * Overriding function from MemoryManager.
     */
	void free(void* ptr) override;

    /**
     * Dump the list of free chunks of memory.
     */
	void dump() override;
};

#endif
