#pragma once

/*
 * Copyright 2012 Google Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdint.h>

namespace Thorn::ATA {
	enum class Command: uint8_t {
		NOP = 0x00,
		CFARequestExtendedError = 0x03,
		DeviceReset = 0x08,
		ReadSectors = 0x20,
		ReadSectorsExt = 0x24,
		ReadDMAExt = 0x25,
		ReadDMAQueuedExt = 0x26,
		ReadNativeMaxAddressExt = 0x27,
		ReadMultipleExt = 0x29,
		ReadStreamDMAExt = 0x2a,
		ReadStreamExt = 0x2b,
		ReadLogExt = 0x2f,
		WriteSectors = 0x30,
		WriteSectorsExt = 0x34,
		WriteDMAExt = 0x35,
		WriteDMAQueuedExt = 0x36,
		SetMaxAddressExt = 0x37,
		CFAWriteSectorsWithoutErase = 0x38,
		WriteMultipleExt = 0x39,
		WriteStreamDMAExt = 0x3a,
		WriteStreamExt = 0x3b,
		WriteDMAFuaExt = 0x3d,
		WriteDMAQueuedFuaExt = 0x3e,
		WriteLogExt = 0x3f,
		ReadVerifySectors = 0x40,
		ReadVerifySectorsExt = 0x42,
		WriteUncorrectableExt = 0x45,
		ReadLogDMAExt = 0x47,
		ConfigureStream = 0x51,
		WriteLogDMAExt = 0x57,
		TrustedReceive = 0x5c,
		TrustedReceiveDMA = 0x5d,
		TrustedSend = 0x5e,
		TrustedSendDMA = 0x5f,
		CFATranslateSector = 0x87,
		ExecuteDeviceDiagnostic = 0x90,
		DownloadMicrocode = 0x92,
		Packet = 0xa0,
		IdentifyPacketDevice = 0xa1,
		Service = 0xa2,
		Smart = 0xb0,
		DeviceConfigurationOverlay = 0xb1,
		NvCache = 0xb6,
		CFAEraseSectors = 0xc0,
		ReadMultiple = 0xc4,
		WriteMultiple = 0xc5,
		SetMultipleMode = 0xc6,
		ReadDMAQueued = 0xc7,
		ReadDMA = 0xc8,
		WriteDMA = 0xca,
		WriteDMAQueued = 0xcc,
		CFAWriteMultipleWithoutErase = 0xcd,
		WriteMultipleFuaExt = 0xce,
		CheckMediaCardType = 0xd1,
		StandbyImmediate = 0xe0,
		IdleImmediate = 0xe1,
		Standby = 0xe2,
		Idle = 0xe3,
		ReadBuffer = 0xe4,
		CheckPowerMode = 0xe5,
		Sleep = 0xe6,
		FlushCache = 0xe7,
		WriteBuffer = 0xe8,
		FlushCacheExt = 0xea,
		IdentifyDevice = 0xec,
		SetFeatures = 0xef,
		SecuritySetPassword = 0xf1,
		SecurityUnlock = 0xf2,
		SecurityErasePrepare = 0xf3,
		SecurityEraseUnit = 0xf4,
		SecurityFreezeLock = 0xf5,
		SecurityDisablePassword = 0xf6,
		ReadNativeMaxAddress = 0xf8,
		SetMaxAddress = 0xf9
	};

	enum class Status: uint8_t {
		Busy = 0x80,
		Ready = 0x40,
		Fault = 0x20,
		Seek = 0x10,
		DRQ = 0x08,
		Corr = 0x04,
		Index = 0x02,
		Err = 0x01,
	};

	enum class MajorRevision {
		ATA4 = 1 << 4,
		ATA5 = 1 << 5,
		ATA6 = 1 << 6,
		ATA7 = 1 << 7,
		ATA8 = 1 << 8,
	};

	struct DeviceInfo {
		uint16_t config;
		uint16_t word1;
		uint16_t specificConfig;
		uint16_t word3_9[7];
		uint16_t serial[10];
		uint16_t word20_22[3];
		uint16_t firmware[4];
		uint16_t model[20];
		uint16_t maxMultiSectors;
		uint16_t trustedFeatures;
		uint16_t capabilities[2];
		uint16_t word51_52[2];
		uint16_t dmaValid;
		uint16_t word54_58[5];
		uint16_t curMultiSectors;
		uint16_t sectors28[2];
		uint16_t word62;
		uint16_t dmaMode;
		uint16_t pioModes;
		uint16_t minNsPerDma;
		uint16_t recNsPerDma;
		uint16_t minNsPerPio;
		uint16_t minNsPerIordyPio;
		uint16_t word69_70[2];
		uint16_t word71_74[4];
		uint16_t queueDepth;
		uint16_t word76_79[4];
		uint16_t majorVersion;
		uint16_t minorVersion;
		uint16_t commandSets[2];
		uint16_t features[4];
		uint16_t dmaModes;
		uint16_t secEraseTime;
		uint16_t esecEraseTime;
		uint16_t curApm;
		uint16_t masterPasswordIdentifier;
		uint16_t resetResult;
		uint16_t acoustics;
		uint16_t streamMinSize;
		uint16_t streamTransferTimeDma;
		uint16_t streamAccessLatency;
		uint16_t streamPerfGran[2];
		uint16_t sectors48[4];
		uint16_t streamTransferTimePio;
		uint16_t word105;
		uint16_t logSectsPerPhys;
		uint16_t interSeekDelay;
		uint16_t naaIeeeOui;
		uint16_t ieeeOuiUid;
		uint16_t uid[2];
		uint16_t word112_116[5];
		uint16_t wordsPerSector[2];
		uint16_t moreFeatures[2];
		uint16_t word121_127[7];
		uint16_t secStatus;
		uint16_t word129_159[31];
		uint16_t cfaPowerMode;
		uint16_t word161_175[15];
		uint16_t mediaSerial[30];
		uint16_t sctCommandTransport;
		uint16_t word207_208[2];
		uint16_t logToPhysAlignment;
		uint16_t wrvSecMode3[2];
		uint16_t vscMode2[2];
		uint16_t nvCacheCap;
		uint16_t nvCacheSize[2];
		uint16_t nvCacheReadMbs;
		uint16_t nvCacheWriteMbs;
		uint16_t nvCacheSpinup;
		uint16_t wrvMode;
		uint16_t word221;
		uint16_t transportMajorVer;
		uint16_t transportMinorVer;
		uint16_t word224_233[10];
		uint16_t min512sPerDnldMicro;
		uint16_t max512sPerDnldMicro;
		uint16_t word236_254[19];
		uint16_t integrity;

		/** The destination string should have a capacity of at least 41 bytes. */
		void copyModel(char *);
	};
}