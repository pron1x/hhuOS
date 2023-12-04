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

        log.debug("Initialized Admin Queue with size %d.", size);
    };

    void NvmeAdminQueue::sendIdentifyCommand(void* physicalDataPtr, uint16_t cns, uint32_t nsid = 0) {
        NvmeQueue::NvmeCommand* command = queue->getSubmissionEntry();
        command->CDW0.CID = queue->getSubmissionSlotNumber() - 1;
        command->CDW0.FUSE = 0;
        command->CDW0.PSDT = 0;
        command->CDW0.OPC = 0x06; // Identify opcode
        command->NSID = nsid;
        command->PRP1 = reinterpret_cast<uint64_t>(physicalDataPtr);

        command->CDW10 = (0 << 16 | cns);
        command->CDW11 = 0;
        command->CDW14 = 0;
        queue->updateSubmissionTail();
        queue->waitUntilComplete();
    }

    void NvmeAdminQueue::attachNamespace(void* physicalDataPtr, uint32_t nsid) {
        NvmeQueue::NvmeCommand* submissionEntry = queue->getSubmissionEntry();
        submissionEntry-> CDW0.CID = queue->getSubmissionSlotNumber() - 1;
        submissionEntry->CDW0.FUSE = 0;
        submissionEntry->CDW0.PSDT = 0;
        submissionEntry->CDW0.OPC = 0x15; // Command Namespace Attachment
        submissionEntry->NSID = nsid;
        submissionEntry->CDW10 = 0; // Controller Attach
        submissionEntry->PRP1 = reinterpret_cast<uint64_t>(physicalDataPtr);
        
        queue->updateSubmissionTail();
        queue->waitUntilComplete();
    }

    }
}