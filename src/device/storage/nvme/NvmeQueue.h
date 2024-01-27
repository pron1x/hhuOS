#ifndef HHUOS_NVMEQUEUE_H
#define HHUOS_NVMEQUEUE_H

#include "NvmeController.h"
#include "lib/util/async/Spinlock.h"
#include <cstdint>

namespace Device::Storage {
    class NvmeController;
    namespace Nvme {
        class NvmeAdminQueue;
        class NvmeQueue;
    }
}

namespace Device::Storage {
    
    namespace Nvme{

    /**
     * @brief Implementation of a Submission and Completion Queue Pair. Both Queues have the same size and id.
     * Provides a raw interface to work with the Queue Pair.
    */
    class NvmeQueue {
        public:
        
        explicit NvmeQueue(NvmeController* nvmeController, uint16_t id, uint32_t size);

        /**
         * @brief Returns the physical pointer to the Submission Queue Buffer
         * @returns The Pointer to the physical memory address
        */
        uint64_t getSubmissionPhysicalAddress() { return subQueuePhysicalPointer; };

        /**
         * @brief Returns the physical pointer to the Completion Queue Buffer
         * @returns The Pointer to the physical memory address
        */
        uint64_t getCompletionPhysicalAddress() { return compQueuePhysicalPointer; };

        /**
         * @brief This function should be called before getting a new NvmeCommand.
         * @returns The slot number of the NvmeCommand that will next be returned
        */
        uint32_t getSubmissionSlotNumber() { return subQueueTail; }

        /**
         * @brief Interrupt Handler for the completion Queue. Checks the Completion Queue for new entries and updates the corresponding Queue Head Doorbell.
        */
        void checkCompletionQueue();
        
        void lockQueue() { lock.acquire(); }
        void unlockQueue() { lock.release(); }


        public:
        struct NvmeCommand {
            struct {
                unsigned OPC        : 8;    // Opcode
                unsigned FUSE       : 2;    // Fused Operation
                unsigned _reserved0 : 4;
                unsigned PSDT       : 2;    // PRP or SGL for Data Transfer (Only PRP supported)
                unsigned CID        : 16;   // Command Identifier
            } CDW0;     // Command information
            uint32_t NSID;          // Namespace Identifier
            uint64_t _reserved1;
            uint64_t MPTR;          // Metadata Pointer
            uint64_t PRP1;          // Data Pointer (DPTR), PRP Entry 1
            uint64_t PRP2;          // Data Pointer (DPTR), PRP Entry 2 or reserved
            uint32_t CDW10;         // Command specific
            uint32_t CDW11;         // Command specific
            uint32_t CDW12;         // Command specific
            uint32_t CDW13;         // Command specific
            uint32_t CDW14;         // Command specific
            uint32_t CDW15;         // Command specific
        };

        struct NvmeCompletionEntry {
            uint32_t DW0;           // Command specific
            uint32_t _reserved0;    
            struct {
                uint16_t SQHD;      // Submission Queue Head Pointer
                uint16_t SQID;      // Submission Queue Identifier
            } DW2;                  // Submission Queue Information
            struct {
                unsigned CID    : 16;   // Command Identifier
                unsigned P      : 1;    // Phase Tag. Identifies if Completion Queue Entry is new
                unsigned SF     : 15;   // Status Field
            } DW3;                  // Command information
        };

        /**
         * @brief Returns the Pointer to the next NvmeCommand in the Submission Queue.
         * @returns A Pointer to the NvmeCommand to be submitted.
        */
        NvmeCommand* getSubmissionEntry();

        /** 
         * @brief Busy waits until the Completion Queue Entry in the slot contains the result
         * @param slot The Queue slot to await an answer from (Command ID)
         * @return A Pointer to the Completion Queue Entry in the slot
        */
        NvmeCompletionEntry* waitUntilComplete(uint32_t slot);

        /**
         * @brief Writes the new Submission Tail Pointer to the Controller Doorbell Register to submit the Commands.
         * This method should be called after each NvmeCommand creation to ensure that no pending commands are overwritten.
        */
        void updateSubmissionTail();

        private:
        static Kernel::Logger log;
        NvmeController* nvme;

        // Identification for submission and completion queue
        uint16_t id;
        // Submission and completion queue size
        uint32_t size;

        // Tail Pointer of the Submission Queue
        uint32_t subQueueTail;
        NvmeCommand* subQueue;
        uint64_t subQueuePhysicalPointer;

        // Head Pointer of the Completion Queue
        uint32_t compQueueHead;
        NvmeCompletionEntry* compQueue;
        uint64_t compQueuePhysicalPointer;

        // Phase bit for new completion queue entries
        uint8_t phase = 1;
        volatile bool waiting = false;
        Util::Async::Spinlock lock;

    };
    }
}


#endif