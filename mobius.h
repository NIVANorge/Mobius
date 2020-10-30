


//NOTE: This is designed to be built as a unity build, i.e. in only a single compilation unit.


/*
TODOs:
	- Better encapsulation of the model_run_state subsystem. Unify lookup systems for parameters, inputs, results, last_results
	- Have to figure out if the initial value equation system we have currently is good.
	- Remove units as model entities entirely and only store / input them as strings? They seem like an unnecessary step right now.
	- (Even more) convenience accessors for the DataSet so that io and application code does not have to understand the inner structure of the DataSet that much.
	- Should entity handles contain their entity type in the upper bits?? May simplify entity system somewhat.
*/


#if !defined(MOBIUS_H)


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



#include "mobius_math.h"
#include "mobius_util.h"
#include "bucket_allocator.h"
#include "datetime.h"
#include "token_string.h"
#include "mobius_model.h"
#include "mobius_data_set.h"
#include "jacobian.h"
#include "mobius_model_run.h"
#include "lexer.h"
#include "mobius_io.h"
#include "mobius_solvers.h"


#define MOBIUS_H
#endif