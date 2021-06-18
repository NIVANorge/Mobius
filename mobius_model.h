
#if !defined(MOBIUS_MODEL_H)


//NOTE: The purpose of having unit_h, input_h, equation_h etc. be structs that contain a numeric handle rather than just letting them be a handle directly is that we can then use the C++ type system to get type safety. Unfortunately, C++ does not allow you to typedef a unique copy of a type that is not interchangable with others. However the type system WILL distinguish between two differently named structs even though they are otherwise equal.

typedef u32 entity_handle;

//NOTE: The ++ and * operators are for convenience when doing iteration
#define MODEL_ENTITY_HANDLE(Type) struct Type \
{ \
	entity_handle Handle; \
	void operator++() { Handle++; }; \
	Type operator*() const { return *this; } \
}; \
bool operator==(const Type &A, const Type &B) { return A.Handle == B.Handle; } \
bool operator!=(const Type &A, const Type &B) { return A.Handle != B.Handle; } \
bool operator<(const Type &A, const Type &B) { return A.Handle < B.Handle; } \
inline bool IsValid(Type H) { return H.Handle > 0; }

MODEL_ENTITY_HANDLE(module_h)
MODEL_ENTITY_HANDLE(unit_h)
MODEL_ENTITY_HANDLE(input_h)
MODEL_ENTITY_HANDLE(equation_h)
MODEL_ENTITY_HANDLE(parameter_h)
MODEL_ENTITY_HANDLE(solver_h)
MODEL_ENTITY_HANDLE(conditional_h)
MODEL_ENTITY_HANDLE(index_set_h)
MODEL_ENTITY_HANDLE(parameter_group_h)
#undef MODEL_ENTITY_HANDLE

//NOTE: Let parameter_x_h cast down to parameter_h
#define MODEL_PARAMETER_HANDLE(Type) struct Type \
{ \
	entity_handle Handle; \
	operator parameter_h() const { return {Handle}; } \
	void operator=(const parameter_h &Other) { Handle = Other.Handle; } \
}; \
bool operator==(const Type &A, const Type &B) { return A.Handle == B.Handle; } \
bool operator!=(const Type &A, const Type &B) { return A.Handle != B.Handle; } \
bool operator<(const Type &A, const Type &B) { return A.Handle < B.Handle; } \
inline bool IsValid(Type H) { return H.Handle > 0; }

//NOTE: The following 4 should just be used during model registration. Internally we should just use parameter_h instead
MODEL_PARAMETER_HANDLE(parameter_double_h)
MODEL_PARAMETER_HANDLE(parameter_uint_h)
MODEL_PARAMETER_HANDLE(parameter_bool_h)
MODEL_PARAMETER_HANDLE(parameter_time_h)
MODEL_PARAMETER_HANDLE(parameter_enum_h)
#undef MODEL_PARAMETER_HANDLE



struct index_t
{
	entity_handle IndexSetHandle;
	u32           Index;
	
	index_t() {};
	
	index_t(index_set_h IndexSet, u32 Index) : IndexSetHandle(IndexSet.Handle), Index(Index) {};
	
	index_t(entity_handle IndexSetHandle, u32 Index) : IndexSetHandle(IndexSetHandle), Index(Index) {};
	
	void operator++() { Index++; }
	
	index_t operator+(s32 Add) const
	{
		index_t Result = *this;
		Result.Index += Add;     //NOTE: This could result in an underflow. Have to see if that is a problem
		return Result;
	}
	
	index_t operator-(s32 Subtract) const
	{
		index_t Result = *this;
		Result.Index -= Subtract;     //NOTE: This could result in an underflow. Have to see if that is a problem
		return Result;
	}
	
	//NOTE: This does NOT check if they came from a different index set. That check has to be done by the caller if needed.
	bool operator<=(const index_t& Other) const { return Index <= Other.Index; }
	
	//NOTE: This does NOT check if they came from a different index set. That check has to be done by the caller if needed.
	bool operator<(const index_t& Other) const { return Index < Other.Index; }
	
	operator size_t() const { return (size_t)Index;	}
};

template <typename handle_type>
using string_map = std::unordered_map<token_string, handle_type, token_string_hash_function>;

template <typename handle_type, typename spec_type>
struct entity_registry
{
	std::vector<spec_type> Specs;
	string_map<handle_type> NameToHandle;   // Entries are organized so that Specs[NameToHandle["Some name"].Handle].Name == "Some name";
	
	entity_registry() { Specs.push_back({}); } //NOTE: Reserving the 0 index as an invalid index. TODO: Should maybe use U32_MAX instead, but would require some rework, and 0 is safer since we can then just 0-initialize everything.
	
	handle_type Register(const char *Name)
	{
		auto Find = NameToHandle.find(Name);
		if(Find != NameToHandle.end())
			return Find->second;
		else
		{
			entity_handle Handle = (entity_handle)Specs.size();
			Specs.push_back({});
			Specs[Handle].Name = Name;
			NameToHandle[Name] = {Handle};
			return {Handle};
		}
	}
	
	bool Has(const char *Name) const { return NameToHandle.find(Name) != NameToHandle.end(); } //TODO: Check to see if this one is necessary
	bool Has(token_string Name) const { return NameToHandle.find(Name) != NameToHandle.end(); }
	
	      spec_type& operator[](handle_type Handle)       { return Specs[Handle.Handle]; }
	const spec_type& operator[](handle_type Handle) const { return Specs[Handle.Handle]; }
	
	size_t Count() const { return Specs.size();	}     //TODO; Should probably return size-1 since the first index is discounted, but we have to make sure it doesn't break other code then.
	
	handle_type begin() { return {1}; }              //NOTE: Start at 1 since 0 is considered invalid
	handle_type end()   { return {(entity_handle)Specs.size()} ; }
	const handle_type begin() const { return {1}; }  //NOTE: Start at 1 since 0 is considered invalid
	const handle_type end()   const { return {(entity_handle)Specs.size()} ; }
};

union parameter_value
{
	double ValDouble;
	u64 ValUInt;
	u64 ValBool;      //NOTE: Since this is a union we don't save space by making the bool smaller any way.
	datetime ValTime; //NOTE: From datetime.h
	
	parameter_value() : ValTime() {}; //NOTE: 0-initializes it.
};

//NOTE: These comparisons should obviously only be used when the type is known!
bool operator==(const parameter_value &A, const parameter_value &B) { return A.ValUInt == B.ValUInt; }
bool operator!=(const parameter_value &A, const parameter_value &B) { return A.ValUInt != B.ValUInt; }

enum parameter_type
{
	ParameterType_Double = 0,
	ParameterType_UInt,
	ParameterType_Bool,
	ParameterType_Time,
	ParameterType_Enum,
};

inline const char *
GetParameterTypeName(parameter_type Type)
{
	//NOTE: It is important that this matches the above parameter_type enum:
	const char *Typenames[5] = {"double", "uint", "bool", "time", "enum"};
	return Typenames[(size_t)Type];
}

struct parameter_spec
{
	const char *Name;
	parameter_type Type;
	parameter_value Min;
	parameter_value Max;
	parameter_value Default;
	
	unit_h Unit;
	const char *Description;
	const char *ShortName;
	
	equation_h IsComputedBy; //NOTE: We allow certain parameters to be computed by an initial value equation rather than being provided by a parameter file.
	bool ShouldNotBeExposed; //NOTE: Any user interface or file handler should not deal with a parameter if ShouldNotBeExposed = true;
	
	parameter_group_h Group;
	
	std::vector<const char *> EnumNames;
	string_map<u32> EnumNameToValue;
	
	//NOTE: This not set before EndModelDefinition:
	//TODO: Should not really store it here though.
	std::vector<index_set_h> IndexSetDependencies;
};

struct unit_spec
{
	const char *Name;
	//NOTE: We don't need to put anything else here at the moment. Maybe eventually?
};

struct module_spec
{
	const char *Name;
	const char *Version;
	const char *Description;
};


struct equation_batch;
struct model_run_state;

typedef std::function<double(model_run_state *)> mobius_equation;

typedef std::function<void(size_t, size_t, double)> mobius_matrix_insertion_function;

#define MOBIUS_SOLVER_FUNCTION(Name) void Name(double h, size_t n, double* x0, double* wk, const equation_batch *Batch, model_run_state *RunState, double AbsErr, double RelErr)
typedef MOBIUS_SOLVER_FUNCTION(mobius_solver_function);
typedef size_t mobius_solver_space_requirement_function(size_t n);

struct parameter_group_spec
{
	const char *Name;
	std::vector<index_set_h> IndexSets;
	
	module_h Module;
	
	std::vector<parameter_h> Parameters;   //TODO: See if we use this anymore. It seems superfluous
};

enum index_set_type
{
	IndexSetType_Basic,
	IndexSetType_Branched,
};

struct index_set_spec
{
	const char *Name;
	index_set_type Type;
	std::vector<const char *> RequiredIndexes;
};

enum equation_type
{
	EquationType_Basic,
	EquationType_ODE,
	EquationType_InitialValue,
	EquationType_Cumulative,
};

inline const char *
GetEquationTypeName(equation_type Type)
{
	//NOTE: It is important that this matches the above enum:
	const char *Typenames[4] = {"basic", "ode", "initialvalue", "cumulative"};
	return Typenames[(size_t)Type];
}


//TODO: Just combine the two below?
template<typename handle_type>
struct dependency_registration
{
	handle_type Handle;
	size_t NumExplicitIndexes;
};

struct result_dependency_registration
{
	equation_h Handle;
	std::vector<index_t> Indexes;
};

//TODO: See if we could unionize some of the data below. Not everything is needed by every type of equation.
struct equation_spec
{
	const char *Name;
	equation_type Type;
	
	module_h Module;
	
	unit_h Unit;
	
	parameter_double_h InitialValue;
	double ExplicitInitialValue;
	bool HasExplicitInitialValue;
	equation_h InitialValueEquation;
	
	bool ResetEveryTimestep;         //NOTE: Only used for Type == EquationType_ODE.
	
	bool EquationIsSet;              //NOTE: Whether or not the equation body has been provided.
	
	index_set_h CumulatesOverIndexSet;   //NOTE: Only used for Type == EquationType_Cumulative.
	equation_h Cumulates;                //NOTE: Only used for Type == EquationType_Cumulative.
	parameter_double_h CumulationWeight; //NOTE: Only used for Type == EquationType_Cumulative.
	
	solver_h Solver;
	conditional_h Conditional;
		
		
	//NOTE: It would be nice to remove the following from the equation_spec and instead just store it in a temporary structure in EndModelDefinition, however it is reused in debug printouts etc. in the model run, so we have to store it in the model object somewhere anyway.
	
	//NOTE: The below are built during EndModelDefinition:
	std::set<index_set_h> IndexSetDependencies;          //NOTE: If the equation is run on a solver, the final index set dependencies of the equation will be those of the solver, not the ones stored here. You should generally use the storage structure to determine the final dependencies rather than this vector unless you are doing something specific in EndModelDefinition.
	std::set<parameter_h> ParameterDependencies;
	std::set<input_h>     InputDependencies;
	std::set<equation_h>  DirectResultDependencies;
	std::set<equation_h>  DirectLastResultDependencies;
	std::set<equation_h>  CrossIndexResultDependencies;
	
	//TODO: The following should probably just be stored separately in a temporary structure in the EndModelDefinition procedure, as it is not reused outside of that procedure.
	std::vector<result_dependency_registration> IndexedResultAndLastResultDependencies;
	
	bool TempVisited; //NOTE: For use in a graph traversal algorithm while resolving dependencies in EndModelDefinition.
	bool Visited;     //NOTE: For use in a graph traversal algorithm while resolving dependencies in EndModelDefinition.
};

struct solver_spec
{
	const char *Name;
	
	double h;           //The desired step size of the solver when errors are tolerable (0.0 - 1.0)
	double RelErr;      //Relative error tolerance (used by some solvers)
	double AbsErr;      //Absolute error tolerance (used by some solvers)
	
	parameter_double_h hParam; //What parameter handle to read in h from (if this is provided).
	
	mobius_solver_function *SolverFunction;
	mobius_solver_space_requirement_function *SpaceRequirement;
	
	bool UsesErrorControl;
	bool UsesJacobian;
	
	conditional_h Conditional;
};

struct conditional_spec
{
	const char *Name;

	parameter_type  SwitchType;
	parameter_h     Switch;
	parameter_value Value;
};

struct input_spec
{
	const char *Name;
	
	unit_h Unit;
	
	bool IsAdditional; // If it was user-specified in the input file, as opposed to being registered by a model building procedure.
	bool ClearToNaN;   // If it should initially be cleared to NaN. Otherwise it is cleared to 0.
	
	std::vector<index_set_h> IndexSetDependencies;
};


//TODO: Find a better name for this struct?
struct iteration_data
{
	array<parameter_h> ParametersToRead;
	array<input_h>     InputsToRead;
	array<equation_h>  ResultsToRead;
	array<equation_h>  LastResultsToRead;
};

struct equation_batch
{
	solver_h          Solver = {};
	
	conditional_h     Conditional;
	//NOTE: The following two could in theory be looked up using the handle above, but we make a separate copy here to avoid the extra lookup during model run.
	parameter_h       ConditionalSwitch = {};
	parameter_value   ConditionalValue;
	
	array<equation_h> Equations;
	array<equation_h> EquationsODE;   //NOTE: Should be empty unless IsValid(Solver)
	
	//NOTE: These are used for optimizing estimation of the Jacobian in case that is needed by a solver.
	array<array<size_t>>     ODEIsDependencyOfODE;
	array<array<equation_h>> ODEIsDependencyOfNonODE;
};

struct equation_batch_group
{
	size_t FirstBatch;
	size_t LastBatch;
	
	array<index_set_h> IndexSets;
	
	array<equation_h>     LastResultsToReadAtBase;  //Unfortunately we need this for LAST_RESULTs of equations with 0 index set dependencies.
	array<iteration_data> IterationData;

	array<equation_h> InitialValueOrder; //NOTE: The initial value setup of equations happens in a different order than the execution order during model run because the intial value equations may have different dependencies than the equations they are initial values for.
};


template<typename handle_type>
struct storage_unit_specifier
{
	array<index_set_h> IndexSets;
	array<handle_type> Handles;
};

struct mobius_model;

template<typename handle_type>
struct storage_structure
{
	array<storage_unit_specifier<handle_type>> Units;
	
	array<size_t> TotalCountForUnit;
	array<size_t> OffsetForUnit;
	array<size_t> UnitForHandle;
	array<size_t> LocationOfHandleInUnit;       // Units[UnitForHandle[H]].Handles[LocationOfHandleInUnit[H]] == H;
	size_t TotalCount;
	
	bool HasBeenSetUp = false;
	
	//NOTE: The model pointer is only here in case we need to look up the name of an index set or handle when reporting an error about misindexing if the MOBIUS_INDEX_BOUNDS_TESTS is turned on. It is not that clean to have this information here, though :(
	const mobius_model *Model;
};

struct mobius_data_set;
typedef std::function<void(mobius_data_set *)> mobius_preprocessing_step;


struct mobius_model
{
	const char *Name;
	
	bucket_allocator BucketMemory;  //NOTE: Used for batch group structure and some string copies.
	
	module_h CurrentModule = {};
	
	entity_registry<module_h,    module_spec>                   Modules;
	entity_registry<equation_h,  equation_spec>               Equations;
	entity_registry<input_h,     input_spec>                     Inputs;
	entity_registry<parameter_h, parameter_spec>             Parameters;
	entity_registry<index_set_h, index_set_spec>             IndexSets;
	entity_registry<parameter_group_h, parameter_group_spec> ParameterGroups;
	entity_registry<solver_h,    solver_spec>                   Solvers;
	entity_registry<conditional_h, conditional_spec>         Conditionals;
	entity_registry<unit_h,      unit_spec>                       Units;
	
	std::vector<mobius_equation> EquationBodies;
	
	array<equation_batch> EquationBatches;
	array<equation_batch_group> BatchGroups;
	
	std::vector<mobius_preprocessing_step> PreprocessingSteps;
	
	timestep_size TimestepSize;
	
	
	timer DefinitionTimer;
	bool Finalized;
	
	~mobius_model();
};


template<typename batch_like>
inline void
ForAllBatchEquations(const batch_like &Batch, std::function<bool(equation_h)> Do)
{
	bool ShouldBreak = false;
	for(equation_h Equation : Batch.Equations)
	{
		ShouldBreak = Do(Equation);
		if(ShouldBreak) break;
	}
	if(!ShouldBreak && IsValid(Batch.Solver))
	{
		for(equation_h Equation : Batch.EquationsODE)
		{
			ShouldBreak = Do(Equation);
			if(ShouldBreak) break;
		}
	}
}


struct mobius_data_set
{
	const mobius_model *Model;
	
	bucket_allocator BucketMemory;   //NOTE: Important! This should only be used for storage of "small" arrays such as IndexCounts or IndexNames, NOT the large arrays such as InputData or ResultData.
	
	parameter_value *ParameterData;
	storage_structure<parameter_h> ParameterStorageStructure;
	
	//TODO: The length of the data blocks should really really be stored here!
	//  Also, why not use array<bla> for more of these below?
	
	double *InputData;
	bool   *InputTimeseriesWasProvided;
	storage_structure<input_h> InputStorageStructure;
	datetime InputDataStartDate;
	bool InputDataHasSeparateStartDate = false; //NOTE: Whether or not a start date was provided for the input data, which is potentially different from the start date of the model run.
	u64 InputDataTimesteps;
	
	double *ResultData;
	storage_structure<equation_h> ResultStorageStructure;
	
	index_t *IndexCounts;
	const char ***IndexNames;  // IndexNames[IndexSet.Handle][IndexNamesToHandle[IndexSet.Handle][IndexName]] == IndexName;
	std::vector<string_map<u32>> IndexNamesToHandle;
	bool AllIndexesHaveBeenSet;
	
	//TODO: could make this array<array<array<index_t>>>, but I don't know if it improves the code..
	array<index_t> **BranchInputs; //BranchInputs[ReachIndexSet][ReachIndex] ...

	bool HasBeenRun;
	u64 TimestepsLastRun;
	datetime StartDateLastRun;
	
	~mobius_data_set();
};

#if !defined(MOBIUS_EQUATION_PROFILING)
#define MOBIUS_EQUATION_PROFILING 0
#endif

struct model_run_state
{
	// The purpose of the model_run_state is to store temporary state that is needed during a model run as well as providing an access point to data that is needed when evaluating equations.
	// There are two use cases.
	// If Running=false, this is a setup run, where the purpose is to register the accesses of all the equations to later determine their dependencies.
	// If Running=true, this is the actual run of the model, where equations should have access to the actual parameter values and so on.
	
	bool Running;
	const mobius_model *Model;
	mobius_data_set *DataSet;
	
	expanded_datetime CurrentTime;
	s64 Timestep; //NOTE: We make this a signed integer so that it can be set to -1 during the "initial value" step.


	bucket_allocator BucketMemory;

	//NOTE: For use during model execution		
	parameter_value *CurParameters;
	double          *CurResults;
	double          *LastResults;
	double          *CurInputs;
	bool            *CurInputWasProvided;
	
	index_t *CurrentIndexes; //NOTE: Contains the current index of each index set during execution.	
	
	double *AllCurResultsBase;
	double *AllLastResultsBase;
	
	double *AllCurInputsBase;
	
	double *AtResult;
	double *AtLastResult;
	
	array<parameter_value> FastParameterLookup;
	array<size_t>          FastInputLookup;
	array<size_t>          FastResultLookup;
	array<size_t>          FastLastResultLookup;
	
	parameter_value *AtParameterLookup;
	size_t *AtInputLookup;
	size_t *AtResultLookup;
	size_t *AtLastResultLookup;
	
	double *SolverTempX0;          //NOTE: Temporary storage for use by solvers
	double *SolverTempWorkStorage; //NOTE: Temporary storage for use by solvers
	double *JacobianTempStorage;   //NOTE: Temporary storage for use by Jacobian estimation
	

	//So that some models can do random generation
	std::mt19937 RandomGenerator;
	
	//NOTE: For use during dependency registration:
	std::vector<dependency_registration<parameter_h>> ParameterDependencies;
	std::vector<dependency_registration<input_h>> InputDependencies;
	std::vector<result_dependency_registration> ResultDependencies;
	std::vector<result_dependency_registration> LastResultDependencies;
	std::vector<index_set_h> DirectIndexSetDependencies;

	
#if MOBIUS_EQUATION_PROFILING
	size_t *EquationHits;
	u64 *EquationTotalCycles;
#endif
	
	//NOTE: For dependency registration run:
	model_run_state(const mobius_model *Model)
	{
		Running = false;
		DataSet = nullptr;
		this->Model = Model;
	}
	
	//NOTE: For proper run:
	model_run_state(mobius_data_set *DataSet)
	{
		Running = true;
		this->DataSet = DataSet;
		this->Model = DataSet->Model;

		BucketMemory.Initialize(1024*1024);

		CurInputs      = BucketMemory.Allocate<double>(Model->Inputs.Count());
		CurParameters  = BucketMemory.Allocate<parameter_value>(Model->Parameters.Count());
		CurResults     = BucketMemory.Allocate<double>(Model->Equations.Count());
		LastResults    = BucketMemory.Allocate<double>(Model->Equations.Count());
		CurInputWasProvided = BucketMemory.Allocate<bool>(Model->Inputs.Count());
		
		CurrentIndexes = BucketMemory.Allocate<index_t>(Model->IndexSets.Count());
		for(index_set_h IndexSet : Model->IndexSets)
			CurrentIndexes[IndexSet.Handle].IndexSetHandle = IndexSet.Handle;
		
		
		Timestep = 0;
		
		SolverTempX0 = nullptr;
		SolverTempWorkStorage = nullptr;
		JacobianTempStorage = nullptr;
		
		//NOTE: Code borrowed from stack exchange. Should really clean it up!
		std::random_device Dev;
		std::mt19937::result_type Seed = Dev() ^ (
            (std::mt19937::result_type)
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()
                ).count() +
            (std::mt19937::result_type)
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()
                ).count() );
		
		RandomGenerator = std::mt19937(Seed);
	}
	
	~model_run_state()
	{
		if(Running)
		{
			BucketMemory.DeallocateAll();
		}
	}
	
	void Clear()
	{
		if(Running)
		{
			memset(CurParameters,  0, sizeof(parameter_value)*Model->Parameters.Count());
			memset(CurInputs,      0, sizeof(double)*Model->Inputs.Count());
			memset(CurInputWasProvided,      0, sizeof(bool)*Model->Inputs.Count());
			memset(CurResults,     0, sizeof(double)*Model->Equations.Count());
			memset(LastResults,    0, sizeof(double)*Model->Equations.Count());
			memset(CurrentIndexes, 0, sizeof(index_t)*Model->IndexSets.Count());
			
			for(index_set_h IndexSet : Model->IndexSets)
				CurrentIndexes[IndexSet.Handle].IndexSetHandle = IndexSet.Handle;
		}
		else
		{
			ParameterDependencies.clear();
			InputDependencies.clear();
			ResultDependencies.clear();
			LastResultDependencies.clear();
			DirectIndexSetDependencies.clear();
		}
	}
};

inline double
CallEquation(const mobius_model *Model, model_run_state *RunState, equation_h Equation)
{
#if MOBIUS_EQUATION_PROFILING
	u64 Begin = __rdtsc();
#endif
	double ResultValue = Model->EquationBodies[Equation.Handle](RunState);
#if MOBIUS_EQUATION_PROFILING
	u64 End = __rdtsc();
	RunState->EquationHits[Equation.Handle]++;
	RunState->EquationTotalCycles[Equation.Handle] += (End - Begin);
#endif
	return ResultValue;
}


#define GET_ENTITY_NAME(Type, NType) \
inline const char * GetName(const mobius_model *Model, Type H) \
{ \
	return Model->NType[H].Name; \
}

GET_ENTITY_NAME(equation_h, Equations)
GET_ENTITY_NAME(input_h, Inputs)
GET_ENTITY_NAME(parameter_h, Parameters)
GET_ENTITY_NAME(index_set_h, IndexSets)
GET_ENTITY_NAME(parameter_group_h, ParameterGroups)
GET_ENTITY_NAME(solver_h, Solvers)
GET_ENTITY_NAME(conditional_h, Conditionals)
GET_ENTITY_NAME(unit_h, Units)
GET_ENTITY_NAME(module_h, Modules)

#undef GET_ENTITY_NAME


inline void
PrintRegistrationErrorHeader(const mobius_model *Model)
{
	if(!Model->Finalized && IsValid(Model->CurrentModule))
		ErrorPrint("While building module ", GetName(Model, Model->CurrentModule), "\" :\n");
}


#define GET_ENTITY_HANDLE(Type, Registry, Typename) \
inline Type Get##Typename##Handle(const mobius_model *Model, const token_string &Name) \
{ \
	Type Handle = {0}; \
	auto Find = Model->Registry.NameToHandle.find(Name); \
	if(Find != Model->Registry.NameToHandle.end()) \
		Handle = Find->second; \
	else \
	{ \
		PrintRegistrationErrorHeader(Model); \
		FatalError("ERROR: Tried to look up the handle of the ", #Typename, " \"", Name, "\", but it was not registered with the model.\n"); \
	} \
	return Handle; \
} \
inline Type Get##Typename##Handle(const mobius_model *Model, const token_string &Name, bool &Success) \
{ \
	Type Handle = {0}; \
	auto Find = Model->Registry.NameToHandle.find(Name); \
	if(Find != Model->Registry.NameToHandle.end()) \
	{ \
		Success = true; \
		Handle = Find->second; \
	} \
	else \
		Success = false; \
	return Handle; \
} \

GET_ENTITY_HANDLE(equation_h, Equations, Equation)
GET_ENTITY_HANDLE(input_h, Inputs, Input)
GET_ENTITY_HANDLE(parameter_double_h, Parameters, ParameterDouble)
GET_ENTITY_HANDLE(parameter_uint_h, Parameters, ParameterUInt)
GET_ENTITY_HANDLE(parameter_bool_h, Parameters, ParameterBool)
GET_ENTITY_HANDLE(parameter_time_h, Parameters, ParameterTime)
GET_ENTITY_HANDLE(parameter_h, Parameters, Parameter)
GET_ENTITY_HANDLE(index_set_h, IndexSets, IndexSet)
GET_ENTITY_HANDLE(parameter_group_h, ParameterGroups, ParameterGroup)
GET_ENTITY_HANDLE(solver_h, Solvers, Solver)
GET_ENTITY_HANDLE(conditional_h, Conditionals, Conditional)
GET_ENTITY_HANDLE(module_h, Modules, Module)

#undef GET_ENTITY_HANDLE


#define REGISTRATION_BLOCK(Model) \
if(Model->Finalized) \
{ \
	FatalError("ERROR: You can not call the function ", __func__, " on the model after it has been finalized using EndModelDefinition.\n"); \
}

inline void
SetTimestepSize(mobius_model *Model, const char *Format)
{
	REGISTRATION_BLOCK(Model);
	Model->TimestepSize = ParseTimestepSize(Format);
	//TODO: This should change the unit of the "Timesteps" parameter!
	//TODO: Do we want to put any restrictions on "roundness" of it? Can be weird if somebody sets a timestep of e.g. 61 seconds.
}

inline void
AddPreprocessingStep(mobius_model *Model, mobius_preprocessing_step PreprocessingStep)
{
	REGISTRATION_BLOCK(Model);
	Model->PreprocessingSteps.push_back(PreprocessingStep);
}

inline unit_h
RegisterUnit(mobius_model *Model, const char *Name = "dimensionless")
{
	REGISTRATION_BLOCK(Model)
	return Model->Units.Register(Name);  //Nothing else here to do unless we put more data on the unit_spec
}

inline module_h
BeginModule(mobius_model *Model, const char *Name, const char *Version)
{
	REGISTRATION_BLOCK(Model)
	module_h Module = Model->Modules.Register(Name);
	Model->Modules[Module].Version = Version;
	Model->CurrentModule = Module;
	return Module;
}

inline void
EndModule(mobius_model *Model)
{
	REGISTRATION_BLOCK(Model)
	Model->CurrentModule = {};
}

inline void
SetModuleDescription(mobius_model *Model, const char *Description)
{
	REGISTRATION_BLOCK(Model)
	Model->Modules[Model->CurrentModule].Description = Description;
}

inline index_set_h
RegisterIndexSet(mobius_model *Model, const char *Name, index_set_type Type = IndexSetType_Basic)
{
	REGISTRATION_BLOCK(Model)
	index_set_h IndexSet = Model->IndexSets.Register(Name);
	Model->IndexSets[IndexSet].Type = Type;
	return IndexSet;
}

inline index_set_h
RegisterIndexSetBranched(mobius_model *Model, const char *Name)
{
	REGISTRATION_BLOCK(Model)
	return RegisterIndexSet(Model, Name, IndexSetType_Branched);
}

inline index_t
RequireIndex(mobius_model *Model, index_set_h IndexSet, const char *IndexName)
{
	REGISTRATION_BLOCK(Model)
	
	index_set_spec &Spec = Model->IndexSets[IndexSet];
	if(Spec.Type != IndexSetType_Basic)
	{
		PrintRegistrationErrorHeader(Model);
		//TODO: Get rid of this requirement? However that may lead to issues with index order in branched index sets later.
		FatalError("ERROR: We only allow requiring indexes for basic index sets, \"", Spec.Name, "\" is of a different type.\n");
	}
	auto Find = std::find(Spec.RequiredIndexes.begin(), Spec.RequiredIndexes.end(), IndexName);
	if(Find != Spec.RequiredIndexes.end())
		return index_t(IndexSet, (u32)std::distance(Spec.RequiredIndexes.begin(), Find)); //NOTE: This is its position in the vector.
	else
	{
		Spec.RequiredIndexes.push_back(IndexName);
		return index_t(IndexSet, (u32)(Spec.RequiredIndexes.size() - 1));
	}
}


template<typename... T>
inline parameter_group_h
RegisterParameterGroup(mobius_model *Model, const char *Name, T... IndexSets)
{
	REGISTRATION_BLOCK(Model)
	parameter_group_h ParameterGroup = Model->ParameterGroups.Register(Name);
	Model->ParameterGroups[ParameterGroup].IndexSets = {IndexSets...};
	Model->ParameterGroups[ParameterGroup].Module = Model->CurrentModule;
	return ParameterGroup;
}


inline input_h
RegisterInput(mobius_model *Model, const char *Name, unit_h Unit = {0}, int ClearToNaN = -1, bool IsAdditional = false)
{
	REGISTRATION_BLOCK(Model)
	input_h Input = Model->Inputs.Register(Name);
	input_spec &Spec = Model->Inputs[Input];
	Spec.IsAdditional = IsAdditional;
	if(ClearToNaN == -1)
		Spec.ClearToNaN = IsAdditional;   // Unless something else is specified, additional inputs are cleared to NaN and model inputs are cleared to 0
	else
		Spec.ClearToNaN = (bool) ClearToNaN;
	
	Spec.Unit = Unit;
	return Input;
}

inline parameter_h
RegisterParameter_(mobius_model *Model, parameter_group_h Group, const char *Name, unit_h Unit, parameter_value Default, parameter_value Min, parameter_value Max, const char *Description, const char *ShortName, parameter_type Type)
{
	parameter_h Parameter = Model->Parameters.Register(Name);
	parameter_spec &Spec = Model->Parameters[Parameter];
	Spec.Type = Type;
	Spec.Default = Default;
	Spec.Min = Min;
	Spec.Max = Max;
	Spec.Group = Group;
	Spec.Unit = Unit;
	Spec.Description = Description;
	Spec.ShortName = ShortName;
	Model->ParameterGroups[Group].Parameters.push_back(Parameter);
	
	return Parameter;
}

inline parameter_double_h
RegisterParameterDouble(mobius_model *Model, parameter_group_h Group, const char *Name, unit_h Unit, double Default, double Min = -DBL_MAX, double Max = DBL_MAX, const char *Description = nullptr, const char *ShortName = nullptr)
{
	REGISTRATION_BLOCK(Model)
	
	parameter_value Default_; Default_.ValDouble = Default;
	parameter_value Min_;     Min_.ValDouble = Min;
	parameter_value Max_;     Max_.ValDouble = Max;
	parameter_h Parameter = RegisterParameter_(Model, Group, Name, Unit, Default_, Min_, Max_, Description, ShortName, ParameterType_Double);
	return {Parameter.Handle};
}

inline parameter_uint_h
RegisterParameterUInt(mobius_model *Model, parameter_group_h Group, const char *Name, unit_h Unit, u64 Default, u64 Min = 0, u64 Max = 0xffffffffffffffff, const char *Description = nullptr, const char *ShortName = nullptr)
{
	REGISTRATION_BLOCK(Model)
	
	parameter_value Default_; Default_.ValUInt = Default;
	parameter_value Min_;     Min_.ValUInt = Min;
	parameter_value Max_;     Max_.ValUInt = Max;
	parameter_h Parameter = RegisterParameter_(Model, Group, Name, Unit, Default_, Min_, Max_, Description, ShortName, ParameterType_UInt);
	return {Parameter.Handle};
}

inline parameter_bool_h
RegisterParameterBool(mobius_model *Model, parameter_group_h Group, const char *Name, bool Default, const char *Description = nullptr, const char *ShortName = nullptr)
{
	REGISTRATION_BLOCK(Model)
	
	parameter_value Default_; Default_.ValBool = Default;
	parameter_value Min_;     Min_.ValBool = false;
	parameter_value Max_;     Max_.ValBool = true;
	parameter_h Parameter = RegisterParameter_(Model, Group, Name, {}, Default_, Min_, Max_, Description, ShortName, ParameterType_Bool);
	return {Parameter.Handle};
}

inline parameter_time_h
RegisterParameterDate(mobius_model *Model, parameter_group_h Group, const char *Name, const char *Default, const char *Min = "1000-1-1", const char *Max = "3000-12-31", const char *Description = nullptr, const char *ShortName = nullptr)
{
	REGISTRATION_BLOCK(Model)
	
	int ParseSuccessCount = 0;
	bool ParseSuccess;
	
	parameter_value Default_; Default_.ValTime = datetime(Default, &ParseSuccess); ParseSuccessCount += (int)ParseSuccess;
	parameter_value Min_;     Min_.ValTime     = datetime(Min, &ParseSuccess);     ParseSuccessCount += (int)ParseSuccess;
	parameter_value Max_;     Max_.ValTime     = datetime(Max, &ParseSuccess);     ParseSuccessCount += (int)ParseSuccess;
	
	if(ParseSuccessCount != 3)
	{
		PrintRegistrationErrorHeader(Model);
		FatalError("ERROR: Unrecognized date format for default, min or max value when registering the parameter \"", Name, "\".\n");
	}
	
	parameter_h Parameter = RegisterParameter_(Model, Group, Name, {}, Default_, Min_, Max_, Description, ShortName, ParameterType_Time);
	return {Parameter.Handle};
}

inline parameter_enum_h
RegisterParameterEnum(mobius_model *Model, parameter_group_h Group, const char *Name, const std::initializer_list<const char *> &EnumNames, const char *Default = nullptr, const char *Description = nullptr, const char *ShortName = nullptr)
{
	REGISTRATION_BLOCK(Model)
	
	parameter_value Default_; Default_.ValUInt = 0; //Temporary
	parameter_value Min_;     Min_.ValUInt = 0;
	parameter_value Max_;     Max_.ValUInt = (u64)EnumNames.size();
	
	if(EnumNames.size() == 0)
	{
		PrintRegistrationErrorHeader(Model);
		FatalError("ERROR: The enum parameter \"", Name, "\" was registered with 0 possible enum values. It has to have at least 1.\n");
	}
	
	parameter_h Parameter = RegisterParameter_(Model, Group, Name, {}, Default_, Min_, Max_, Description, ShortName, ParameterType_Enum);
	parameter_spec &Spec = Model->Parameters[Parameter];
	
	u32 Idx = 0;
	s64 FoundDefault = -1;
	for(const char *EnumName : EnumNames)
	{
		const char *C = EnumName;
		while(*C != 0)
		{
			if(!isalpha(*C) && *C != '_' && !(C != EnumName && isdigit(*C)))
			{
				PrintRegistrationErrorHeader(Model);
				FatalError("ERROR: The enum parameter \"", Name, "\" was given the possible value \"", EnumName, "\", which contains characters that are not alphabetical, numerical, or '_'.\n");
			}
			++C;
		}
		
		if(Default && strcmp(EnumName, Default)==0) FoundDefault = (s64)Idx;
		
		Spec.EnumNameToValue[EnumName] = Idx;
		++Idx;
	}
	
	Spec.EnumNames = EnumNames; //NOTE: vector copy
	
	if(Default && (FoundDefault == -1))
	{
		PrintRegistrationErrorHeader(Model);
		FatalError("ERROR: The enum parameter \"", Name, "\" was given the default value \"", Default, "\" which does not appear on its list of possible values.\n");
	}
	
	if(FoundDefault >= 0)
		Spec.Default.ValUInt = (u64)FoundDefault;
	
	return {Parameter.Handle};
}

inline void
ParameterIsComputedBy(mobius_model *Model, parameter_h Parameter, equation_h Equation, bool ShouldNotBeExposed = true)
{
	REGISTRATION_BLOCK(Model)
	
	parameter_spec &Spec  = Model->Parameters[Parameter];
	equation_spec &EqSpec = Model->Equations[Equation];
	if(EqSpec.Type != EquationType_InitialValue)
	{
		PrintRegistrationErrorHeader(Model);
		FatalError("ERROR: Tried to set the equation \"", EqSpec.Name, "\" to compute the parameter \"", Spec.Name, "\", but \"", EqSpec.Name, "\" is not an initial value equation.\n");
	}
	if(EqSpec.Unit != Spec.Unit)
	{
		PrintRegistrationErrorHeader(Model);
		FatalError("ERROR: The equation \"", EqSpec.Name, "\" has a different unit from the parameter \"", Spec.Name, "\", that it is set to compute.\n");
	}
	
	Spec.IsComputedBy = Equation;
	Spec.ShouldNotBeExposed = ShouldNotBeExposed;
}

inline void
HideParameter(mobius_model *Model, parameter_h Parameter, bool ShouldNotBeExposed=true)
{
	REGISTRATION_BLOCK(Model)
	
	Model->Parameters[Parameter].ShouldNotBeExposed = ShouldNotBeExposed;
}


inline void
SetEquation(mobius_model *Model, equation_h Equation, mobius_equation EquationBody, bool Override = false)
{
	//REGISTRATION_BLOCK(Model) //NOTE: We can't use REGISTRATION_BLOCK since the user don't call the SetEquation explicitly, it is called through the macro EQUATION, and so the error message would be confusing.
	if(Model->Finalized)
	{
		PrintRegistrationErrorHeader(Model);
		FatalError("ERROR: You can not define an EQUATION body for the model after it has been finalized using EndModelDefinition.\n");
	}
	
	if(!Override && Model->Equations[Equation].EquationIsSet)
	{
		PrintRegistrationErrorHeader(Model);
		FatalError("ERROR: The equation body for \"", GetName(Model, Equation), "\" is already defined. It can not be defined twice unless it is explicitly overridden using EQUATION_OVERRIDE.\n");
	}
	
	Model->EquationBodies[Equation.Handle] = EquationBody;
	Model->Equations[Equation].EquationIsSet = true;
}

static void
SetSolver(mobius_model *Model, equation_h Equation, solver_h Solver)
{
	REGISTRATION_BLOCK(Model)
	
	equation_type Type = Model->Equations[Equation].Type;
	if(Type != EquationType_Basic && Type != EquationType_ODE)
	{
		PrintRegistrationErrorHeader(Model);
		FatalError("ERROR: Tried to set a solver for the equation \"", GetName(Model, Equation), "\", but it is not a basic equation or ODE equation, and so can not be given a solver.\n");
	}
	Model->Equations[Equation].Solver = Solver;
}

static void
SetConditional(mobius_model *Model, equation_h Equation, conditional_h Conditional)
{
	REGISTRATION_BLOCK(Model)
	equation_type Type = Model->Equations[Equation].Type;
	if(Type != EquationType_Basic && Type != EquationType_Cumulative)
	{
		PrintRegistrationErrorHeader(Model);
		FatalError("ERROR: Tried to set a conditional execution for the equation \"", GetName(Model, Equation), "\", but it is not a basic or cumulative equation, and so can not be given a conditional directly.\n");
	}
	Model->Equations[Equation].Conditional = Conditional;
}

static void
SetConditional(mobius_model *Model, solver_h Solver, conditional_h Conditional)
{
	REGISTRATION_BLOCK(Model)
	Model->Solvers[Solver].Conditional = Conditional;
}

static equation_h
RegisterEquation_(mobius_model *Model, const char *Name, unit_h Unit, solver_h Solver = {}, conditional_h Conditional = {}, equation_type Type = EquationType_Basic)
{
	REGISTRATION_BLOCK(Model)
	
	equation_h Equation = Model->Equations.Register(Name);
	
	if(Model->EquationBodies.size() <= Equation.Handle)
		Model->EquationBodies.resize(Equation.Handle + 1, {});
	
	equation_spec &Spec = Model->Equations[Equation];
	
	Spec.Type = Type;
	Spec.Unit = Unit;
	Spec.Module = Model->CurrentModule;
	Spec.Conditional = Conditional;
	
	if(IsValid(Solver))
		SetSolver(Model, Equation, Solver);
	
	return Equation;
}

static equation_h
RegisterEquation(mobius_model *Model, const char *Name, unit_h Unit)
{
	REGISTRATION_BLOCK(Model)
	return RegisterEquation_(Model, Name, Unit, {}, {}, EquationType_Basic);
}

static equation_h
RegisterEquation(mobius_model *Model, const char *Name, unit_h Unit, solver_h Solver)
{
	REGISTRATION_BLOCK(Model)
	return RegisterEquation_(Model, Name, Unit, Solver, {}, EquationType_Basic);
}

static equation_h
RegisterEquation(mobius_model *Model, const char *Name, unit_h Unit, conditional_h Conditional)
{
	REGISTRATION_BLOCK(Model)
	return RegisterEquation_(Model, Name, Unit, {}, Conditional, EquationType_Basic);
}

inline equation_h
RegisterEquationODE(mobius_model *Model, const char *Name, unit_h Unit, solver_h Solver = {})
{
	REGISTRATION_BLOCK(Model)
	return RegisterEquation_(Model, Name, Unit, Solver, {}, EquationType_ODE);
}

inline equation_h
RegisterEquationInitialValue(mobius_model *Model, const char *Name, unit_h Unit)
{
	REGISTRATION_BLOCK(Model)
	return RegisterEquation_(Model, Name, Unit, {}, {}, EquationType_InitialValue);
}

//NOTE: CumulateResult is implemented in mobius_data_set.cpp
static double CumulateResult(mobius_data_set *DataSet, equation_h Result, index_set_h CumulateOverIndexSet, index_t *CurrentIndexes, double *LookupBase);
static double CumulateResult(mobius_data_set *DataSet, equation_h Result, index_set_h CumulateOverIndexSet, index_t *CurrentIndexes, double *LookupBase, parameter_double_h Weight);

inline equation_h
RegisterEquationCumulative(mobius_model *Model, const char *Name, equation_h Cumulates, index_set_h CumulatesOverIndexSet, parameter_double_h Weight = {})
{
	REGISTRATION_BLOCK(Model)
	
	equation_spec &CumulateSpec = Model->Equations[Cumulates];
	if(CumulateSpec.Type == EquationType_InitialValue)
	{
		PrintRegistrationErrorHeader(Model);
		FatalError("ERROR: The cumulation equation \"", Name, "\" was set to cumulate an initial value equation \"", CumulateSpec.Name, "\". This is not supported.\n");
	}
	
	unit_h Unit = Model->Equations[Cumulates].Unit;
	equation_h Equation = RegisterEquation_(Model, Name, Unit, {}, {}, EquationType_Cumulative);
	Model->Equations[Equation].CumulatesOverIndexSet = CumulatesOverIndexSet;
	Model->Equations[Equation].Cumulates = Cumulates;
	Model->Equations[Equation].CumulationWeight = Weight;
	
	if(IsValid(Weight))
	{
		SetEquation(Model, Equation,
			[Cumulates, CumulatesOverIndexSet, Weight] (model_run_state *RunState) -> double
			{
				return CumulateResult(RunState->DataSet, Cumulates, CumulatesOverIndexSet, RunState->CurrentIndexes, RunState->AllCurResultsBase, Weight);
			}
		);
	}
	else
	{
		SetEquation(Model, Equation,
			[Cumulates, CumulatesOverIndexSet] (model_run_state *RunState) -> double
			{
				return CumulateResult(RunState->DataSet, Cumulates, CumulatesOverIndexSet, RunState->CurrentIndexes, RunState->AllCurResultsBase);
			}
		);
	}
	
	return Equation;
}

//TODO: Give warnings or errors when setting a initial value on an equation that already has one.
inline void
SetInitialValue(mobius_model *Model, equation_h Equation, parameter_double_h InitialValue)
{
	REGISTRATION_BLOCK(Model)
	Model->Equations[Equation].InitialValue = InitialValue;
	
	if(Model->Equations[Equation].Unit != Model->Parameters[InitialValue].Unit)
	{
		PrintRegistrationErrorHeader(Model);
		WarningPrint("WARNING: The equation \"", GetName(Model, Equation), "\" was registered with a different unit than its initial value parameter \"", GetName(Model, InitialValue), "\".\n");
	}
}

inline void
SetInitialValue(mobius_model *Model, equation_h Equation, double Value)
{
	REGISTRATION_BLOCK(Model)
	Model->Equations[Equation].ExplicitInitialValue = Value;
	Model->Equations[Equation].HasExplicitInitialValue = true;
}

inline void
SetInitialValue(mobius_model *Model, equation_h Equation, equation_h InitialValueEquation)
{
	REGISTRATION_BLOCK(Model)
	equation_type Type = Model->Equations[InitialValueEquation].Type;
	if(Type != EquationType_InitialValue && Type != EquationType_Basic)
	{
		PrintRegistrationErrorHeader(Model);
		//NOTE: We found out that sometimes we want the ability to force an equation to be its own initial value equation. So we also allow basic equations
		FatalError("ERROR: Tried to set the equation \"", GetName(Model, InitialValueEquation), "\" as an initial value of another equation, but it was not registered as an equation of type EquationInitialValue or Equation.\n");
	}
	
	Model->Equations[Equation].InitialValueEquation = InitialValueEquation;
	
	if(Model->Equations[Equation].Unit != Model->Equations[InitialValueEquation].Unit)
	{
		PrintRegistrationErrorHeader(Model);
		WarningPrint("WARNING: The equation \"", GetName(Model, Equation), "\" was registered with a different unit than its initial value equation \"", GetName(Model, InitialValueEquation), "\".\n");
	}
}

inline void
ResetEveryTimestep(mobius_model *Model, equation_h Equation)
{
	REGISTRATION_BLOCK(Model)
	equation_spec &Spec = Model->Equations[Equation];
	if(Spec.Type != EquationType_ODE)
	{
		PrintRegistrationErrorHeader(Model);
		FatalError("ERROR: Called ResetEveryTimestep on the equation \"", Spec.Name, "\", but this functionality is only available for ODE equations.\n");
	}
	Spec.ResetEveryTimestep = true;
}

#define MOBIUS_SOLVER_SETUP_FUNCTION(Name) void Name(solver_spec *SolverSpec)
typedef MOBIUS_SOLVER_SETUP_FUNCTION(mobius_solver_setup_function);


static solver_h
RegisterSolver(mobius_model *Model, const char *Name, parameter_double_h hParam, mobius_solver_setup_function *SetupFunction)
{
	REGISTRATION_BLOCK(Model)
	solver_h Solver = Model->Solvers.Register(Name);
	solver_spec &Spec = Model->Solvers[Solver];
	SetupFunction(&Spec);
	if(!Spec.SolverFunction)
	{
		PrintRegistrationErrorHeader(Model);
		FatalError("The solver algorithm for the solver ", Name, " does not have a SolverFunction!\n");
	}
	if(!Spec.SpaceRequirement)
	{
		PrintRegistrationErrorHeader(Model);
		FatalError("The solver algorithm for the solver ", Name, " does not specify a SpaceRequirement function to say how much storage it needs to be allocated for it.\n");
	}
	
	Spec.hParam = hParam;
	return Solver;
}

static solver_h
RegisterSolver(mobius_model *Model, const char *Name, double h, mobius_solver_setup_function *SetupFunction)
{
	REGISTRATION_BLOCK(Model)
	solver_h Solver = Model->Solvers.Register(Name);
	solver_spec &Spec = Model->Solvers[Solver];
	SetupFunction(&Spec);
	Spec.h = h;
	return Solver;
}

//TODO: Make version of this that takes parametric h too.
static solver_h
RegisterSolver(mobius_model *Model, const char *Name, double h, mobius_solver_setup_function *SetupFunction, double RelErr, double AbsErr)
{
	REGISTRATION_BLOCK(Model)
	
	solver_h Solver = RegisterSolver(Model, Name, h, SetupFunction);
	solver_spec &Spec = Model->Solvers[Solver];
	
	if(!Spec.UsesErrorControl)
	{
		PrintRegistrationErrorHeader(Model);
		WarningPrint("WARNING: Registered error tolerances with the solver \"", Name, "\", but the attached solver function does not support error control.\n");
	}
	
	Spec.RelErr = RelErr;
	Spec.AbsErr = AbsErr;
	
	return Solver;
}

static conditional_h
RegisterConditionalExecution(mobius_model *Model, const char *Name, parameter_bool_h Switch, bool Value)
{
	REGISTRATION_BLOCK(Model)
	
	conditional_h Conditional = Model->Conditionals.Register(Name);
	conditional_spec &Spec = Model->Conditionals[Conditional];
	Spec.SwitchType = ParameterType_Bool;
	Spec.Switch = Switch;
	Spec.Value.ValBool = Value;
	
	return Conditional;
}

static conditional_h
RegisterConditionalExecution(mobius_model *Model, const char *Name, parameter_uint_h Switch, u64 Value)
{
	REGISTRATION_BLOCK(Model)
	
	conditional_h Conditional = Model->Conditionals.Register(Name);
	conditional_spec &Spec = Model->Conditionals[Conditional];
	Spec.SwitchType = ParameterType_UInt;
	Spec.Switch = Switch;
	Spec.Value.ValUInt = Value;
	
	return Conditional;
}

static conditional_h
RegisterConditionalExecution(mobius_model *Model, const char *Name, parameter_enum_h Switch, const char *Value)
{
	REGISTRATION_BLOCK(Model)
	
	conditional_h Conditional = Model->Conditionals.Register(Name);
	conditional_spec &Spec = Model->Conditionals[Conditional];
	Spec.SwitchType = ParameterType_UInt;
	Spec.Switch = Switch;
	parameter_spec &SwitchSpec = Model->Parameters[Switch];
	
	auto Find = SwitchSpec.EnumNameToValue.find(Value);
	if(Find == SwitchSpec.EnumNameToValue.end())
	{
		PrintRegistrationErrorHeader(Model);
		FatalError("ERROR: While registering the conditional execution \"", Name, "\", the enum parameter \"", SwitchSpec.Name, "\" was not registered with the possible the value \"", Value, "\".\n");
	}
	
	Spec.Value.ValUInt = Find->second;
	
	return Conditional;
}

static conditional_h
GetConditional(const mobius_model *Model, equation_h Equation)
{
	const equation_spec &Spec = Model->Equations[Equation];
	conditional_h Conditional = Spec.Conditional;
	if(IsValid(Spec.Solver))
	{
		const solver_spec &SolverSpec = Model->Solvers[Spec.Solver];
		Conditional = SolverSpec.Conditional;
	}
	return Conditional;
}

inline void
AddInputIndexSetDependency(mobius_model *Model, input_h Input, index_set_h IndexSet)
{
	REGISTRATION_BLOCK(Model)
	Model->Inputs[Input].IndexSetDependencies.push_back(IndexSet);
}

#undef REGISTRATION_BLOCK





////////////////////////////////////////////////
// All of the below are value accessors for use in EQUATION bodies (ONLY)!
////////////////////////////////////////////////


#define PARAMETER(ParH, ...) (RunState__->Running ? GetCurrentParameter(RunState__, ParH, ##__VA_ARGS__) : RegisterParameterDependency(RunState__, ParH, ##__VA_ARGS__))
#define INPUT(InputH) (RunState__->Running ? GetCurrentInput(RunState__, InputH) : RegisterInputDependency(RunState__, InputH))
#define RESULT(ResultH, ...) (RunState__->Running ? GetCurrentResult(RunState__, ResultH, ##__VA_ARGS__) : RegisterResultDependency(RunState__, ResultH, ##__VA_ARGS__))
#define LAST_RESULT(ResultH, ...) (RunState__->Running ? GetLastResult(RunState__, ResultH, ##__VA_ARGS__) : RegisterLastResultDependency(RunState__, ResultH, ##__VA_ARGS__))
#define EARLIER_RESULT(ResultH, StepBack, ...) (RunState__->Running ? GetEarlierResult(RunState__, ResultH, (StepBack), ##__VA_ARGS__) : RegisterLastResultDependency(RunState__, ResultH, ##__VA_ARGS__))
#define INPUT_WAS_PROVIDED(InputH) (RunState__->Running ? GetIfInputWasProvided(RunState__, InputH) : RegisterInputDependency(RunState__, InputH))
#define IF_INPUT_ELSE_PARAMETER(InputH, ParameterH) (RunState__->Running ? GetCurrentInputOrParameter(RunState__, InputH, ParameterH) : RegisterInputAndParameterDependency(RunState__, InputH, ParameterH))


inline const expanded_datetime &
GetCurrentTime(model_run_state *RunState)
{
	return RunState->CurrentTime;
}

#define CURRENT_TIME() (GetCurrentTime(RunState__))

#define CURRENT_TIMESTEP() (RunState__->Timestep)

#define EQUATION(Model, ResultH, Def) \
SetEquation(Model, ResultH, \
 [=] (model_run_state *RunState__) { \
 Def \
 } \
);

#define EQUATION_OVERRIDE(Model, ResultH, Def) \
SetEquation(Model, ResultH, \
 [=] (model_run_state *RunState__) { \
 Def \
 } \
 , true \
);


//NOTE: These inline functions are used for type safety, which we don't get from macros.
//NOTE: We don't provide direct access to Time parameters since we want to encapsulate their storage. Instead we have accessor macros like CURRENT_DAY_OF_YEAR.
inline double
GetCurrentParameter(model_run_state *RunState, parameter_double_h Parameter)
{
	return RunState->CurParameters[Parameter.Handle].ValDouble;
}

inline u64
GetCurrentParameter(model_run_state *RunState, parameter_uint_h Parameter)
{
	return RunState->CurParameters[Parameter.Handle].ValUInt;
}

inline bool
GetCurrentParameter(model_run_state *RunState, parameter_bool_h Parameter)
{
	return RunState->CurParameters[Parameter.Handle].ValBool;
}

inline u64
GetCurrentParameter(model_run_state *RunState, parameter_enum_h Parameter)
{
	return RunState->CurParameters[Parameter.Handle].ValUInt;
}


//NOTE: Defined in mobius_data_set.h
template<typename handle_type>
size_t OffsetForHandle(storage_structure<handle_type> &Structure, const index_t* CurrentIndexes, const index_t *IndexCounts, const index_t *OverrideIndexes, size_t OverrideCount, handle_type Handle);

template<typename... T> double
GetCurrentParameter(model_run_state *RunState, parameter_double_h Parameter, T... Indexes)
{
	mobius_data_set *DataSet = RunState->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};
	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, (parameter_h)Parameter);
	return DataSet->ParameterData[Offset].ValDouble;
}

template<typename... T> u64
GetCurrentParameter(model_run_state *RunState, parameter_uint_h Parameter, T... Indexes)
{
	mobius_data_set *DataSet = RunState->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};
	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, (parameter_h)Parameter);
	return DataSet->ParameterData[Offset].ValUInt;
}

template<typename... T> bool
GetCurrentParameter(model_run_state *RunState, parameter_bool_h Parameter, T... Indexes)
{
	mobius_data_set *DataSet = RunState->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};
	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, (parameter_h)Parameter);
	return DataSet->ParameterData[Offset].ValBool;
}

template<typename... T> u64
GetCurrentParameter(model_run_state *RunState, parameter_enum_h Parameter, T... Indexes)
{
	mobius_data_set *DataSet = RunState->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};
	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, (parameter_h)Parameter);
	return DataSet->ParameterData[Offset].ValUInt;
}


inline double
GetCurrentResult(model_run_state *RunState, equation_h Result)
{
	return RunState->CurResults[Result.Handle];
}

inline double 
GetLastResult(model_run_state *RunState, equation_h LastResult)
{
	return RunState->LastResults[LastResult.Handle];
}

template<typename... T> double
GetCurrentResult(model_run_state *RunState, equation_h Result, T... Indexes)
{
	mobius_data_set *DataSet = RunState->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};
	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, Result);
	return RunState->AllCurResultsBase[Offset];
}

template<typename... T> double
GetLastResult(model_run_state *RunState, equation_h Result, T... Indexes)
{
	mobius_data_set *DataSet = RunState->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};

	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, Result);
	return RunState->AllLastResultsBase[Offset];
}

template<typename... T> double
GetEarlierResult(model_run_state *RunState, equation_h Result, u64 StepBack, T...Indexes)
{
	mobius_data_set *DataSet = RunState->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};
	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, Result);
	
	//TODO: Make proper accessor for this that belongs to mobius_data_set.h so that this file does not need to have knowledge of the inner workings of the storage system.
	double *Initial = DataSet->ResultData + Offset;
	//NOTE: Initial points to the initial value (adding TotalCount once gives us timestep 0)
	if(StepBack > RunState->Timestep)
	{
		return *Initial;
	}
	return *(Initial + ( (RunState->Timestep+1) - StepBack)*(RunState->DataSet->ResultStorageStructure.TotalCount));
}

inline double
GetCurrentInput(model_run_state *RunState, input_h Input)
{
	return RunState->CurInputs[Input.Handle];
}


inline bool
GetIfInputWasProvided(model_run_state * RunState, input_h Input)
{
	return RunState->CurInputWasProvided[Input.Handle];
}

inline double
GetCurrentInputOrParameter(model_run_state *RunState, input_h Input, parameter_double_h Parameter)
{
	if(GetIfInputWasProvided(RunState, Input)) return GetCurrentInput(RunState, Input);
	return GetCurrentParameter(RunState, Parameter);
}

template<typename... T> double
RegisterParameterDependency(model_run_state *RunState, parameter_h Parameter, T... Indexes)
{
	size_t OverrideCount = sizeof...(Indexes);
	RunState->ParameterDependencies.push_back({Parameter, OverrideCount});
	
	return 0.0;
}

inline double
RegisterInputDependency(model_run_state *RunState, input_h Input)
{
	RunState->InputDependencies.push_back({Input, 0});
	return 0.0;
}

inline double
RegisterInputAndParameterDependency(model_run_state *RunState, input_h Input, parameter_double_h Parameter)
{
	RegisterInputDependency(RunState, Input);
	RegisterParameterDependency(RunState, Parameter);

	return 0.0;
}

template<typename... T> double
RegisterResultDependency(model_run_state *RunState, equation_h Result, T... Indexes)
{
	std::vector<index_t> IndexVec = {Indexes...};
	RunState->ResultDependencies.push_back({Result, IndexVec});
	
	return 0.0;
}

template<typename... T> double
RegisterLastResultDependency(model_run_state *RunState, equation_h Result, T... Indexes)
{
	std::vector<index_t> IndexVec = {Indexes...};
	RunState->LastResultDependencies.push_back({Result, IndexVec});
	
	return 0.0;
}

//TODO: SET_RESULT is not that nice, and can interfere with how the dependency system works if used incorrectly. It is included to get PERSiST and some other models to work, but should be used with care!
#define SET_RESULT(ResultH, Value, ...) {if(RunState__->Running){SetResult(RunState__, Value, ResultH, ##__VA_ARGS__);}}

template<typename... T> void
SetResult(model_run_state *RunState, double Value, equation_h Result, T... Indexes)
{
	mobius_data_set *DataSet = RunState->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};
	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, Result);
	RunState->AllCurResultsBase[Offset] = Value;
}

void
SetResult(model_run_state *RunState, double Value, equation_h Result)
{
	mobius_data_set *DataSet = RunState->DataSet;
	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, nullptr, 0, Result);
	RunState->AllCurResultsBase[Offset] = Value;
	RunState->CurResults[Result.Handle] = Value;
}


#define INDEX_COUNT(IndexSetH) (RunState__->Running ? (RunState__->DataSet->IndexCounts[IndexSetH.Handle]) : 1)
#define CURRENT_INDEX(IndexSetH) (RunState__->Running ? GetCurrentIndex(RunState__, IndexSetH) : RegisterIndexSetDependency(RunState__, IndexSetH))
#define FIRST_INDEX(IndexSetH) (index_t(IndexSetH, 0))
#define INDEX_NUMBER(IndexSetH, Index) (index_t(IndexSetH, (u32)Index))
#define INPUT_COUNT(IndexSetH) (RunState__->Running ? GetInputCount(RunState__, IndexSetH) : 0)

inline index_t
RegisterIndexSetDependency(model_run_state *RunState, index_set_h IndexSet)
{
	RunState->DirectIndexSetDependencies.push_back(IndexSet);
	return index_t(IndexSet, 0);
}

inline index_t
GetCurrentIndex(model_run_state *RunState, index_set_h IndexSet)
{
	return RunState->CurrentIndexes[IndexSet.Handle];
}

inline size_t
GetInputCount(model_run_state *RunState, index_set_h IndexSet)
{
	index_t Current = GetCurrentIndex(RunState, IndexSet);
	return RunState->DataSet->BranchInputs[IndexSet.Handle][Current].Count;
}


//NOTE: Ideally we just want to iterate over the  BranchInputs[IndexSet.Handle][Branch] array. The only complicated part is that it can't do that in the registration run, and instead has to iterate over another object that has just one index.

inline array<index_t>&
BranchInputs(model_run_state *RunState, index_set_h IndexSet, index_t Index)
{
	//TODO: This could break if you do nested branch iteration, but we don't have a use-case for that...
	static index_t        DummyIndex;
	static array<index_t> DummyIndexes;
	
	if(RunState->Running)
		return RunState->DataSet->BranchInputs[IndexSet.Handle][Index.Index];
	
	DummyIndex = index_t { IndexSet.Handle, 0};
	DummyIndexes.Count = 1;
	DummyIndexes.Data = &DummyIndex;
	return DummyIndexes;
}

#define BRANCH_INPUTS(IndexSet) BranchInputs(RunState__, IndexSet, CURRENT_INDEX(IndexSet))

inline u64
UniformRandomU64(model_run_state *RunState, u64 Low, u64 High)
{
	std::uniform_int_distribution<u64> Distribution(Low, High);
	return Distribution(RunState->RandomGenerator);
}

inline double
UniformRandomDouble(model_run_state *RunState, double Low, double High)
{
	std::uniform_real_distribution<double> Distribution(Low, High);
	return Distribution(RunState->RandomGenerator);
}

#define UNIFORM_RANDOM_UINT(Low, High) (UniformRandomU64(RunState__, Low, High))
#define UNIFORM_RANDOM_DOUBLE(Low, High) (UniformRandomDouble(RunState__, Low, High))



#define MOBIUS_MODEL_H
#endif
