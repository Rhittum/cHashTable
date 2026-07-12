#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

// For Windows
#if defined (_WIN32) || defined (_WIN64)
#include <windows.h>
#include <wincrypt.h>
#endif

// For Linux/MacOS/BSD
#if defined (__linux__) || defined (__FreeBSD__) || defined (__APPLE__)
#include <sys/random.h>
#include <unistd.h>
#include <fcntl.h>
#endif

int get_secure_random_bytes(void *buf, size_t nbytes) {
#if defined (_WIN32) || defined (_WIN64)
	HCRYPTPROV hProvider = 0;
	if (!CryptAquireContext(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
		return -1;
	}
	if (!CryptGenRandom(hProvider, (DWORD)nbytes, (BYTE*)buf)) {
		CryptReleaseContext(hProvider, 0);
		return -1;
	}
	CryptReleaseContext(hProvider, 0);
	return 0;

#elif defined (__linux__)
	ssize_t result = getrandom(buf, nbytes, 0);
	if (result == (ssize_t)nbytes) {
		return 0;
	}

	int fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0) return -1;
	ssize_t read_bytes = read(fd, buf, nbytes);
	close(fd);
	return (read_bytes == (ssize_t)nbytes) ? 0 : 1;

#elif defined (__APPLE__) || defined (__FreeBSD__)
	arc4random_buf(buf, nbytes);
	return 0;

#else
	return -1;
#endif
}
