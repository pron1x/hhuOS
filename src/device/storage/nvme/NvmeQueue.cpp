#include "NvmeQueue.h"
#include "NvmeController.h"

#include "kernel/log/Logger.h"

namespace Device::Storage {
Kernel::Logger NvmeQueue::log = Kernel::Logger::get("NVMEQueue");

    NvmeQueue::NvmeQueue(const NvmeController* nvmeController) {

    };

}