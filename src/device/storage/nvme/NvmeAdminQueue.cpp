#include "NvmeController.h"
#include "NvmeAdminQueue.h"
#include "NvmeQueue.h"

#include "kernel/log/Logger.h"
#include "kernel/service/MemoryService.h"
#include "kernel/system/System.h"

namespace Device::Storage {
    namespace Nvme {
    Kernel::Logger NvmeAdminQueue::log = Kernel::Logger::get("NVMEAdmin");

    void NvmeAdminQueue::Init(NvmeController* nvmeController, uint32_t size) {
        id = 0;
        nvme = nvmeController;
        queue = new NvmeQueue(nvme, id, size);

        nvme->setAdminQueueRegisters(queue->getSubmissionPhysicalAddress(), queue->getCompletionPhysicalAddress());

        log.info("Initialized Admin Queue with size %d.", size);
    };

    void NvmeAdminQueue::identifyController(void* physicalDataPtr) {
        NvmeQueue::NvmeCommand* submissionEntry = queue->getSubmissionEntry();
        submissionEntry->CDW0.CID = 1;
        submissionEntry->CDW0.FUSE = 0;
        submissionEntry->CDW0.PSDT = 0;
        submissionEntry->CDW0.OPC = 0x06;
        submissionEntry->PRP1 = reinterpret_cast<uint64_t>(physicalDataPtr);
        // Identify controller that is processing the command
        submissionEntry->CDW10 = 0x01;
        submissionEntry->CDW11 = 0;
        submissionEntry->CDW14 = 0;
        queue->updateSubmissionTail();
    };
    }
}