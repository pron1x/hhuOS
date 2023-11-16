#ifndef HHUOS_NVMEADMINQUEUE_H
#define HHUOS_NVMEADMINQUEUE_H

#include "NvmeController.h"
#include "NvmeQueue.h"
#include <cstdint>

namespace Device::Storage {
    namespace Nvme {
    class NvmeAdminQueue {
        public:
        
        void Init(NvmeController* nvmeController, uint32_t size);

        void identifyController(void* physicalDataPtr);
        
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

    };
    }
}


#endif