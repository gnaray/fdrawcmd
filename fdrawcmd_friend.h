#pragma once

// This file must be included after the dependent types are defined.
// For example after including Platform.h.

NTSTATUS WaitIndex(PEXTRA_DEVICE_EXTENSION edx, bool CalcSpinTime = false, int Revolutions = 1);
#ifndef TESTER_APP
NTSTATUS CommandReadId(PEXTRA_DEVICE_EXTENSION edx, UCHAR flags, UCHAR phead);
#else
NTSTATUS CommandReadId(const PEXTRA_DEVICE_EXTENSION edx, UCHAR flags, UCHAR phead, long& elapsed_abstime);
#endif
