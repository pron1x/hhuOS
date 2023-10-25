#include "NvmeController.h"

#include "kernel/log/Logger.h"
#include "kernel/system/System.h"
#include "kernel/service/MemoryService.h"
#include "device/pci/Pci.h"
#include "device/pci/PciDevice.h"

namespace Device::Storage {
Kernel::Logger NvmeController::log = Kernel::Logger::get("NVME");

    NvmeController::NvmeController(const PciDevice &pciDevice) {
        log.info("Initializing controller [0x%04x:0x%04x]", pciDevice.getVendorId(), pciDevice.getDeviceId());

        auto &memoryService = Kernel::System::getService<Kernel::MemoryService>();

        //Enable Bus Master DMA and Memory Space Access
        uint16_t command = pciDevice.readWord(Pci::COMMAND);
        command |= Pci::Command::BUS_MASTER | Pci::Command::MEMORY_SPACE;
        pciDevice.writeWord(Pci::COMMAND, command);

        // Calculate size for memory mapping
        uint32_t bar0 = pciDevice.readDoubleWord(Pci::BASE_ADDRESS_0);
        pciDevice.writeDoubleWord(Pci::BASE_ADDRESS_0, 0xFFFFFFFF);
        uint32_t size = pciDevice.readDoubleWord(Pci::BASE_ADDRESS_0) & 0xFFFFFFF0;
        pciDevice.writeDoubleWord(Pci::BASE_ADDRESS_0, bar0);

        size = ~size + 1;

        // Get physical address from BAR0 + BAR1;
        uint32_t bar1 = pciDevice.readDoubleWord(Pci::BASE_ADDRESS_1);
        uint64_t physicalAddress = ((bar0 & 0xFFFFFFF0) + ((bar1 &0xFFFFFFFF) << 32));

        // Map physical address to memory
        void* virtualAddress = memoryService.mapIO(physicalAddress, size);

        uint32_t* Version = reinterpret_cast<uint32_t*>(virtualAddress + (uint8_t)ControllerRegister::VS); // Version Offset: 0x8;
        uint32_t mjr, mnr, ter, ver;
        ver = *Version;
        mjr = (ver >> 16) & 0xFFFF;
        mnr = (ver >> 8) & 0xFF;
        ter = ver & 0xFF;

        log.info("Controller Version: %d.%d.%d", mjr, mnr, ter);

        /**
         * Due to hhuOS being 32bit, we split the Capabilities into lower and upper part.
        */
        uint32_t capabilitiesUpper = *(reinterpret_cast<uint32_t*>(virtualAddress + 0x4));
        uint32_t capabilitiesLower = *(reinterpret_cast<uint32_t*>(virtualAddress));

        log.info("Capabilites: %x %x", capabilitiesUpper, capabilitiesLower);

        uint16_t maxQueueEnties = capabilitiesLower & 0xFFFF;

        /**
         * Controller needs to support NVM command subset
        */
        uint8_t CSS = ((capabilitiesUpper >> 5) & 0xFF);
        uint8_t nvmCommand = CSS & 0x1;
        uint8_t adminCommand = (CSS >> 7) & 0x1;

        /**
         * Doorbell Stride is used to calculate Submission/Completion Queue Offsets
        */
        uint32_t doorbellStride = 1 << (2 + ((capabilitiesUpper) & 0xF));

        log.info("Max Queue Entries supported: %d", maxQueueEnties);
        log.info("Command sets supported. NVM command set %d, Admin only: %d", nvmCommand, adminCommand);
        log.info("Doorbell Stride: %d", doorbellStride);


        /**
         * hhuOS has 4kB aligned, if minPageSize is > 4kB Controller can't be initialized. 
         * This shouldn't happen with V1.4 controllers
        */
        uint32_t minPageSize = 1 << (12 + ((capabilitiesUpper >> 48) & 0xF));
        uint32_t maxPageSize = 1 << (12 + ((capabilitiesUpper >> 52) & 0xF));
        log.info("Min page size: %d, Max page size: %d", minPageSize, maxPageSize);

        /**
         * Worst case wait time for CC.RDY to flip after CC.EN flips.
         * The field is in 500ms units so we multiply by 500.
        */
       uint32_t timeout = ((capabilitiesLower >> 24) & 0xFF) * 500;
       log.info("Worst case timeout: %dms", timeout);

       ControllerConfiguration conf;
       conf.cc =  *reinterpret_cast<uint32_t*>(virtualAddress + (uint8_t)ControllerRegister::CC);
       log.info("Enabled: %x", conf.bits.EN);

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
}