// MultiScanOfflineTest.cpp : Defines the entry point for the console application.
//

#include "Platform.h"
#include "fdrawcmd.h"
#include "MultiScan.h"
#include "Sorters.h"

#include <assert.h>
#include <vector>

int KdCounter = 0;

constexpr long Rpm300SpinTime = 200000;

constexpr long NO_MORE_CHRN_OF_THIS_TRACK = -1;
constexpr long LOOP_CHRNS_OF_THIS_TRACK = -2;

typedef struct tagReadSectorChrn
{
	tagReadSectorChrn(const FD_CHRN& Chrn, long Time)
		: Chrn(Chrn), Time(Time), Status(STATUS_SUCCESS)
	{
	}

	tagReadSectorChrn(NTSTATUS Status, long Time)
		: Chrn(), Time(Time), Status(Status)
	{
	}

	tagReadSectorChrn(long Time)
		: tagReadSectorChrn(STATUS_FLOPPY_ID_MARK_NOT_FOUND, Time)
	{
	}

	tagReadSectorChrn()
		: tagReadSectorChrn(NO_MORE_CHRN_OF_THIS_TRACK)
	{
	}

	FD_CHRN Chrn;
	long Time;
	NTSTATUS Status;
}
ReadSectorChrn, *PReadSectorChrn;

typedef struct tagFloppyDisk
{
	int TestNumber;
	FD_MULTI_SCAN_PARAMS pp;
	EXTRA_DEVICE_EXTENSION edx;
	std::vector<ReadSectorChrn> ReadSectorChrns;

	int SectorIndex;
	int TrackRetry;
	int TrackRetryPrevious;
	int LoopCounter;
}
FloppyDisk, *PFloppyDisk;

NTSTATUS InitTestCase(FloppyDisk& floppy_disk)
{
	switch (floppy_disk.TestNumber)
	{
	case 1: // Blank disk, rpm=300.
	{
		floppy_disk.pp.track_retries = 1; // Thus enough adding 1 ReadSectorChrn.
		constexpr long AdjustedTracktime = Rpm300SpinTime * 9 / 10; // Much less than Rpm300SpinTime so indeed the returned status is tested.
		floppy_disk.ReadSectorChrns.push_back(ReadSectorChrn(STATUS_FLOPPY_ID_MARK_NOT_FOUND, AdjustedTracktime));
		floppy_disk.ReadSectorChrns.push_back(ReadSectorChrn());
		break;
	}
	case 2: // 3 sectors looped, rpm=300.
	{
		constexpr long Times[] = { 1600, 67000, 133000, LOOP_CHRNS_OF_THIS_TRACK };
		constexpr FD_CHRN Chrns[] = { FD_CHRN{ 13, 1, 5, 2 }, FD_CHRN{ 13, 1, 7, 2 }, FD_CHRN{ 13, 1, 9, 2 }, FD_CHRN() };
		constexpr auto TimesSup = sizeof(Times) / sizeof(Times[0]);
		constexpr auto ChrnsIndexSup = sizeof(Chrns) / sizeof(Chrns[0]);
		if (ChrnsIndexSup != TimesSup)
		{
			KdPrint(("Chrns and Times size must match.\n"));
			return STATUS_INVALID_PARAMETER;
		}
		for (int i = 0; i < TimesSup; i++)
			floppy_disk.ReadSectorChrns.push_back(ReadSectorChrn(Chrns[i], Times[i]));
		break;
	}
	case 3: // 3 sectors, sector 7 mergeable, sector 9 is duplicated, rpm=300, disk is faster just within speed tolerance.
	{
		floppy_disk.pp.track_retries = 1;
		floppy_disk.pp.flags = FD_OPTION_MFM;
		floppy_disk.pp.byte_tolerance_of_time = BYTE_TOLERANCE_OF_TIME;
		floppy_disk.edx.DataRate = FD_RATE_250K;

		const long CustomSpinTime = 199783;
		const auto TimeToleranceOfTime = GetFmOrMfmDataBytesTime(floppy_disk.edx.DataRate, floppy_disk.pp.flags, floppy_disk.pp.byte_tolerance_of_time);

		// The sector 7 at 67000 should be merged with next sector 7,
		// the sector 9 at 133000 should not be merged with next sector 9.
		const long Times[] = { 2000, 67000, CommonTimeMax(67000, TimeToleranceOfTime),
			133000, CommonTimeMax(133000, TimeToleranceOfTime) + 1, 2000 + CustomSpinTime };
		constexpr FD_CHRN Chrns[] = { FD_CHRN{ 13, 1, 5, 2 }, FD_CHRN{ 13, 1, 7, 2 }, FD_CHRN{ 13, 1, 7, 2 },
			FD_CHRN{ 13, 1, 9, 2 }, FD_CHRN{ 13, 1, 9, 2 }, FD_CHRN{ 13, 1, 5, 2 } };
		constexpr auto TimesSup = sizeof(Times) / sizeof(Times[0]);
		constexpr auto ChrnsIndexSup = sizeof(Chrns) / sizeof(Chrns[0]);
		if (ChrnsIndexSup != TimesSup)
		{
			KdPrint(("Chrns and Times size must match.\n"));
			return STATUS_INVALID_PARAMETER;
		}
		for (int i = 0; i < TimesSup; i++)
			floppy_disk.ReadSectorChrns.push_back(ReadSectorChrn(Chrns[i], Times[i]));
		break;
	}
	case 4: // Live data.
	{
		floppy_disk.pp.track_retries = 3;
		floppy_disk.pp.byte_tolerance_of_time = BYTE_TOLERANCE_OF_TIME;
		floppy_disk.edx.DataRate = FD_RATE_250K;

		constexpr long Times[] = {
			/*36979, 55046,*/ 109348, 163498, 181587, 309124, NO_MORE_CHRN_OF_THIS_TRACK,
			36992, 55051, 109370, 163515, 181608, 236771, NO_MORE_CHRN_OF_THIS_TRACK,
			36978, 109358, 127421, 145446, 163506, 181591, 236758, NO_MORE_CHRN_OF_THIS_TRACK,
		};
		constexpr FD_CHRN Chrns[] = {
			/*FD_CHRN{ 0, 0, 2, 2 }, FD_CHRN{ 0, 0, 8, 2 },*/ FD_CHRN{ 0, 0, 4, 2 }, FD_CHRN{ 0, 0, 11, 2 }, FD_CHRN{ 0, 0, 6, 2 }, FD_CHRN{ 0, 0, 4, 2 }, FD_CHRN(),
			FD_CHRN{ 0, 0, 2, 2 }, FD_CHRN{ 0, 0, 8, 2 }, FD_CHRN{ 0, 0, 4, 2 }, FD_CHRN{ 0, 0, 11, 2 }, FD_CHRN{ 0, 0, 6, 2 }, FD_CHRN{ 0, 0, 2, 2 }, FD_CHRN(),
			FD_CHRN{ 0, 0, 2, 2 }, FD_CHRN{ 0, 0, 4, 2 }, FD_CHRN{ 0, 0, 10, 2 }, FD_CHRN{ 0, 0, 5, 2 }, FD_CHRN{ 0, 0, 11, 2 }, FD_CHRN{ 0, 0, 6, 2 }, FD_CHRN{ 0, 0, 2, 2 }, FD_CHRN(),
		};
		constexpr auto TimesSup = sizeof(Times) / sizeof(Times[0]);
		constexpr auto ChrnsIndexSup = sizeof(Chrns) / sizeof(Chrns[0]);
		if (ChrnsIndexSup != TimesSup)
		{
			KdPrint(("Chrns and Times size must match.\n"));
			return STATUS_INVALID_PARAMETER;
		}
		for (int i = 0; i < TimesSup; i++)
			floppy_disk.ReadSectorChrns.push_back(ReadSectorChrn(Chrns[i], Times[i]));
		break;
	}
	case 5: // Live data, failed thus returning all different CHRNs by setting byte_tolerance_of_time = 0.
	{
		floppy_disk.pp.track_retries = 10;
		floppy_disk.pp.byte_tolerance_of_time = 0;
		//floppy_disk.edx.SpinTime = 199798;
		floppy_disk.edx.DataRate = FD_RATE_250K;

		constexpr FD_CHRN ChrnBase = { 0, 0, 2, 2 };
		constexpr FD_CHRN CHRNDummy = { 0, 0, 0, 0 };
		constexpr int Sectors[] = {
			2, 8, 9, 4, 10, 5, 11, 6, -1,
			9, 4, 10, 5, 11, 6, 2, -1,
			2, 4, 11, 6, 12, 2, -1,
			2, 4, 10, 5, 11, 6, 2, -1,
			2, 8, 9, 4, 11, 12, 2, -1,
			2, 4, 11, 12, 2, -1,
			2, 4, 10, 5, 11, 6, -1,
			2, 8, 9, 4, 11, 6, -1,
			2, 4, 10, 5, 11, 6, 2, -1,
			2, 8, 9, 4, 11, 6, 2, -1,
		};
		constexpr long Times[] = {
			36987, 55067, 91296, 109365, 127413, 145454, 163522, 181607, NO_MORE_CHRN_OF_THIS_TRACK,
			91328, 109381, 127428, 145468, 163545, 181622, 236796, NO_MORE_CHRN_OF_THIS_TRACK,
			37004, 109375, 163543, 181716, 199701, 236797, NO_MORE_CHRN_OF_THIS_TRACK,
			37010, 109374, 127428, 145480, 163542, 181626, 236798, NO_MORE_CHRN_OF_THIS_TRACK,
			37027, 55089, 91329, 109375, 163547, 199715, 236806, NO_MORE_CHRN_OF_THIS_TRACK,
			37001, 109365, 163536, 199703, 236792, NO_MORE_CHRN_OF_THIS_TRACK,
			37005, 109370, 127424, 145468, 163541, 181627, NO_MORE_CHRN_OF_THIS_TRACK,
			37013, 55093, 91327, 109384, 163553, 181634, NO_MORE_CHRN_OF_THIS_TRACK,
			37008, 109382, 127444, 145474, 163554, 181630, 236801, NO_MORE_CHRN_OF_THIS_TRACK,
			37018, 55094, 91327, 109387, 163548, 181634, 236808, NO_MORE_CHRN_OF_THIS_TRACK,
		};
		constexpr auto TimesSup = sizeof(Times) / sizeof(Times[0]);
		constexpr auto SectorsSup = sizeof(Sectors) / sizeof(Sectors[0]);
		if (SectorsSup != TimesSup)
		{
			KdPrint(("Secctors and Times size must match.\n"));
			return HTTP_E_STATUS_UNEXPECTED_CLIENT_ERROR;
		}
		for (int i = 0; i < TimesSup; i++)
		{
			if (Times[i] < 0)
				floppy_disk.ReadSectorChrns.push_back(ReadSectorChrn(Times[i]));
			else
			{
				auto chrn = ChrnBase;
				chrn.sector = Sectors[i];
				floppy_disk.ReadSectorChrns.push_back(ReadSectorChrn(chrn, Times[i]));
			}
		};
		break;
	}
	case 6: // Live data, failed at FoundFirstHeaders thus returning all different CHRNs by setting byte_tolerance_of_time = 0.
	{
		floppy_disk.pp.track_retries = 2;
		floppy_disk.pp.flags = FD_OPTION_MFM;
		floppy_disk.pp.byte_tolerance_of_time = 0;
		//floppy_disk.edx.SpinTime = 199783;
		floppy_disk.edx.DataRate = FD_RATE_250K;

		constexpr FD_CHRN ChrnBase = { 0, 0, 63, 2 };
		constexpr FD_CHRN CHRNDummy = { 0, 0, 0, 0 };
		constexpr int Sectors[] = {
			2, 4, 10, 5, 11, 6, 12, -1,
			2, 9, 4, 11, 6, 2, -1,
		};
		constexpr long Times[] = {
			36968, 109339, 127397, 145438, 163503, 181592, 199658, NO_MORE_CHRN_OF_THIS_TRACK,
			36986, 91316, 109355, 163532, 181622, 236765, NO_MORE_CHRN_OF_THIS_TRACK,
		};

		constexpr auto TimesSup = sizeof(Times) / sizeof(Times[0]);
		constexpr auto SectorsSup = sizeof(Sectors) / sizeof(Sectors[0]);
		if (SectorsSup != TimesSup)
		{
			KdPrint(("Secctors and Times size must match.\n"));
			return HTTP_E_STATUS_UNEXPECTED_CLIENT_ERROR;
		}
		for (int i = 0; i < TimesSup; i++)
		{
			if (Times[i] < 0)
				floppy_disk.ReadSectorChrns.push_back(ReadSectorChrn(Times[i]));
			else
			{
				auto chrn = ChrnBase;
				chrn.sector = Sectors[i];
				floppy_disk.ReadSectorChrns.push_back(ReadSectorChrn(chrn, Times[i]));
			}
		};
		break;
	}
	default:
		throw std::runtime_error("Unknown TestNumber");
	}
	return STATUS_SUCCESS;
}

FloppyDisk floppy_disk;

NTSTATUS WaitIndex(const PEXTRA_DEVICE_EXTENSION edx, bool CalcSpinTime /*= false*/, int Revolutions /*= 1*/)
{
	floppy_disk.TrackRetry++;
	if (CalcSpinTime && edx->SpinTime == 0)
		edx->SpinTime = Rpm300SpinTime;
	floppy_disk.LoopCounter = 0;
	return STATUS_SUCCESS;
}

// The flags and phead are members of pp. The pp is passed only to create test cases easier.
NTSTATUS CommandReadId(const PEXTRA_DEVICE_EXTENSION edx, UCHAR flags, UCHAR phead, long& elapsed_abstime)
{
	int ReadSectorChrnsSize = static_cast<int>(floppy_disk.ReadSectorChrns.size());
	if (floppy_disk.TrackRetry > floppy_disk.TrackRetryPrevious)
	{
		floppy_disk.TrackRetryPrevious = floppy_disk.TrackRetry;
		for (; floppy_disk.SectorIndex < ReadSectorChrnsSize
			&& floppy_disk.ReadSectorChrns[floppy_disk.SectorIndex].Time >= 0; floppy_disk.SectorIndex++)
			;
		if (floppy_disk.SectorIndex < ReadSectorChrnsSize)
			floppy_disk.SectorIndex++;
		KdPrint(("Next track_retries #%d\n", floppy_disk.TrackRetryPrevious));
	}
	if (floppy_disk.SectorIndex < ReadSectorChrnsSize
		&& floppy_disk.ReadSectorChrns[floppy_disk.SectorIndex].Time == LOOP_CHRNS_OF_THIS_TRACK)
	{
		for (floppy_disk.SectorIndex--; floppy_disk.SectorIndex >= 0
			&& floppy_disk.ReadSectorChrns[floppy_disk.SectorIndex].Time >= 0; floppy_disk.SectorIndex--)
			;
		floppy_disk.SectorIndex++;
		floppy_disk.LoopCounter++;
		// If LOOP found itself thus nothing to loop then consider this case as end of track in next if.
	}
	if (floppy_disk.SectorIndex >= ReadSectorChrnsSize
		|| floppy_disk.ReadSectorChrns[floppy_disk.SectorIndex].Time == NO_MORE_CHRN_OF_THIS_TRACK
		|| floppy_disk.ReadSectorChrns[floppy_disk.SectorIndex].Time == LOOP_CHRNS_OF_THIS_TRACK)
	{
		elapsed_abstime = edx->SpinTime * 2;
		return STATUS_FLOPPY_ID_MARK_NOT_FOUND;
	}
	elapsed_abstime = floppy_disk.ReadSectorChrns[floppy_disk.SectorIndex].Time + floppy_disk.LoopCounter * edx->SpinTime;
	const auto& Status = floppy_disk.ReadSectorChrns[floppy_disk.SectorIndex].Status;
	const auto& Chrn = floppy_disk.ReadSectorChrns[floppy_disk.SectorIndex++].Chrn;
	if (Chrn.IsEmpty())
	{
		return Status;
	}
	edx->FifoOut[3] = Chrn.cyl;
	edx->FifoOut[4] = Chrn.head;
	edx->FifoOut[5] = Chrn.sector;
	edx->FifoOut[6] = Chrn.size;
	return STATUS_SUCCESS;
}

int main()
{
	floppy_disk.pp.flags = FD_OPTION_MFM;
	floppy_disk.pp.head = 0;
	floppy_disk.pp.track_retries = 1;
	floppy_disk.pp.byte_tolerance_of_time = BYTE_TOLERANCE_OF_TIME;

	floppy_disk.edx.SpinTime = 0;
	floppy_disk.edx.DataRate = FD_RATE_250K;
	floppy_disk.TestNumber = 6;
	if (InitTestCase(floppy_disk) != STATUS_SUCCESS)
	{
		KdPrint(("InitTestCase failed for TestNumber %d\n", floppy_disk.TestNumber));
		return 1;
	}

	UCHAR IoBuffer[0x8000];
	floppy_disk.edx.IoBuffer = IoBuffer;
	floppy_disk.edx.IoBufferSize = sizeof(IoBuffer) / sizeof(IoBuffer[0]);

	UCHAR WorkMemory[0x8000];
	floppy_disk.edx.WorkMemory = WorkMemory;
	floppy_disk.edx.WorkMemorySize = sizeof(WorkMemory) / sizeof(WorkMemory[0]);

	floppy_disk.SectorIndex = 0;
	floppy_disk.TrackRetry = -1;
	floppy_disk.TrackRetryPrevious = floppy_disk.TrackRetry + 1;
	floppy_disk.LoopCounter = 0;

	UCHAR OutBuffer[0x8000];
	ULONG OutSize = sizeof(OutBuffer) / sizeof(OutBuffer[0]);
	const int OutHeaderIndexSup = (OutSize - sizeof(FD_TIMED_MULTI_SCAN_RESULT)) / sizeof(FD_TIMED_MULTI_ID_HEADER);
	KdPrint(("1: OutHeaderIndexSup = %d\n", OutHeaderIndexSup));

	// TimedMultiScanTrack requires:
	// edx.DataRate, edx.WorkMemory, edx.WorkMemorySize, edx.SpinTime, edx.IoBuffer, edx.FifoOUt[3..6]
	// FD_MULTI_SCAN_PARAMS
	NTSTATUS status = TimedMultiScanTrack(&floppy_disk.edx, &floppy_disk.pp, OutHeaderIndexSup);
	const auto pOut = (PFD_TIMED_MULTI_SCAN_RESULT)floppy_disk.edx.IoBuffer;
	const auto HeaderCount = pOut->count;
	const auto Headers = pOut->HeaderArray();
	for (int i = 0; i < HeaderCount; i++)
		KdPrint(("%d. Header: sector=%d, reltime=%ld, rev=%d\n", i, Headers[i].sector, Headers[i].reltime, Headers[i].revolution));
	OutBuffer[0] = 0;
	return 0;
}
