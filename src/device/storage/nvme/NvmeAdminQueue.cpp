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
        uint32_t cid = queue->getSubmissionSlotNumber();        
        NvmeQueue::NvmeCommand* command = queue->getSubmissionEntry();
        command->CDW0.CID = cid;
        command->CDW0.FUSE = 0;
        command->CDW0.PSDT = 0;
        command->CDW0.OPC = OPC_IDENTIFY; // Identify opcode
        command->NSID = nsid;
        command->PRP1 = reinterpret_cast<uint64_t>(physicalDataPtr);

        command->CDW10 = (0 << 16 | cns);
        command->CDW11 = 0;
        command->CDW14 = 0;
        queue->updateSubmissionTail();
        queue->waitUntilComplete();
    }

    void NvmeAdminQueue::attachNamespace(void* physicalDataPtr, uint32_t nsid) {
        // FIXME: The command cancels with Error 0x02!
        uint32_t cid = queue->getSubmissionSlotNumber();
        NvmeQueue::NvmeCommand* submissionEntry = queue->getSubmissionEntry();
        submissionEntry-> CDW0.CID = cid;
        submissionEntry->CDW0.FUSE = 0;
        submissionEntry->CDW0.PSDT = 0;
        submissionEntry->CDW0.OPC = OPC_NS_ATTACHMENT; // Command Namespace Attachment
        submissionEntry->NSID = nsid;
        submissionEntry->CDW10 = 0; // Controller Attach
        submissionEntry->PRP1 = reinterpret_cast<uint64_t>(physicalDataPtr);
        
        queue->updateSubmissionTail();
        queue->waitUntilComplete();
    }

    NvmeQueue* NvmeAdminQueue::createNewQueue(uint16_t queueId, uint32_t size) {
        // Create queue object
        NvmeQueue* ioqueue = new NvmeQueue(nvme, queueId, size);
        // Submit command to create completion queue
        uint32_t cid = queue->getSubmissionSlotNumber();
        NvmeQueue::NvmeCommand* command = queue->getSubmissionEntry();
        command->CDW0.CID = cid;
        command->CDW0.FUSE = 0;
        command->CDW0.PSDT = 0;
        command->CDW0.OPC = OPC_CREATE_IO_COMPLETION_QUEUE;
        // PRP Entry 1 contains base memory address, since we use physical contigous memory regions
        command->PRP1 = queue->getCompletionPhysicalAddress();
        // DWORD10 contains the queue size and identifier
        command->CDW10 = (size << 16 | queueId );
        // DWORD11 contains interrupt vector, interrupt enable and physical contiguity information
        command->CDW11 = (0 << 16 | 1 << 1 | 1);
        // Submit the command
        queue->updateSubmissionTail();
        queue->waitUntilComplete();

        // Create command to create submission queue
        cid = queue->getSubmissionSlotNumber();
        command = queue->getSubmissionEntry();
        command->CDW0.CID = cid;
        command->CDW0.FUSE = 0;
        command->CDW0.PSDT = 0;
        command->CDW0.OPC = OPC_CREATE_IO_SUBMISSION_QUEUE;
        // PRP Entry 1 contains base memory address, since we use physical contigous memory regions
        command->PRP1 = queue->getSubmissionPhysicalAddress();
        // DWORD10 contains queue size and identifier
        command->CDW10 = (size << 16 | queueId );
        // DWORD11 contains completion queue identifier, queue priority and physical contiguity information
        command->CDW11 = (queueId << 16 | 0b11 << 1 | 1); // 0b11 is low priority, field ignored since we do not use wighted round robin arbitration
        // DWORD12 contains NVM Set information. Cleared to 0 to not associate queue with specific NVM Set
        command->CDW12 = 0;

        // Submit the command
        queue->updateSubmissionTail();
        queue->waitUntilComplete();
        nvme->registerQueueInterruptHandler(queueId, ioqueue);

        log.debug("I/O Queue [%d] created.", queueId);
        return ioqueue;
    }

    }
}