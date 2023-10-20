#include "NvmeController.h"

#include "kernel/log/Logger.h"
#include "device/pci/Pci.h"
#include "device/pci/PciDevice.h"

namespace Device::Storage {
Kernel::Logger NvmeController::log = Kernel::Logger::get("NVME");

    NvmeController::NvmeController(const PciDevice &pciDevice) {
        log.info("Initializing controller [0x%04x:0x%04x]", pciDevice.getVendorId(), pciDevice.getDeviceId());
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