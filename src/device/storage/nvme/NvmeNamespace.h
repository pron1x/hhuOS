#ifndef HHUOS_NVMENAMESPACE_H
#define HHUOS_NVMENAMESPACE_H

#include "device/storage/StorageDevice.h"
#include "NvmeController.h"

namespace Device::Storage {
namespace Nvme {
    class NvmeNamespace : StorageDevice {
        public:
        const uint32_t id;

        explicit NvmeNamespace(NvmeController* nvme, uint32_t id, uint64_t blocks, uint32_t blocksize);

        ~NvmeNamespace() = default;

        uint32_t getSectorSize() override;

        uint64_t getSectorCount() override;

        uint32_t read(uint8_t *buffer, uint32_t startSector, uint32_t sectorCount) override;

        uint32_t write(const uint8_t *buffer, uint32_t startSector, uint32_t sectorCount) override;

        private:
        NvmeController* nvme;
        uint64_t blocks;
        uint32_t blockSize;
    };
}
}



#endif