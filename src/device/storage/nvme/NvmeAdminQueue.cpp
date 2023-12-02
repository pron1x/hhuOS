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
        nvme->registerQueueInterruptHandler(id, queue);

        log.info("Initialized Admin Queue with size %d.", size);
    };

    void NvmeAdminQueue::identifyController(void* physicalDataPtr) {
        NvmeQueue::NvmeCommand* submissionEntry = queue->getSubmissionEntry();
        submissionEntry->CDW0.CID = queue->getSubmissionSlotNumber() - 1;
        submissionEntry->CDW0.FUSE = 0;
        submissionEntry->CDW0.PSDT = 0;
        submissionEntry->CDW0.OPC = 0x06;
        submissionEntry->NSID = 0;
        submissionEntry->PRP1 = reinterpret_cast<uint64_t>(physicalDataPtr);
        // Identify controller that is processing the command
        submissionEntry->CDW10 = (0 << 16 | 0x01 << 0);
        submissionEntry->CDW11 = 0;
        submissionEntry->CDW14 = 0;
        queue->updateSubmissionTail();
        queue->waitUntilComplete();
    };

    void NvmeAdminQueue::getNamespaceList(void* physicalDataPtr) {
        NvmeQueue::NvmeCommand* submissionEntry = queue->getSubmissionEntry();
        submissionEntry->CDW0.CID = queue->getSubmissionSlotNumber() - 1;
        submissionEntry->CDW0.FUSE = 0;
        submissionEntry->CDW0.PSDT = 0;
        submissionEntry->CDW0.OPC = 0x06;
        submissionEntry->NSID = 0;
        submissionEntry->PRP1 = reinterpret_cast<uint64_t>(physicalDataPtr);
        // Get a list of active Namespaces
        submissionEntry->CDW10 = (0 << 16 | 0x02 << 0);
        submissionEntry->CDW11 = 0;
        submissionEntry->CDW14 = 0;
        queue->updateSubmissionTail();
        queue->waitUntilComplete();
    }

    void NvmeAdminQueue::identifyNamespace(void* physicalDataPtr, uint32_t nsid) {
        NvmeQueue::NvmeCommand* submissionEntry = queue->getSubmissionEntry();
        submissionEntry->CDW0.CID = queue->getSubmissionSlotNumber() - 1;
        submissionEntry->CDW0.FUSE = 0;
        submissionEntry->CDW0.PSDT = 0;
        submissionEntry->CDW0.OPC = 0x06;
        submissionEntry->NSID = nsid;
        submissionEntry->PRP1 = reinterpret_cast<uint64_t>(physicalDataPtr);
        // Get Identify Namespace Data structure
        submissionEntry->CDW10 = (0 << 16 | 0x00 << 0);
        submissionEntry->CDW11 = 0;
        submissionEntry->CDW14 = 0;
        queue->updateSubmissionTail();
        queue->waitUntilComplete();
    }
    }
}