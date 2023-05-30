#pragma once

#include "Platform.h"
#include "fdrawcmd.h"
#include "fdrawcmd_friend.h"
#include "QuickSort.h"
#include "Sorters.h"

extern int KdCounter;

#define MAX_SECTORS_PER_TRACK 255
#define BYTE_TOLERANCE_OF_TIME 64 // See SAMdisk's Track class (COMPARE_TOLERANCE_BITS / 16).



constexpr long AdjustValueIntoRange(_In_ const long Value, _In_ const long RangeMin, _In_ const long RangeMax)
{
	return Value < RangeMin ? RangeMin : (Value > RangeMax ? RangeMax : Value);
}

/*
* First interval is [s1, e1], where s1 <= e1.
* Second interval is [s2, e2], where s2 <= e2.
*/
constexpr bool IsIntersecting(_In_ const long s1, _In_ const long e1, _In_ const long s2, _In_ const long e2)
{
	return s1 <= e2 && s2 <= e1;
}

/*
* First interval is [s1, e1], where s1 can be > e1 because of modulo range.
* Second interval is [s2, e2], where s2 can be > e2 because of modulo range.
*/
inline bool IsModIntersecting(_In_ const long s1, _In_ const long e1, _In_ const long s2, _In_ const long e2)
{
	auto result = false;
	if (s1 <= e1 && s2 <= e2) // [s1,e1], [s2,e2] are not wrapped.
		result = s1 <= e2 && s2 <= e1;
	else if ((s1 > e1 && s2 <= e2) || (s1 <= e1 && s2 > e2)) // [s1,e1] is wrapped, [s2,e2] is not wrapped or inversed.
		result = s1 <= e2 || s2 <= e1;
	else if (s1 > e1 && s2 > e2) // [s1,e1], [s2,e2] are wrapped.
		result = true; // Intersects at wrapping.
	return result;
}



constexpr int DataRateCodeAsDataRateBitsPerSec(_In_ const BYTE DataRate)
{
	return DataRate == FD_RATE_500K ? 500'000
		: (DataRate == FD_RATE_300K ? 300'000
			: (DataRate == FD_RATE_250K ? 250'000
				: (DataRate == FD_RATE_1M ? 1'000'000
					: 250'000))); // Impossible case but must return something.
}

/*
* Return the number of microseconds for given (default 1) bytes at the given rate.
* T should be 32 or 64 bits integer. 32 bits can handle Len_bytes <= 1073.
* Because 1000000 * 2 * Len_bytes must be <= 2147483647 (int_max).
* Encoding can be FD_OPTION_FM or FD_OPTION_MFM.
*/
template <typename T>
constexpr T GetFmOrMfmDataBytesTime_T(_In_ const BYTE DataRate, _In_ const BYTE Encoding, _In_ const T Len_bytes = 1)
{
	return Len_bytes * 1'000'000 * (Encoding == FD_OPTION_FM ? 2 : 1) / (DataRateCodeAsDataRateBitsPerSec(DataRate) / 8);
}

/*
* Return the number of microseconds for given (default 1) bytes at the given rate.
* To return result of "Len_bytes > 1073" case the Len_bytes must be <= 33'554'431.
* Because the minimum of DataRateCodeAsDataRateBitsPerSec() is 250'000 thus the
* result = 1'000'000 * 2 * Len_bytes / (250'000 / 8) = 64 * Len_bytes.
* result = 64 * Len_bytes must be <= 2'147'483'647 so Len_bytes must be <= 33'554'431.
* Encoding can be FD_OPTION_FM or FD_OPTION_MFM.
*/
constexpr int GetFmOrMfmDataBytesTime(_In_ const BYTE DataRate, _In_ const BYTE Encoding, _In_ const int Len_bytes = 1)
{
	return Len_bytes <= 1073 ? GetFmOrMfmDataBytesTime_T(DataRate, Encoding, Len_bytes)
		: (int)GetFmOrMfmDataBytesTime_T(DataRate, Encoding, (LONGLONG)Len_bytes);
}



#pragma pack(push,1)



typedef struct tagFD_CHRN
{
	constexpr bool IsEmpty() const
	{
		return cyl == 0 && head == 0 && sector == 0 && size == 0;
	}
	BYTE cyl, head, sector, size;
}
FD_CHRN, *PFD_CHRN;

constexpr bool operator ==(const tagFD_CHRN& lhs, const tagFD_CHRN& rhs)
{
	return lhs.cyl == rhs.cyl && lhs.head == rhs.head && lhs.sector == rhs.sector && lhs.size == rhs.size;
}

// Ordered by cyl, head, sector, size incremented.
constexpr bool operator <(const tagFD_CHRN& lhs, const tagFD_CHRN& rhs)
{
	return lhs.cyl < rhs.cyl || (lhs.cyl == rhs.cyl && (lhs.head < rhs.head
		|| (lhs.head == rhs.head && (lhs.sector < rhs.sector
			|| (lhs.sector == rhs.sector && lhs.size < rhs.size)))));
}

constexpr bool operator !=(const tagFD_CHRN& lhs, const tagFD_CHRN& rhs)
{
	return !(lhs == rhs);
}
constexpr bool operator >=(const tagFD_CHRN& lhs, const tagFD_CHRN& rhs)
{
	return !(lhs < rhs);
}
constexpr bool operator >(const tagFD_CHRN& lhs, const tagFD_CHRN& rhs)
{
	return rhs < lhs;
}
constexpr bool operator <=(const tagFD_CHRN& lhs, const tagFD_CHRN& rhs)
{
	return !(lhs > rhs);
}



typedef BYTE MERGE_FLAG;
enum MERGE_FLAG_ENUM { MERGE_FLAG_NOTHING = 0, MERGE_FLAG_MERGING = 254, MERGE_FLAG_MERGED = 255 };



// Ordered by compiler.
typedef struct tagFD_TIMED_MULTI_ID_HEADER_WORK
{
	long abstime;   // time absolute to index (in microseconds).
	FD_CHRN chrn;
	MERGE_FLAG merge_flag;
}
FD_TIMED_MULTI_ID_HEADER_WORK, *PFD_TIMED_MULTI_ID_HEADER_WORK;

constexpr bool operator ==(const tagFD_TIMED_MULTI_ID_HEADER_WORK& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK& rhs)
{
	return lhs.abstime == rhs.abstime && lhs.chrn == rhs.chrn && lhs.merge_flag == rhs.merge_flag;
}

constexpr bool operator !=(const tagFD_TIMED_MULTI_ID_HEADER_WORK& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK& rhs)
{
	return !(lhs == rhs);
}



// Ordered by CHRN.
typedef struct tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN : tagFD_TIMED_MULTI_ID_HEADER_WORK
{
}
FD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN, *PFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN;

static_assert(sizeof(FD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN) == sizeof(FD_TIMED_MULTI_ID_HEADER_WORK), "FD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN must not declare variables");

constexpr bool operator ==(const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN& rhs)
{
	return lhs.chrn == rhs.chrn;
}

constexpr bool operator <(const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN& rhs)
{
	return lhs.chrn < rhs.chrn;
}

constexpr bool operator !=(const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN& rhs)
{
	return !(lhs == rhs);
}
constexpr bool operator >=(const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN& rhs)
{
	return !(lhs < rhs);
}
constexpr bool operator >(const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN& rhs)
{
	return rhs < lhs;
}
constexpr bool operator <=(const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN& rhs)
{
	return !(lhs > rhs);
}



// Ordered by CHRN, abstime, merge_flag incremented.
typedef struct tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN_ABSTIME : tagFD_TIMED_MULTI_ID_HEADER_WORK
{
}
FD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN_ABSTIME, *PFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN_ABSTIME;

static_assert(sizeof(FD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN_ABSTIME) == sizeof(FD_TIMED_MULTI_ID_HEADER_WORK), "FD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN_ABSTIME must not declare variables");

constexpr bool operator ==(const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN_ABSTIME& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN_ABSTIME& rhs)
{
	return lhs.abstime == rhs.abstime && lhs.chrn == rhs.chrn && lhs.merge_flag == rhs.merge_flag;
}

constexpr bool operator <(const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN_ABSTIME& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN_ABSTIME& rhs)
{
	return ((lhs.abstime < rhs.abstime || (lhs.abstime == rhs.abstime && lhs.merge_flag < rhs.merge_flag)) && lhs.chrn == rhs.chrn) || lhs.chrn < rhs.chrn;
}

constexpr bool operator !=(const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN_ABSTIME& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN_ABSTIME& rhs)
{
	return !(lhs == rhs);
}
constexpr bool operator >=(const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN_ABSTIME& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN_ABSTIME& rhs)
{
	return !(lhs < rhs);
}
constexpr bool operator >(const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN_ABSTIME& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN_ABSTIME& rhs)
{
	return rhs < lhs;
}
constexpr bool operator <=(const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN_ABSTIME& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_CHRN_ABSTIME& rhs)
{
	return !(lhs > rhs);
}



// Ordered by abstime, CHRN, merge_flag incremented.
struct tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN : tagFD_TIMED_MULTI_ID_HEADER_WORK
{
};
typedef struct tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN
FD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN, *PFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN;

static_assert(sizeof(FD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN) == sizeof(FD_TIMED_MULTI_ID_HEADER_WORK), "FD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN must not declare variables");

constexpr bool operator ==(const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN& rhs)
{
	return lhs.abstime == rhs.abstime && lhs.chrn == rhs.chrn && lhs.merge_flag == rhs.merge_flag;
}

constexpr bool operator <(const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN& rhs)
{
	return lhs.abstime < rhs.abstime || (lhs.abstime == rhs.abstime && (lhs.chrn < rhs.chrn || (lhs.chrn == rhs.chrn && lhs.merge_flag < rhs.merge_flag)));
}

constexpr bool operator !=(const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN& rhs)
{
	return !(lhs == rhs);
}
constexpr bool operator >=(const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN& rhs)
{
	return !(lhs < rhs);
}
constexpr bool operator >(const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN& rhs)
{
	return rhs < lhs;
}
constexpr bool operator <=(const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN& lhs, const tagFD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN& rhs)
{
	return !(lhs > rhs);
}


/*
 * Ordered by reltime, revolution, cyl, head, sector, size incremented.
 */
typedef struct tagFD_TIMED_MULTI_ID_HEADER_EXT : tagFD_TIMED_MULTI_ID_HEADER
{
	inline void AssignCHRN(_In_ const FD_CHRN& chrn)
	{
		cyl = chrn.cyl;
		head = chrn.head;
		sector = chrn.sector;
		size = chrn.size;
	}

	inline void AssignTime(_In_ const long Time, _In_ const long Tracktime)
	{
		if (Tracktime <= 0)
		{
			revolution = 0;
			reltime = 0;
		}
		else
		{
			revolution = (BYTE)(Time / Tracktime);
			reltime = Time % Tracktime;
		}
	}

	inline void AssignWorkHeader(_In_ const tagFD_TIMED_MULTI_ID_HEADER_WORK& WorkHeader, _In_ const long Tracktime)
	{
		this->AssignCHRN(WorkHeader.chrn);
		this->AssignTime(WorkHeader.abstime, Tracktime);
	}
}
FD_TIMED_MULTI_ID_HEADER_EXT, *PFD_TIMED_MULTI_ID_HEADER_EXT;

static_assert(sizeof(FD_TIMED_MULTI_ID_HEADER_EXT) == sizeof(FD_TIMED_MULTI_ID_HEADER), "FD_TIMED_MULTI_ID_HEADER_EXT must not declare variables");

constexpr bool operator ==(const tagFD_TIMED_MULTI_ID_HEADER_EXT& lhs, const tagFD_TIMED_MULTI_ID_HEADER_EXT& rhs)
{
	return lhs.reltime == rhs.reltime && lhs.cyl == rhs.cyl && lhs.head == rhs.head && lhs.sector == rhs.sector && lhs.size == rhs.size && lhs.revolution == rhs.revolution;
}

constexpr bool operator <(const tagFD_TIMED_MULTI_ID_HEADER_EXT& lhs, const tagFD_TIMED_MULTI_ID_HEADER_EXT& rhs)
{
	return lhs.reltime < rhs.reltime || (lhs.reltime == rhs.reltime
		&& (lhs.revolution < rhs.revolution || (lhs.revolution == rhs.revolution
			&& (lhs.cyl < rhs.cyl || (lhs.cyl == rhs.cyl && (lhs.head < rhs.head
				|| (lhs.head == rhs.head && (lhs.sector < rhs.sector
					|| (lhs.sector == rhs.sector && lhs.size < rhs.size)))))))));
}

constexpr bool operator !=(const tagFD_TIMED_MULTI_ID_HEADER_EXT& lhs, const tagFD_TIMED_MULTI_ID_HEADER_EXT& rhs)
{
	return !(lhs == rhs);
}
constexpr bool operator >=(const tagFD_TIMED_MULTI_ID_HEADER_EXT& lhs, const tagFD_TIMED_MULTI_ID_HEADER_EXT& rhs)
{
	return !(lhs < rhs);
}
constexpr bool operator >(const tagFD_TIMED_MULTI_ID_HEADER_EXT& lhs, const tagFD_TIMED_MULTI_ID_HEADER_EXT& rhs)
{
	return rhs < lhs;
}
constexpr bool operator <=(const tagFD_TIMED_MULTI_ID_HEADER_EXT& lhs, const tagFD_TIMED_MULTI_ID_HEADER_EXT& rhs)
{
	return !(lhs > rhs);
}



#pragma pack(pop)



constexpr long CommonTimeMin(_In_ const long Time, _In_ const int TimeToleranceOfTime)
{
	return Time >= TimeToleranceOfTime ? Time - TimeToleranceOfTime : 0;
}

constexpr long CommonTimeMax(_In_ const long Time, _In_ const int TimeToleranceOfTime)
{
	return Time + TimeToleranceOfTime;
}

constexpr long TolerancedTimeMin(_In_ const long Time, _In_ const int TimeToleranceOfTime)
{
	return CommonTimeMin(Time, TimeToleranceOfTime);
}

constexpr long TolerancedTimeMax(_In_ const long Time, _In_ const int TimeToleranceOfTime)
{
	return CommonTimeMax(Time, TimeToleranceOfTime);
}

long DetermineTracktimeByMergingFirstHeaders(_In_ FD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN FoundAllHeaders[],
	_In_ const int FoundAllHeadersSup,
	_In_ const FD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN FoundFirstHeaders[], _In_ const int FoundFirstHeadersCount,
	_In_ const long Tracktime, _In_ const int TimeToleranceOfTime);

int MergeWorkHeadersWithUpRevsIntoOutHeaders(_In_ FD_TIMED_MULTI_ID_HEADER_WORK_AS_ABSTIME_CHRN FoundAllHeaders[],
	_In_ const int FoundAllHeadersSup,
	_Out_ FD_TIMED_MULTI_ID_HEADER_EXT OutHeaders[], _In_ const int OutHeadersSup,
	_In_ const long Tracktime, _In_ const int TimeToleranceOfTime);

NTSTATUS TimedMultiScanTrack(_Inout_ const PEXTRA_DEVICE_EXTENSION edx, _In_ const PFD_MULTI_SCAN_PARAMS pp, _In_ const int OutHeaderIndexSup);
