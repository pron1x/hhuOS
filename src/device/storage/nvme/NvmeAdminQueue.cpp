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

        log.trace("Initialized Admin Queue with size %d.", size);
    };

    void NvmeAdminQueue::sendIdentifyCommand(void* physicalDataPtr, uint16_t cns, uint32_t nsid = 0) {
        queue->lockQueue();
        uint32_t slot = queue->getSubmissionSlotNumber();        
        NvmeQueue::NvmeCommand* command = queue->getSubmissionEntry();
        command->CDW0.CID = slot;
        command->CDW0.FUSE = 0;
        command->CDW0.PSDT = 0;
        command->CDW0.OPC = OPC_IDENTIFY; // Identify opcode
        command->NSID = nsid;
        command->PRP1 = reinterpret_cast<uint64_t>(physicalDataPtr);

        command->CDW10 = (0 << 16 | cns);
        command->CDW11 = 0;
        command->CDW14 = 0;
        queue->unlockQueue();
        queue->updateSubmissionTail();
        queue->waitUntilComplete(slot);
    }

    /**
     * Attaches a namespace to the controller. This command will complete succesfully if the namespace is not yet attached to the controller.
     * In the likely case that the namespace is already attached to the controller, it will return error code 0x18.
     * If namespace management/attachment commands are not supported or nvme subsystems are not used by the controller,
     * the command will return error code 0x2. Most of the times the namespaces are already attached if that happens.
    */
    void NvmeAdminQueue::attachNamespace(uint16_t controllerId, uint32_t nsid) {
        // Prepare Controller List
        auto &memoryService = Kernel::System::getService<Kernel::MemoryService>();
        uint16_t* controllerList = reinterpret_cast<uint16_t*>(memoryService.mapIO(4096));
        controllerList[0] = 1;
        controllerList[1] = controllerId;

        //Prepare Command
        queue->lockQueue();
        uint32_t slot = queue->getSubmissionSlotNumber();
        NvmeQueue::NvmeCommand* submissionEntry = queue->getSubmissionEntry();
        submissionEntry-> CDW0.CID = slot;
        submissionEntry->CDW0.FUSE = 0;
        submissionEntry->CDW0.PSDT = 0;
        submissionEntry->CDW0.OPC = OPC_NS_ATTACHMENT; // Command Namespace Attachment
        submissionEntry->NSID = nsid;
        submissionEntry->CDW10 = 0; // Controller Attach (0), Controller Detach (1)
        submissionEntry->PRP1 = reinterpret_cast<uint64_t>(memoryService.getPhysicalAddress(controllerList));
        submissionEntry->PRP2 = 0;
        
        queue->unlockQueue();
        queue->updateSubmissionTail();
        NvmeQueue::NvmeCompletionEntry* result = queue->waitUntilComplete(slot);
        uint8_t statusCode = result->DW3.SF & 0xFF;
        uint8_t statusCodeType = (result->DW3.SF >> 8) & 0b111;
        log.trace("[Attach Namespace %d] Status Code: %x, Status Code Type: %x", nsid, statusCode, statusCodeType);
        memoryService.freeUserMemory(controllerList);
    }

    NvmeQueue* NvmeAdminQueue::createNewQueue(uint16_t queueId, uint32_t size) {
        NvmeQueue* ioqueue = new NvmeQueue(nvme, queueId, size);
        queue->lockQueue();
        // Create completion queue first
        uint32_t slot = queue->getSubmissionSlotNumber();
        NvmeQueue::NvmeCommand* command = queue->getSubmissionEntry();
        command->CDW0.CID = slot;
        command->CDW0.FUSE = 0;
        command->CDW0.PSDT = 0;
        command->CDW0.OPC = OPC_CREATE_IO_COMPLETION_QUEUE;
        // PRP Entry 1 contains base memory address, since we use physical contigous memory regions
        command->PRP1 = ioqueue->getCompletionPhysicalAddress();
        command->PRP2 = 0;
        // DWORD10 contains the queue size and identifier
        command->CDW10 = (size << 16 | queueId );
        // DWORD11 contains interrupt vector, interrupt enable and physical contiguity information
        command->CDW11 = (0 << 16 | 1 << 1 | 1);
        // Submit the command
        queue->unlockQueue();
        queue->updateSubmissionTail();
        queue->waitUntilComplete(slot);

        // Then create the submission queue
        queue->lockQueue();
        slot = queue->getSubmissionSlotNumber();
        command = queue->getSubmissionEntry();
        command->CDW0.CID = slot;
        command->CDW0.FUSE = 0;
        command->CDW0.PSDT = 0;
        command->CDW0.OPC = OPC_CREATE_IO_SUBMISSION_QUEUE;
        // PRP Entry 1 contains base memory address, since we use physical contigous memory regions
        command->PRP1 = ioqueue->getSubmissionPhysicalAddress();
        command->PRP2 = 0;
        // DWORD10 contains queue size and identifier
        command->CDW10 = (size << 16 | queueId );
        // DWORD11 contains completion queue identifier, queue priority and physical contiguity information
        command->CDW11 = (queueId << 16 | 0b11 << 1 | 1); // 0b11 is low priority, field ignored since we do not use weighted round robin arbitration
        // DWORD12 contains NVM Set information. Cleared to 0 to not associate queue with specific NVM Set
        command->CDW12 = 0;

        // Submit the command
        queue->unlockQueue();
        queue->updateSubmissionTail();
        queue->waitUntilComplete(slot);
        nvme->registerQueueInterruptHandler(queueId, ioqueue);

        log.trace("I/O Queue [%d] created.", queueId);
        return ioqueue;
    }

    }
}