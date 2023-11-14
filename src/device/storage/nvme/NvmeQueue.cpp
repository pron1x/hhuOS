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

        this->id = id;
        this->size = size;
        this->compQueueHead = 0;
        this->subQueueTail = 0;

        // Allocate memory for submission and completion queue
        this->subQueue = reinterpret_cast<NvmeCommand*>(memoryService.mapIO(size * sizeof(NvmeCommand)));
        this->subQueuePhysicalPointer = reinterpret_cast<uint64_t>(memoryService.getPhysicalAddress(subQueue));

        this->compQueue = reinterpret_cast<NvmeCompletionEntry*>(memoryService.mapIO(size * sizeof(NvmeCompletionEntry)));
        this->compQueuePhysicalPointer = reinterpret_cast<uint64_t>(memoryService.getPhysicalAddress(compQueue));

        log.info("Initialized Queue %d with size %d.", id, size);
    };

    NvmeQueue::NvmeCommand* NvmeQueue::getSubmissionEntry() {
        auto ptr = &subQueue[subQueueTail];
        subQueueTail = (subQueueTail + 1) % size;
        return ptr;
    }

    void NvmeQueue::updateSubmissionTail() {
        log.info("Updating Submission Queue[%d] Tail Doorbell to %d.", id, subQueueTail);
        nvme->setQueueTail(id, subQueueTail);
    }
}