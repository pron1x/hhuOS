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

        void setAdminQueueRegisters(uint64_t submission, uint64_t completion);

        void setQueueTail(uint32_t id, uint32_t entry);

        void plugin() override;

        void trigger(const Kernel::InterruptFrame &frame) override;
        
        private:
        static Kernel::Logger log;
        /**
         * hhuOS is 32bit, if access to 64 bit registerscis required, cast to 64bit pointer.
        */
        uint32_t* crBaseAddress;
        uint32_t doorbellStride;
        uint32_t timeout;
        static const uint32_t NVME_QUEUE_ENTRIES = 2;  // Define queue size

        void mapBaseAddressRegister(const PciDevice &pciDevice);

        /**
         * Calculates the offset for queue y doorbell register
         * @param y Queue Y of which to get the offset
         * @param completionQueue 0 for submission, 1 for completion
        */
        uint32_t getQueueDoorbellOffset(const uint32_t y, const uint8_t completionQueue);

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
        
    };
}


#endif