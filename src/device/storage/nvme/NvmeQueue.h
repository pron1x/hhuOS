#ifndef HHUOS_NVMEQUEUE_H
#define HHUOS_NVMEQUEUE_H

#include "NvmeController.h"
#include <cstdint>

namespace Device {
    class PciDevice;
    namespace Storage {
        class NvmeQueue;
    }
}
namespace Kernel {
    class Logger;
    struct InterruptFrame;
}

namespace Device::Storage {
    
    class NvmeQueue {
        public:
        /**
         * Constructor
        */
        explicit NvmeQueue(NvmeController* nvmeController, uint16_t id, uint32_t size);
        
        private:
        static Kernel::Logger log;

        struct NvmeCommand {
            struct {
                uint8_t OPC;                // Opcode
                unsigned FUSE       : 2;    // Fused Operation
                unsigned _reserved0 : 4;
                unsigned PSDT       : 2;    // PRP or SGL for Data Transfer (Only PRP supported)
                unsigned CID        : 16;   // Command Identifier
            } CDW0;     // Command information
            uint64_t NSID;          // Namespace Identifier
            uint64_t _reserved1;
            uint64_t MPTR;          // Metadata Pointer
            uint64_t PRP1;          // PRP Entry 1
            uint64_t PRP2;          // PRP Entry 2 or reserved
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

        public:

        private:
        NvmeController* nvme;

        // Identification for submission and completion queue.
        uint16_t id;
        // Submission and completion queue size
        uint32_t size;

        uint32_t subQueueTail;
        NvmeCommand* subQueue;

        uint32_t compQueueHead;
        NvmeCompletionEntry* compQueue;


    };
}


#endif