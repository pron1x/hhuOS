#ifndef HHUOS_NVMECONTROLLER_H
#define HHUOS_NVMECONTROLLER_H

#include <cstdint>

#include "kernel/log/Logger.h"
#include "kernel/interrupt/InterruptHandler.h"
#include "device/pci/PciDevice.h"

#include "device/storage/nvme/NvmeAdminQueue.h"

#include "lib/util/collection/ArrayList.h"
namespace Device::Storage {
    class NvmeController;
    namespace Nvme {
        class NvmeAdminQueue;
        class NvmeQueue;
    }
}

namespace Device::Storage {
    
    class NvmeController : public Kernel::InterruptHandler {
        public:

        explicit NvmeController(const PciDevice &pciDevice);

        ~NvmeController() override = default;

        static void initializeAvailableControllers();

        void initialize();

        /** Sets the Admin Queue Registers Base Addresses for Submission and Completion Queues*/
        void setAdminQueueRegisters(uint64_t submission, uint64_t completion);

        /**
         * Updates the Tail Doorbell Register for the specified queue to the new entry
         * @param id Queue id for which to update the tail register
         * @param entry Entry to write into the Tail Doorbell Register
         * TODO: Update function name to be more precise (Submission queues use tail doorbell register)
        */
        void setQueueTail(uint32_t id, uint32_t entry);

        /**
         * Updates the Head Doorbell Register for the specified queue to the new entry
         * @param id Queue id for which to update the tail register
         * @param entry Entry to write into the Head Doorbell Register
        */
        void setQueueHead(uint32_t id, uint32_t entry);

        /**
         * Registers a queue object to be called when the controller receives an interrupt
         * @param id Id of the queue
         * @param queue The queue object which will be called to check it's completion entries
        */
        void registerQueueInterruptHandler(uint32_t id, Nvme::NvmeQueue* queue);

        void setInterruptMask(uint32_t queueId);

        void clearInterruptMask(uint32_t queueId);

        void plugin() override;

        void trigger(const Kernel::InterruptFrame &frame) override;

        Nvme::NvmeAdminQueue adminQueue;
        
        private:

        static Kernel::Logger log;
        const Device::PciDevice* pci;

        Util::ArrayList<Nvme::NvmeQueue*> queues;

        /**
         * hhuOS is 32bit, if access to 64 bit registers is required, cast to 64bit pointer.
        */
        uint32_t* crBaseAddress;
        uint32_t doorbellStride;
        uint32_t timeout;
        uint16_t maxQueueEnties;
        uint32_t minPageSize;
        uint32_t maxPageSize;
        uint32_t maxDataTransfer;

        static const uint32_t NVME_QUEUE_ENTRIES = 2;  // Define queue size

        void mapBaseAddressRegister(const PciDevice &pciDevice);

        /**
         * Calculates the offset for queue y doorbell register
         * @param y Queue Y of which to get the offset
         * @param completionQueue 0 for submission, 1 for completion
        */
        uint32_t getQueueDoorbellOffset(const uint32_t y, const uint8_t completionQueue);
        

        // Enums / Structs for NVMe Controllers

        enum ControllerRegister : uint32_t {
            LCAP    = 0x0,      // Lower Controller Capabilities, 32bit
            UCAP    = 0x4,      // Upper Controller Capabilites, 32bit
            VS      = 0x8,      // Version, 32bit
            INTMS   = 0xC,      // Interrupt Mask Set, 32bit
            INTMC   = 0x10,     // Interrupt Mask Clear, 32bit
            CC      = 0x14,     // Controller Configuration, 32bit
            CSTS    = 0x1C,     // Controller Status, 32bit
            NSSR    = 0x20,     // NVM Subsystem Reset, 32bit (Optional)
            AQA     = 0x24,     // Admin Queue Attributes, 32bit
            ASQ     = 0x28,     // Admin Submission Queue Base Address, 64bit
            ACQ     = 0x30,     // Admin Completion Queue Base Address, 64bit
            CMBLOC  = 0x38,     // Controller Memory Buffer Location, 32bit (Optional)
            CMBSZ   = 0x3C,     // Controller Memory Buffer Size, 32bit (Optional)
            BPINFO  = 0x40,     // Boot Partition Information, 32bit (Optional)
            BPRSEL  = 0x44,     // Boot Partition Read Select, 32bit (Optional)
            BPMBL   = 0x48,     // Boot Partition Memory Buffer Location, 64bit (Optional)
            CMBMSC  = 0x50,     // Controller Memory Buffer Memory Space Control, 64bit (Optional)
            CMBSTS  = 0x58,     // Controller Memory Buffer Status, 32bit (Optional)
            PMRCAP  = 0xE00,    // Persistent Memory Region Capabilites, 32bit (Optional)
            PMRCTL  = 0xE04,    // Persisten Memory Region Control, 32bit (Optional)
            PMRSTS  = 0xE08,    // Persisten Memory Region Status, 32bit (Optional)
            PMREBS  = 0xE0C,    // Persisten Memory Region Elasticity Buffer Size, 32bit
            PMRSWTP = 0xE10,    // Persisten Memory Region Sustained Write Throughput, 32bit
            PMRMSC  = 0xE14     // Persisten Memory Region Memory Space Control, 64bit (Optional)
        };

        union lControllerCapabilities {
            uint32_t lCAP;
            struct {
                unsigned MQES       : 16;   // Maximum Queue Entries Supported
                unsigned CQR        : 1;    // Contigous Queues Required
                unsigned AMS        : 2;    // Arbitration Mechanism Supported
                unsigned _reserved0 : 5;
                unsigned TO         : 8;    // Timeout
            } bits;
        };

        union uControllerCapabilities {
            uint32_t uCAP;
            struct {
                unsigned DSTRD      : 4;    // Doorbell Stride
                unsigned NSSRS      : 1;    // NVM Subsystem Reset Supported
                unsigned CSS        : 8;    // Command Sets Supported
                unsigned BPS        : 1;    // Boot Partition Support
                unsigned _reserved2 : 2;
                unsigned MPSMIN     : 4;    // Memory Page Size Minimum (2 ^ (12 + MPSMIN))
                unsigned MPSMAX     : 4;    // Memory Page Size Maximum (2 ^ (12 + MPSMAX))
                unsigned PMRS       : 1;    // Persisten Memory Region Supported
                unsigned CMBS       : 1;    // Controller Memory Buffer Supported
                unsigned _reserved3 : 6;
            } bits;
        };

        union ControllerConfiguration {
            uint32_t cc;
            struct {
                unsigned EN         : 1;    // Enable
                unsigned _reserved0 : 3;
                unsigned CSS        : 3;    // I/O Command Set Selected
                unsigned MPS        : 4;    // Memory Page Size
                unsigned AMS        : 3;    // Arbitration Mechanism Selected
                unsigned SHN        : 2;    // Shutdown Notification
                unsigned IOSQES     : 4;    // I/O Submission Queue Entry Size
                unsigned IOCQES     : 4;    // I/O Completion Queue Entry Size
                unsigned _reserved1 : 8;
            } bits;
        };

        union ControllerStatus {
            uint32_t csts;
            struct {
                unsigned RDY        : 1;    // Ready
                unsigned CFS        : 1;    // Controller Fatal Status
                unsigned SHST       : 2;    // Shtudown Status
                unsigned NSSRO      : 1;    // NVM Subsystem Reset Occurred
                unsigned PP         : 1;    // Processing Paused
                unsigned _reserved0 : 26;
            } bits;
        };

        struct NvmeNamespaceInfo {
            uint64_t NSZE;
            uint64_t NCAP;
            uint64_t NUSE;
            uint8_t NSFEAT;
            uint8_t NLBAF;
            uint8_t FLBAS;
            uint8_t MC;
            uint8_t DPC;
            uint8_t DPS;
            uint8_t NMIC;
            uint8_t RESCAP;
            uint8_t FPI;
            uint8_t DLFEAT;
            uint16_t NAWUN;
            uint16_t NAWUPF;
            uint16_t NACWU;
            uint16_t NABSN;
            uint16_t NABO;
            uint16_t NABSPF;
            uint16_t NOIOB;
            uint64_t NVMCAP[2];
            uint16_t NPWG;
            uint16_t NPWA;
            uint16_t NPDG;
            uint16_t NPDA;
            uint16_t NOWS;
            uint8_t _reserved0[18];
            uint32_t ANAGRPID;
            uint8_t _reserved1[3];
            uint8_t NSATTR;
            uint16_t NVMSETID;
            uint16_t ENDGID;
            uint8_t NGUID[16];
            uint64_t EUI64;
            uint32_t LBAFormat[16];
            uint8_t _reserved2[191];
            uint8_t _vendor[3711];
        };

        // NvmeNamespace Information. Block amount and Block size should be uint64_t, not supported by hhuOS currently
        struct NvmeNamespace {
            uint32_t id;
            uint32_t blocks;
            uint32_t blockSize;
        };

        // Controller Supports up to 1024 namespaces
        NvmeNamespace namespaces[1024];
    };
}


#endif