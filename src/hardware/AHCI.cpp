// Some code is from https://github.com/fido2020/Lemon-OS
// Some code is from Haiku (https://github.com/haiku/haiku)

#include "arch/x86_64/Interrupts.h"
#include "hardware/AHCI.h"
#include "Kernel.h"
#include "lib/printf.h"

namespace Thorn::AHCI {
	std::vector<Controller> controllers;

	const char *deviceTypes[5] = {"Null", "SATA", "SEMB", "PortMultiplier", "SATAPI"};

	constexpr static int SPIN_COUNT = 10000;

	Controller::Controller(PCI::Device *device_): device(device_) {
		memset(ports, 0, sizeof(ports));
	}

	void Controller::init(Kernel &kernel) {
		memset(ports, 0, sizeof(ports));

		device->setBusMastering(true);
		device->setInterrupts(true);
		device->setMemorySpace(true);

		abar = (HBAMemory *) device->getBAR(5);
		printf("abar: 0x%lx\n", abar);

		kernel.pager.identityMap(abar, MMU_CACHE_DISABLED);
		kernel.pager.identityMap((char *) abar + 0x1000, MMU_CACHE_DISABLED);

		printf("cap=%x cap2=%x", abar->cap, abar->cap2);
		printf(" caps =");
		for (uint8_t capability: device->capabilities)
			printf(" 0x%x", capability);
		printf(", vs=%x\n", abar->vs);

		abar->ghc = abar->ghc | GHC_HR;
		while (abar->ghc & GHC_HR) {
			Kernel::wait(1, 1000);
		}

		uint8_t irq = device->allocateVector(PCI::Vector::Any);
		if (irq == 0xff)
			printf("[AHCI::Controller::init] Failed to allocate vector\n");
		printf("[AHCI::Controller::init] Assigning IRQ %d\n", irq);

		uint32_t pi = abar->pi;

		while (!(abar->ghc & GHC_ENABLE)) {
			abar->ghc = abar->ghc | GHC_ENABLE;
			// TODO: how to wait without interfering with preemption?
			Kernel::wait(1, 1000);
		}

		// abar->ghc = abar->ghc | GHC_ENABLE | GHC_HR;
		// abar->ghc = abar->ghc | GHC_ENABLE;
		// abar->ghc = abar->ghc | GHC_IE;

		abar->ghc = abar->ghc | GHC_IE | GHC_ENABLE | GHC_HR;

		printf("[AHCI::Controller::init] Enabled: %y (0x%x)\n", abar->ghc & GHC_ENABLE, abar->ghc);

		x86_64::IDT::add(irq, +[] { printf("SATA IRQ triggered\n"); });

		abar->is = 0xffffffff;

		for (int i = 0; i < 32; ++i) {
			if ((pi >> i) & 1) {
				HBAPort &port = abar->ports[i];

				/*
				// Disable transitions to partial or slumber state
				port.sctl = port.sctl | SCTL_PORT_IPM_NOPART | SCTL_PORT_IPM_NOSLUM;
				// Clear IRQ status bits
				port.is = port.is;
				// Clear error bits
				port.serr = port.serr;
				// Power up device
				port.cmd = port.cmd | HBA_PxCMD_POD;
				// Spin up device
				port.cmd = port.cmd | HBA_PxCMD_SUD;
				// Activate link
				port.cmd = (port.cmd & ~HBA_PxCMD_ICC) | HBA_PxCMD_ICC_ACTIVE;
				//*/

				if (((port.ssts >> 8) & 0x0f) != HBA_PORT_IPM_ACTIVE || (port.ssts & HBA_PxSSTS_DET) != HBA_PxSSTS_DET_PRESENT) {
					printf("Skipping port %d (%y %y) %x %x\n", i, ((port.ssts >> 8) & 0x0f) != HBA_PORT_IPM_ACTIVE,
						(port.ssts & HBA_PxSSTS_DET) != HBA_PxSSTS_DET_PRESENT, port.ssts, port.serr);
					// printf("SATA status: 0x%x\n", port.ssts);
					// printf("SATA error: 0x%x\n", port.serr);
					continue;
				}

				if (!(port.sig == SIG_ATAPI || port.sig == SIG_PM || port.sig == SIG_SEMB)) {
					printf("Found SATA drive on port %d\n", i);
					// printf("SATA status: 0x%x\n", port.ssts);
					// printf("SATA error: 0x%x\n", port.serr);
				}
			}
		}
	}

	void Port::init() {
		type = identifyDevice();
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

	void Port::identify(ATA::DeviceInfo &out) {
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

		// printf("[Port::identify] Type: %s\n", deviceTypes[(int) identifyDevice()]);

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

		spin = SPIN_COUNT;
		while ((registers->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin--)
			Kernel::wait(1, 1000);

		if (0 <= spin) {
			printf("[Port::identify] Port hung\n");
			return;
		}

		registers->is = 0xffffffff;
		registers->ie = 0xffffffff;

		start();
		registers->sact = registers->sact | (1 << slot);
		registers->ci = registers->ci | (1 << slot);

		spin = SPIN_COUNT;
		while ((registers->ci & (1 << slot)) && spin--) {
			if (registers->is & HBA_PxIS_TFES) {  // Task file error
				printf("[Port::identify] Disk error 1 (serr: %x)\n", registers->serr);
				stop();
				return;
			}
			Kernel::wait(1, 1000);
		}

		stop();

		spin = SPIN_COUNT;
		while ((registers->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin--)
			Kernel::wait(1, 1000);

		if (registers->is & HBA_PxIS_TFES) {
			printf("[Port::identify] Disk error 2 (serr: %x)\n", registers->serr);
			stop();
		} else {
			memcpy(&out, physicalBuffers[0], sizeof(out));
		}
	}

	Port::Port(HBAPort *port, HBAMemory *memory): registers(port), abar(memory) {
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
			while ((registers->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin--)
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
			while (((registers->ssts & HBA_PxSSTS_DET_PRESENT) != HBA_PxSSTS_DET_PRESENT) && spin--)
				Kernel::wait(1, 1000);

			if ((registers->tfd & 0xff) == 0xff)
				Kernel::wait(1, 2);

			registers->serr = 0;
			registers->is = 0;

			spin = 1000;
			while ((registers->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin--)
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
		printf("initial tfd: %u / %b\n", registers->tfd, registers->tfd);
		abar->ghc = 1 << 31;
		printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, registers->tfd, registers->tfd);
		abar->ghc = 1;
		printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, registers->tfd, registers->tfd);
		abar->ghc = 1 << 31;
		printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, registers->tfd, registers->tfd);
		abar->ghc = 2;
		printf("tfd before stop: %u / %b\n", registers->tfd, registers->tfd);
		stop();
		printf("tfd after stop:  %u / %b\n", registers->tfd, registers->tfd);
		registers->cmd = registers->cmd & ~HBA_PxCMD_CR;
		registers->cmd = registers->cmd & ~HBA_PxCMD_FR;
		registers->cmd = registers->cmd & ~HBA_PxCMD_ST;
		registers->cmd = registers->cmd & ~HBA_PxCMD_FRE;
		printf("tfd after cmds:  %u / %b\n", registers->tfd, registers->tfd);
		// cmd = cmd & ~0xc009;
		registers->serr = 0xffff;
		registers->is = 0;

		printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, registers->tfd, registers->tfd);
		x86_64::PageMeta4K &pager = Kernel::getPager();

		if (registers->clb || registers->clbu) {
			printf("Freeing CLB: 0x%lx :: %d\n", getCLB(), pager.freeEntry(getCLB()));
			pager.freeEntry(getCLB());
		}
		void *addr = pager.allocateFreePhysicalAddress();
		pager.identityMap(addr, MMU_CACHE_DISABLED);
		setCLB(addr);
		memset(addr, 0, 1024);

		printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, registers->tfd, registers->tfd);

		if (registers->fb || registers->fbu) {
			printf("Freeing FB: 0x%lx :: %d\n", getFB(), pager.freeEntry(getFB()));
			pager.freeEntry(getFB());
		}
		addr = pager.allocateFreePhysicalAddress();
		pager.identityMap(addr, MMU_CACHE_DISABLED);
		setFB(addr);
		memset(addr, 0, 256);

		printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, registers->tfd, registers->tfd);

		HBACommandHeader *header = (HBACommandHeader *) getCLB();
		addr = pager.allocateFreePhysicalAddress();
		pager.identityMap(addr, MMU_CACHE_DISABLED);
		uintptr_t base = reinterpret_cast<uintptr_t>(addr);

		for (int i = 0; i < 16; ++i) {
			header[i].prdtl = 8;
			header[i].setCTBA((void *) (base + i * 256));
			memset(header[i].getCTBA(), 0, 256);
		}

		printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, registers->tfd, registers->tfd);

		addr = pager.allocateFreePhysicalAddress();
		pager.identityMap(addr, MMU_CACHE_DISABLED);
		base = (uintptr_t) addr;
		printf("CTBA base: 0x%lx\n", base);

		for (int i = 0; i < 16; ++i) {
			header[i].prdtl = 8;
			header[i].setCTBA((void *) (base + i * 256));
			memset(header[i].getCTBA(), 0, 256);
		}

		printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, registers->tfd, registers->tfd);
		start();
		printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, registers->tfd, registers->tfd);
		registers->is = 0;
		registers->ie = 0;
		printf("[%s:%d] tfd: %u / %b\n", __FILE__, __LINE__, registers->tfd, registers->tfd);
	}

	void Port::start() {
		registers->cmd = registers->cmd | HBA_PxCMD_FRE;
		registers->cmd = registers->cmd | HBA_PxCMD_ST;
		registers->is = 0; // ?
	}

	void Port::stop() {
		registers->cmd = registers->cmd & ~HBA_PxCMD_ST;
		registers->cmd = registers->cmd & ~HBA_PxCMD_FRE;
		int spin = SPIN_COUNT;
		while (spin--) {
			if (!(registers->cmd & HBA_PxCMD_FR) && !(registers->cmd & HBA_PxCMD_CR))
				break;
			Kernel::wait(1, 1000);
		}

		if (spin <= 0)
			printf("[Port::stop] Port hung\n");

		registers->cmd = registers->cmd & ~HBA_PxCMD_FRE;
	}

	void Port::setCLB(void *address) {
		registers->clb  = reinterpret_cast<uintptr_t>(address) & 0xffffffff;
		registers->clbu = reinterpret_cast<uintptr_t>(address) >> 32;
	}

	void * Port::getCLB() const {
		return (void *) (static_cast<uintptr_t>(registers->clb) | (static_cast<uintptr_t>(registers->clbu) << 32));
	}

	void Port::setFB(void *address) {
		registers->fb  = reinterpret_cast<uintptr_t>(address) & 0xffffffff;
		registers->fbu = reinterpret_cast<uintptr_t>(address) >> 32;
	}

	void * Port::getFB() const {
		return (void *) (static_cast<uintptr_t>(registers->fb) | (static_cast<uintptr_t>(registers->fbu) << 32));
	}

	Port::AccessStatus Port::access(uint64_t lba, uint32_t count, void *buffer, bool write) {
		registers->ie = 0xffffffff;
		registers->is = 0;

		int slot = getCommandSlot();
		if (slot == -1) {
			printf("[HBAPort::access] Invalid slot.\n");
			return AccessStatus::BadSlot;
		}

		registers->serr = 0;
		registers->tfd = 0;

		HBACommandHeader &header = commandList[slot];
		header.cfl = sizeof(FISRegH2D) / sizeof(uint32_t);

		header.atapi = type == DeviceType::SATAPI;
		header.write = write;
		header.clearBusy = false;
		header.prefetchable = false;
		header.prdbc = 0;
		header.pmport = 0;

		HBACommandTable &table = *commandTables[slot];
		memset(&table, 0, sizeof(table));

		table.prdtEntry[0].dba = reinterpret_cast<uintptr_t>(buffer) & 0xffffffff;
		table.prdtEntry[0].dbaUpper = (reinterpret_cast<uintptr_t>(buffer) >> 32) & 0xffffffff;
		table.prdtEntry[0].dbc = BLOCKSIZE * count - 1;
		table.prdtEntry[0].interrupt = true;

		FISRegH2D &fis = (FISRegH2D &) table.cfis;
		memset(&fis, 0, sizeof(fis));

		fis.type = FISType::RegH2D;
		fis.c = true;
		fis.pmport = 0;
		fis.command = write? ATA::Command::WriteDMAExt : ATA::Command::ReadDMAExt;
		fis.lba0 = lba & 0xff;
		fis.lba1 = (lba >> 8) & 0xff;
		fis.lba2 = (lba >> 16) & 0xff;
		fis.device = 1 << 6;
		fis.lba3 = (lba >> 24) & 0xff;
		fis.lba4 = (lba >> 32) & 0xff;
		fis.lba5 = (lba >> 40) & 0xff;
		fis.countLow = count & 0xff;
		fis.countHigh = (count >> 8) & 0xff;
		fis.control = 8;

		int spin = SPIN_COUNT;
		while ((registers->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin--)
			Kernel::wait(1, 1000);

		if (spin <= 0) {
			printf("[Port::access] Port is hung\n");
			return AccessStatus::Hung;
		}

		registers->ie = 0xffffffff;
		registers->is = 0xffffffff;

		start();
		registers->ci = registers->ci | (1 << slot);

		spin = SPIN_COUNT;
		while((registers->ci & (1 << slot)) && spin--) {
			if (registers->is & HBA_PxIS_TFES) {
				printf("[Port::access] Disk error (serr: %x)\n", registers->serr);
				stop();
				return AccessStatus::DiskError;
			}
		}

		if (spin <= 0) {
			printf("[Port::access] Port is hung\n");
			return AccessStatus::Hung;
		}

		spin = SPIN_COUNT;
		while ((registers->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin--)
			Kernel::wait(1, 1000);

		stop();

		if (spin <= 0) {
			printf("[Port::access] Port hung\n");
			return AccessStatus::Hung;
		}

		if (registers->is & HBA_PxIS_TFES) {
			printf("[Port::access] Disk error 2 (serr: %x)\n", registers->serr);
			return AccessStatus::DiskError;
		}

		return AccessStatus::Success;
	}

	Port::AccessStatus Port::read(uint64_t lba, uint32_t count, void *buffer) {
		uint64_t block_count = (count + BLOCKSIZE - 1) / BLOCKSIZE;
		char *cbuffer = reinterpret_cast<char *>(buffer);
		// TODO: synchronization
		unsigned buffer_index = 1;

		while (8 <= block_count && count) {
			unsigned size = BLOCKSIZE * 8;
			if (count < size)
				size = count;
			if (access(lba, 8, physicalBuffers[buffer_index], false) != AccessStatus::Success)
				return AccessStatus::DiskError;
			memcpy(cbuffer, physicalBuffers[buffer_index], size);
			cbuffer += size;
			lba += 8;
			block_count -= 8;
			count -= size;
		}

		while (2 <= block_count && count) {
			unsigned size = BLOCKSIZE * 2;
			if (count < size)
				size = count;
			if (access(lba, 2, physicalBuffers[buffer_index], false) != AccessStatus::Success)
				return AccessStatus::DiskError;
			memcpy(cbuffer, physicalBuffers[buffer_index], size);
			cbuffer += size;
			lba += 2;
			block_count -= 2;
			count -= size;
		}

		while (block_count && count) {
			unsigned size = BLOCKSIZE;
			if (count < size)
				size = count;
			if (access(lba, 1, physicalBuffers[buffer_index], false) != AccessStatus::Success)
				return AccessStatus::DiskError;
			memcpy(cbuffer, physicalBuffers[buffer_index], size);
			cbuffer += size;
			++lba;
			--block_count;
			count -= size;
		}

		return AccessStatus::Success;
	}

	Port::AccessStatus Port::readBytes(size_t count, size_t offset, void *buffer) {
		size_t total_bytes_read = 0;
		uint64_t lba = offset / BLOCKSIZE;
		offset %= BLOCKSIZE;
		char read_buffer[BLOCKSIZE];
		AccessStatus status;

		while (0 < count) {
			if ((status = read(lba, BLOCKSIZE, read_buffer)) != AccessStatus::Success)
				return status;
			const size_t to_copy = BLOCKSIZE - offset < count? BLOCKSIZE - offset : count;
			memcpy(static_cast<char *>(buffer) + total_bytes_read, read_buffer + offset, to_copy);
			total_bytes_read += to_copy;
			count -= to_copy;
			offset = 0;
			++lba;
		}

		return AccessStatus::Success;
	}

	Port::AccessStatus Port::write(uint64_t lba, uint32_t count, const void *buffer) {
		uint64_t block_count = (count + BLOCKSIZE - 1) / BLOCKSIZE;
		const char *cbuffer = reinterpret_cast<const char *>(buffer);
		// TODO: synchronization
		unsigned buffer_index = 1;

		while (block_count-- && count) {
			size_t size = count < BLOCKSIZE? count : BLOCKSIZE;
			memcpy(physicalBuffers[buffer_index], cbuffer, size);
			AccessStatus status;
			if ((status = access(lba, 1, physicalBuffers[buffer_index], true)) != AccessStatus::Success)
				return status;
			cbuffer += size;
			++lba;
		}

		return AccessStatus::Success;
	}

	Port::AccessStatus Port::writeBytes(size_t count, size_t offset, const void *buffer) {
		// const size_t original_count = count;
		uint64_t lba = offset / BLOCKSIZE;
		offset %= BLOCKSIZE;

		if (count % BLOCKSIZE == 0 && offset == 0)
			return write(lba, count, buffer);

		AccessStatus status = AccessStatus::Success;
		char write_buffer[BLOCKSIZE] = {0};
		const char *cbuffer = static_cast<const char *>(buffer);

		if (offset != 0) {
			if ((status = read(lba, BLOCKSIZE, write_buffer)) != AccessStatus::Success)
				return status;
			const size_t to_write = (BLOCKSIZE - offset) < count? BLOCKSIZE - offset : count;
			memcpy(write_buffer + offset, cbuffer, to_write);
			if ((status = write(lba, BLOCKSIZE, write_buffer) ) != AccessStatus::Success)
				return status;
			count -= to_write;
			++lba;
			cbuffer += to_write;
		}

		while (0 < count) {
			if (count < BLOCKSIZE) {
				if ((status = read(lba, BLOCKSIZE, write_buffer)) != AccessStatus::Success)
					return status;
				memcpy(write_buffer, cbuffer, count);
				if ((status = write(lba, BLOCKSIZE, write_buffer)) != AccessStatus::Success)
					return status;
				break;
			} else {
				if ((status = write(lba, BLOCKSIZE, write_buffer)) != AccessStatus::Success)
					return status;
				count -= BLOCKSIZE;
				cbuffer += BLOCKSIZE;
				++lba;
			}
		}

		// if (4 < original_count) {
		// 	printf("original_count = %lu\n", original_count);
		// 	Kernel::wait(1, 1);
		// }
		return status;
	}

	ATA::DeviceInfo & Port::getInfo() {
		if (identified)
			return info;
		identify(info);
		identified = true;
		return info;
	}

	void HBACommandHeader::setCTBA(void *address) {
		ctba  = (reinterpret_cast<uintptr_t>(address)) & 0xffffffff;
		ctbau = (reinterpret_cast<uintptr_t>(address)) >> 32;
	}

	void * HBACommandHeader::getCTBA() const {
		return (void *) ((uintptr_t) ctba | ((uintptr_t) ctbau << 32));
	}
}
