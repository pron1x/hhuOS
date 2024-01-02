#include "NvmeController.h"
#include "NvmeAdminQueue.h"
#include "NvmeQueue.h"

#include "kernel/log/Logger.h"
#include "kernel/system/System.h"
#include "kernel/service/MemoryService.h"
#include "kernel/service/InterruptService.h"
#include "kernel/interrupt/InterruptVector.h"

#include "device/pci/Pci.h"
#include "device/pci/PciDevice.h"

#include "lib/util/async/Thread.h"
#include "lib/util/time/Timestamp.h"
#include "lib/util/math/Math.h"

namespace Device::Storage {
    Kernel::Logger NvmeController::log = Kernel::Logger::get("NVME");

    NvmeController::NvmeController(const PciDevice &pciDevice) {
        log.info("Initializing NVMe Controller [0x%04x:0x%04x]", pciDevice.getVendorId(), pciDevice.getDeviceId());
        pci = &pciDevice;

        //Enable Bus Master DMA and Memory Space Access
        uint16_t command = pciDevice.readWord(Pci::COMMAND);
        command |= Pci::Command::BUS_MASTER | Pci::Command::MEMORY_SPACE;
        pciDevice.writeWord(Pci::COMMAND, command);

        mapBaseAddressRegister(pciDevice);

        // TODO: Introduce check for controller version? (Entry sizes could change)
        uint32_t mjr, mnr, ter, ver;
        ver = crBaseAddress[ControllerRegister::VS/sizeof(uint32_t)];
        mjr = (ver >> 16) & 0xFFFF;
        mnr = (ver >> 8) & 0xFF;
        ter = ver & 0xFF;

        log.info("Controller Version: %d.%d.%d", mjr, mnr, ter);

        /**
         * Due to hhuOS being 32bit, we split the Capabilities into lower and upper part.
        */

        lControllerCapabilities lcap = {};
        uControllerCapabilities ucap = {};
        lcap.lCAP = crBaseAddress[ControllerRegister::LCAP/sizeof(uint32_t)];
        ucap.uCAP = crBaseAddress[ControllerRegister::UCAP/sizeof(uint32_t)];

        log.debug("Capabilites: %x %x", ucap.uCAP, lcap.lCAP);
        log.debug("MQES: %x CQR: %x AMS: %x TO: %x DSTRD: %x NSSRS: %x CSS: %x BPS: %x MPSMIN: %x MPSMAX: %x PMRS: %x CMBS: %x", 
                lcap.bits.MQES, lcap.bits.CQR, lcap.bits.AMS, lcap.bits.TO, ucap.bits.DSTRD, ucap.bits.NSSRS, ucap.bits.CSS, 
                ucap.bits.BPS, ucap.bits.MPSMIN, ucap.bits.MPSMAX, ucap.bits.PMRS, ucap.bits.CMBS);

        maxQueueEnties = lcap.bits.MQES;

        /**
         * Controller needs to support NVM command subset
         * 
        */
       
        uint8_t nvmCommand = (ucap.bits.CSS >> 0) & 0x1;
        uint8_t adminCommand = ((ucap.bits.CSS) >> 7) & 0x1;
        if(nvmCommand == 0) {
            log.warn("No I/O Command Set supported! [NVM: %x | Admin: %x]", nvmCommand, adminCommand);
        }

        /**
         * Doorbell Stride is used to calculate Submission/Completion Queue Offsets
        */

        doorbellStride = ucap.bits.DSTRD;

        log.debug("Max Queue Entries supported: %d", maxQueueEnties);
        log.debug("Doorbell Stride: %d", doorbellStride);

        /**
         * hhuOS has 4kB aligned, if minPageSize is > 4kB Controller can't be initialized. 
         * This shouldn't happen with V1.4 controllers
        */
        minPageSize = 1 << (12 + ucap.bits.MPSMIN);
        maxPageSize = 1 << (12 + ucap.bits.MPSMAX);
        log.debug("Min page size: %d, Max page size: %d", minPageSize, maxPageSize);

        /**
         * Worst case wait time for CC.RDY to flip after CC.EN flips.
         * The field is in 500ms units so we multiply by 500.
        */
        timeout = lcap.bits.TO * 500;
        log.debug("Worst case timeout: %dms", timeout);

        ControllerConfiguration conf;
        ControllerStatus status;
        conf.cc = crBaseAddress[ControllerRegister::CC/sizeof(uint32_t)];
        status.csts = crBaseAddress[ControllerRegister::CSTS/sizeof(uint32_t)];

        /**
         * Controller reset: disable, configure and reenable controller
        */

        //Check if controller needs reset: Ready or Fatal State
        if(status.bits.RDY || status.bits.CFS) {
            log.info("Controller needs to be reset.");
            if(status.bits.CFS) { 
                log.warn("Controller in fatal state."); 
            }
            if(status.bits.SHST == SHST_NORMAL_OPERATION || status.bits.CFS) { // Need to check for both fatal state and no shutdown notification
                // Full shutdown required
                log.info("Shutting down controller...");
                conf.cc = crBaseAddress[ControllerRegister::CC/sizeof(uint32_t)];
                conf.bits.SHN = SHN_ABRUPT; // Abrupt shutdown notification due to fatal state
                crBaseAddress[ControllerRegister::CC/sizeof(uint32_t)] = conf.cc;
                Util::Async::Thread::sleep(Util::Time::Timestamp::ofMilliseconds(timeout)); // Wait for shutdown to complete

                status.csts = crBaseAddress[ControllerRegister::CSTS/sizeof(uint32_t)];
                if(status.bits.SHST != SHST_COMPLETE) { //shutdown not complete, wait more
                    Util::Async::Thread::sleep(Util::Time::Timestamp::ofMilliseconds(timeout));
                    status.csts = crBaseAddress[ControllerRegister::CSTS/sizeof(uint32_t)];
                    if(status.bits.SHST != SHST_COMPLETE) {
                        // Failed to shutdown controller
                        log.error("Failed to shutdown controller!");
                        // Controller cannot be initialized, check how to exit constructor!
                    }
                }
            } //Shutdown should be completed
            log.info("Resetting controller...");
            conf.cc = crBaseAddress[ControllerRegister::CC/sizeof(uint32_t)];
            conf.bits.EN = 0;
            crBaseAddress[ControllerRegister::CC/sizeof(uint32_t)] = conf.cc;
            Util::Async::Thread::sleep(Util::Time::Timestamp::ofMilliseconds(timeout)); // Wait worst case timeout for RDY to flip to 0
            
            status.csts = crBaseAddress[ControllerRegister::CSTS/sizeof(uint32_t)];
            if(status.bits.RDY != 0) {
                // Wait more
                Util::Async::Thread::sleep(Util::Time::Timestamp::ofMilliseconds(timeout));
                status.csts = crBaseAddress[ControllerRegister::CSTS/sizeof(uint32_t)];
                if(status.bits.RDY != 0) {
                    log.warn("Failed to reset/disable the controller!");
                    // Controller cannot be initialized, check how to exit constructor!
                }
            }
        } else {
            log.info("Controller does not need to be reset.");
        }
        
        log.debug("Configuring controller admin queue.");
        /**
         * Controller is ready to be configured. Refer to 7.6.1 Initialization.
        */

        crBaseAddress[ControllerRegister::AQA/sizeof(uint32_t)] = ((NVME_QUEUE_ENTRIES << 16) + NVME_QUEUE_ENTRIES); // Set Queue Size

        // Initializes and configures both Admin Queue registers
        adminQueue = Nvme::NvmeAdminQueue();
        adminQueue.Init(this, NVME_QUEUE_ENTRIES);

        /**
         * Write Controller Configuration to select arbitration mechanism, memory page size, command set and I/O Queue size
        */
        log.debug("Configuring controller AMS, MPS and CSS.");
        conf.cc = crBaseAddress[ControllerRegister::CC/sizeof(uint32_t)];
        conf.bits.AMS = 0b000;  // Round Robin
        conf.bits.MPS = 0;      // Set Memory Page Size (4096)
        conf.bits.CSS = 0b000;  // NVM Command Set
        conf.bits.IOCQES = NVME_QUEUE_ENTRIES;
        conf.bits.IOSQES = NVME_QUEUE_ENTRIES;
        crBaseAddress[ControllerRegister::CC/sizeof(uint32_t)] = conf.cc;

        // Set enable to 1
        log.info("Enabling controller.");
        conf.cc = crBaseAddress[ControllerRegister::CC/sizeof(uint32_t)];
        conf.bits.EN = 1;
        crBaseAddress[ControllerRegister::CC/sizeof(uint32_t)] = conf.cc;
        
        status.csts = crBaseAddress[ControllerRegister::CSTS/sizeof(uint32_t)];        
        // Wait for RDY to be 1
        if(status.bits.RDY == 0) {
            Util::Async::Thread::sleep(Util::Time::Timestamp::ofMilliseconds(timeout));
        }
        
        status.csts = crBaseAddress[ControllerRegister::CSTS/sizeof(uint32_t)];
        conf.cc = crBaseAddress[ControllerRegister::CC/sizeof(uint32_t)];
        log.info("NVMe Controller configured. (RDY: %x Enabled: %x)", status.bits.RDY, conf.bits.EN);
    }

    void NvmeController::initialize() {
        // Send identify Command
        // Record max transfer size (+ other stuff?)
        // Search + attach namespaces (aka drives)
        // Create I/O Completion + Submission queue
        auto &memoryService = Kernel::System::getService<Kernel::MemoryService>();
        uint8_t* info = reinterpret_cast<uint8_t*>(memoryService.mapIO(4096));

        // Identify the controller
        adminQueue.sendIdentifyCommand(memoryService.getPhysicalAddress(info), 0x01, 0);
        
        uint8_t type = info[111];
        if(type != 0x1) {
            log.warn("Controller is not an I/O Controller!");
        }
        maxDataTransfer = (1 << info[77]) * minPageSize;
        // TODO: Check why controller ID field returns 0
        id = reinterpret_cast<uint16_t*>(info)[39];
        
        // Reuse the info memory block for namespace list
        uint32_t* nsList = reinterpret_cast<uint32_t*>(info);

        // Identify the namespace list
        adminQueue.sendIdentifyCommand(memoryService.getPhysicalAddress(nsList), 0x02, 0);
        
        // Get new memory for namespace info
        NvmeNamespaceInfo* nsInfo = reinterpret_cast<NvmeNamespaceInfo*>(memoryService.mapIO(4096));
        
        // Memory for attachNamespace command
        // Get info on all namespaces (List ends on namespace id 0)
        for(int i = 0; i < 1024; i++) {
            if(nsList[i] == 0) {
                break;
            }            
            // Get namespace info data
            adminQueue.sendIdentifyCommand(memoryService.getPhysicalAddress(nsInfo), 0, nsList[i]);
            // Set ID and block amount
            uint32_t nsid = nsList[i];
            uint64_t blocks = nsInfo->NSZE;

            // Read LBA Format and block size from LBA Format
            // TODO: Assure that it's correct, seems to work now (FLBAS returns 0)
            uint8_t lbaSlot = nsInfo->FLBAS & 0x0F;
            log.trace("NS[%d]: lbaSlot: %x", nsList[i], lbaSlot);

            uint32_t blockSize = 1 << (((nsInfo->LBAFormat[lbaSlot]) >> 16 ) & 0xFF);

            namespaces.add(new Nvme::NvmeNamespace(this, nsid, blocks, blockSize));

            // BUG: The blocksize logs as 0, debugger confirmed that the namespace object gets initialized and returns the correct value
            log.debug("Namespace [%d] found. Blocks: %d, Blocksize: %d bytes", 
                    nsid, namespaces.get(i)->getSectorCount(), namespaces.get(i)->getSectorSize());
            adminQueue.attachNamespace(this->id, nsid);
            log.debug("Attached namespace.");
        }

        // Create the first I/O Queue Pair
        // FIXME: Enabled interrupts probably cause issues, check
        // HACK: According to OSDevWiki I/O Queue should be created BEFORE namespace identification/attachment.
        // This causes interrupt flodding (for some unknown reason).
        ioqueue = adminQueue.createNewQueue(1, NVME_QUEUE_ENTRIES);

        memoryService.freeUserMemory(info);
        memoryService.freeUserMemory(nsInfo);
    }

    void NvmeController::initializeAvailableControllers() {
        log.setLevel(log.DEBUG);
        auto devices = Pci::search(Pci::Class::MASS_STORAGE, PCI_SUBCLASS_NVME);
        for (const auto &device : devices) {
            auto *controller = new NvmeController(device);
            controller->plugin();
            controller->initialize();
        }
        log.setLevel(log.INFO);
    }

    uint32_t NvmeController::performRead(Nvme::NvmeNamespace* ns, uint8_t* buffer, uint32_t startBlock, uint32_t blockCount) {
        // We don't need to do anything to read 0 blocks
        if(blockCount == 0) {
            return 0;
        }
        auto &memoryService = Kernel::System::getService<Kernel::MemoryService>();

        /**
         * Read command uses DWORD10, DWORD11, DWORD12, DWORD13, DWORD14 and DWORD15.
         * DWORD10/DWORD11:
         *      Address of starting logical block.
         *      DWORD10 bits 31:00
         *      DWORD11 bits 63:32
         * DWORD12:
         *      15:00 Number of blocks to be read
         *      29:26 Protection Information Field, unused in this implementation
         *      30    Force Unit Access, unused and cleared to 0
         *      31    Limited Retry, Cleared to 0, controller applies all available error recovery to return data
         * DWORD13:
         *      03:00 Access Frequency: Cleared to 0 to provide no frequency information
         *      05:04 Access Latency: Cleared to 0 to provide no latency information
         *      06    Sequential Request: If 1, command is part of sequential read that includes multiple reads
         *      07    Incompressibe: Cleared to 0 to provide no compression information
         * DWORD14:
         *      Expected Initial Logical Block Reference Tag
         *      No end-to-end protection support, so cleared to 0
         * DWORD15
         *      Expected Logical Block Application Tag
         *      Expected Logical Block Application Tag Mask
         *      No end-to-end protection support, so cleared to 0
        */
        // Calculate required Memory for data transfer
        // This should fit into a uint32_t, otherwise buffer could not have been created
        uint32_t totalBytesToRead = ns->getSectorSize() * blockCount;
        // If bytesToRead is exactly 1 page, we write the physical Pointer into PRP1
        // If bytesToRead is exactly or smaller than 2 pages, we write physical Pointer of page 1 into PRP1
        //  and pointer of page 2 into PRP2
        // If bytesToRead is greater than 2 pages, we create a PRP List and put the PRPList pointer into PRP1
        //  and PRP2. 

        // Since the blocks to read field in a command takes a 16 bit number, we need to calculate the amount of commands to send.
        uint32_t commandsToSend = (blockCount - 1) / UINT16_MAX + 1; // Quick division and rounding up
        uint32_t maxBytesPerCommand = UINT16_MAX * ns->getSectorSize();
        uint32_t bytesLeft = totalBytesToRead;
        uint32_t cStartBlock = startBlock;
        for(uint32_t i = 0; i < commandsToSend; i++) {
            uint8_t* data;
            uint64_t prp1, prp2;
            // Bytes to read in this command
            uint32_t commandBytes = bytesLeft >= maxBytesPerCommand ? maxBytesPerCommand : bytesLeft;
            // Create the PRP Entries
            if(commandBytes <= 4096 * 2) {
                // We do not need to create a PRP List. Fill PRP1, Fill PRP2 if two pages are required.
                data = reinterpret_cast<uint8_t*>(memoryService.mapIO(bytesLeft));
                prp1 = reinterpret_cast<uint64_t>(memoryService.getPhysicalAddress(data));
                prp2 = bytesLeft <= 4096 ? 0 : reinterpret_cast<uint64_t>(memoryService.getPhysicalAddress(data+4096));
            } else {
                // We need to create a PRP List and potentially account for multiple commands that need to be sent.
                // Calculate the bytes to be read in this command
                data = reinterpret_cast<uint8_t*>(memoryService.mapIO(commandBytes));

                // Calculate amount of page pointers we're adding to the list
                uint32_t dataPages = (commandBytes - 1) / 4096 + 1; // Quick division and rounding up
                // Allocate the prp list memory
                // FIXME: prpList memory possibly needs extra space for prp list page linking entries!
                // TODO: Free PRP List memory!
                uint64_t* prpList = reinterpret_cast<uint64_t*>(memoryService.mapIO(dataPages * 4096));
                uint32_t pageSlot = 0;
                // Add pointers to the physical pages to the prp list
                for(uint32_t p = 0; p < dataPages; p++) {
                    // If we cross a prp list memory page boundary, we need to link to the next page if more pages need to be added
                    if(p % (4096/sizeof(uint64_t)) == 0 && p+1 != dataPages) { // p mod amount of pointers per memory page AND not the last pointer that is added
                        // Get the next necessary prp list page pointer. prplist + current page / sizeof(uint64_t) multiplied by pointers per page
                        prpList[pageSlot++] = reinterpret_cast<uint64_t>(memoryService.getPhysicalAddress(prpList + (p / sizeof(uint64_t)) * (4096 / sizeof(uint64_t))));
                        // Add data memory page to prp list
                        prpList[pageSlot++] = reinterpret_cast<uint64_t>(memoryService.getPhysicalAddress(data + (4096 * p)));
                    } else {
                        // Add data memory page to prp list
                        prpList[pageSlot++] = reinterpret_cast<uint64_t>(memoryService.getPhysicalAddress(data + (4096 * p)));
                    }
                }
                prp1 = reinterpret_cast<uint64_t>(memoryService.getPhysicalAddress(prpList));
                prp2 = prpList[0]; // First memory page

                // Calculate bytesLeft for next command.
                bytesLeft -= maxBytesPerCommand;
            }


            // Fill the command
            ioqueue->lockQueue();
            uint32_t cid = ioqueue->getSubmissionSlotNumber();
            Nvme::NvmeQueue::NvmeCommand* command = ioqueue->getSubmissionEntry();
            command->CDW0.CID = cid;
            command->CDW0.FUSE = 0;
            command->CDW0.PSDT = 0;
            command->CDW0.OPC = 1;

            command->NSID = ns->id;
            command->MPTR = 0;
            
            command->PRP1 = prp1;
            command->PRP2 = prp2;

            command->CDW10 = cStartBlock;
            command->CDW11 = 0;

            command->CDW12 = (0 << 31 | 0 << 30 | 0 << 26 | blockCount);
            command->CDW13 = (0 << 7 | 0 << 6 | 0 << 4 | 0);
            command->CDW14 = 0;
            command->CDW15 = 0;

            ioqueue->unlockQueue();
            ioqueue->updateSubmissionTail();
            ioqueue->waitUntilComplete(cid);
            // Copy the data from data Pointer to buffer, taking into account number of commands already sent.
            // This can (and should?) be simpler by using the 'Address' template and .copyRange method. Otherwise simplify with uint64_t pointers
            for(uint32_t j = 0; j < commandBytes; j++) {
                buffer[i * maxBytesPerCommand + j] = data[j];
            }
            // Free data pointer. Can't reuse due to potential size differences.
            // Potentially possible to reuse same pointer and avoid getting/freeing mem in loop by fix-sized reads.
            memoryService.freeUserMemory(data);
        }
        // All commands sent and data read, return number of blocks read (all).
        return blockCount;
    }

    void NvmeController::setQueueTail(uint32_t id, uint32_t entry) {
        log.trace("Setting Queue[%d] Tail Doorebell to %d", id, entry);
        crBaseAddress[getQueueDoorbellOffset(id, 0)] = entry;
    }

    void NvmeController::setQueueHead(uint32_t id, uint32_t entry) {
        log.trace("Setting Queue[%d] Head Doorbell to %d", id, entry);
        crBaseAddress[getQueueDoorbellOffset(id, 1)] = entry;
    }

    void NvmeController::registerQueueInterruptHandler(uint32_t id, Nvme::NvmeQueue* queue) {
        queues.add(queue);
    }

    void NvmeController::setInterruptMask(uint32_t queueId) {
        crBaseAddress[ControllerRegister::INTMS / sizeof(uint32_t)] = 1 << queueId;
    }

    void NvmeController::clearInterruptMask(uint32_t queueId) {
        crBaseAddress[ControllerRegister::INTMC / sizeof(uint32_t)] = 1 << queueId;
    }

    void NvmeController::plugin() {
        auto &interruptService = Kernel::System::getService<Kernel::InterruptService>();
        interruptService.assignInterrupt(Kernel::InterruptVector::FREE3, *this);
        interruptService.allowHardwareInterrupt(pci->getInterruptLine());
    }

    void NvmeController::trigger(const Kernel::InterruptFrame &frame) {
        log.trace("Received Interrupt: %x", frame.interrupt);
        for(Nvme::NvmeQueue* queue : queues) {
            queue->checkCompletionQueue();
        }
    }

    void NvmeController::mapBaseAddressRegister(const PciDevice &pciDevice) {
        auto &memoryService = Kernel::System::getService<Kernel::MemoryService>();

        uint32_t bar0 = pciDevice.readDoubleWord(Pci::BASE_ADDRESS_0);
        pciDevice.writeDoubleWord(Pci::BASE_ADDRESS_0, 0xFFFFFFFF);
        uint32_t size = pciDevice.readDoubleWord(Pci::BASE_ADDRESS_0) & 0xFFFFFFF0;
        pciDevice.writeDoubleWord(Pci::BASE_ADDRESS_0, bar0);

        size = ~size + 1;

        // Get physical address from BAR0 + BAR1;
        uint32_t bar1 = pciDevice.readDoubleWord(Pci::BASE_ADDRESS_1);
        uint64_t physicalAddress = ((uint64_t)(bar0 & 0xFFFFFFF0) + ((uint64_t)(bar1 &0xFFFFFFFF) << 32));

        // Map physical address to memory
        crBaseAddress = reinterpret_cast<uint32_t*>(memoryService.mapIO(physicalAddress, size));
    };

    void NvmeController::setAdminQueueRegisters(uint64_t submission, uint64_t completion) {
        reinterpret_cast<uint64_t*>(crBaseAddress)[ControllerRegister::ASQ/sizeof(uint64_t)] = submission;
        reinterpret_cast<uint64_t*>(crBaseAddress)[ControllerRegister::ACQ/sizeof(uint64_t)] = completion;
    };

    uint32_t NvmeController::getQueueDoorbellOffset(const uint32_t y, const uint8_t completionQueue) {
        return (0x1000 + ((2 * y + completionQueue) * (4 << doorbellStride)))/sizeof(uint32_t);
    };

}