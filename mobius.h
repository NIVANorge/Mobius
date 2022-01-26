


//NOTE: This is designed to be built as a unity build, i.e. in only a single compilation unit.


#if !defined(MOBIUS_H)


#ifdef _WIN32
#include <windows.h>
#include <oleauto.h>
#endif


#include <stdint.h>
#include <stdlib.h>
#include <functional>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <map>
#include <set>
#include <iostream>
#include <string.h>
#include <assert.h>
#include <float.h>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <codecvt>
#include <random>


//NOTE: we use the intrin header for __rdtsc(); The intrinsic is in different headers for different compilers. If you compile with a different compiler than what is already set up you have to add in some lines below.
#if defined(__GNUC__) || defined(__GNUG__)
	#include <x86intrin.h>
#elif defined(_MSC_VER)
	#include <intrin.h>
#endif


typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t  s8;


//NOTE: We allow the error handling to be replaced by the application. This is for instance useful for the python wrapper.
#if !defined(MOBIUS_ERROR_OVERRIDE)
void
ErrorPrint() {}

template<typename t, typename... v>
void
ErrorPrint(t Value, v... Tail)
{
	std::cerr << Value;
	ErrorPrint(Tail...);
}

template<typename... v>
void
FatalError(v... Tail)
{
	ErrorPrint(Tail...);
	exit(1);
}	

void
WarningPrint() {}

template<typename t, typename... v>
void
WarningPrint(t Value, v... Tail)
{
	std::cout << Value;
	WarningPrint(Tail...);
}
#endif



#include "Src/mobius_math.h"
#include "Src/mobius_util.h"
#include "Src/bucket_allocator.h"
#include "Src/token_string.h"
#include "Src/datetime.h"
#include "Src/mobius_model.h"
#include "Src/mobius_data_set.h"
#include "Src/jacobian.h"
#include "Src/mobius_model_run.h"
#include "Src/lexer.h"
#include "Src/mobius_io.h"
#include "Src/spreadsheet_io.h"
#include "Src/mobius_solvers.h"


#define MOBIUS_H
#endif