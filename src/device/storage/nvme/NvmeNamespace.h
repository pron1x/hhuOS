#ifndef HHUOS_NVMENAMESPACE_H
#define HHUOS_NVMENAMESPACE_H

#include "device/storage/StorageDevice.h"
#include "NvmeController.h"

namespace Device::Storage {
namespace Nvme {
    class NvmeNamespace : StorageDevice {
        public:

        explicit NvmeNamespace(NvmeController* nvme, uint16_t id, uint64_t blocks, uint64_t blocksize);

        ~NvmeNamespace() = default;

        uint32_t getSectorSize() override { return blockSize; };

        uint64_t getSectorCount() override { return blocks; };

        uint32_t read(uint8_t *buffer, uint32_t startSector, uint32_t sectorCount) override;

        uint32_t write(const uint8_t *buffer, uint32_t startSector, uint32_t sectorCount) override;

        private:
        NvmeController* nvme;
        uint16_t id;
        uint64_t blocks;
        uint32_t blockSize;
    };
}
}



#endif