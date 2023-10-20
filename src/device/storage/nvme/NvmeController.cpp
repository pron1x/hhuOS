#include "NvmeController.h"

#include "kernel/log/Logger.h"

namespace Device::Storage {
Kernel::Logger NvmeController::log = Kernel::Logger::get("NVME");

    NvmeController::NvmeController(const PciDevice &pciDevice) {

    }

    void NvmeController::plugin() {

    }

    void NvmeController::trigger(const Kernel::InterruptFrame &frame) {
        
    }
}