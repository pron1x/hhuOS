#ifndef HHUOS_NVMECONTROLLER_H
#define HHUOS_NVMECONTROLLER_H

#include <cstdint>

#include "kernel/interrupt/InterruptHandler.h"

namespace Device {
    class PciDevice;
    namespace Storage {
        class NvmeDevice;
    }
}
namespace Kernel {
    class Logger;
    struct InterruptFrame;
}

namespace Device::Storage {
    
    class NvmeController : public Kernel::InterruptHandler {
        public:
        /**
         * Constructor
        */
       explicit NvmeController(const PciDevice &pciDevice);

       /**
        * Destructor
       */
        ~NvmeController() override = default;

        void plugin() override;

        void trigger(const Kernel::InterruptFrame &frame) override;
        
        private:
        static Kernel::Logger log;

    };
}


#endif