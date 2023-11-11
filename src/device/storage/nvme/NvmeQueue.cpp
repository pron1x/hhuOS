#include "NvmeQueue.h"
#include "NvmeController.h"

#include "kernel/log/Logger.h"
#include "kernel/service/MemoryService.h"
#include "kernel/system/System.h"

namespace Device::Storage {
Kernel::Logger NvmeQueue::log = Kernel::Logger::get("NVMEQueue");

    NvmeQueue::NvmeQueue(NvmeController* nvmeController, uint16_t id, uint32_t size) {
        nvme = nvmeController;
        auto &memoryService = Kernel::System::getService<Kernel::MemoryService>();

        id = id;
        size = size;
        compQueueHead = 0;
        subQueueTail = 0;

        // Allocate memory for submission and completion queue
        subQueue = reinterpret_cast<NvmeCommand*>(memoryService.mapIO(size * sizeof(NvmeCommand)));
        subQueuePhysicalPointer = reinterpret_cast<uint64_t>(memoryService.getPhysicalAddress(subQueue));

        compQueue = reinterpret_cast<NvmeCompletionEntry*>(memoryService.mapIO(size * sizeof(NvmeCompletionEntry)));
        compQueuePhysicalPointer = reinterpret_cast<uint64_t>(memoryService.getPhysicalAddress(compQueue));

        log.info("Initialized Queue %d with size %d.", id, size);
    };

}