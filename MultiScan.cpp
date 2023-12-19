#include "MultiScan.h"

/*
* The FoundAllHeaders array must be sorted by abstime and CHRN incremented.
* Returns the merged time .
* The FoundAllHeadersIndexLastMerged argument is output variable.
*/
long MergeQuasiSameWorkHeadersAt(_Inout_ FD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN FoundAllHeaders[], _In_ const int FoundAllHeadersSize,
	_In_ int FoundAllHeadersIndex, _Out_ int& FoundAllHeadersIndexLastMerged, _In_ const int TimeToleranceOfTime, _In_ const MERGE_FLAG_ENUM FlagMergedAs)
{
//	KdPrint(("MergeQuasiSameWorkHeadersAt(): calculating time for #%d\n", FoundAllHeadersIndex));
	if (FlagMergedAs != MERGE_FLAG_NOTHING)
		FoundAllHeaders[FoundAllHeadersIndex].merge_flag = (MERGE_FLAG)FlagMergedAs;
	const auto& FoundAllHeadersIndexCHRN = FoundAllHeaders[FoundAllHeadersIndex].chrn;
	auto LowestTime = FoundAllHeaders[FoundAllHeadersIndex].abstime;
	auto AbstimeSum = LowestTime;
	auto AbstimeSumContributor = 1;
	const auto LowestTimeCommonTimeMax = CommonTimeMax(LowestTime, TimeToleranceOfTime);
	const auto TimeMax = TolerancedTimeMax(LowestTimeCommonTimeMax, TimeToleranceOfTime);
	FoundAllHeadersIndexLastMerged = FoundAllHeadersIndex++;
	for (; FoundAllHeadersIndex < FoundAllHeadersSize
		&& FoundAllHeaders[FoundAllHeadersIndex].abstime <= TimeMax; FoundAllHeadersIndex++)
	{
		if (FoundAllHeaders[FoundAllHeadersIndex].chrn != FoundAllHeadersIndexCHRN || FoundAllHeaders[FoundAllHeadersIndex].merge_flag != (MERGE_FLAG)MERGE_FLAG_NOTHING)
			continue;
		if (FlagMergedAs != MERGE_FLAG_NOTHING)
			FoundAllHeaders[FoundAllHeadersIndex].merge_flag = (MERGE_FLAG)FlagMergedAs;
		AbstimeSum += FoundAllHeaders[FoundAllHeadersIndex].abstime;
		AbstimeSumContributor++;
		FoundAllHeadersIndexLastMerged = FoundAllHeadersIndex;
	}
	if (AbstimeSumContributor > 1)
	{
		const auto HighestTimeCommonTimeMin = CommonTimeMin(FoundAllHeaders[FoundAllHeadersIndexLastMerged].abstime, TimeToleranceOfTime);
		LowestTime = AdjustValueIntoRange(AbstimeSum / AbstimeSumContributor, HighestTimeCommonTimeMin, LowestTimeCommonTimeMax);
	}
//	KdPrint(("MergeQuasiSameWorkHeadersAt(): calculated time is %ld with last #%d\n", LowestTime, FoundAllHeadersIndexLastMerged));
	return LowestTime;
}

inline void UpdateWorkHeadersMergeFlag(_Inout_ FD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN FoundAllHeaders[],
	_In_ const int FoundAllHeadersIndexStart, _In_ const int FoundAllHeadersIndexEnd, _In_ const MERGE_FLAG_ENUM MergeFlag)
{
	for (auto FoundAllHeadersIndex = FoundAllHeadersIndexStart; FoundAllHeadersIndex <= FoundAllHeadersIndexEnd; FoundAllHeadersIndex++)
	{
		if (FoundAllHeaders[FoundAllHeadersIndex].merge_flag == (MERGE_FLAG)MERGE_FLAG_MERGING)
			FoundAllHeaders[FoundAllHeadersIndex].merge_flag = (MERGE_FLAG)MergeFlag;
	}
}

/*
* The FoundAllHeaders array must be sorted by abstime and CHRN incremented.
* The Tracktime must be > 0.
* Returns the determined tracktime (> 0 if found or 0 if not found) in DetermineTracktime mode or the merged time in not DetermineTracktime mode.
* The FoundAllHeadersIndexSearch argument is output variable.
*/
long MergeWorkHeaderAndProcessItsUpRevs(_Inout_ FD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN FoundAllHeaders[],
	_In_ const int FoundAllHeadersSize, _In_ const int FoundAllHeadersIndex,
	_In_ const long Tracktime, _In_ const int TimeToleranceOfTime,
	_Out_ int& FoundAllHeadersIndexSearch, _In_ bool DetermineTracktime)
{
//	KdPrint(("MergeWorkHeaderAndProcessItsUpRevs(): merging #%d\n", FoundAllHeadersIndex));
	const auto& FoundAllHeadersIndexCHRN = FoundAllHeaders[FoundAllHeadersIndex].chrn;
	int FoundAllHeadersIndexLastMerged;
	const auto TimeCommon = MergeQuasiSameWorkHeadersAt(FoundAllHeaders, FoundAllHeadersSize, FoundAllHeadersIndex,
		FoundAllHeadersIndexLastMerged, TimeToleranceOfTime, DetermineTracktime ? MERGE_FLAG_NOTHING : MERGE_FLAG_MERGED);
//	KdPrint(("MergeWorkHeaderAndProcessItsUpRevs(): merged #%d\n", FoundAllHeadersIndex));
	const auto TimeCommonLowestTime = FoundAllHeaders[FoundAllHeadersIndex].abstime;
	const auto TimeCommonHighestTime = FoundAllHeaders[FoundAllHeadersIndexLastMerged].abstime;
	const auto TimeCommonLowerableBy = TimeCommonHighestTime - TolerancedTimeMax(TimeCommon, TimeToleranceOfTime); // <= 0
	const auto TimeCommonRaisableBy = TimeCommonLowestTime - TolerancedTimeMin(TimeCommon, TimeToleranceOfTime); // >= 0
	const auto TimeCommonLowerableMin = TimeCommon + TimeCommonLowerableBy;
	const auto TimeCommonRaisableMax = TimeCommon + TimeCommonRaisableBy;
	auto TimeTolerancedRev1TimeMin = TolerancedTimeMin(TimeCommonLowerableMin, TimeToleranceOfTime);
	auto TimeTolerancedRev1TimeMax = TolerancedTimeMax(TimeCommonRaisableMax, TimeToleranceOfTime);
	auto FoundAllHeadersIndexSearchUpRev = FoundAllHeadersIndexSearch;
	for (bool FirstCycle = true; ; ) // Find same CHRNs in up revs.
	{
		TimeTolerancedRev1TimeMin += Tracktime;
		const auto Rev1TimeMin = TolerancedTimeMin(CommonTimeMin(TimeTolerancedRev1TimeMin, TimeToleranceOfTime), TimeToleranceOfTime);
		for (; FoundAllHeadersIndexSearchUpRev < FoundAllHeadersSize
			&& FoundAllHeaders[FoundAllHeadersIndexSearchUpRev].abstime < Rev1TimeMin; FoundAllHeadersIndexSearchUpRev++)
			;
		if (FirstCycle) // Used for making searching faster. Storing only the search index of next rev (not other revs) since most Headers are found in this rev.
		{
			FirstCycle = false;
			FoundAllHeadersIndexSearch = FoundAllHeadersIndexSearchUpRev;
		}
		if (FoundAllHeadersIndexSearchUpRev >= FoundAllHeadersSize)
			break;
		auto FoundAllHeadersIndexUpRev = FoundAllHeadersIndexSearchUpRev;
		TimeTolerancedRev1TimeMax += Tracktime;
		const auto Rev1TimeMax = TimeTolerancedRev1TimeMax;
		for (; FoundAllHeadersIndexUpRev < FoundAllHeadersSize
			&& FoundAllHeaders[FoundAllHeadersIndexUpRev].abstime <= Rev1TimeMax; FoundAllHeadersIndexUpRev++)
		{
			if (FoundAllHeaders[FoundAllHeadersIndexUpRev].chrn != FoundAllHeadersIndexCHRN || FoundAllHeaders[FoundAllHeadersIndexUpRev].merge_flag != (MERGE_FLAG)MERGE_FLAG_NOTHING)
				continue;
//			KdPrint(("MergeWorkHeaderAndProcessItsUpRevs(): merging #%d\n", FoundAllHeadersIndexUpRev));
			const auto Rev1TimeCommon = MergeQuasiSameWorkHeadersAt(FoundAllHeaders, FoundAllHeadersSize, FoundAllHeadersIndexUpRev,
				FoundAllHeadersIndexLastMerged, TimeToleranceOfTime, DetermineTracktime ? MERGE_FLAG_NOTHING : MERGE_FLAG_MERGING);
//			KdPrint(("MergeWorkHeaderAndProcessItsUpRevs(): merged uprev #%d\n", FoundAllHeadersIndexUpRev));
			const auto Rev1TimeLowestTime = FoundAllHeaders[FoundAllHeadersIndexUpRev].abstime;
			const auto Rev1TimeHighestTime = FoundAllHeaders[FoundAllHeadersIndexLastMerged].abstime;
			// Guaranteed that Rev1TimeMin <= Rev1TimeLowestTime <= Rev1TimeCommon <= Rev1TimeHighestTime <= Rev1TimeMax.
			const auto Rev1TimeCommonLowerableBy = Rev1TimeHighestTime - TolerancedTimeMax(Rev1TimeCommon, TimeToleranceOfTime); // <= 0
			const auto Rev1TimeCommonRaisableBy = Rev1TimeLowestTime - TolerancedTimeMin(Rev1TimeCommon, TimeToleranceOfTime); // >= 0
			const auto Rev1TimeCommonLowerableMin = Rev1TimeCommon + Rev1TimeCommonLowerableBy;
			const auto Rev1TimeCommonRaisableMax = Rev1TimeCommon + Rev1TimeCommonRaisableBy;
			// Rev1 time common falls into rev1 toleranced interval?
			if (IsIntersecting(TimeTolerancedRev1TimeMin, TimeTolerancedRev1TimeMax, Rev1TimeCommonLowerableMin, Rev1TimeCommonRaisableMax))
			{
				if (DetermineTracktime)
					return Rev1TimeCommon - TimeCommon;
				UpdateWorkHeadersMergeFlag(FoundAllHeaders, FoundAllHeadersIndexUpRev, FoundAllHeadersIndexLastMerged, MERGE_FLAG_MERGED);
			}
			else
				UpdateWorkHeadersMergeFlag(FoundAllHeaders, FoundAllHeadersIndexUpRev, FoundAllHeadersIndexLastMerged, MERGE_FLAG_NOTHING);
		}
		if (FoundAllHeadersIndexUpRev >= FoundAllHeadersSize || DetermineTracktime) // Pairing only headers of base rev and next to base rev in DetermineTracktime mode.
			break;
	}
	return DetermineTracktime ? 0 : TimeCommon;
}

/*
* The FoundAllHeaders and FoundFirstHeaders array must be sorted by abstime and CHRN incremented.
* The Tracktime must be > 0.
* Returns the adjusted (determined) tracktime if succeeded else 0.
*/
long DetermineTracktimeByMergingFirstHeaders(_In_ FD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN FoundAllHeaders[],
	_In_ const int FoundAllHeadersSize,
	_In_ const FD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN FoundFirstHeaders[], _In_ const int FoundFirstHeadersCount,
	_In_ const long Tracktime, _In_ const int TimeToleranceOfTime)
{
	KdPrint(("DetermineTracktimeByMergingFirstHeaders(): starting, Tracktime %ld\n", Tracktime));
	auto FoundAllHeadersIndex = 0;
	auto FoundAllHeadersIndexSearch = 1;
	for (auto FoundFirstHeadersIndex = 0; FoundFirstHeadersIndex < FoundFirstHeadersCount; FoundFirstHeadersIndex++)
	{
		const auto& FoundFirstHeadersCHRN = FoundFirstHeaders[FoundFirstHeadersIndex].chrn;
//		KdPrint(("DetermineTracktimeByMergingFirstHeaders(): #%d. first sector %hhu with time %ld\n",
//			FoundFirstHeadersIndex, FoundFirstHeadersCHRN.sector, FoundFirstHeaders[FoundFirstHeadersIndex].abstime));
		for ( ; FoundAllHeadersIndex < FoundAllHeadersSize; FoundAllHeadersIndex++)
		{
			if (FoundAllHeaders[FoundAllHeadersIndex].chrn != FoundFirstHeadersCHRN)
			{
//				KdPrint(("DetermineTracktimeByMergingFirstHeaders(): #%d. sector %hhu with time %ld skipped\n",
//					FoundAllHeadersIndex, FoundAllHeaders[FoundAllHeadersIndex].chrn.sector, FoundAllHeaders[FoundAllHeadersIndex].abstime));
				continue;
			}
			const auto AdjustedTracktime = MergeWorkHeaderAndProcessItsUpRevs(FoundAllHeaders, FoundAllHeadersSize,
				FoundAllHeadersIndex, Tracktime, TimeToleranceOfTime, FoundAllHeadersIndexSearch, true);
//			KdPrint(("DetermineTracktimeByMergingFirstHeaders(): #%d. sector %hhu with time %ld produced tracktime %ld\n",
//				FoundAllHeadersIndex, FoundFirstHeadersCHRN.sector, FoundAllHeaders[FoundAllHeadersIndex].abstime, AdjustedTracktime));
			if (AdjustedTracktime > 0)
			{
				KdPrint(("DetermineTracktimeByMergingFirstHeaders(): finished, AdjustedTracktime %ld\n", AdjustedTracktime));
				return AdjustedTracktime;
			}
			FoundAllHeadersIndex++; // Together with break.
			break; // Only pair of first Headers of given CHRN is considered. If there is no pair, try next CHRN.
		}
	}
	KdPrint(("DetermineTracktimeByMergingFirstHeaders(): finished\n"));
	return 0;
}

/*
* The FoundAllHeaders array must be sorted by abstime and CHRN incremented.
* The Tracktime must be > 0.
* Returns the count of out headers of merged found all Headers.
*/
int MergeWorkHeadersWithUpRevsIntoOutHeaders(_In_ FD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN FoundAllHeaders[],
	_In_ const int FoundAllHeadersSize,
	_Out_ FD_TIMED_MULTI_ID_HEADER_EXT OutHeaders[], _In_ const int OutHeadersSup,
	_In_ const long Tracktime, _In_ const int TimeToleranceOfTime)
{
	auto OutHeadersNewCount = 0;
	auto FoundAllHeadersIndexSearch = 1;
	KdPrint(("MergeWorkHeadersWithUpRevsIntoOutHeaders(): starting, FoundAllHeadersSup=%d, OutHeadersSup=%d\n", FoundAllHeadersSize, OutHeadersSup));
	for (auto FoundAllHeadersIndex = 0; FoundAllHeadersIndex < FoundAllHeadersSize; FoundAllHeadersIndex++)
	{
//		KdPrint(("MergeWorkHeadersWithUpRevsIntoOutHeaders(): checking #%d\n", FoundAllHeadersIndex));
		if (FoundAllHeaders[FoundAllHeadersIndex].merge_flag != (MERGE_FLAG)MERGE_FLAG_NOTHING)
			continue;
		if (OutHeadersNewCount >= OutHeadersSup)
			return -1;
//		KdPrint(("MergeWorkHeadersWithUpRevsIntoOutHeaders(): merging #%d\n", FoundAllHeadersIndex));
		const auto MergedTime = MergeWorkHeaderAndProcessItsUpRevs(FoundAllHeaders, FoundAllHeadersSize,
			FoundAllHeadersIndex, Tracktime, TimeToleranceOfTime, FoundAllHeadersIndexSearch, false);
//		KdPrint(("MergeWorkHeadersWithUpRevsIntoOutHeaders(): merged #%d, sector=%hhu, time=%ld\n", FoundAllHeadersIndex, FoundAllHeaders[FoundAllHeadersIndex].chrn.sector, MergedTime));
		OutHeaders[OutHeadersNewCount].AssignCHRN(FoundAllHeaders[FoundAllHeadersIndex].chrn);
		OutHeaders[OutHeadersNewCount++].AssignTime(MergedTime, Tracktime);
	}
	KdPrint(("MergeWorkHeadersWithUpRevsIntoOutHeaders(): finished, FoundAllHeadersSup=%d, OutHeadersNewCount=%d\n", FoundAllHeadersSize,OutHeadersNewCount));
	return OutHeadersNewCount;
}



/*
* Def.: the [spintime] is the time elapsed while the disk rotates once.
* It does not depend on the tracks i.e. it assumes constant disk rotation
* speed from first to last track.
* Def.: the [tracktime] is the time elapsed while the track rotates once.
* The tracktime is the track dependent version of spintime. Usually the
* tracktime is the same for each track but there are floppy drives which
* change the rotation speed of disk per track equalling the datarate (Kb/mm)
* on inner and outer tracks as well.
* Def.: [CHRN] is the full identifier of a sector and only that. It does not
* mean the data of that sector at all.
* Def.: the [revolution] is 1 revolution (i.e. rotation) of track.
*
* This method is similar to TimedScanTrack but works quite differently.
* This method is intended to scan a track more times to find weak or close to
* index hole sectors better.
* The goal is to explore all possible CHRNs even if more time and processing
* are necessary. The scanning of a track is administrated as fast as
* possible in order to detect close CHRNs. Each scanning starts at
* index hole and lasts for at least spintime or the start of first detected
* CHRN plus the spintime +-tolerance. This creates the opportunity to
* find CHRNs close to index hole even when track_retries = 1.
*
* Def.: the [abstime] of a CHRN is the time duration between the start of
* measuring the time and the event of finding that CHRN.
* Def.: the [reltime] is the abstime modulo tracktime.
* Def.: Two CHRNs are said to be [quasi same] if the reltimes are
* within the time tolerance of tracktime and the CHRNs are the same.
* Def.: The [work header] is a temporary header containing information about
* a detected sector during the scanning process.
* Def.: The [late-work-headers] is the array of work headers which is the
* shorter name of found late headers array. Late header means that the header
* is found after at least 1 revolution. It is also referred as first header
* because it is first in a revolution, however not in the first revolution.
* The first revolution can be incomplete because sectors close to the index
* hole might not appear in it.
* Def.: The [all-work-headers] is the array of work headers which is the
* shorter name of found all headers array.
*
* If the track_retries specifies (-n) auto mode then scanning of track is
* retried more (at least n) times while the headers array containing the CHRNs
* has been changed in the last n track_retries i.e. scanning stops after n
* track_retries without finding a new CHRN (a CHRN which is not quasi same as
* any already found CHRNs).
* If the track_retries specifies (n) normal mode then n scanning of track is
* performed.
* Note that the highest the track_retries the more time it takes so it is more
* waiting also when scanning a blank track.
*
* The algorithm consist of the following steps:
* 1) Collecting CHRNs from floppy track into all-work-headers as fast as
* possible. The scanning is performed track_retries times
* (at most 255) or until the work buffer is full. The scanning is also limited
* by the time calculated from spintime and time of first found CHRN plus the
* spintime +-tolerance. Each scanning of track starts with WaitIndex thus
* the time variance remains small enough. In case of auto track_retries new
* CHRNs must be detected so an array of late-work-headers of each different CHRNs
* is maintained and the new CHRNs are searched in this array.
* 2) Sorting the collected late-work-headers by abstime and CHRNs.
* 3) Sorting the collected all-work-headers by abstime and CHRNs.
* 4) Calculating tracktime by the first pair of all-work-headers in CurrentTrackTime distance.
* 5) Merge all-work-headers into out headers and also convert abstime to reltime and revolution.
* 6) Sorting the out headers by default (i.e. reltime, rev, CHRN).
*
* The returned tracktime is the edx->SpinTime. The edx->SpinTime is the
* measured time of revolution (measured either before scanning a track if it
* was not measured earlier or by calling IOCTL_FD_GET_TRACK_TIME earlier).
* This edx->SpinTime might be adjusted by the algorithm providing a more
* precise spintime.
*
* The output header contains count amount of headers. A header contain a
* CHRN along with the reltime and revolution (the number of revolutions).
* Thus the caller can calculate the absolute time when the header was detected
* by tracktime * revolution + reltime. However if everything goes well then
* the revolution is always 0.
*
* About why firstseen value is not returned by this method. The problem is
* that a CHRN is not visible either because it is close to the index hole
* and is not always visible on the 1st pass or because it is weak CHRN and
* is not always visible at all.
* In former case the CHRN appears in the 2nd revolution and could be handled
* well, in latter case the CHRN appears randomly and it is totally another
* case.
* Consequently the firstseen value would have sense only when the track is
* strong and as such the first CHRN seen close to index hole is trusted.
* However it is difficult to detect whether the track is weak or strong.
*
* Input:
* - edx (PEXTRA_DEVICE_EXTENSION)
*   - DataRate
*   - FifoOut
*   - IoBuffer
*   - SpinTime
*   - WorkBuffer
*   - WorkBufferSize
* - pp (PFD_MULTI_SCAN_PARAMS)
*   - flags: MFM (0x40) or FM (0x0).
*   - head: 0 or 1.
*   - track_retries: -n (auto mode), n (normal mode), where n > 0. Note the
*     WorkBuffer limit of CHRNs below. If CHRNs overflows the method finishes
*     scanning.
*     -n: scan track more (at least n) times (auto mode).
*     n: scan n times (normal mode).
*   - byte_tolerance_of_time: 5 (very tight), 10 (tight), 20 (normal (SAMdisk)).
* - OutHeaderIndexSup: the maximum possible size of out headers array.
* The WorkBuffer where the result is stored temporary is 0x8000 sized (big
* enough but not too big) (see WORK_MEMORY_SIZE). It is split into two header
* arrays: found first headers, found all headers. The former array takes
* 255 * 9 = 2295 bytes thus latter array has 32768 - 2295 = 30473 bytes.
* Currently k headers of CHRNs requires 8 + 9 * k bytes so theoretically
* (30473 - 8) / 9 = 3385 headers can be read at most into the work header
* array which should me more than enough.
* E.g. a track of ED disk can have approximately 144 amount of 128 bytes
* sectors which fills up the WorkBuffer after 3385 / 144 = 23 scannings of
* track if no sectors are quasi same. Even in this extreme case 23 scannings
* is good enough.
* A more common example is a track of HD disk having 18 amount of 512 bytes
* sectors. With appropriate merging it never fills up the WorkBuffer because
* it takes 18 * 9 = 162 bytes only.
*
* Output: po (PFD_TIMED_MULTI_SCAN_RESULT)
* - tracktime
* - track_retries
* - byte_tolerance_of_time
* - count
* - HeaderArray (FD_TIMED_MULTI_ID_HEADER):
*   - reltime
*   - revolution
*   - cyl
*   - head
*   - sector
*   - size
*/
NTSTATUS TimedMultiScanTrack(_Inout_ const PEXTRA_DEVICE_EXTENSION edx, _In_ const PFD_MULTI_SCAN_PARAMS pp, _In_ const int OutHeaderIndexSup)
{
	if (pp->track_retries == 0)
		return STATUS_INVALID_PARAMETER;
	const bool IsAutoTrackRetries = pp->track_retries < 0;
	int TrackRetriesRequested = IsAutoTrackRetries ? -pp->track_retries : pp->track_retries;
	// The algorithm here calculates with +-half tolerance so dividing specified byte_tolerance_of_time by 2.
	const int ByteToleranceOfTime = (pp->byte_tolerance_of_time < 0 ? BYTE_TOLERANCE_OF_TIME : pp->byte_tolerance_of_time) / 2;
	const auto TimeToleranceOfTime = GetFmOrMfmDataBytesTime(edx->DataRate, pp->flags, ByteToleranceOfTime);

	// IoBuffer will contain result: multi scan result and out headers.
	// Note that IoBuffer is overwritten by WaitIndex method (by calling
	// CommandReadWrite method) so it can store only final result but not
	// temporary data. Therefore WorkBuffer (WorkMemory) is required.

	constexpr auto FoundFirstHeadersSup = MAX_SECTORS_PER_TRACK;
	constexpr auto SizeOfFoundFirstHeaders = FoundFirstHeadersSup * sizeof(FD_TIMED_MULTI_ID_HEADER_WORK);
	const int FoundAllHeadersIndexSup = (edx->WorkMemorySize - SizeOfFoundFirstHeaders) / sizeof(FD_TIMED_MULTI_ID_HEADER_WORK);
	// WorkMemory contains found first headers and work (found all) headers.
	const auto FoundFirstHeaders = (PFD_TIMED_MULTI_ID_HEADER_WORK)edx->WorkMemory;
	auto FoundFirstHeadersCount = 0;
	const auto FoundAllHeaders = &FoundFirstHeaders[FoundFirstHeadersSup];
	auto FoundAllHeadersCount = 0;
	long CurrentTracktime = 0;
	auto TrackRetries = 0;
	NTSTATUS status = STATUS_SUCCESS;

	KdPrint(("#%d: TimedMultiScanTrack(): start method, OutHeaderIndexSup=%d\n", KdCounter++, OutHeaderIndexSup));
	// 1) Reading IDAMs and storing them as all-work-headers, plus storing the late-work-headers.
	while (NT_SUCCESS(status)) // Read track (next track_retries).
	{
		TrackRetries++;
		status = WaitIndex(edx, edx->SpinTime == 0);
#ifndef TESTER_APP
		LARGE_INTEGER iStart = KeQueryPerformanceCounter(NULL);
#endif
		if (CurrentTracktime == 0)
			CurrentTracktime = edx->SpinTime;
		KdPrint(("#%d: TimedMultiScanTrack(): called WaitIndex, status=%08lX, Tracktime=%ld\n", KdCounter++, status, CurrentTracktime));
		auto ulDiff1RevTimeMin = CurrentTracktime;
		auto ulDiff1RevTimeMax = CurrentTracktime;

		int FoundAllHeadersCountStart = FoundAllHeadersCount;
		bool FoundNewHeader = false;
		while (NT_SUCCESS(status)) // Read next IDAM of this track_retries.
		{
			// Read a single header.
#ifndef TESTER_APP
			status = CommandReadId(edx, pp->flags, pp->head);
			LARGE_INTEGER iFreq = { 0 }, iNow = KeQueryPerformanceCounter(&iFreq);
			long ulDiff = (long)((iNow.QuadPart - iStart.QuadPart) * 1000000i64 / iFreq.QuadPart);
#else
			long ulDiff;
			status = CommandReadId(edx, pp->flags, pp->head, ulDiff);
#endif
//			KdPrint(("#%d: TimedMultiScanTrack(): called CommandReadId, status=%08lX\n", KdCounter++, status));
			if (!NT_SUCCESS(status))
			{
				// Tolerate blank track.
				if (status == STATUS_FLOPPY_ID_MARK_NOT_FOUND)
					status = STATUS_TIMEOUT;
				// Tolerate some ID header errors.
				else if (status == STATUS_CRC_ERROR || status == STATUS_NONEXISTENT_SECTOR)
				{
					KdPrint(("#%d: !!! ReadId error (%X)\n", KdCounter++, status));
					status = STATUS_SUCCESS;
				}
			}
			else
			{

				if (FoundAllHeadersCount == FoundAllHeadersCountStart)
				{
					// Determining min max time of expected CHRN pair at next revolution.
					ulDiff1RevTimeMin = TolerancedTimeMin(
						CommonTimeMin(ulDiff, TimeToleranceOfTime) + CurrentTracktime,
						TimeToleranceOfTime);
					ulDiff1RevTimeMax = TolerancedTimeMax( // Earlier ulDiffSup.
						CommonTimeMax(ulDiff, TimeToleranceOfTime) + CurrentTracktime,
						TimeToleranceOfTime);
				}
				if (ulDiff < ulDiff1RevTimeMax)
				{
					FoundAllHeaders[FoundAllHeadersCount].chrn.cyl = edx->FifoOut[3];
					FoundAllHeaders[FoundAllHeadersCount].chrn.head = edx->FifoOut[4];
					FoundAllHeaders[FoundAllHeadersCount].chrn.sector = edx->FifoOut[5];
					FoundAllHeaders[FoundAllHeadersCount].chrn.size = edx->FifoOut[6];
					FoundAllHeaders[FoundAllHeadersCount].abstime = ulDiff;
					FoundAllHeaders[FoundAllHeadersCount].merge_flag = (MERGE_FLAG)MERGE_FLAG_NOTHING;
//					KdPrint(("#%d: scanning: found %hhu at %ld in rev %d\n", KdCounter++, edx->FifoOut[5], ulDiff, TrackRetries));
					if (++FoundAllHeadersCount >= FoundAllHeadersIndexSup)
						ulDiff = ulDiff1RevTimeMax; // Finish reading, Headers buffer is full.
				}

			}
			if (ulDiff >= ulDiff1RevTimeMin || status == STATUS_TIMEOUT)
			{	// Update the FoundFirstHeaders array.
//				KdPrint(("#%d: TimedMultiScanTrack(): time is up, ulDiff=%ld, ulDiff1RevTimeMin=%ld\n", KdCounter++, ulDiff, ulDiff1RevTimeMin));
				InsertionSort((PFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN)FoundAllHeaders, FoundAllHeadersCount, FoundAllHeadersCountStart);
				const auto FoundFirstHeadersCountNew = MergeNewFromSortedToSortedInplace(
					(PFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN)FoundAllHeaders, FoundAllHeadersCount,
					(PFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN)FoundFirstHeaders, FoundFirstHeadersCount, FoundFirstHeadersSup, FoundAllHeadersCountStart);
				FoundNewHeader = FoundFirstHeadersCount < FoundFirstHeadersCountNew; // Note that FoundFirstHeadersCountNew can be < 0.
//				KdPrint(("#%d: MergeNewFromSortedToSortedInplace returned: FoundFirstHeadersCountNew=%d, FoundFirstHeadersCount=%d\n",
//					KdCounter++, FoundFirstHeadersCountNew, FoundFirstHeadersCount));
				if (FoundNewHeader)
					FoundFirstHeadersCount = FoundFirstHeadersCountNew;
				break; // Finish reading of IDAMs of this track_retries, time is up.
			}
		}
		if (FoundAllHeadersCount >= FoundAllHeadersIndexSup)
			break; // Finish reading of IDAMs, Headers buffer is full.
		if (IsAutoTrackRetries && FoundNewHeader)
			TrackRetriesRequested = -pp->track_retries + 1;
//		KdPrint(("#%d: TimedMultiScanTrack(): finished Rev (%d), IsAutoTrackRetries=%hhu, TrackRetriesRequested=%d, FoundAllHeadersCount=%d\n",
//			KdCounter++, TrackRetries, IsAutoTrackRetries, TrackRetriesRequested, FoundAllHeadersCount));
		if (--TrackRetriesRequested == 0 || TrackRetries == 255)
			break;
	}
	KdPrint(("#%d: TimedMultiScanTrack(): finished all scannings, TrackRetries=%d, FoundAllHeadersCount=%d\n", KdCounter++, TrackRetries, FoundAllHeadersCount));
	const auto pOut = (PFD_TIMED_MULTI_SCAN_RESULT)edx->IoBuffer;

	for (;;)
	{
		// 2) Sorting the collected late-work-headers by abstime and CHRNs.
		InsertionSort((PFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN)FoundFirstHeaders, FoundFirstHeadersCount);
		// 3) Sorting the collected all-work-headers by abstime and CHRNs.
		InsertionSort((PFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN)FoundAllHeaders, FoundAllHeadersCount);
		// 4) Calculating tracktime by the first pair of all-work-headers in CurrentTrackTime distance.
//		KdPrint(("#%d: TimedMultiScanTrack(): DetermineTracktimeByMergingFirstHeaders, Tracktime=%ld\n", KdCounter++, CurrentTracktime));
		const auto AdjustedTracktime = DetermineTracktimeByMergingFirstHeaders((PFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN)FoundAllHeaders, FoundAllHeadersCount,
			(PFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN)FoundFirstHeaders, FoundFirstHeadersCount, CurrentTracktime, TimeToleranceOfTime);
//		KdPrint(("#%d: TimedMultiScanTrack(): DetermineTracktimeByMergingFirstHeaders.end, AdjustedTracktime=%ld\n", KdCounter++, AdjustedTracktime));
		if (AdjustedTracktime > 0)
		{
			CurrentTracktime = AdjustedTracktime;
			edx->SpinTime = CurrentTracktime;
		}
		// 5) Merge all-work-headers into out headers and also convert abstime to reltime and revolution.
		const auto OutHeaders = (PFD_TIMED_MULTI_ID_HEADER_EXT)pOut->HeaderArray();
		FoundAllHeadersCount = MergeWorkHeadersWithUpRevsIntoOutHeaders((PFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN)FoundAllHeaders, FoundAllHeadersCount,
			OutHeaders, OutHeaderIndexSup, CurrentTracktime, TimeToleranceOfTime);
		if (FoundAllHeadersCount < 0)
		{
			FoundAllHeadersCount = 0;
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		// 6) Sorting the out headers by default (i.e. reltime, rev, CHRN).
		InsertionSort(OutHeaders, FoundAllHeadersCount);
		break;
	}

	pOut->track_retries = (BYTE)TrackRetries;
	pOut->byte_tolerance_of_time = (BYTE)ByteToleranceOfTime * 2; // Used +-half tolerance.
	pOut->count = (WORD)FoundAllHeadersCount;
	pOut->tracktime = CurrentTracktime;
	KdPrint(("#%d: Count = %hu, TrackTime = %lu us\n",
		KdCounter++, pOut->count, pOut->tracktime));

	return status;
}
