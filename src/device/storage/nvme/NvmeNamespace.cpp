#include "NvmeNamespace.h"

namespace Device::Storage {
namespace Nvme {

    NvmeNamespace::NvmeNamespace(NvmeController* nvme, uint32_t id, uint64_t blocks, uint32_t blocksize) {
        this->nvme = nvme;
        this->id = id;
        this->blocks = blocks;
        this->blockSize = blocksize;
    }

    uint32_t NvmeNamespace::getSectorSize() {
        return blockSize;
    }

    uint64_t NvmeNamespace::getSectorCount() { 
        return blocks; 
    };

    uint32_t NvmeNamespace::read(uint8_t *buffer, uint32_t startSector, uint32_t sectorCount) {
        return 0;
    };

    uint32_t NvmeNamespace::write(const uint8_t *buffer, uint32_t startSector, uint32_t sectorCount) {
        return 0;
    };
}
}