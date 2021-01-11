#include "fs/dsFAT/dsFAT.h"

namespace DsOS::FS::DsFAT {
	int DsFATDriver::rename(const char *path, const char *newpath) {
		return 0;
	}

	int DsFATDriver::release(const char *path, FileInfo &) {
		return 0;
	}

	int DsFATDriver::statfs(const char *, DriverStats &) {
		return 0;
	}

	int DsFATDriver::utimens(const char *path, const timespec &) {
		return 0;
	}

	int DsFATDriver::create(const char *path, mode_t mode, FileInfo &) {
		return 0;
	}

	int DsFATDriver::write(const char *path, const char *buffer, size_t size, off_t offset, FileInfo &) {
		return 0;
	}

	int DsFATDriver::mkdir(const char *path, mode_t mode) {
		return 0;
	}

	int DsFATDriver::truncate(const char *path, off_t size) {
		return 0;
	}

	int DsFATDriver::ftruncate(const char *path, off_t size, FileInfo &) {
		return 0;
	}

	int DsFATDriver::rmdir(const char *path) {
		return 0;
	}

	int DsFATDriver::unlink(const char *path) {
		return 0;
	}

	int DsFATDriver::open(const char *path, FileInfo &) {
		return 0;
	}

	int DsFATDriver::read(const char *path, char *buffer, size_t size, off_t offset, FileInfo &) {
		return 0;
	}

	int DsFATDriver::readdir(const char *path, void *buffer, DirFiller filler, off_t offset, FileInfo &) {
		return 0;
	}

	int DsFATDriver::getattr(const char *path, FileStats &) {
		return 0;
	}
}
