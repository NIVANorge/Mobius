


//NOTE: This is designed to be built as a unity build, i.e. in only a single compilation unit.


/*
TODOs:
	- PrintPartialDependencyTrace gives incorrect information sometimes about what equation is involved when a solver is the one having a dependency.
	- Better encapsulation of the value_set_accessor subsystem. Unify lookup systems for parameters, inputs, results, last_results
	- Have to figure out if the initial value equation system we have currently is good.
	- Clean up the input tokenizer. Maybe just use fscanf for reading numbers, but it is actually a little complicated since we have to figure out the type in any case.
	- Remove units as model entities entirely and only store / input them as strings? They seem like an unnecessary step right now.
	- Manage the memory for all the data in the equation batch structure in such a way that it is aligned with how it will be read. (will have to not use std::vector in that case...)
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
#include <fstream>
#include <string.h>
#include <assert.h>
#include <float.h>
#include <cmath>
#include <sstream>
#include <iomanip>

//TODO: Does this intrinsic header exist for all compilers? We only use it for __rdtsc();
#include <x86intrin.h>


typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;


//NOTE: We allow the error handling to be replaced by the application. This is for instance useful for the python wrapper.
#if !defined(MOBIUS_PARTIAL_ERROR)
	#define MOBIUS_PARTIAL_ERROR(Msg) \
		std::cerr << Msg;
#endif

#if !defined(MOBIUS_FATAL_ERROR)	
	#define MOBIUS_FATAL_ERROR(Msg) \
	{MOBIUS_PARTIAL_ERROR(Msg) \
	exit(1);}
#endif

#include "mobius_math.h"
#include "mobius_util.h"
#include "datetime.h"
#include "token_string.h"
#include "mobius_model.h"
#include "mobius_data_set.cpp"
#include "jacobian.cpp"
#include "mobius_model.cpp"
#include "lexer.cpp"
#include "mobius_io.cpp"
#include "mobius_solvers.h"


#define MOBIUS_H
#endif