#include "NvmeController.h"

#include "kernel/log/Logger.h"
#include "kernel/system/System.h"
#include "kernel/service/MemoryService.h"
#include "device/pci/Pci.h"
#include "device/pci/PciDevice.h"

#include "lib/util/async/Thread.h"
#include "lib/util/time/Timestamp.h"

namespace Device::Storage {
Kernel::Logger NvmeController::log = Kernel::Logger::get("NVME");

    NvmeController::NvmeController(const PciDevice &pciDevice) {
        log.info("Initializing controller [0x%04x:0x%04x]", pciDevice.getVendorId(), pciDevice.getDeviceId());

        auto &memoryService = Kernel::System::getService<Kernel::MemoryService>();

        //Enable Bus Master DMA and Memory Space Access
        uint16_t command = pciDevice.readWord(Pci::COMMAND);
        command |= Pci::Command::BUS_MASTER | Pci::Command::MEMORY_SPACE;
        pciDevice.writeWord(Pci::COMMAND, command);

        mapBaseAddressRegister(pciDevice);

        uint32_t* Version = reinterpret_cast<uint32_t*>(crBaseAddress + (uint8_t)ControllerRegister::VS);
        uint32_t mjr, mnr, ter, ver;
        ver = *Version;
        mjr = (ver >> 16) & 0xFFFF;
        mnr = (ver >> 8) & 0xFF;
        ter = ver & 0xFF;

        log.info("Controller Version: %d.%d.%d", mjr, mnr, ter);

        /**
         * Due to hhuOS being 32bit, we split the Capabilities into lower and upper part.
         * TODO: Try and turn Capabilities into Union/Struct.
         * Due to the 32bit limitation, 2 structs will be needed.
        */

        lControllerCapabilities lcap = {};
        uControllerCapabilities ucap = {};
        ucap.uCAP = *(reinterpret_cast<uint32_t*>(crBaseAddress + 0x4));
        lcap.lCAP = *(reinterpret_cast<uint32_t*>(crBaseAddress));

        log.info("Capabilites: %x %x", ucap.uCAP, lcap.lCAP);
        log.info("MQES: %x CQR: %x AMS: %x TO: %x DSTRD: %x NSSRS: %x CSS: %x BPS: %x MPSMIN: %x MPSMAX: %x PMRS: %x CMBS: %x", 
                lcap.bits.MQES, lcap.bits.CQR, lcap.bits.AMS, lcap.bits.TO, ucap.bits.DSTRD, ucap.bits.NSSRS, ucap.bits.CSS, 
                ucap.bits.BPS, ucap.bits.MPSMIN, ucap.bits.MPSMAX, ucap.bits.PMRS, ucap.bits.CMBS);

        uint16_t maxQueueEnties = lcap.bits.MQES;

        /**
         * Controller needs to support NVM command subset
        */
       
        uint8_t nvmCommand = (ucap.bits.CSS >> 0) & 0x1;
        uint8_t adminCommand = ((ucap.bits.CSS) >> 7) & 0x1;

        /**
         * Doorbell Stride is used to calculate Submission/Completion Queue Offsets
        */

        uint32_t doorbellStride = 1 << (2 + ucap.bits.DSTRD);

        log.info("Max Queue Entries supported: %d", maxQueueEnties);
        log.info("Command sets supported. NVM command set %d, Admin only: %d. Bits: %x", nvmCommand, adminCommand, ucap.bits.CSS);
        log.info("Doorbell Stride: %d", doorbellStride);


        /**
         * hhuOS has 4kB aligned, if minPageSize is > 4kB Controller can't be initialized. 
         * This shouldn't happen with V1.4 controllers
        */

        uint32_t minPageSize = 1 << (12 + ucap.bits.MPSMIN);
        uint32_t maxPageSize = 1 << (12 + ucap.bits.MPSMAX);
        log.info("Min page size: %d, Max page size: %d", minPageSize, maxPageSize);

        /**
         * Worst case wait time for CC.RDY to flip after CC.EN flips.
         * The field is in 500ms units so we multiply by 500.
        */

        uint32_t timeout = lcap.bits.TO * 500;
        log.info("Worst case timeout: %dms", timeout);

        ControllerConfiguration* conf = reinterpret_cast<ControllerConfiguration*>(crBaseAddress) + ControllerRegister::CC/sizeof(ControllerConfiguration);
        log.info("Enabled: %x", conf->bits.EN);

        ControllerStatus* status = reinterpret_cast<ControllerStatus*>(crBaseAddress) + ControllerRegister::CSTS/sizeof(ControllerStatus);
        log.info("Ready: %x", status->bits.RDY);

        /**
         * Controller reset: disable, configure and reenable controller
        */

        conf->bits.EN = 0;
        log.info("Controller ready status: %x (Enabled: %x)", status->bits.RDY, conf->bits.EN);

        /**
         * Wait time specified in CAP.TO if RDY reads 1 after disabling
        */

        if(status->bits.RDY != 0) {Util::Async::Thread::sleep(Util::Time::Timestamp::ofMilliseconds(timeout));}
        log.info("Controller ready status (after potential timeout): %x (Enabled: %x)", status->bits.RDY, conf->bits.EN);

        /**
         * Write Controller Configuration to include correct MemoryPageSize and Command Set
        */

        conf->bits.MPS = 0;  // Set Memory Page Size (4096)
        conf->bits.CSS = 0;  // NVM Command Set

        /**
         * Create Admin Submission and Admin Completion Queue Memory, write it in controller register.
         * TODO: Queue size can be calculated from queue entry amount -> implement queue entry struct
         * TODO: Tail and Headdoorbells point at queue entries, create as pointers to queue entries.
        */

        void* aSubQueueVirtual = memoryService.mapIO(NVME_QUEUE_ENTRIES * sizeof(NvmeCommand));
        void* aSubQueuePhysical = memoryService.getPhysicalAddress(aSubQueueVirtual);

        void* aCmpQueueVirtual = memoryService.mapIO(NVME_QUEUE_ENTRIES * sizeof(NvmeCompletionEntry));
        void* aCmpQueuePhysical = memoryService.getPhysicalAddress(aCmpQueueVirtual);

        *reinterpret_cast<uint64_t*>(crBaseAddress + (uint8_t)ControllerRegister::ACQ) = reinterpret_cast<uint64_t>(aCmpQueuePhysical);    // Set Admin Completion Queue Address
        *reinterpret_cast<uint64_t*>(crBaseAddress + (uint8_t)ControllerRegister::ASQ) = reinterpret_cast<uint64_t>(aSubQueuePhysical);    // Set Admin Submission Queue Address

        
        /**
         * TODO: AQA Struct 
        */
        *reinterpret_cast<uint32_t*>(crBaseAddress + (uint8_t)ControllerRegister::AQA) = ((NVME_QUEUE_ENTRIES << 16) + NVME_QUEUE_ENTRIES); // Set Queue Size
        log.info("AQA: %x", ((NVME_QUEUE_ENTRIES << 16) + NVME_QUEUE_ENTRIES));
        
        // Set enable to 1
        conf->bits.EN = 1;

        // Wait for RDY to be 1
        log.info("Controller Status after reenable: %x (Enabled: %x)", status->bits.RDY, conf->bits.EN);
        if(status->bits.RDY == 0) {
            Util::Async::Thread::sleep(Util::Time::Timestamp::ofMilliseconds(timeout));
        }
        log.info("NVMe Controller configured and reenabled + ready! (RDY: %x Enabled: %x)", status->bits.RDY, conf->bits.EN);

    }

    void NvmeController::initializeAvailableControllers() {
        auto devices = Pci::search(Pci::Class::MASS_STORAGE, 0x08);
        for (const auto &device : devices) {
            auto *controller = new NvmeController(device);
        }
    }

    void NvmeController::plugin() {

    }

    void NvmeController::trigger(const Kernel::InterruptFrame &frame) {

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
        uint64_t physicalAddress = ((bar0 & 0xFFFFFFF0) + ((bar1 &0xFFFFFFFF) << 32));

        // Map physical address to memory
        crBaseAddress = memoryService.mapIO(physicalAddress, size);
    };
}