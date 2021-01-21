#include "hardware/AHCI.h"
#include "Kernel.h"
#include "lib/printf.h"

namespace DsOS::AHCI {
	PCI::Device *controller = nullptr;
	HBAMemory *abar = nullptr;

	const char *deviceTypes[5] = {"Null", "SATA", "SEMB", "PortMultiplier", "SATAPI"};

	DeviceType HBAPort::identifyDevice() volatile {
		const uint8_t ipm = (ssts >> 8) & 0x0f;
		const uint8_t det = ssts & 0x0f;

		if (det != HBA_PORT_DET_PRESENT || ipm != HBA_PORT_IPM_ACTIVE)
			return DeviceType::Null;

		switch (sig) {
			case SIG_ATAPI:
				return DeviceType::SATAPI;
			case SIG_SEMB:
				return DeviceType::SEMB;
			case SIG_PM:
				return DeviceType::PortMultiplier;
			default:
				return DeviceType::SATA;
		}
	}

	int HBAPort::getCommandSlot() volatile {
		int slots = sact | ci;
		const int command_slots = (abar->cap & 0xf00) >> 8;
		for (int i = 0; i < command_slots; ++i) {
			if ((slots & 1) == 0)
				return i;
			slots >>= 1;
		}

		return -1;
	}

	void HBAPort::rebase(volatile HBAMemory &abar) volatile {
		abar.ghc = 1 << 31;
		abar.ghc = 1;
		abar.ghc = 1 << 31;
		abar.ghc = 2;
		stop();
		cmd = cmd & ~HBA_PxCMD_CR;
		cmd = cmd & ~HBA_PxCMD_FR;
		cmd = cmd & ~HBA_PxCMD_ST;
		cmd = cmd & ~HBA_PxCMD_FRE;
		// cmd = cmd & ~0xc009;
		serr = 0xffff;
		is = 0;

		x86_64::PageMeta4K &pager = Kernel::getPager();

		if (clb || clbu)
			printf("Freeing CLB: 0x%lx -> %d\n", getCLB(), pager.freeEntry(getCLB()));
		void *addr = pager.allocateFreePhysicalAddress();
		pager.identityMap(addr);
		pager.orMeta(addr, MMU_CACHE_DISABLED);
		setCLB(addr);
		memset(addr, 0, 1024);

		if (fb || fbu)
			printf("Freeing FB: 0x%lx -> %d\n", getFB(), pager.freeEntry(getFB()));
		addr = pager.allocateFreePhysicalAddress();
		pager.identityMap(addr);
		pager.orMeta(addr, MMU_CACHE_DISABLED);
		setFB(addr);
		memset(addr, 0, 256);

		HBACommandHeader *header = (HBACommandHeader *) getCLB();
		addr = pager.allocateFreePhysicalAddress();
		pager.identityMap(addr);
		pager.orMeta(addr, MMU_CACHE_DISABLED);
		uintptr_t base = (uintptr_t) addr;

		for (int i = 0; i < 16; ++i) {
			header[i].prdtl = 8;
			header[i].setCTBA((void *) (base + i * 256));
			memset(header[i].getCTBA(), 0, 256);
		}

		addr = pager.allocateFreePhysicalAddress();
		pager.identityMap(addr);
		pager.orMeta(addr, MMU_CACHE_DISABLED);
		base = (uintptr_t) addr;
		printf("CTBA base: 0x%lx\n", base);

		for (int i = 0; i < 16; ++i) {
			header[i].prdtl = 8;
			header[i].setCTBA((void *) (base + i * 256));
			memset(header[i].getCTBA(), 0, 256);
		}

		start();
		is = 0;
		ie = 0;
	}

	void HBAPort::start() volatile {
		cmd = cmd | HBA_PxCMD_FRE;
		cmd = cmd | HBA_PxCMD_ST;
		is = 0; // ?
	}

	void HBAPort::stop() volatile {
		cmd = cmd & ~HBA_PxCMD_ST;
		cmd = cmd & ~HBA_PxCMD_FRE;
		for (;;)
			if (!(cmd & HBA_PxCMD_FR) && !(cmd & HBA_PxCMD_CR))
				break;
		cmd = cmd & ~HBA_PxCMD_FRE;
	}

	void HBAPort::setCLB(void *address) volatile {
		clb  = ((uintptr_t) address) & 0xffffffff;
		clbu = ((uintptr_t) address) >> 32;
	}

	void * HBAPort::getCLB() const volatile {
		return (void *) ((uintptr_t) clb | ((uintptr_t) clbu << 32));
	}

	void HBAPort::setFB(void *address) volatile {
		fb  = ((uintptr_t) address) & 0xffffffff;
		fbu = ((uintptr_t) address) >> 32;
	}

	void * HBAPort::getFB() const volatile {
		return (void *) ((uintptr_t) fb | ((uintptr_t) fbu << 32));
	}

	void HBAMemory::probe() volatile {
		for (int i = 0; i < 32; ++i)
			if (pi & (1 << i)) {
				const DeviceType type = ports[i].identifyDevice();
				if (type != DeviceType::Null && !(ports[i].cmd & 1))
					ports[i].cmd = ports[i].cmd | 1;
				printf("Rebasing %d (type: %d).\n", i, type);
				ports[i].rebase(*this);
			}
	}

	void HBACommandHeader::setCTBA(void *address) volatile {
		ctba  = ((uintptr_t) address) & 0xffffffff;
		ctbau = ((uintptr_t) address) >> 32;
	}

	void * HBACommandHeader::getCTBA() const volatile {
		return (void *) ((uintptr_t) ctba | ((uintptr_t) ctbau << 32));
	}
}
