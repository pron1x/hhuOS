#ifndef HHUOS_NVMEADMINQUEUE_H
#define HHUOS_NVMEADMINQUEUE_H

#include "NvmeController.h"
#include "NvmeQueue.h"
#include <cstdint>

namespace Device::Storage {
    class NvmeController;
    namespace Nvme {
        class NvmeAdminQueue;
        class NvmeQueue;
    }
}

namespace Device::Storage {
    namespace Nvme {
    class NvmeAdminQueue {
        public:
        
        void Init(NvmeController* nvmeController, uint32_t size);

        /**
         * Send the identify command. Refer to NVMe Reference 1.4 section 5.15
         * @param physicalDataPtr The physical memory address of a continous 4096 byte memory region
         * @param cns The CNS Value to use for the command. Refer to Section 5.15.1 - Figure 244
         * @param nsid The Namespace ID if required by the command.
        */
        void sendIdentifyCommand(void* physicalDataPtr, uint16_t cns, uint32_t nsid);

        void attachNamespace(void* physicalDataPtr, uint32_t nsid);

        NvmeQueue* createNewQueue(uint16_t id, uint32_t size);
        
        private:

        
        public:

        private:
        static Kernel::Logger log;
        NvmeController* nvme;
        NvmeQueue* queue;

        // Identification for submission and completion queue. Always 0 for admin
        uint16_t id = 0;
        // Submission and completion queue size
        uint32_t size;

        // OP Code constants
        static const constexpr uint8_t OPC_CREATE_IO_SUBMISSION_QUEUE = 0x01;
        static const constexpr uint8_t OPC_CREATE_IO_COMPLETION_QUEUE = 0x05;
        static const constexpr uint8_t OPC_IDENTIFY = 0x06;
        static const constexpr uint8_t OPC_NS_ATTACHMENT = 0x15;

    };
    }
}


#endif