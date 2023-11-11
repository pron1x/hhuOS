#ifndef HHUOS_NVMEADMINQUEUE_H
#define HHUOS_NVMEADMINQUEUE_H

#include "NvmeController.h"
#include "NvmeQueue.h"
#include <cstdint>

namespace Device {
    class PciDevice;
    namespace Storage {
        class NvmeAdminQueue;
    }
}
namespace Kernel {
    class Logger;
    struct InterruptFrame;
}

namespace Device::Storage {
    
    class NvmeAdminQueue {
        public:
        
        void Init(NvmeController* nvmeController, uint32_t size);
        
        private:
        static Kernel::Logger log;

        
        public:

        private:
        NvmeController* nvme;
        NvmeQueue* queue;

        // Identification for submission and completion queue. Always 0 for admin
        uint16_t id = 0;
        // Submission and completion queue size
        uint32_t size;

    };
}


#endif