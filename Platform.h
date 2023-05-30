#pragma once

#ifndef TESTER_APP
#include "stddcls.h"
#include "driver.h"
#else
#ifdef _WIN32
#include <windows.h>
#include <winioctl.h> // Requires preincluded windows.h.
#else
typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef unsigned int	DWORD; // Originally long which is 32 bits except in Unix-like systems where 64 bits.
typedef long long		LONGLONG;
#define STATUS_TIMEOUT					((NTSTATUS)0x00000102L) // Comes from windows.h.
// Probably need more typedefs and defines on Unix.
#endif
#include <stdio.h>
#define KdPrint(_x_) printf _x_ // Using normal printf instead of KdPrint in user space.
// Shorted version of _EXTRA_DEVICE_EXTENSION taken from wdm.h.
typedef struct _EXTRA_DEVICE_EXTENSION
{
	PUCHAR IoBuffer;
	ULONG IoBufferSize;
	PUCHAR WorkMemory;
	ULONG WorkMemorySize;

	UCHAR FifoOut[16];

	ULONG SpinTime;

	UCHAR DataRate;
} EXTRA_DEVICE_EXTENSION, *PEXTRA_DEVICE_EXTENSION;
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

#define STATUS_SUCCESS					((NTSTATUS)0x00000000L)    // ntsubauth
#define STATUS_FLOPPY_ID_MARK_NOT_FOUND	((NTSTATUS)0xC0000165L)
#define STATUS_CRC_ERROR				((NTSTATUS)0xC000003FL)
#define STATUS_NONEXISTENT_SECTOR		((NTSTATUS)0xC0000015L)
#define STATUS_BUFFER_TOO_SMALL			((NTSTATUS)0xC0000023L)
#define STATUS_INVALID_PARAMETER		((DWORD   )0xC000000DL)
#endif
