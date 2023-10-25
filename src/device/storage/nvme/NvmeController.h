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

    };
}


#endif