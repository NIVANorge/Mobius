
#if !defined(MOBIUS_UTIL_H)

#include <chrono>
#include <ctime>

inline void *
AllocClearedArray_(size_t ElementSize, size_t ArrayLen)
{
	void *Result = malloc(ElementSize * ArrayLen);
	if(!Result)
	{
		MOBIUS_FATAL_ERROR("ERROR: Ran out of memory." << std::endl);
	}
	memset(Result, 0, ElementSize * ArrayLen);
	return Result;
}
#define AllocClearedArray(Type, ArrayLen) (Type *)AllocClearedArray_(sizeof(Type), ArrayLen)

inline void *
CopyArray_(size_t ElementSize, size_t VecLen, void *Data)
{
	void *Result = malloc(ElementSize * VecLen);
	memcpy(Result, Data, ElementSize * VecLen);
	return Result;
}
#define CopyArray(Type, ArrayLen, Array) (Type *)CopyArray_(sizeof(Type), ArrayLen, Array)

struct timer
{
	std::chrono::time_point<std::chrono::high_resolution_clock> Begin;
};

inline timer
BeginTimer()
{
	//TODO: Make OS specific timers to get the best granularity possible (i.e. QueryPerformanceCounter on Windows etc.)
	timer Result;
	Result.Begin = std::chrono::high_resolution_clock::now();
	return Result;
}

inline u64
GetTimerMilliseconds(timer *Timer)
{
	auto End = std::chrono::high_resolution_clock::now();
	double Ms = std::chrono::duration_cast<std::chrono::milliseconds>(End - Timer->Begin).count();
	return (u64)Ms;
}

#define MOBIUS_UTIL_H
#endif