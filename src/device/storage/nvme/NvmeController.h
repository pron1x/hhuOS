#ifndef HHUOS_NVMECONTROLLER_H
#define HHUOS_NVMECONTROLLER_H

#include <cstdint>

#include "kernel/interrupt/InterruptHandler.h"

namespace Device {
    class PciDevice;
    namespace Storage {
        class NvmeDevice;
    }
}
namespace Kernel {
    class Logger;
    struct InterruptFrame;
}

namespace Device::Storage {
    
    class NvmeController : public Kernel::InterruptHandler {
        public:
        /**
         * Constructor
        */
       explicit NvmeController(const PciDevice &pciDevice);

       /**
        * Destructor
       */
        ~NvmeController() override = default;

        static void initializeAvailableControllers();

        void plugin() override;

        void trigger(const Kernel::InterruptFrame &frame) override;
        
        private:
        static Kernel::Logger log;
        void* crBaseAddress;
        uint32_t doorbellStride;
        uint32_t timeout;
        static const uint32_t NVME_QUEUE_ENTRIES = 2;  // Define queue size

        void mapBaseAddressRegister(const PciDevice &pciDevice);

        enum ControllerRegister : uint32_t {
            CAP     = 0x0,      // Controller Capabilities, 64bit
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
                unsigned MQES : 16;
                unsigned CQR : 1;
                unsigned AMS : 2;
                unsigned reserved0 : 5;
                unsigned TO : 8;
            } bits;
        };

        union uControllerCapabilities {
            uint32_t uCAP;
            struct {
                unsigned DSTRD : 4;
                unsigned NSSRS : 1;
                unsigned CSS : 8;
                unsigned BPS : 1;
                unsigned reserved2 : 2;
                unsigned MPSMIN : 4;
                unsigned MPSMAX : 4;
                unsigned PMRS : 1;
                unsigned CMBS : 1;
                unsigned reserved3 : 6;
            } bits;
        };

        union ControllerConfiguration {
            uint32_t cc;
            struct {
                unsigned EN         : 1;
                unsigned _RESERVED0 : 3;
                unsigned CSS        : 3;
                unsigned MPS        : 4;
                unsigned AMS        : 3;
                unsigned SHN        : 2;
                unsigned IOSQES     : 4;
                unsigned IOCQES     : 4;
                unsigned _RESERVED1 : 8;
            } bits;
        };

        union ControllerStatus {
            uint32_t csts;
            struct {
                unsigned RDY    : 1;
                unsigned CFS    : 1;
                unsigned SHST   : 2;
                unsigned NSSRO  : 1;
                unsigned PP     : 1;
                unsigned _RSRVD : 26;
            } bits;
        };


        struct NvmeCommand {
            struct {
                uint8_t OPC;
                unsigned FUSE : 2;
                unsigned reserved : 4;
                unsigned PSDT : 2;
                unsigned CID : 16;
            } CDW0;
            uint64_t NSID;
            uint64_t _reserved0;
            uint64_t MPTR;
            uint64_t PRP1;
            uint64_t PRP2;
            uint32_t CDW10;
            uint32_t CDW11;
            uint32_t CDW12;
            uint32_t CDW13;
            uint32_t CDW14;
            uint32_t CDW15;
        };

        struct NvmeCompletionEntry {
            uint32_t DW0;
            uint32_t reserved0;
            struct {
                uint16_t SQHeadPointer;
                uint16_t SQIdentifier;
            } DW2;
            struct {
                unsigned CommandIdentifier : 16;
                unsigned P : 1;
                unsigned SF : 15;
            } DW3;
        };

    };
}


#endif