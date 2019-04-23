
#if !defined(MOBIUS_MODEL_H)


//NOTE: The purpose of having unit_h, input_h, equation_h etc. be structs that contain a numeric handle rather than just letting them be a handle directly is that we can then use the C++ type system to get type safety. Unfortunately, C++ does not allow you to typedef a unique copy of a type that is not interchangable with others. However the type system WILL distinguish between two differently named structs even though they are otherwise equal.

typedef u32 entity_handle;

#define MODEL_ENTITY_HANDLE(Type) struct Type \
{ \
	entity_handle Handle; \
}; \
bool operator==(const Type &A, const Type &B) { return A.Handle == B.Handle; } \
bool operator!=(const Type &A, const Type &B) { return A.Handle != B.Handle; } \
bool operator<(const Type &A, const Type &B) { return A.Handle < B.Handle; } \
inline bool IsValid(Type H) { return H.Handle > 0; }

MODEL_ENTITY_HANDLE(unit_h)

MODEL_ENTITY_HANDLE(input_h)
MODEL_ENTITY_HANDLE(equation_h)
MODEL_ENTITY_HANDLE(parameter_double_h)
MODEL_ENTITY_HANDLE(parameter_uint_h)
MODEL_ENTITY_HANDLE(parameter_bool_h)
MODEL_ENTITY_HANDLE(parameter_time_h)

MODEL_ENTITY_HANDLE(solver_h)

MODEL_ENTITY_HANDLE(index_set_h)

MODEL_ENTITY_HANDLE(parameter_group_h)

#undef MODEL_ENTITY_HANDLE

enum entity_type   //NOTE: Is currently only used so that the storage_structure knows what it is storing and can ask the Model for the name associated to a handle if an error occurs.
{
	EntityType_Parameter,
	EntityType_Input,
	EntityType_Equation,
};

inline const char *
GetEntityTypeName(entity_type Type)
{
	//NOTE: It is important that this matches the above enum:
	const char *Typenames[3] = {"parameter", "input", "equation"};
	return Typenames[(size_t)Type];
}


struct index_t
{
	entity_handle IndexSetHandle;
	u32           Index;
	
	index_t() {};
	
	index_t(index_set_h IndexSet, u32 Index) : IndexSetHandle(IndexSet.Handle), Index(Index) {};
	
	index_t(entity_handle IndexSetHandle, u32 Index) : IndexSetHandle(IndexSetHandle), Index(Index) {};
	
	void operator++()
	{
		Index++;
	}
	
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
	
	bool operator<=(const index_t& Other) const
	{
		//NOTE: This does NOT check if they came from a different index set. That check has to be done by the caller if needed.
		return Index <= Other.Index;
	}
	
	bool operator<(const index_t& Other) const
	{
		//NOTE: This does NOT check if they came from a different index set. That check has to be done by the caller if needed.
		return Index < Other.Index;
	}
	
	operator size_t() const
	{
		return (size_t)Index;
	}
};

union parameter_value
{
	double ValDouble;
	u64 ValUInt;
	u64 ValBool; //NOTE: Since this is a union we don't save space by making the bool smaller any way.
	datetime ValTime; //NOTE: From datetime.h
	
	parameter_value() : ValTime() {}; //NOTE: 0-initializes it.
};


enum parameter_type
{
	ParameterType_Double = 0,
	ParameterType_UInt,
	ParameterType_Bool,
	ParameterType_Time,
};

inline const char *
GetParameterTypeName(parameter_type Type)
{
	//NOTE: It is important that this matches the above enum:
	const char *Typenames[4] = {"double", "uint", "bool", "time"};
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
	
	equation_h IsComputedBy; //NOTE: We allow certain parameters to be computed by an initial value equation rather than being provided by a parameter file.
	bool ShouldNotBeExposed; //NOTE: Any user interface or file handler should not deal with a parameter if ShouldNotBeExposed = true;
	
	parameter_group_h Group;
	
	//NOTE: This not set before EndModelDefinition:
	std::vector<index_set_h> IndexSetDependencies;
};

struct unit_spec
{
	const char *Name;
	//NOTE: We don't need to put anything else here at the moment. Maybe eventually?
};


struct value_set_accessor;

typedef std::function<double(value_set_accessor *)> mobius_equation;

typedef std::function<void(double *, double *)> mobius_solver_equation_function;
typedef std::function<void(size_t, size_t, double)> mobius_matrix_insertion_function;
typedef std::function<void(double *, mobius_matrix_insertion_function &)> mobius_solver_jacobi_function;

#define MOBIUS_SOLVER_FUNCTION(Name) void Name(double h, size_t n, double* x0, double* wk, const mobius_solver_equation_function &EquationFunction, const mobius_solver_jacobi_function &JacobiFunction, double AbsErr, double RelErr)
typedef MOBIUS_SOLVER_FUNCTION(mobius_solver_function);

struct parameter_group_spec
{
	const char *Name;
	parameter_group_h ParentGroup;
	std::vector<parameter_group_h> ChildrenGroups;
	index_set_h IndexSet;
	std::vector<entity_handle> Parameters;
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

struct dependency_registration
{
	entity_handle Handle;
	size_t NumExplicitIndexes;
};

struct result_dependency_registration
{
	entity_handle Handle;
	std::vector<index_t> Indexes;
};

//TODO: See if we could unionize some of the data below. Not everything is needed by every type of equation.
struct equation_spec
{
	const char *Name;
	equation_type Type;
	
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
	
	//NOTE: The below are built during EndModelDefinition:
	std::set<index_set_h> IndexSetDependencies;          //NOTE: If the equation is run on a solver, the final index set dependencies of the equation will be those of the solver, not the ones stored here. You should generally use the storage structure to determine the final dependencies rather than this vector unless you are doing something specific in EndModelDefinition.
	std::set<entity_handle>  ParameterDependencies;
	std::set<input_h>     InputDependencies;
	std::set<equation_h>  DirectResultDependencies;
	std::set<equation_h>  DirectLastResultDependencies;
	std::set<equation_h>  CrossIndexResultDependencies;
	
	std::vector<result_dependency_registration> IndexedResultAndLastResultDependencies; //TODO: Maybe don't store these here, we could keep them separately just locally in EndModelDefinition.
	
	bool TempVisited; //NOTE: For use in a graph traversal algorithm while resolving dependencies.
	bool Visited;     //NOTE: For use in a graph traversal algorithm while resolving dependencies
};

struct solver_spec
{
	const char *Name;
	
	double h;
	double RelErr;
	double AbsErr;
	
	mobius_solver_function *SolverFunction;
	
	bool UsesErrorControl;
	bool UsesJacobian;
	
	//NOTE: These are built during EndModelDefinition:
	std::set<index_set_h> IndexSetDependencies;
	std::vector<equation_h> EquationsToSolve;
	std::set<equation_h> DirectResultDependencies;
	std::set<equation_h> CrossIndexResultDependencies;
	
	bool TempVisited; //NOTE: For use in a graph traversal algorithm while resolving dependencies.
	bool Visited;     //NOTE: For use in a graph traversal algorithm while resolving dependencies
};

struct input_spec
{
	const char *Name;
	
	unit_h Unit;
	
	bool IsAdditional;
	
	std::vector<index_set_h> IndexSetDependencies;
};


//TODO: Find a better name for this struct?
struct iteration_data
{
	std::vector<entity_handle> ParametersToRead;
	std::vector<input_h>       InputsToRead;
	std::vector<equation_h>    ResultsToRead;
	std::vector<equation_h>    LastResultsToRead;
};

enum equation_batch_type
{
	BatchType_Regular,
	BatchType_Solver,
};

struct equation_batch
{ 
	equation_batch_type Type;
	std::vector<equation_h> Equations;
	
	solver_h Solver;                      //NOTE: Only for Type==BatchType_Solver.
	std::vector<equation_h> EquationsODE; //NOTE: Only for Type==BatchType_Solver.
	
	std::vector<equation_h> InitialValueOrder; //NOTE: The initial value setup of equations happens in a different order than the execution order during model run because the intial value equations may have different dependencies than the equations they are initial values for.
	
	//NOTE: These are used for optimizing estimation of the Jacobian in case that is needed by a solver.
	std::vector<std::vector<size_t>> ODEIsDependencyOfODE;
	std::vector<std::vector<equation_h>> ODEIsDependencyOfNonODE;
};

struct equation_batch_group
{
	std::vector<index_set_h> IndexSets;
	
	std::vector<equation_h>     LastResultsToReadAtBase;  //Unfortunately we need this for LAST_RESULTs of equations with 0 index set dependencies.
	std::vector<iteration_data> IterationData;
	size_t FirstBatch;
	size_t LastBatch;
};

struct storage_unit_specifier
{
	std::vector<index_set_h> IndexSets;
	std::vector<entity_handle> Handles;
};

struct mobius_model;

struct storage_structure
{
	std::vector<storage_unit_specifier> Units;
	
	size_t *TotalCountForUnit;
	size_t *OffsetForUnit;
	size_t *UnitForHandle;
	size_t *LocationOfHandleInUnit;       // Units[UnitForHandle[H]].Handles[LocationOfHandleInUnit[H]] == H;
	size_t TotalCount;
	
	bool HasBeenSetUp = false;
	
	//NOTE: The following two are only here in case we need to look up the name of an index set or handle when reporting an error about misindexing if the MOBIUS_INDEX_BOUNDS_TESTS is turned on. It is not that clean to have this information here, though :(
	const mobius_model *Model;
	entity_type Type;
	
	~storage_structure();
};

typedef std::unordered_map<token_string, entity_handle, token_string_hash_function> string_map;

struct mobius_data_set;
typedef std::function<void(mobius_data_set *)> mobius_preprocessing_step;

struct mobius_model
{
	const char *Name;
	const char *Version;
	
	entity_handle FirstUnusedEquationHandle;
	string_map EquationNameToHandle;
	std::vector<mobius_equation> Equations;
	std::vector<equation_spec> EquationSpecs;
	
	entity_handle FirstUnusedInputHandle;
	string_map InputNameToHandle;
	std::vector<input_spec> InputSpecs;
	
	entity_handle FirstUnusedParameterHandle;
	string_map ParameterNameToHandle;
	std::vector<parameter_spec> ParameterSpecs;
	
	entity_handle FirstUnusedIndexSetHandle;
	string_map IndexSetNameToHandle;
	std::vector<index_set_spec> IndexSetSpecs;
	
	entity_handle FirstUnusedParameterGroupHandle;
	string_map ParameterGroupNameToHandle;
	std::vector<parameter_group_spec> ParameterGroupSpecs;
	
	entity_handle FirstUnusedSolverHandle;
	string_map SolverNameToHandle;
	std::vector<solver_spec> SolverSpecs;
	
	entity_handle FirstUnusedUnitHandle;
	string_map UnitNameToHandle;
	std::vector<unit_spec> UnitSpecs;
	
	std::vector<equation_batch> EquationBatches;
	std::vector<equation_batch_group> BatchGroups;
	
	std::vector<mobius_preprocessing_step> PreprocessingSteps;
	
	timer DefinitionTimer;
	bool Finalized;
};

#define FOR_ALL_BATCH_EQUATIONS(Batch, Do) \
for(equation_h Equation : Batch.Equations) { Do } \
if(Batch.Type == BatchType_Solver) { for(equation_h Equation : Batch.EquationsODE) { Do } }

#if !defined(MOBIUS_EQUATION_PROFILING)
#define MOBIUS_EQUATION_PROFILING 0
#endif

//TODO: The name "inputs" here is confusing, since there is already a different concept called input.
//TODO: Couldn't this just be a std::vector?
struct branch_inputs
{
	size_t Count;
	index_t *Inputs;
};

struct mobius_data_set
{
	const mobius_model *Model;
	
	parameter_value *ParameterData;
	storage_structure ParameterStorageStructure;
	
	double *InputData;
	bool   *InputTimeseriesWasProvided;
	storage_structure InputStorageStructure;
	datetime InputDataStartDate;
	bool InputDataHasSeparateStartDate = false; //NOTE: Whether or not a start date was provided for the input data, which is potentially different from the start date of the model run.
	u64 InputDataTimesteps;
	
	double *ResultData;
	storage_structure ResultStorageStructure;
	
	index_t *IndexCounts;
	const char ***IndexNames;  // IndexNames[IndexSet.Handle][IndexNamesToHandle[IndexSet.Handle][IndexName]] == IndexName;
	std::vector<string_map> IndexNamesToHandle;
	bool AllIndexesHaveBeenSet;
	
	branch_inputs **BranchInputs; //BranchInputs[ReachIndexSet][ReachIndex] ...
	
	std::vector<parameter_value> FastParameterLookup;
	std::vector<size_t> FastInputLookup;
	std::vector<size_t> FastResultLookup;
	std::vector<size_t> FastLastResultLookup;
	
	double *x0; //NOTE: Temporary storage for use by solvers
	double *wk; //NOTE: Temporary storage for use by solvers

	bool HasBeenRun;
	u64 TimestepsLastRun;
	
	
	~mobius_data_set();
};

struct value_set_accessor
{
	// The purpose of the value set accessor is to store state during the run of the model as well as providing access to various values to each equation that gets evaluated.
	// There are two use cases.
	// If Running=false, this is a setup run, where the purpose is to register the accesses of all the equations to later determine their dependencies.
	// If Running=true, this is the actual run of the model, where equations should have access to the actual parameter values and so on.
	
	bool Running;
	const mobius_model *Model;
	mobius_data_set *DataSet;
	
	s32 DayOfYear;
	s32 DaysThisYear;
	s64 Timestep; //NOTE: We make this a signed integer so that it can be set to -1 during the "initial value" step.


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
	
	
	parameter_value *AtParameterLookup;
	size_t *AtInputLookup;
	size_t *AtResultLookup;
	size_t *AtLastResultLookup;

	
	//NOTE: For use during dependency registration:
	std::vector<dependency_registration> ParameterDependencies;
	std::vector<dependency_registration> InputDependencies;
	std::vector<result_dependency_registration> ResultDependencies;
	std::vector<result_dependency_registration> LastResultDependencies;
	std::vector<index_set_h> DirectIndexSetDependencies;

	
#if MOBIUS_EQUATION_PROFILING
	size_t *EquationHits;
	u64 *EquationTotalCycles;
#endif
	
	//NOTE: For dependency registration run:
	value_set_accessor(const mobius_model *Model)
	{
		Running = false;
		DataSet = 0;
		this->Model = Model;
	}
	
	//NOTE: For proper run:
	value_set_accessor(mobius_data_set *DataSet)
	{
		Running = true;
		this->DataSet = DataSet;
		this->Model = DataSet->Model;

		CurInputs      = AllocClearedArray(double, Model->FirstUnusedInputHandle);
		CurParameters  = AllocClearedArray(parameter_value, Model->FirstUnusedParameterHandle);
		CurResults     = AllocClearedArray(double, Model->FirstUnusedEquationHandle);
		LastResults    = AllocClearedArray(double, Model->FirstUnusedEquationHandle);
		CurInputWasProvided = AllocClearedArray(bool, Model->FirstUnusedInputHandle);
		
		CurrentIndexes = AllocClearedArray(index_t, Model->FirstUnusedIndexSetHandle);
		for(entity_handle IndexSetHandle = 1; IndexSetHandle < Model->FirstUnusedIndexSetHandle; ++IndexSetHandle)
		{
			CurrentIndexes[IndexSetHandle].IndexSetHandle = IndexSetHandle;
		}
		
		DayOfYear = 0;
		DaysThisYear = 365;
		Timestep = 0;
	}
	
	~value_set_accessor()
	{
		if(Running)
		{
			free(CurParameters);
			free(CurInputs);
			free(CurInputWasProvided);
			free(CurResults);
			free(LastResults);
			free(CurrentIndexes);

		}
	}
	
	void Clear()
	{
		if(Running)
		{
			memset(CurParameters,  0, sizeof(parameter_value)*Model->FirstUnusedParameterHandle);
			memset(CurInputs,      0, sizeof(double)*Model->FirstUnusedInputHandle);
			memset(CurInputWasProvided,      0, sizeof(bool)*Model->FirstUnusedInputHandle);
			memset(CurResults,     0, sizeof(double)*Model->FirstUnusedEquationHandle);
			memset(LastResults,    0, sizeof(double)*Model->FirstUnusedEquationHandle);
			memset(CurrentIndexes, 0, sizeof(index_t)*Model->FirstUnusedIndexSetHandle);
			
			for(entity_handle IndexSetHandle = 1; IndexSetHandle < Model->FirstUnusedIndexSetHandle; ++IndexSetHandle)
			{
				CurrentIndexes[IndexSetHandle].IndexSetHandle = IndexSetHandle;
			}
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
CallEquation(const mobius_model *Model, value_set_accessor *ValueSet, equation_h Equation)
{
#if MOBIUS_EQUATION_PROFILING
	u64 Begin = __rdtsc();
#endif
	double ResultValue = Model->Equations[Equation.Handle](ValueSet);
#if MOBIUS_EQUATION_PROFILING
	u64 End = __rdtsc();
	ValueSet->EquationHits[Equation.Handle]++;
	ValueSet->EquationTotalCycles[Equation.Handle] += (End - Begin);
#endif
	return ResultValue;
}



#define GET_ENTITY_NAME(Type, NType) \
inline const char * GetName(const mobius_model *Model, Type H) \
{ \
	return Model->NType##Specs[H.Handle].Name; \
}

GET_ENTITY_NAME(equation_h, Equation)
GET_ENTITY_NAME(input_h, Input)
GET_ENTITY_NAME(parameter_double_h, Parameter)
GET_ENTITY_NAME(parameter_uint_h, Parameter)
GET_ENTITY_NAME(parameter_bool_h, Parameter)
GET_ENTITY_NAME(parameter_time_h, Parameter)
GET_ENTITY_NAME(index_set_h, IndexSet)
GET_ENTITY_NAME(parameter_group_h, ParameterGroup)
GET_ENTITY_NAME(solver_h, Solver)
GET_ENTITY_NAME(unit_h, Unit)

#undef GET_ENTITY_NAME

inline const char *
GetParameterName(const mobius_model *Model, entity_handle ParameterHandle) //NOTE: In case we don't know the type of the parameter and just want the name.
{
	return Model->ParameterSpecs[ParameterHandle].Name;
}

inline const char *
GetName(const mobius_model *Model, entity_type Type, entity_handle Handle)
{
	switch(Type)
	{
		case EntityType_Parameter:
		return GetParameterName(Model, Handle);
		break;
		
		case EntityType_Input:
		return GetName(Model, input_h {Handle});
		break;
		
		case EntityType_Equation:
		return GetName(Model, equation_h {Handle});
		break;
	}
}

#define GET_ENTITY_HANDLE(Type, Typename, Typename2) \
inline Type Get##Typename2##Handle(const mobius_model *Model, const token_string &Name) \
{ \
	entity_handle Handle = 0; \
	auto Find = Model->Typename##NameToHandle.find(Name); \
	if(Find != Model->Typename##NameToHandle.end()) \
	{ \
		Handle = Find->second; \
	} \
	else \
	{ \
		MOBIUS_FATAL_ERROR("ERROR: Tried to look up the handle of the " << #Typename << " \"" << Name << "\", but it was not registered with the model." << std::endl); \
	} \
	return { Handle }; \
}

GET_ENTITY_HANDLE(equation_h, Equation, Equation)
GET_ENTITY_HANDLE(input_h, Input, Input)
GET_ENTITY_HANDLE(parameter_double_h, Parameter, ParameterDouble)
GET_ENTITY_HANDLE(parameter_uint_h, Parameter, ParameterUInt)
GET_ENTITY_HANDLE(parameter_bool_h, Parameter, ParameterBool)
GET_ENTITY_HANDLE(parameter_time_h, Parameter, ParameterTime)
GET_ENTITY_HANDLE(index_set_h, IndexSet, IndexSet)
GET_ENTITY_HANDLE(parameter_group_h, ParameterGroup, ParameterGroup)
GET_ENTITY_HANDLE(solver_h, Solver, Solver)

#undef GET_ENTITY_HANDLE

inline entity_handle
GetParameterHandle(const mobius_model *Model, const token_string &Name) //NOTE: In case we don't know the type of the parameter and just want the handle.
{
	entity_handle Handle = 0;
	auto Find = Model->ParameterNameToHandle.find(Name);
	if(Find != Model->ParameterNameToHandle.end())
	{
		Handle = Find->second;
	}
	else
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried to find the Parameter \"" << Name << "\", but it was not registered with the model." << std::endl);
	}
	return Handle;
}


#define REGISTRATION_BLOCK(Model) \
if(Model->Finalized) \
{ \
	MOBIUS_FATAL_ERROR("ERROR: You can not call the function " << __func__ << " on the model after it has been finalized using EndModelDefinition." << std::endl); \
}

#define REGISTER_MODEL_ENTITY(Model, Typename, Handlename, Name) \
auto Find = Model->Typename##NameToHandle.find(Name); \
if(Find != Model->Typename##NameToHandle.end()) \
{ \
	Handlename.Handle = Find->second; \
} \
else \
{ \
	Handlename.Handle = Model->FirstUnused##Typename##Handle++; \
	Model->Typename##NameToHandle[Name] = Handlename.Handle; \
} \
if(Model->Typename##Specs.size() <= Handlename.Handle) \
{ \
	Model->Typename##Specs.resize(Handlename.Handle + 1, {}); \
} \
Model->Typename##Specs[Handlename.Handle].Name = Name;

void AddPreprocessingStep(mobius_model *Model, mobius_preprocessing_step PreprocessingStep)
{
	REGISTRATION_BLOCK(Model);
	
	Model->PreprocessingSteps.push_back(PreprocessingStep);
}

inline unit_h
RegisterUnit(mobius_model *Model, const char *Name = "dimensionless")
{
	REGISTRATION_BLOCK(Model)
	
	unit_h Unit = {};
	
	REGISTER_MODEL_ENTITY(Model, Unit, Unit, Name);
	
	return Unit;
}

inline index_set_h
RegisterIndexSet(mobius_model *Model, const char *Name, index_set_type Type = IndexSetType_Basic)
{
	REGISTRATION_BLOCK(Model)

	index_set_h IndexSet = {};
	REGISTER_MODEL_ENTITY(Model, IndexSet, IndexSet, Name);
	
	Model->IndexSetSpecs[IndexSet.Handle].Type = Type;
	
	return IndexSet;
}

inline index_set_h
RegisterIndexSetBranched(mobius_model *Model, const char *Name)
{
	REGISTRATION_BLOCK(Model)
	
	index_set_h IndexSet = RegisterIndexSet(Model, Name, IndexSetType_Branched);
	
	return IndexSet;
}

inline index_t
RequireIndex(mobius_model *Model, index_set_h IndexSet, const char *IndexName)
{
	REGISTRATION_BLOCK(Model)
	
	index_set_spec &Spec = Model->IndexSetSpecs[IndexSet.Handle];
	if(Spec.Type != IndexSetType_Basic)
	{
		//TODO: Get rid of this requirement? However that may lead to issues with index order in branched index sets later.
		MOBIUS_FATAL_ERROR("ERROR: We only allow requiring indexes for basic index sets, " << Spec.Name << " is of a different type." << std::endl);
	}
	auto Find = std::find(Spec.RequiredIndexes.begin(), Spec.RequiredIndexes.end(), IndexName);
	if(Find != Spec.RequiredIndexes.end())
	{
		return index_t(IndexSet, (u32)std::distance(Spec.RequiredIndexes.begin(), Find)); //NOTE: This is its position in the vector.
	}
	else
	{
		Spec.RequiredIndexes.push_back(IndexName);
		return index_t(IndexSet, (u32)(Spec.RequiredIndexes.size() - 1));
	}
}


inline parameter_group_h
RegisterParameterGroup(mobius_model *Model, const char *Name, index_set_h IndexSet = {0})
{
	REGISTRATION_BLOCK(Model)
	
	parameter_group_h ParameterGroup = {};
	REGISTER_MODEL_ENTITY(Model, ParameterGroup, ParameterGroup, Name)
	
	Model->ParameterGroupSpecs[ParameterGroup.Handle].IndexSet = IndexSet;
	
	return ParameterGroup;
}

inline void
SetParentGroup(mobius_model *Model, parameter_group_h Child, parameter_group_h Parent)
{
	REGISTRATION_BLOCK(Model)
	
	parameter_group_spec &ChildSpec = Model->ParameterGroupSpecs[Child.Handle];
	parameter_group_spec &ParentSpec = Model->ParameterGroupSpecs[Parent.Handle];
	if(IsValid(ChildSpec.ParentGroup) && ChildSpec.ParentGroup.Handle != Parent.Handle)
	{
		MOBIUS_FATAL_ERROR("ERROR: Setting a parent group for the parameter group " << ChildSpec.Name << ", but it already has a different parent group " << GetName(Model, ChildSpec.ParentGroup) << ".");
	}
	ChildSpec.ParentGroup = Parent;
	ParentSpec.ChildrenGroups.push_back(Child);
}

inline input_h
RegisterInput(mobius_model *Model, const char *Name, unit_h Unit = {0}, bool IsAdditional = false)
{
	REGISTRATION_BLOCK(Model)
	
	input_h Input = {};
	REGISTER_MODEL_ENTITY(Model, Input, Input, Name)
	
	input_spec &Spec = Model->InputSpecs[Input.Handle];
	Spec.IsAdditional = IsAdditional;
	Spec.Unit = Unit;
	
	return Input;
}

inline parameter_double_h
RegisterParameterDouble(mobius_model *Model, parameter_group_h Group, const char *Name, unit_h Unit, double Default, double Min = -DBL_MAX, double Max = DBL_MAX, const char *Description = 0)
{
	REGISTRATION_BLOCK(Model)
	
	parameter_double_h Parameter = {};
	REGISTER_MODEL_ENTITY(Model, Parameter, Parameter, Name)
	
	parameter_spec &Spec = Model->ParameterSpecs[Parameter.Handle];
	Spec.Type = ParameterType_Double;
	Spec.Default.ValDouble = Default;
	Spec.Min.ValDouble = Min;
	Spec.Max.ValDouble = Max;
	Spec.Group = Group;
	Spec.Unit = Unit;
	Spec.Description = Description;
	
	Model->ParameterGroupSpecs[Group.Handle].Parameters.push_back(Parameter.Handle);

	return Parameter;
}

inline parameter_uint_h
RegisterParameterUInt(mobius_model *Model, parameter_group_h Group, const char *Name, unit_h Unit, u64 Default, u64 Min = 0, u64 Max = 0xffffffffffffffff, const char *Description = 0)
{
	REGISTRATION_BLOCK(Model)
	
	parameter_uint_h Parameter = {};
	REGISTER_MODEL_ENTITY(Model, Parameter, Parameter, Name)
	
	parameter_spec &Spec = Model->ParameterSpecs[Parameter.Handle];
	Spec.Type = ParameterType_UInt;
	Spec.Default.ValUInt = Default;
	Spec.Min.ValUInt = Min;
	Spec.Max.ValUInt = Max;
	Spec.Group = Group;
	Spec.Unit = Unit;
	Spec.Description = Description;
	
	Model->ParameterGroupSpecs[Group.Handle].Parameters.push_back(Parameter.Handle);
	
	return Parameter;
}

inline parameter_bool_h
RegisterParameterBool(mobius_model *Model, parameter_group_h Group, const char *Name, bool Default, const char *Description = 0)
{
	REGISTRATION_BLOCK(Model)
	
	parameter_bool_h Parameter = {};
	REGISTER_MODEL_ENTITY(Model, Parameter, Parameter, Name)
	
	parameter_spec &Spec = Model->ParameterSpecs[Parameter.Handle];
	Spec.Type = ParameterType_Bool;
	Spec.Default.ValBool = Default;
	Spec.Min.ValBool = false;
	Spec.Max.ValBool = true;
	Spec.Group = Group;
	Spec.Description = Description;
	
	Model->ParameterGroupSpecs[Group.Handle].Parameters.push_back(Parameter.Handle);
	
	return Parameter;
}

inline parameter_time_h
RegisterParameterDate(mobius_model *Model, parameter_group_h Group, const char *Name, const char *Default, const char *Min = "1000-1-1", const char *Max = "3000-12-31", const char *Description = 0)
{
	REGISTRATION_BLOCK(Model)
	
	parameter_time_h Parameter = {};
	REGISTER_MODEL_ENTITY(Model, Parameter, Parameter, Name)
	
	parameter_spec &Spec = Model->ParameterSpecs[Parameter.Handle];
	Spec.Type = ParameterType_Time;
	
	bool ParseSuccessAll = true;
	bool ParseSuccess;
	Spec.Default.ValTime = datetime(Default, &ParseSuccess);
	ParseSuccessAll = ParseSuccessAll && ParseSuccess;
	Spec.Min.ValTime = datetime(Min, &ParseSuccess);
	ParseSuccessAll = ParseSuccessAll && ParseSuccess;
	Spec.Max.ValTime = datetime(Max, &ParseSuccess);
	ParseSuccessAll = ParseSuccessAll && ParseSuccess;
	
	if(!ParseSuccessAll)
	{
		MOBIUS_FATAL_ERROR("ERROR: Unrecognized date format for default, min or max value when registering the parameter " << Name << std::endl);
	}
	
	Spec.Group = Group;
	Spec.Description = Description;
	
	Model->ParameterGroupSpecs[Group.Handle].Parameters.push_back(Parameter.Handle);

	return Parameter;
}

inline void
ParameterIsComputedBy(mobius_model *Model, parameter_double_h Parameter, equation_h Equation, bool ShouldNotBeExposed = true)
{
	parameter_spec &Spec  = Model->ParameterSpecs[Parameter.Handle];
	equation_spec &EqSpec = Model->EquationSpecs[Equation.Handle];
	if(EqSpec.Type != EquationType_InitialValue)
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried to set the equation " << EqSpec.Name << " to compute the parameter " << Spec.Name << ", but " << EqSpec.Name << " is not an initial value equation." << std::endl);
	}
	if(EqSpec.Unit != Spec.Unit)
	{
		MOBIUS_FATAL_ERROR("ERROR: The equation " << EqSpec.Name << " has a different unit from the parameter " << Spec.Name << ", that it is trying to compute." << std::endl);
	}
	
	Spec.IsComputedBy = Equation;
	Spec.ShouldNotBeExposed = ShouldNotBeExposed;
}

inline void
ParameterIsComputedBy(mobius_model *Model, parameter_uint_h Parameter, equation_h Equation, bool ShouldNotBeExposed = true)
{
	parameter_spec &Spec  = Model->ParameterSpecs[Parameter.Handle];
	equation_spec &EqSpec = Model->EquationSpecs[Equation.Handle];
	if(EqSpec.Type != EquationType_InitialValue)
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried to set the equation " << EqSpec.Name << " to compute the parameter " << Spec.Name << ", but " << EqSpec.Name << " is not an initial value equation." << std::endl);
	}
	if(EqSpec.Unit != Spec.Unit)
	{
		MOBIUS_FATAL_ERROR("ERROR: The equation " << EqSpec.Name << " has a different unit from the parameter " << Spec.Name << ", that it is trying to compute." << std::endl);
	}
	
	Spec.IsComputedBy = Equation;
	Spec.ShouldNotBeExposed = ShouldNotBeExposed;
}

inline void
SetEquation(mobius_model *Model, equation_h Equation, mobius_equation EquationBody, bool Override = false)
{
	//REGISTRATION_BLOCK(Model) //NOTE: We can't use REGISTRATION_BLOCK since the user don't call the SetEquation explicitly, it is called through the macro EQUATION, and so they would not understand the error message.
	if(Model->Finalized)
	{
		MOBIUS_FATAL_ERROR("ERROR: You can not define an EQUATION body for the model after it has been finalized using EndModelDefinition." << std::endl);
	}
	
	if(!Override && Model->EquationSpecs[Equation.Handle].EquationIsSet)
	{
		MOBIUS_FATAL_ERROR("ERROR: The equation body for " << GetName(Model, Equation) << " is already defined. It can not be defined twice unless it is explicitly overridden." << std::endl);
	}
	
	Model->Equations[Equation.Handle] = EquationBody;

	Model->EquationSpecs[Equation.Handle].EquationIsSet = true;
}

static equation_h
RegisterEquation(mobius_model *Model, const char *Name, unit_h Unit, equation_type Type = EquationType_Basic)
{
	REGISTRATION_BLOCK(Model)
	
	equation_h Equation = {};
	REGISTER_MODEL_ENTITY(Model, Equation, Equation, Name)
	
	if(Model->Equations.size() <= Equation.Handle)
	{
		Model->Equations.resize(Equation.Handle + 1, {});
	}
	
	Model->EquationSpecs[Equation.Handle].Type = Type;
	Model->EquationSpecs[Equation.Handle].Unit = Unit;
	
	return Equation;
}

inline equation_h
RegisterEquationODE(mobius_model *Model, const char *Name, unit_h Unit)
{
	REGISTRATION_BLOCK(Model)
	
	return RegisterEquation(Model, Name, Unit, EquationType_ODE);
}

inline equation_h
RegisterEquationInitialValue(mobius_model *Model, const char *Name, unit_h Unit)
{
	REGISTRATION_BLOCK(Model)
	
	return RegisterEquation(Model, Name, Unit, EquationType_InitialValue);
}

//NOTE: CumulateResult is implemented in mobius_data_set.cpp
static double CumulateResult(mobius_data_set *DataSet, equation_h Result, index_set_h CumulateOverIndexSet, index_t *CurrentIndexes, double *LookupBase);
static double CumulateResult(mobius_data_set *DataSet, equation_h Result, index_set_h CumulateOverIndexSet, index_t *CurrentIndexes, double *LookupBase, parameter_double_h Weight);

inline equation_h
RegisterEquationCumulative(mobius_model *Model, const char *Name, equation_h Cumulates, index_set_h CumulatesOverIndexSet, parameter_double_h Weight = {})
{
	REGISTRATION_BLOCK(Model)
	
	equation_spec &CumulateSpec = Model->EquationSpecs[Cumulates.Handle];
	if(CumulateSpec.Type == EquationType_InitialValue)
	{
		MOBIUS_FATAL_ERROR("ERROR: The cumulation equation " << Name << " was set to cumulate an initial value equation (" << CumulateSpec.Name << "). This is not supported." << std::endl);
	}
	
	unit_h Unit = Model->EquationSpecs[Cumulates.Handle].Unit;
	equation_h Equation = RegisterEquation(Model, Name, Unit, EquationType_Cumulative);
	Model->EquationSpecs[Equation.Handle].CumulatesOverIndexSet = CumulatesOverIndexSet;
	Model->EquationSpecs[Equation.Handle].Cumulates = Cumulates;
	Model->EquationSpecs[Equation.Handle].CumulationWeight = Weight;
	
	if(IsValid(Weight))
	{
		SetEquation(Model, Equation,
			[Cumulates, CumulatesOverIndexSet, Weight] (value_set_accessor *ValueSet) -> double
			{
				return CumulateResult(ValueSet->DataSet, Cumulates, CumulatesOverIndexSet, ValueSet->CurrentIndexes, ValueSet->AllCurResultsBase, Weight);
			}
		);
	}
	else
	{
		SetEquation(Model, Equation,
			[Cumulates, CumulatesOverIndexSet] (value_set_accessor *ValueSet) -> double
			{
				return CumulateResult(ValueSet->DataSet, Cumulates, CumulatesOverIndexSet, ValueSet->CurrentIndexes, ValueSet->AllCurResultsBase);
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
	
	Model->EquationSpecs[Equation.Handle].InitialValue = InitialValue;
	
	if(Model->EquationSpecs[Equation.Handle].Unit != Model->ParameterSpecs[InitialValue.Handle].Unit)
	{
		std::cout << "WARNING: The equation " << GetName(Model, Equation) << " was registered with a different unit than its initial value parameter " << GetName(Model, InitialValue) << std::endl;
	}
}

inline void
SetInitialValue(mobius_model *Model, equation_h Equation, double Value)
{
	REGISTRATION_BLOCK(Model)
	
	Model->EquationSpecs[Equation.Handle].ExplicitInitialValue = Value;
	Model->EquationSpecs[Equation.Handle].HasExplicitInitialValue = true;
}

inline void
SetInitialValue(mobius_model *Model, equation_h Equation, equation_h InitialValueEquation)
{
	REGISTRATION_BLOCK(Model)
	if(Model->EquationSpecs[InitialValueEquation.Handle].Type != EquationType_InitialValue)
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried to set the equation " << GetName(Model, InitialValueEquation) << " as an initial value of another equation, but it was not registered as an equation of type EquationInitialValue." << std::endl);
	}
	
	Model->EquationSpecs[Equation.Handle].InitialValueEquation = InitialValueEquation;
	
	if(Model->EquationSpecs[Equation.Handle].Unit != Model->EquationSpecs[InitialValueEquation.Handle].Unit)
	{
		std::cout << "WARNING: The equation " << GetName(Model, Equation) << " was registered with a different unit than its initial value equation " << GetName(Model, InitialValueEquation) << std::endl;
	}
}

inline void
ResetEveryTimestep(mobius_model *Model, equation_h Equation)
{
	REGISTRATION_BLOCK(Model)
	
	equation_spec &Spec = Model->EquationSpecs[Equation.Handle];
	
	if(Spec.Type != EquationType_ODE)
	{
		MOBIUS_FATAL_ERROR("ERROR: Called ResetEveryTimestep on the equation " << Spec.Name << ", but this functionality is only available for ODE equations." << std::endl);
	}
	
	Spec.ResetEveryTimestep = true;
}

#define MOBIUS_SOLVER_SETUP_FUNCTION(Name) void Name(solver_spec *SolverSpec)
typedef MOBIUS_SOLVER_SETUP_FUNCTION(mobius_solver_setup_function);

static solver_h
RegisterSolver(mobius_model *Model, const char *Name, double h, mobius_solver_setup_function *SetupFunction)
{
	REGISTRATION_BLOCK(Model)
	
	solver_h Solver = {};
	REGISTER_MODEL_ENTITY(Model, Solver, Solver, Name)
	
	if(h <= 0.0 || h > 1.0)
	{
		MOBIUS_FATAL_ERROR("ERROR: The timestep of the solver " << Name << " can not be smaller than 0.0 or larger than 1.0" << std::endl);
	}
	
	solver_spec &Spec = Model->SolverSpecs[Solver.Handle];
	
	SetupFunction(&Spec);
	
	Spec.h = h;
	
	return Solver;
}

static solver_h
RegisterSolver(mobius_model *Model, const char *Name, double h, mobius_solver_setup_function *SetupFunction, double RelErr, double AbsErr)
{
	solver_h Solver = RegisterSolver(Model, Name, h, SetupFunction);
	
	solver_spec &Spec = Model->SolverSpecs[Solver.Handle];
	
	if(!Spec.UsesErrorControl)
	{
		std::cout << "WARNING: Registered error tolerances with the solver " << Name << ", but the attached solver function does not support error control." << std::endl;
	}
	
	Spec.RelErr = RelErr;
	Spec.AbsErr = AbsErr;
	
	return Solver;
}

static void
SetSolver(mobius_model *Model, equation_h Equation, solver_h Solver)
{
	REGISTRATION_BLOCK(Model)
	
	equation_type Type = Model->EquationSpecs[Equation.Handle].Type;
	if(Type != EquationType_Basic && Type != EquationType_ODE)
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried to set a solver for the equation " << GetName(Model, Equation) << ", but it is not a basic equation or ODE equation, and so can not be given a solver." << std::endl);
	}
	Model->EquationSpecs[Equation.Handle].Solver = Solver;
}

inline void
AddInputIndexSetDependency(mobius_model *Model, input_h Input, index_set_h IndexSet)
{
	REGISTRATION_BLOCK(Model)
	
	input_spec &Spec = Model->InputSpecs[Input.Handle];
	Spec.IndexSetDependencies.push_back(IndexSet);
}


#undef REGISTER_MODEL_ENTITY
#undef REGISTRATION_BLOCK





////////////////////////////////////////////////
// All of the below are value accessors for use in EQUATION bodies (ONLY)!
////////////////////////////////////////////////



/////////// IMPORTANT NOTE: ////////////////////
// ##__VA_ARGS__ with double hashes to eliminate superfluous commas may not be supported on all compilers. The alternative __VA_OPT__(,) is not supported until C++20
// Try to detect if this does not work so that the user does not get a barrage of unintelligible error messages?
////////////////////////////////////////////////

////////// IMPORTANT NOTE: /////////////////////
// It may actually be pretty bad to return unsigned ints to the user because it is very easy to get integer underflows when using them. Is there any
// reason not to use signed integers as a parameter type instead of unsigned? (One could just set the min value to 0 if one wants to).
////////////////////////////////////////////////

#define PARAMETER(ParH, ...) (ValueSet__->Running ? GetCurrentParameter(ValueSet__, ParH, ##__VA_ARGS__) : RegisterParameterDependency(ValueSet__, ParH, ##__VA_ARGS__))
#define INPUT(InputH) (ValueSet__->Running ? GetCurrentInput(ValueSet__, InputH) : RegisterInputDependency(ValueSet__, InputH))
#define RESULT(ResultH, ...) (ValueSet__->Running ? GetCurrentResult(ValueSet__, ResultH, ##__VA_ARGS__) : RegisterResultDependency(ValueSet__, ResultH, ##__VA_ARGS__))
#define LAST_RESULT(ResultH, ...) (ValueSet__->Running ? GetLastResult(ValueSet__, ResultH, ##__VA_ARGS__) : RegisterLastResultDependency(ValueSet__, ResultH, ##__VA_ARGS__))
#define EARLIER_RESULT(ResultH, StepBack, ...) (ValueSet__->Running ? GetEarlierResult(ValueSet__, ResultH, (StepBack), ##__VA_ARGS__) : RegisterLastResultDependency(ValueSet__, ResultH, ##__VA_ARGS__))
#define INPUT_WAS_PROVIDED(InputH) (ValueSet__->Running ? GetIfInputWasProvided(ValueSet__, InputH) : RegisterInputDependency(ValueSet__, InputH))
#define IF_INPUT_ELSE_PARAMETER(InputH, ParameterH) (ValueSet__->Running ? GetCurrentInputOrParameter(ValueSet__, InputH, ParameterH) : RegisterInputAndParameterDependency(ValueSet__, InputH, ParameterH))


#define CURRENT_DAY_OF_YEAR() (ValueSet__->DayOfYear)
#define DAYS_THIS_YEAR() (ValueSet__->DaysThisYear)

#define EQUATION(Model, ResultH, Def) \
SetEquation(Model, ResultH, \
 [=] (value_set_accessor *ValueSet__) { \
 Def \
 } \
);

#define EQUATION_OVERRIDE(Model, ResultH, Def) \
SetEquation(Model, ResultH, \
 [=] (value_set_accessor *ValueSet__) { \
 Def \
 } \
 , true \
);


//NOTE: These inline functions are used for type safety, which we don't get from macros.
//NOTE: We don't provide direct access to Time parameters since we want to encapsulate their storage. Instead we have accessor macros like CURRENT_DAY_OF_YEAR.
inline double
GetCurrentParameter(value_set_accessor *ValueSet, parameter_double_h Parameter)
{
	return ValueSet->CurParameters[Parameter.Handle].ValDouble;
}

inline u64
GetCurrentParameter(value_set_accessor *ValueSet, parameter_uint_h Parameter)
{
	return ValueSet->CurParameters[Parameter.Handle].ValUInt;
}

inline bool
GetCurrentParameter(value_set_accessor *ValueSet, parameter_bool_h Parameter)
{
	return ValueSet->CurParameters[Parameter.Handle].ValBool;
}

//NOTE: This does NOT do error checking to see if we provided too many override indexes or if they were out of bounds! We could maybe add an option to do that
// that is compile-out-able?
size_t OffsetForHandle(storage_structure &Structure, const index_t* CurrentIndexes, const index_t *IndexCounts, const index_t *OverrideIndexes, size_t OverrideCount, entity_handle Handle);
//size_t OffsetForHandle(storage_structure &Structure, const index_t *CurrentIndexes, const index_t *IndexCounts, entity_handle Handle);


template<typename... T> double
GetCurrentParameter(value_set_accessor *ValueSet, parameter_double_h Parameter, T... Indexes)
{
	mobius_data_set *DataSet = ValueSet->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};
	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, Parameter.Handle);
	return DataSet->ParameterData[Offset].ValDouble;
}

template<typename... T> u64
GetCurrentParameter(value_set_accessor *ValueSet, parameter_uint_h Parameter, T... Indexes)
{
	mobius_data_set *DataSet = ValueSet->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};
	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, Parameter.Handle);
	return DataSet->ParameterData[Offset].ValUInt;
}

template<typename... T> bool
GetCurrentParameter(value_set_accessor *ValueSet, parameter_bool_h Parameter, T... Indexes)
{
	mobius_data_set *DataSet = ValueSet->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};
	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, Parameter.Handle);
	return DataSet->ParameterData[Offset].ValBool;
}


inline double
GetCurrentResult(value_set_accessor *ValueSet, equation_h Result)
{
	return ValueSet->CurResults[Result.Handle];
}

inline double 
GetLastResult(value_set_accessor *ValueSet, equation_h LastResult)
{
	return ValueSet->LastResults[LastResult.Handle];
}

template<typename... T> double
GetCurrentResult(value_set_accessor *ValueSet, equation_h Result, T... Indexes)
{
	mobius_data_set *DataSet = ValueSet->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};
	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, Result.Handle);
	return ValueSet->AllCurResultsBase[Offset];
}

template<typename... T> double
GetLastResult(value_set_accessor *ValueSet, equation_h Result, T... Indexes)
{
	mobius_data_set *DataSet = ValueSet->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};

	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, Result.Handle);
	return ValueSet->AllLastResultsBase[Offset];
}

template<typename... T> double
GetEarlierResult(value_set_accessor *ValueSet, equation_h Result, u64 StepBack, T...Indexes)
{
	mobius_data_set *DataSet = ValueSet->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};
	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, Result.Handle);
	
	//TODO: Make proper accessor for this that belongs to mobius_data_set.cpp so that this file does not need to have knowledge of the inner workings of the storage system.
	double *Initial = DataSet->ResultData + Offset;
	//NOTE: Initial points to the initial value (adding TotalCount once gives us timestep 0)
	if(StepBack > ValueSet->Timestep)
	{
		return *Initial;
	}
	return *(Initial + ( (ValueSet->Timestep+1) - StepBack)*(ValueSet->DataSet->ResultStorageStructure.TotalCount));
}

inline double
GetCurrentInput(value_set_accessor *ValueSet, input_h Input)
{
	return ValueSet->CurInputs[Input.Handle];
}


inline bool
GetIfInputWasProvided(value_set_accessor * ValueSet, input_h Input)
{
	return ValueSet->CurInputWasProvided[Input.Handle];
}

inline double
GetCurrentInputOrParameter(value_set_accessor *ValueSet, input_h Input, parameter_double_h Parameter)
{
	if(GetIfInputWasProvided(ValueSet, Input)) return GetCurrentInput(ValueSet, Input);
	return GetCurrentParameter(ValueSet, Parameter);
}




template<typename... T> double
RegisterParameterDependency(value_set_accessor *ValueSet, parameter_double_h Parameter, T... Indexes)
{
	size_t OverrideCount = sizeof...(Indexes);
	ValueSet->ParameterDependencies.push_back({Parameter.Handle, OverrideCount});
	
	return 0.0;
}

template<typename... T> double
RegisterParameterDependency(value_set_accessor *ValueSet, parameter_uint_h Parameter, T... Indexes)
{
	size_t OverrideCount = sizeof...(Indexes);
	ValueSet->ParameterDependencies.push_back({Parameter.Handle, OverrideCount});
	
	return 0.0;
}

template<typename... T> double
RegisterParameterDependency(value_set_accessor *ValueSet, parameter_bool_h Parameter, T... Indexes)
{
	size_t OverrideCount = sizeof...(Indexes);
	ValueSet->ParameterDependencies.push_back({Parameter.Handle, OverrideCount});
	
	return 0.0;
}

inline double
RegisterInputDependency(value_set_accessor *ValueSet, input_h Input)
{
	ValueSet->InputDependencies.push_back({Input.Handle, 0});
	return 0.0;
}

inline double
RegisterInputAndParameterDependency(value_set_accessor *ValueSet, input_h Input, parameter_double_h Parameter)
{
	RegisterInputDependency(ValueSet, Input);
	RegisterParameterDependency(ValueSet, Parameter);

	return 0.0;
}

template<typename... T> double
RegisterResultDependency(value_set_accessor *ValueSet, equation_h Result, T... Indexes)
{
	std::vector<index_t> IndexVec = {Indexes...};
	ValueSet->ResultDependencies.push_back({Result.Handle, IndexVec});
	
	return 0.0;
}

template<typename... T> double
RegisterLastResultDependency(value_set_accessor *ValueSet, equation_h Result, T... Indexes)
{
	std::vector<index_t> IndexVec = {Indexes...};
	ValueSet->LastResultDependencies.push_back({Result.Handle, IndexVec});
	
	return 0.0;
}

//TODO: SET_RESULT is not that nice, and can interfere with how the dependency system works if used incorrectly. It was only included to get Persist to work. However, Persist should probably be rewritten to remove that necessity.
#define SET_RESULT(ResultH, Value, ...) {if(ValueSet__->Running){SetResult(ValueSet__, Value, ResultH, ##__VA_ARGS__);}}

template<typename... T> void
SetResult(value_set_accessor *ValueSet, double Value, equation_h Result, T... Indexes)
{
	mobius_data_set *DataSet = ValueSet->DataSet;
	const size_t OverrideCount = sizeof...(Indexes);
	index_t OverrideIndexes[OverrideCount] = {Indexes...};
	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, OverrideIndexes, OverrideCount, Result.Handle);
	ValueSet->AllCurResultsBase[Offset] = Value;
}


#define INDEX_COUNT(IndexSetH) (ValueSet__->Running ? (ValueSet__->DataSet->IndexCounts[IndexSetH.Handle]) : 1)
#define CURRENT_INDEX(IndexSetH) (ValueSet__->Running ? GetCurrentIndex(ValueSet__, IndexSetH) : RegisterIndexSetDependency(ValueSet__, IndexSetH))
#define FIRST_INDEX(IndexSetH) (index_t(IndexSetH, 0))
#define INDEX_NUMBER(IndexSetH, Index) (index_t(IndexSetH, (u32)Index))
#define INPUT_COUNT(IndexSetH) (ValueSet__->Running ? GetInputCount(ValueSet__, IndexSetH) : 0)

inline index_t
RegisterIndexSetDependency(value_set_accessor *ValueSet, index_set_h IndexSet)
{
	ValueSet->DirectIndexSetDependencies.push_back(IndexSet);
	return index_t(IndexSet, 0);
}

inline index_t
GetCurrentIndex(value_set_accessor *ValueSet, index_set_h IndexSet)
{
	return ValueSet->CurrentIndexes[IndexSet.Handle];
}

inline size_t
GetInputCount(value_set_accessor *ValueSet, index_set_h IndexSet)
{
	index_t Current = GetCurrentIndex(ValueSet, IndexSet);
	return ValueSet->DataSet->BranchInputs[IndexSet.Handle][Current].Count;
}


//TODO: The branch input iterator is more complicated than it needs to be. Could be redesigned.

inline size_t
BranchInputIteratorEnd(value_set_accessor *ValueSet, index_set_h IndexSet, index_t Branch)
{
	return ValueSet->Running ? ValueSet->DataSet->BranchInputs[IndexSet.Handle][Branch].Count : 1; //NOTE: The count is always one greater than the largest index, so it is guaranteed to be invalid.
}

struct branch_input_iterator
{
	index_t *InputIndexes;
	size_t CurrentInputIndexIndex;
	
	void operator++() { CurrentInputIndexIndex++; }
	
	const index_t& operator*(){ return InputIndexes[CurrentInputIndexIndex]; }

	bool operator!=(const size_t& Idx) { return CurrentInputIndexIndex != Idx; }
	
};

inline branch_input_iterator
BranchInputIteratorBegin(value_set_accessor *ValueSet, index_set_h IndexSet, index_t Branch)
{
	static index_t DummyData;
	DummyData = index_t(IndexSet, 0);
	
	branch_input_iterator Iterator;
	Iterator.InputIndexes = ValueSet->Running ? ValueSet->DataSet->BranchInputs[IndexSet.Handle][Branch].Inputs : &DummyData;
	Iterator.CurrentInputIndexIndex = 0;
	return Iterator;
}

#define BRANCH_INPUT_BEGIN(IndexSetH) (BranchInputIteratorBegin(ValueSet__, IndexSetH, CURRENT_INDEX(IndexSetH)))
#define BRANCH_INPUT_END(IndexSetH) (BranchInputIteratorEnd(ValueSet__, IndexSetH, CURRENT_INDEX(IndexSetH)))

#define FOREACH_INPUT(IndexSetH, Body) \
for(auto Input = BRANCH_INPUT_BEGIN(IndexSetH); Input != BRANCH_INPUT_END(IndexSetH); ++Input) \
{ \
Body \
}

#define MOBIUS_MODEL_H
#endif
