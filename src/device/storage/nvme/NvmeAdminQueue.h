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
    /**
     * @brief Interface for Nvme Admin Commands. Configures the internal queue and controller registers in the init function.
    */
    class NvmeAdminQueue {
        public:
        
        /**
         * @brief Initializes the Admin Queue. Creates the internal Queue Pair and sets the approprioate controller registers.
         * @param nvmeController Pointer to the NVMe Controller the Admin Queue is created for
         * @param size The amount of entry slots for the queues.
        */
        void Init(NvmeController* nvmeController, uint32_t size);

        /**
         * @brief Send the identify command. Refer to NVMe Reference 1.4 section 5.15
         * @param physicalDataPtr The physical memory address of a continous 4096 byte memory region
         * @param cns The CNS Value to use for the command. Refer to Section 5.15.1 - Figure 244
         * @param nsid The Namespace ID if required by the command.
        */
        void sendIdentifyCommand(void* physicalDataPtr, uint16_t cns, uint32_t nsid);

        /**
         * @brief Send the Namespace Attachment command.
         * @param controllerId The NVM Subsystem unique ID of the Controller to attach
         * @param nsid The ID of the namespace to attach
        */
        void attachNamespace(uint16_t controllerId, uint32_t nsid);

        /**
         * @brief Creates a new Nvme I/O Queue Pair and registers it as I/O Queues.
         * @param queueId The ID of the new Queue Pair. Can not be 0 and must be incremented for every new Queue Pair
         * @param size The amount of slots in the queue.
         * @returns The pointer to the new NvmeQueue object
        */
        NvmeQueue* createNewQueue(uint16_t queueId, uint32_t size);
        
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