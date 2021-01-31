// Some code is from https://github.com/fido2020/Lemon-OS

#include "arch/x86_64/Interrupts.h"
#include "hardware/AHCI.h"
#include "Kernel.h"
#include "lib/printf.h"

namespace DsOS::AHCI {
	std::vector<Controller> *controllers;

	const char *deviceTypes[5] = {"Null", "SATA", "SEMB", "PortMultiplier", "SATAPI"};

	Controller::Controller(PCI::Device *device_): device(device_) {
		memset(ports, 0, sizeof(ports));
	}

	void Controller::init(Kernel &kernel) {
		uint16_t command = device->getCommand();
		command = (command & ~PCI::COMMAND_INT_DISABLE) | PCI::COMMAND_MEMORY | PCI::COMMAND_MASTER;
		device->setCommand(command);
		abar = (HBAMemory *) (uintptr_t) (device->getBAR(5) & ~0xf);
		printf("abar: 0x%lx\n", abar);
		kernel.pager.identityMap(abar, MMU_CACHE_DISABLED);
		kernel.pager.identityMap((char *) abar + 0x1000, MMU_CACHE_DISABLED);
		printf("cap=%b", abar->cap);
		printf(" caps=");
		for (uint8_t capability: device->capabilities)
			printf("0x%x ", capability);
		printf("\n");
		// abar->probe();
		// abar->cap = abar->cap | (1 << 31);
		// abar->ghc = abar->ghc | (1 << 31);
		uint8_t irq = device->allocateVector(PCI::Vector::Any);
		if (irq == 0xff)
			printf("[AHCI::Controller::init] Failed to allocate vector\n");
		uint32_t pi = abar->pi;
		while (!(abar->ghc & GHC_ENABLE)) {
			abar->ghc = abar->ghc | GHC_ENABLE;
			// TODO: how to wait without interfering with preemption?
			Kernel::wait(1, 1000);
		}
		abar->ghc = abar->ghc | GHC_IE;
		printf("[AHCI::Controller::init] Enabled: %y\n", abar->ghc & GHC_ENABLE);

		x86_64::IDT::add(irq, +[] { printf("SATA IRQ triggered\n"); });
		abar->is = 0xffffffff;

		for (int i = 0; i < 32; ++i) {
			if ((pi >> i) & 1) {
				volatile HBAPort &port = abar->ports[i];
				if (((port.ssts >> 8) & 0x0f) != HBA_PORT_IPM_ACTIVE || (port.ssts & HBA_PxSSTS_DET) != HBA_PxSSTS_DET_PRESENT) {
					printf("Skipping port %d\n", i);
					continue;
				}

				if (!(port.sig == SIG_ATAPI || port.sig == SIG_PM || port.sig == SIG_SEMB)) {
					printf("Found SATA drive on port %d\n", i);
				}
			}
		}
	}

	DeviceType Port::identifyDevice() {
		const uint8_t ipm = (registers->ssts >> 8) & 0x0f;
		const uint8_t det = registers->ssts & 0x0f;

		if (det != HBA_PORT_DET_PRESENT || ipm != HBA_PORT_IPM_ACTIVE)
			return DeviceType::Null;

		switch (registers->sig) {
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

	void Port::identify() {
		registers->ie = 0xffffffff;
		registers->is = 0;
		registers->tfd = 0;
		int spin = 0;
		int slot = getCommandSlot();
		if (slot == -1) {
			printf("[Port::identify] Couldn't find command slot\n");
			return;
		}

		HBACommandHeader &header = commandList[slot];
		header.cfl = sizeof(FISRegH2D) / sizeof(uint32_t);
		header.atapi = false;
		header.write = false;
		header.clearBusy = false;
		header.prefetchable = false;
		header.prdbc = 0;
		header.pmport = 0;

		HBACommandTable *table = commandTables[slot];
		memset(table, 0, sizeof(HBACommandTable));

		table->prdtEntry[0].dba = (uintptr_t) physicalBuffers[0] & 0xffffffff;
		table->prdtEntry[0].dbaUpper = ((uintptr_t) physicalBuffers[0] >> 32) & 0xffffffff;
		table->prdtEntry[0].dbc = BLOCKSIZE - 1;
		table->prdtEntry[0].interrupt = true;

		FISRegH2D *cfis = (FISRegH2D *) table->cfis;
		memset(cfis, 0, sizeof(FISRegH2D));

		printf("[Port::identify] Type: %s\n", deviceTypes[(int) identifyDevice()]);

		// A lot of these are probably redundant because of the memset.
		cfis->type = FISType::RegH2D;
		cfis->c = true;
		cfis->pmport = 0;
		if (identifyDevice() == DeviceType::SATAPI)
			cfis->command = ATA::Command::IdentifyPacketDevice;
		else
			cfis->command = ATA::Command::IdentifyDevice;
		cfis->lba0 = 0;
		cfis->lba1 = 0;
		cfis->lba2 = 0;
		cfis->device = 0;
		cfis->lba3 = 0;
		cfis->lba4 = 0;
		cfis->lba5 = 0;
		cfis->countLow = cfis->countHigh = 0;
		cfis->control = 0;

		spin = 100;
		while (spin-- && (registers->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)))
			Kernel::wait(1, 1000);

		if (1'000'000 <= spin) {
			printf("[Port::identify] Port hung\n");
			return;
		}

		registers->is = 0xffffffff;
		registers->ie = 0xffffffff;

		start();
		registers->sact = registers->sact | (1 << slot);
		registers->ci = registers->ci | (1 << slot);

		while (registers->ci & (1 << slot))
			if (registers->is & HBA_PxIS_TFES) {  // Task file error
				printf("[Port::identify] Disk error 1 (serr: %x)\n", registers->serr);
				stop();
				return;
			}

		stop();

		spin = 100;
		while (spin-- && (registers->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)))
			Kernel::wait(1, 1000);

		if (registers->is & HBA_PxIS_TFES) {
			printf("[Port::identify] Disk error 2 (serr: %x)\n", registers->serr);
			stop();
		}
	}

	Port::Port(volatile HBAPort *port, volatile HBAMemory *memory): registers(port), abar(memory) {
		registers->cmd = registers->cmd & ~HBA_PxCMD_ST;
		registers->cmd = registers->cmd & ~HBA_PxCMD_FRE;
		stop();

		if (!Kernel::instance) {
			printf("[Port::Port] Kernel instance is null!\n");
			for (;;) asm("hlt");
		}

		auto &pager = Kernel::instance->pager;

		void *addr = pager.allocateFreePhysicalAddress();
		pager.identityMap(addr, MMU_CACHE_DISABLED);
		setCLB(addr);
		memset(addr, 0, 4096);

		addr = pager.allocateFreePhysicalAddress();
		pager.identityMap(addr, MMU_CACHE_DISABLED);
		setFB(addr);
		memset(addr, 0, 4096);

		commandList = (HBACommandHeader *) getCLB();
		memset(commandList, 0, sizeof(HBACommandHeader));

		fis = (HBAFIS *) getFB();
		memset(fis, 0, sizeof(HBAFIS));

		fis->dsfis.type = FISType::DMASetup;
		fis->psfis.type = FISType::PIOSetup;
		fis->rfis.type = FISType::RegD2H;
		fis->sdbfis[0] = static_cast<uint8_t>(FISType::DevBits);

		for (int i = 0; i < 8; ++i) {
			commandList[i].prdtl = 1;

			addr = pager.allocateFreePhysicalAddress();
			pager.identityMap(addr, MMU_CACHE_DISABLED);
			commandList[i].ctba = (uintptr_t) addr & 0xffffffff;
			commandList[i].ctbau = (uintptr_t) addr >> 32;
			commandTables[i] = (HBACommandTable *) addr;
			memset(addr, 0, 4096);
		}

		registers->sctl = registers->sctl | (SCTL_PORT_IPM_NOPART | SCTL_PORT_IPM_NOSLUM | SCTL_PORT_IPM_NODSLP);

		if (abar->cap & CAP_SALP)
			registers->cmd = registers->cmd & ~HBA_PxCMD_ASP;

		registers->is = 0;
		registers->ie = 1;
		registers->fbs = registers->fbs & ~0xfffff000U;

		registers->cmd = registers->cmd | HBA_PxCMD_POD;
		registers->cmd = registers->cmd | HBA_PxCMD_SUD;

		Kernel::wait(1, 100);

		{
			int spin = 1000;
			while (spin-- && (registers->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)))
				Kernel::wait(1, 1000);

			if (spin <= 0) {
				printf("[Port::Port] Port hung\n");
				// Reset the port.
				registers->sctl = SCTL_PORT_DET_INIT | SCTL_PORT_IPM_NOPART | SCTL_PORT_IPM_NOSLUM | SCTL_PORT_IPM_NODSLP;
			}

			Kernel::wait(1, 100);
			registers->sctl = registers->sctl & ~HBA_PxSSTS_DET;
			Kernel::wait(1, 100);

			spin = 200;
			while (spin-- && (registers->ssts & HBA_PxSSTS_DET_PRESENT) != HBA_PxSSTS_DET_PRESENT)
				Kernel::wait(1, 1000);

			if ((registers->tfd & 0xff) == 0xff)
				Kernel::wait(1, 2);

			registers->serr = 0;
			registers->is = 0;

			spin = 1000;
			while (spin-- && registers->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ))
				Kernel::wait(1, 1000);

			if (spin <= 0)
				printf("[Port::Port] Port hung\n");
		}

		if ((registers->ssts & HBA_PxSSTS_DET) != HBA_PxSSTS_DET_PRESENT) {
			printf("[Port::Port] Device not present (DET: %x)\n", registers->ssts & HBA_PxSSTS_DET);
			status = Status::Error;
			return;
		}

		for (unsigned i = 0; i < 8; ++i) {
			pager.identityMap(physicalBuffers[i] = pager.allocateFreePhysicalAddress());
			// buffers[i] = Memory::KernelAllocate4KPages(1);
			// Memory::KernelMapVirtualMemory4K(physBuffers[i], (uintptr_t)buffers[i], 1);
		}

		status = Status::Active;


	}

	int Port::getCommandSlot() {
		int slots = registers->sact | registers->ci;
		const int command_slots = (abar->cap & 0xf00) >> 8;
		for (int i = 0; i < command_slots; ++i) {
			if ((slots & 1) == 0)
				return i;
			slots >>= 1;
		}

		return -1;
	}

	void Port::rebase() {
		// printf("initial tfd: %u / %b\n", tfd, tfd);
		abar->ghc = 1 << 31;
		// printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, tfd, tfd);
		abar->ghc = 1;
		// printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, tfd, tfd);
		abar->ghc = 1 << 31;
		// printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, tfd, tfd);
		abar->ghc = 2;
		// printf("tfd before stop: %u / %b\n", tfd, tfd);
		stop();
		// printf("tfd after stop:  %u / %b\n", tfd, tfd);
		registers->cmd = registers->cmd & ~HBA_PxCMD_CR;
		registers->cmd = registers->cmd & ~HBA_PxCMD_FR;
		registers->cmd = registers->cmd & ~HBA_PxCMD_ST;
		registers->cmd = registers->cmd & ~HBA_PxCMD_FRE;
		// printf("tfd after cmds:  %u / %b\n", tfd, tfd);
		// cmd = cmd & ~0xc009;
		registers->serr = 0xffff;
		registers->is = 0;

		// printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, tfd, tfd);
		x86_64::PageMeta4K &pager = Kernel::getPager();

		if (registers->clb || registers->clbu)
			// printf("Freeing CLB: 0x%lx :: %d\n", getCLB(), pager.freeEntry(getCLB()));
			pager.freeEntry(getCLB());
		void *addr = pager.allocateFreePhysicalAddress();
		pager.identityMap(addr, MMU_CACHE_DISABLED);
		setCLB(addr);
		memset(addr, 0, 1024);

		// printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, tfd, tfd);

		if (registers->fb || registers->fbu)
			// printf("Freeing FB: 0x%lx :: %d\n", getFB(), pager.freeEntry(getFB()));
			pager.freeEntry(getFB());
		addr = pager.allocateFreePhysicalAddress();
		pager.identityMap(addr, MMU_CACHE_DISABLED);
		setFB(addr);
		memset(addr, 0, 256);

		// printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, tfd, tfd);

		HBACommandHeader *header = (HBACommandHeader *) getCLB();
		addr = pager.allocateFreePhysicalAddress();
		pager.identityMap(addr, MMU_CACHE_DISABLED);
		uintptr_t base = (uintptr_t) addr;

		for (int i = 0; i < 16; ++i) {
			header[i].prdtl = 8;
			header[i].setCTBA((void *) (base + i * 256));
			memset(header[i].getCTBA(), 0, 256);
		}

		// printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, tfd, tfd);

		addr = pager.allocateFreePhysicalAddress();
		pager.identityMap(addr, MMU_CACHE_DISABLED);
		base = (uintptr_t) addr;
		// printf("CTBA base: 0x%lx\n", base);

		for (int i = 0; i < 16; ++i) {
			header[i].prdtl = 8;
			header[i].setCTBA((void *) (base + i * 256));
			memset(header[i].getCTBA(), 0, 256);
		}

		// printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, tfd, tfd);
		start();
		// printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, tfd, tfd);
		registers->is = 0;
		registers->ie = 0;
		// printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, tfd, tfd);
	}

	void Port::start() {
		registers->cmd = registers->cmd | HBA_PxCMD_FRE;
		registers->cmd = registers->cmd | HBA_PxCMD_ST;
		registers->is = 0; // ?
	}

	void Port::stop() {
		registers->cmd = registers->cmd & ~HBA_PxCMD_ST;
		registers->cmd = registers->cmd & ~HBA_PxCMD_FRE;
		for (;;)
			if (!(registers->cmd & HBA_PxCMD_FR) && !(registers->cmd & HBA_PxCMD_CR))
				break;
		registers->cmd = registers->cmd & ~HBA_PxCMD_FRE;
	}

	void Port::setCLB(void *address) {
		registers->clb  = ((uintptr_t) address) & 0xffffffff;
		registers->clbu = ((uintptr_t) address) >> 32;
	}

	void * Port::getCLB() const {
		return (void *) ((uintptr_t) registers->clb | ((uintptr_t) registers->clbu << 32));
	}

	void Port::setFB(void *address) {
		registers->fb  = ((uintptr_t) address) & 0xffffffff;
		registers->fbu = ((uintptr_t) address) >> 32;
	}

	void * Port::getFB() const {
		return (void *) ((uintptr_t) registers->fb | ((uintptr_t) registers->fbu << 32));
	}

	Port::AccessStatus Port::access(uint64_t lba, uint32_t count, void *buffer, bool write) {
		registers->ie = 0xffffffff;
		registers->is = 0;
		int spin = 0;
		int slot = getCommandSlot();
		if (slot == -1) {
			printf("[HBAPort::access] Invalid slot.\n");
			return AccessStatus::BadSlot;
		}

		registers->serr = 0;
		registers->tfd = 0;

		return AccessStatus::Success;
	}

	// void HBAMemory::probe() volatile {
	// 	for (int i = 0; i < 32; ++i)
	// 		if (pi & (1 << i)) {
	// 			// const DeviceType type = ports[i].identifyDevice();
	// 			// if (type != DeviceType::Null && !(ports[i].cmd & 1))
	// 			// 	ports[i].cmd = ports[i].cmd | 1;
	// 			// printf("Rebasing %d (type: %d).\n", i, type);
	// 			ports[i].rebase(*this);
	// 		}
	// }

	void HBACommandHeader::setCTBA(void *address) volatile {
		ctba  = ((uintptr_t) address) & 0xffffffff;
		ctbau = ((uintptr_t) address) >> 32;
	}

	void * HBACommandHeader::getCTBA() const volatile {
		return (void *) ((uintptr_t) ctba | ((uintptr_t) ctbau << 32));
	}
}
