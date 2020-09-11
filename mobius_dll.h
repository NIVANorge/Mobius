
#include <sstream>

static int Dll_GlobalErrorCode = 0;
std::stringstream Dll_GlobalErrstream;

static int Dll_GlobalWarningCode = 0;
std::stringstream Dll_GlobalWarningStream;

void
ErrorPrint() {}

template<typename t, typename... v>
void
ErrorPrint(t Value, v... Tail)
{
	Dll_GlobalErrorCode = 1;
	Dll_GlobalErrstream << Value;
	ErrorPrint(Tail...);
}

template<typename... v>
void
FatalError(v... Tail)
{
	ErrorPrint(Tail...);
	throw 1;
}	

void
WarningPrint() {}

template<typename t, typename... v>
void
WarningPrint(t Value, v... Tail)
{
	Dll_GlobalWarningCode = 1;
	Dll_GlobalWarningStream << Value;
	WarningPrint(Tail...);
}

#define MOBIUS_ERROR_OVERRIDE	

#include "mobius.h"


#define CHECK_ERROR_BEGIN \
try {

#define CHECK_ERROR_END \
} catch(int Errcode) { \
}

#if (defined(_WIN32) || defined(_WIN64))
	#define DLLEXPORT extern "C" __declspec(dllexport)
#elif (defined(__unix__) || defined(__linux__) || defined(__unix) || defined(unix))
	#define DLLEXPORT extern "C" __attribute((visibility("default")))
#endif

DLLEXPORT int
DllEncounteredError(char *ErrmsgOut)
{
	std::string ErrStr = Dll_GlobalErrstream.str();
	strcpy(ErrmsgOut, ErrStr.c_str());
	
	int Code = Dll_GlobalErrorCode;
	
	Dll_GlobalErrorCode = 0;
	Dll_GlobalErrstream.str(std::string());
	
	return Code;
}

DLLEXPORT int
DllEncounteredWarning(char *WarningmsgOut)
{
	std::string WarnStr = Dll_GlobalWarningStream.str();
	strcpy(WarningmsgOut, WarnStr.c_str());
	
	int Code = Dll_GlobalWarningCode;
	
	Dll_GlobalWarningCode = 0;
	Dll_GlobalWarningStream.str(std::string());
	
	return Code;
}

//This one has to be provided in each separate application
mobius_model *
DllBuildModel();

DLLEXPORT void *
DllSetupModel(char *ParameterFilename, char *InputFilename)
{
	CHECK_ERROR_BEGIN
	
	mobius_model *Model = DllBuildModel();
	
	ReadInputDependenciesFromFile(Model, InputFilename);
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
	
	ReadParametersFromFile(DataSet, ParameterFilename);
	ReadInputsFromFile(DataSet, InputFilename);
	
	return (void *)DataSet;
	
	CHECK_ERROR_END
	
	return nullptr;
}

DLLEXPORT void *
DllSetupModelBlankIndexSets(char *InputFilename)
{
	CHECK_ERROR_BEGIN
	
	mobius_model *Model = DllBuildModel();
	ReadInputDependenciesFromFile(Model, InputFilename);
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
	
	return (void *)DataSet;
	
	CHECK_ERROR_END
	
	return nullptr;
}

DLLEXPORT void
DllReadInputs(void *DataSetPtr, char *InputFilename)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	ReadInputsFromFile(DataSet, InputFilename);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllReadParameters(void *DataSetPtr, char *ParameterFilename)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	ReadParametersFromFile(DataSet, ParameterFilename);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllSetIndexes(void *DataSetPtr, char *IndexSetName, u64 IndexCount, char **IndexNames)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	std::vector<token_string> IndexNames2;
	IndexNames2.reserve(IndexCount);
	for(size_t Idx = 0; Idx < IndexCount; ++Idx) IndexNames2.push_back(token_string(IndexNames[Idx]));
	
	SetIndexes(DataSet, token_string(IndexSetName), IndexNames2);
	
	if(DataSet->AllIndexesHaveBeenSet)
	{
		AllocateParameterStorage(DataSet);
	}
	
	CHECK_ERROR_END
}

struct dll_branch_index
{
	char *IndexName;
	u64 BranchCount;
	char **BranchNames;
};
DLLEXPORT void
DllSetBranchIndexes(void *DataSetPtr, char *IndexSetName, u64 IndexCount, dll_branch_index *Indexes)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	std::vector<std::pair<token_string, std::vector<token_string>>> Inputs;
	Inputs.reserve(IndexCount);
	for(size_t Idx = 0; Idx < IndexCount; ++Idx)
	{
		dll_branch_index &Data = Indexes[Idx];
		std::pair<token_string, std::vector<token_string>> Branches;
		Branches.first = token_string(Data.IndexName);
		Branches.second.reserve(Data.BranchCount);
		for(size_t Bch = 0; Bch < Data.BranchCount; ++Bch) Branches.second.push_back(token_string(Data.BranchNames[Bch]));
		Inputs.push_back(Branches);
	}
	
	SetBranchIndexes(DataSet, token_string(IndexSetName), Inputs);
	
	if(DataSet->AllIndexesHaveBeenSet)
	{
		AllocateParameterStorage(DataSet);
	}
	
	CHECK_ERROR_END
}

DLLEXPORT const char *
DllGetModelName(void *DataSetPtr)
{
	return ((mobius_data_set *)DataSetPtr)->Model->Name;
}

DLLEXPORT void
DllRunModel(void *DataSetPtr)
{
	CHECK_ERROR_BEGIN
	
	RunModel((mobius_data_set *)DataSetPtr);
	
	CHECK_ERROR_END
}

DLLEXPORT void *
DllCopyDataSet(void *DataSetPtr, bool CopyResults)
{
	CHECK_ERROR_BEGIN
	
	return (void *)CopyDataSet((mobius_data_set *)DataSetPtr, CopyResults);
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllDeleteDataSet(void *DataSetPtr)
{
	CHECK_ERROR_BEGIN
	
	delete (mobius_data_set *)DataSetPtr;
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllDeleteModelAndDataSet(void *DataSetPtr)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	mobius_model *Model = (mobius_model *)DataSet->Model;
	
	//NOTE: Unfortunately it seems like the dataset destructor has to look up stuff in the model, so the dataset must always be destroyed first.
	delete DataSet;
	delete Model;
	
	CHECK_ERROR_END
}


DLLEXPORT u64
DllGetTimesteps(void *DataSetPtr)
{
	//This returns the amount of timesteps the last time the model was run.
	CHECK_ERROR_BEGIN
	
	return ((mobius_data_set *)DataSetPtr)->TimestepsLastRun;
	
	CHECK_ERROR_END
	
	return 0;
}


DLLEXPORT u64
DllGetNextTimesteps(void *DataSetPtr)
{
	//This returns the amount of timesteps set for the next run
	CHECK_ERROR_BEGIN
	
	return GetTimesteps((mobius_data_set *)DataSetPtr);
	
	CHECK_ERROR_END
	
	return 0;
}


DLLEXPORT void
DllGetStartDate(void *DataSetPtr, char *WriteTo)
{
	CHECK_ERROR_BEGIN
	//IMPORTANT: This assumes that WriteTo is a char* buffer that is long enough (i.e. at least 30-ish bytes)
	//IMPORTANT: This is NOT thread safe since TimeString is not thread safe.
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	
	char *TimeStr = DataSet->StartDateLastRun.ToString();
	strcpy(WriteTo, TimeStr);
	
	CHECK_ERROR_END
}

DLLEXPORT timestep_size
DllGetTimestepSize(void *DataSetPtr)
{
	CHECK_ERROR_BEGIN
	
	return ((mobius_data_set *) DataSetPtr)->Model->TimestepSize;
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetInputTimesteps(void *DataSetPtr)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	return DataSet->InputDataTimesteps;
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetInputStartDate(void *DataSetPtr, char *WriteTo)
{
	CHECK_ERROR_BEGIN
	//IMPORTANT: This assumes that WriteTo is a char* buffer that is long enough (i.e. at least 30-ish bytes)
	//IMPORTANT: This is NOT thread safe since TimeString is not thread safe.
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	
	char *TimeStr = GetInputStartDate(DataSet).ToString();
	strcpy(WriteTo, TimeStr);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllGetResultSeries(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, double *WriteTo)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	
	size_t Timesteps = DataSet->TimestepsLastRun;
	
	GetResultSeries(DataSet, Name, IndexNames, (size_t)IndexCount, WriteTo, Timesteps);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllGetInputSeries(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, double *WriteTo, bool AlignWithResults)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	
	size_t Timesteps = GetTimesteps(DataSet);
	if(!AlignWithResults)
	{
		Timesteps = DataSet->InputDataTimesteps;
	}
	
	GetInputSeries(DataSet, Name, IndexNames, (size_t)IndexCount, WriteTo, Timesteps, AlignWithResults);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllSetParameterDouble(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, double Val)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	parameter_value Value;
	Value.ValDouble = Val;
	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_Double);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllSetParameterUInt(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, u64 Val)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	parameter_value Value;
	Value.ValUInt = Val;
	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_UInt);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllSetParameterBool(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, bool Val)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	parameter_value Value;
	Value.ValBool = Val;
	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_Bool);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllSetParameterTime(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, char *Val)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	parameter_value Value;
	bool ParseSuccess;
	Value.ValTime = datetime(Val, &ParseSuccess);
	if(!ParseSuccess)
		FatalError("ERROR: Unrecognized date/time format \"", Val, "\" provided for the value of the parameter ", Name, '\n');

	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_Time);
	
	CHECK_ERROR_END
}

DLLEXPORT double
DllGetParameterDouble(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount)
{
	CHECK_ERROR_BEGIN
	
	return GetParameterValue((mobius_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, ParameterType_Double).ValDouble;
	
	CHECK_ERROR_END
	
	return 0.0;
}

DLLEXPORT u64
DllGetParameterUInt(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount)
{
	CHECK_ERROR_BEGIN
	
	return GetParameterValue((mobius_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, ParameterType_UInt).ValUInt;
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT bool
DllGetParameterBool(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount)
{
	CHECK_ERROR_BEGIN
	
	return GetParameterValue((mobius_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, ParameterType_Bool).ValBool;
	
	CHECK_ERROR_END
	
	return false;
}

DLLEXPORT void
DllGetParameterTime(void *DataSetPtr, const char *Name, char **IndexNames, u64 IndexCount, char *WriteTo)
{
	//IMPORTANT: This assumes that WriteTo is a char* buffer that is long enough (i.e. at least 30-ish bytes)
	//IMPORTANT: This is NOT thread safe since datetime::ToString is not thread safe.
	CHECK_ERROR_BEGIN
	
	datetime DateTime = GetParameterValue((mobius_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, ParameterType_Time).ValTime;
	char *TimeStr = DateTime.ToString();
	strcpy(WriteTo, TimeStr);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllGetParameterDoubleMinMax(void *DataSetPtr, char *Name, double *MinOut, double *MaxOut)
{
	CHECK_ERROR_BEGIN
	//TODO: We don't check that the requested parameter was of type double!
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	const parameter_spec &Spec = DataSet->Model->Parameters[GetParameterHandle(DataSet->Model, Name)];
	if(Spec.Type != ParameterType_Double)
		FatalError("ERROR: Requested the min and max values of ", Name, " using DllGetParameterDoubleMinMax, but it is not of type double.\n");
	
	*MinOut = Spec.Min.ValDouble;
	*MaxOut = Spec.Max.ValDouble;
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllGetParameterUIntMinMax(void *DataSetPtr, char *Name, u64 *MinOut, u64 *MaxOut)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	const parameter_spec &Spec = DataSet->Model->Parameters[GetParameterHandle(DataSet->Model, Name)];
	if(Spec.Type != ParameterType_UInt)
		FatalError("ERROR: Requested the min and max values of ", Name, " using DllGetParameterUIntMinMax, but it is not of type uint.\n");

	*MinOut = Spec.Min.ValUInt;
	*MaxOut = Spec.Max.ValUInt;
	
	CHECK_ERROR_END
}

DLLEXPORT const char *
DllGetParameterDescription(void *DataSetPtr, char *Name)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	const parameter_spec &Spec = DataSet->Model->Parameters[GetParameterHandle(DataSet->Model, Name)];
	return Spec.Description;
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT const char *
DllGetParameterShortName(void *DataSetPtr, char *Name)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	const parameter_spec &Spec = DataSet->Model->Parameters[GetParameterHandle(DataSet->Model, Name)];
	return Spec.ShortName;
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT const char *
DllGetParameterUnit(void *DataSetPtr, char *Name)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	const parameter_spec &Spec = DataSet->Model->Parameters[GetParameterHandle(DataSet->Model, Name)];
	return GetName(DataSet->Model, Spec.Unit);
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT const char *
DllGetResultUnit(void *DataSetPtr, char *Name)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	const equation_spec &Spec = DataSet->Model->Equations[GetEquationHandle(DataSet->Model, Name)];
	return GetName(DataSet->Model, Spec.Unit);
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT const char *
DllGetInputUnit(void *DataSetPtr, char *Name)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	const input_spec &Spec = DataSet->Model->Inputs[GetInputHandle(DataSet->Model, Name)];
	if(IsValid(Spec.Unit))
		return GetName(DataSet->Model, Spec.Unit);
	else
		return "";
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllWriteParametersToFile(void *DataSetPtr, char *Filename)
{
	CHECK_ERROR_BEGIN
	
	WriteParametersToFile((mobius_data_set *)DataSetPtr, (const char *)Filename);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllSetInputSeries(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, double *InputData, u64 InputDataLength, bool AlignWithResults)
{
	CHECK_ERROR_BEGIN
	
	SetInputSeries((mobius_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, InputData, (size_t)InputDataLength, AlignWithResults);
	
	CHECK_ERROR_END
}


//TODO: Some of the following should be wrapped into accessors in mobius_data_set.cpp, because we are creating too many dependencies on implementation details here:

DLLEXPORT u64
DllGetIndexSetsCount(void *DataSetPtr)
{
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	return DataSet->Model->IndexSets.Count() - 1;
}

DLLEXPORT void
DllGetIndexSets(void *DataSetPtr, const char **NamesOut, const char **TypesOut)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	const mobius_model *Model = DataSet->Model;
	for(index_set_h IndexSet : Model->IndexSets)
	{
		const index_set_spec &Spec = Model->IndexSets[IndexSet];
		NamesOut[IndexSet.Handle - 1] = Spec.Name;
		TypesOut[IndexSet.Handle - 1] = Spec.Type == IndexSetType_Basic ? "basic" : "branched";
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetIndexCount(void *DataSetPtr, char *IndexSetName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	index_set_h IndexSet = GetIndexSetHandle(DataSet->Model, IndexSetName);
	return DataSet->IndexCounts[IndexSet.Handle];
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetIndexes(void *DataSetPtr, char *IndexSetName, const char **NamesOut)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	index_set_h IndexSet = GetIndexSetHandle(DataSet->Model, IndexSetName);
	for(size_t IdxIdx = 0; IdxIdx < DataSet->IndexCounts[IndexSet.Handle]; ++IdxIdx)
	{
		NamesOut[IdxIdx] = DataSet->IndexNames[IndexSet.Handle][IdxIdx];
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetParameterIndexSetsCount(void *DataSetPtr, char *ParameterName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	
	if(!DataSet->ParameterStorageStructure.HasBeenSetUp) return 0;
	
	parameter_h Parameter = GetParameterHandle(DataSet->Model, ParameterName);
	size_t UnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[Parameter.Handle];
	return DataSet->ParameterStorageStructure.Units[UnitIndex].IndexSets.Count;
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetParameterIndexSets(void *DataSetPtr, char *ParameterName, const char **NamesOut)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	
	if(!DataSet->ParameterStorageStructure.HasBeenSetUp) return;
	
	parameter_h Parameter = GetParameterHandle(DataSet->Model, ParameterName);
	size_t UnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[Parameter.Handle];
	size_t Idx = 0;
	for(index_set_h IndexSet : DataSet->ParameterStorageStructure.Units[UnitIndex].IndexSets)
	{
		NamesOut[Idx] = GetName(DataSet->Model, IndexSet);
		++Idx;
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetResultIndexSetsCount(void *DataSetPtr, char *ResultName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	
	if(!DataSet->ResultStorageStructure.HasBeenSetUp) return 0;
	
	equation_h Equation = GetEquationHandle(DataSet->Model, ResultName);
	size_t UnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Equation.Handle];
	return DataSet->ResultStorageStructure.Units[UnitIndex].IndexSets.Count;
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetResultIndexSets(void *DataSetPtr, char *ResultName, const char **NamesOut)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	
	if(!DataSet->ResultStorageStructure.HasBeenSetUp) return;
	
	equation_h Equation = GetEquationHandle(DataSet->Model, ResultName);
	size_t UnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Equation.Handle];
	size_t Idx = 0;
	for(index_set_h IndexSet : DataSet->ResultStorageStructure.Units[UnitIndex].IndexSets)
	{
		NamesOut[Idx] = GetName(DataSet->Model, IndexSet);
		++Idx;
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetInputIndexSetsCount(void *DataSetPtr, char *InputName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	
	if(!DataSet->InputStorageStructure.HasBeenSetUp) return 0;
	
	input_h Input = GetInputHandle(DataSet->Model, InputName);
	size_t UnitIndex = DataSet->InputStorageStructure.UnitForHandle[Input.Handle];
	return DataSet->InputStorageStructure.Units[UnitIndex].IndexSets.Count;
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetInputIndexSets(void *DataSetPtr, char *InputName, const char **NamesOut)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	
	if(!DataSet->InputStorageStructure.HasBeenSetUp) return;
	
	input_h Input = GetInputHandle(DataSet->Model, InputName);
	size_t UnitIndex = DataSet->InputStorageStructure.UnitForHandle[Input.Handle];
	size_t Idx = 0;
	for(index_set_h IndexSet : DataSet->InputStorageStructure.Units[UnitIndex].IndexSets)
	{
		NamesOut[Idx] = GetName(DataSet->Model, IndexSet);
		++Idx;
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetAllModulesCount(void *DataSetPtr)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	
	return DataSet->Model->Modules.Count()-1;
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetAllModules(void *DataSetPtr, const char **NamesOut, const char **VersionsOut)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	
	for(module_h Module : DataSet->Model->Modules)
	{
		NamesOut[Module.Handle-1] = DataSet->Model->Modules[Module].Name;
		VersionsOut[Module.Handle-1] = DataSet->Model->Modules[Module].Version;
	}
	
	CHECK_ERROR_END
}

DLLEXPORT const char *
DllGetModuleDescription(void *DataSetPtr, const char *ModuleName)
{
	CHECK_ERROR_BEGIN
	
	const mobius_model *Model = ((mobius_data_set *)DataSetPtr)->Model;
	return Model->Modules[GetModuleHandle(Model, ModuleName)].Description;
	
	CHECK_ERROR_END
	
	return nullptr;
}

DLLEXPORT bool
DllIsParameterGroupName(void *DataSetPtr, char *Name)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	
	return DataSet->Model->ParameterGroups.NameToHandle.find(Name) != DataSet->Model->ParameterGroups.NameToHandle.end();
	
	CHECK_ERROR_END
	
	return false;
}


DLLEXPORT u64
DllGetParameterGroupIndexSetsCount(void *DataSetPtr, char *ParameterGroupName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	parameter_group_h ParameterGroup = GetParameterGroupHandle(DataSet->Model, ParameterGroupName);
	
	return (u64)DataSet->Model->ParameterGroups[ParameterGroup].IndexSets.size();
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetParameterGroupIndexSets(void *DataSetPtr, char *ParameterGroupName, const char **NamesOut)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	parameter_group_h ParameterGroup = GetParameterGroupHandle(DataSet->Model, ParameterGroupName);
	
	const std::vector<index_set_h> &IndexSets = DataSet->Model->ParameterGroups[ParameterGroup].IndexSets;
	for(size_t Idx = 0; Idx < IndexSets.size(); ++Idx)
		NamesOut[Idx] = GetName(DataSet->Model, IndexSets[Idx]);
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetAllParameterGroupsCount(void *DataSetPtr, const char *ModuleName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	const mobius_model *Model = DataSet->Model;
	
	bool All = ModuleName && strcmp(ModuleName, "__all!!__") == 0;  //NOTE: Kind of stupid way to do it. Basically there are two default cases. Either we want groups not belonging to the top level or we want all parameter groups. Can't handle both using ModuleName==0
	
	module_h Module = {0};
	
	if(ModuleName && strlen(ModuleName) > 0 && !All)
		Module = GetModuleHandle(Model, ModuleName);
	
	u64 Count = 0;
	
	for(parameter_group_h ParameterGroup : Model->ParameterGroups)
	{
		const parameter_group_spec &Spec = Model->ParameterGroups[ParameterGroup];
		if(All || Module == Spec.Module) ++Count;
	}
	
	return Count;
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetAllParameterGroups(void *DataSetPtr, const char **NamesOut, const char *ModuleName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	const mobius_model *Model = DataSet->Model;
	
	bool All = ModuleName && strcmp(ModuleName, "__all!!__") == 0;
	
	module_h Module = {0};
	if(ModuleName && strlen(ModuleName) > 0 && !All)
		Module = GetModuleHandle(Model, ModuleName);
	
	size_t Idx = 0;
	for(parameter_group_h ParameterGroup : Model->ParameterGroups)
	{
		const parameter_group_spec &Spec = Model->ParameterGroups[ParameterGroup];
		if(All || Module == Spec.Module)
		{
			NamesOut[Idx] = Spec.Name;
			++Idx;
		}
	}
	
	CHECK_ERROR_END

}

DLLEXPORT u64
DllGetAllParametersCount(void *DataSetPtr, const char *GroupName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	const mobius_model *Model = DataSet->Model;
	
	parameter_group_h Group = {0};
	if(GroupName && strlen(GroupName) > 0)
		Group = GetParameterGroupHandle(Model, GroupName);
	
	u64 Count = 0;
	for(parameter_h Parameter : Model->Parameters)
	{
		const parameter_spec &Spec = Model->Parameters[Parameter];
		if(!Spec.ShouldNotBeExposed && (!IsValid(Group) || Group == Spec.Group)) ++Count;
	}
	
	return Count;
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetAllParameters(void *DataSetPtr, const char **NamesOut, const char **TypesOut, const char *GroupName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	const mobius_model *Model = DataSet->Model;
	
	parameter_group_h Group = {0};
	if(GroupName && strlen(GroupName) > 0)
		Group = GetParameterGroupHandle(Model, GroupName);
	
	size_t Idx = 0;
	for(parameter_h Parameter : Model->Parameters)
	{
		const parameter_spec &Spec = Model->Parameters[Parameter];
		if(!Spec.ShouldNotBeExposed && (!IsValid(Group) || Group == Spec.Group))
		{
			NamesOut[Idx] = Spec.Name;
			TypesOut[Idx] = GetParameterTypeName(Spec.Type);
			
			++Idx;
		}
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetAllResultsCount(void *DataSetPtr, const char *ModuleName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	
	bool All = ModuleName && strcmp(ModuleName, "__all!!__") == 0; //See comment on parameter groups above
	
	module_h Module = {};
	if(ModuleName && strlen(ModuleName) > 0 && !All)
		Module = GetModuleHandle(DataSet->Model, ModuleName);
	
	if(All)
		return (u64)(DataSet->Model->Equations.Count() - 1);
	else
	{
		u64 Count = 0;
		for(equation_h Equation : DataSet->Model->Equations)
			if(DataSet->Model->Equations[Equation].Module == Module) ++Count;
		return Count;
	}
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetAllResults(void *DataSetPtr, const char **NamesOut, const char **TypesOut, const char *ModuleName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	
	bool All = ModuleName && strcmp(ModuleName, "__all!!__") == 0;
	
	module_h Module = {};
	if(ModuleName && strlen(ModuleName) > 0 && !All)
		Module = GetModuleHandle(DataSet->Model, ModuleName);
	
	size_t Idx = 0;
	for(equation_h Equation : DataSet->Model->Equations)
	{
		const equation_spec &Spec = DataSet->Model->Equations[Equation];
		if(All || Spec.Module == Module)
		{
			NamesOut[Idx] = Spec.Name;
			TypesOut[Idx] = GetEquationTypeName(Spec.Type);
			++Idx;
		}
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetAllInputsCount(void *DataSetPtr)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	return (u64)(DataSet->Model->Inputs.Count() - 1);
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetAllInputs(void *DataSetPtr, const char **NamesOut, const char **TypesOut)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	for(input_h Input : DataSet->Model->Inputs)
	{
		const input_spec &Spec = DataSet->Model->Inputs[Input];
		NamesOut[Input.Handle - 1] = Spec.Name;
		TypesOut[Input.Handle - 1] = Spec.IsAdditional ? "additional" : "required";
	}
	
	CHECK_ERROR_END
}

DLLEXPORT bool
DllInputWasProvided(void *DataSetPtr, const char *Name, char **IndexNames, u64 IndexCount)
{
	CHECK_ERROR_BEGIN
	
	return InputSeriesWasProvided((mobius_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount);
	
	CHECK_ERROR_END
	
	return false;
}


DLLEXPORT bool
DllResultWasComputed(void *DataSetPtr, const char *Name, char **IndexNames, u64 IndexCount)
{
	//TODO!!!!!!!!!!!!!!
	
	CHECK_ERROR_BEGIN
	
	CHECK_ERROR_END
	
	return true;
}


DLLEXPORT u64
DllGetBranchInputsCount(void *DataSetPtr, const char *IndexSetName, const char *IndexName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	index_set_h IndexSet = GetIndexSetHandle(DataSet->Model, IndexSetName);
	
	index_set_type Type = DataSet->Model->IndexSets[IndexSet].Type;
	if(Type != IndexSetType_Branched)
		FatalError("ERROR: Tried to read branch inputs from the index set ", IndexSetName, ", but that is not a branched index set.\n");
	
	index_t Index = GetIndex(DataSet, IndexSet, IndexName);
	
	return (u64)DataSet->BranchInputs[IndexSet.Handle][Index].Count;
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetBranchInputs(void *DataSetPtr, const char *IndexSetName, const char *IndexName, const char **BranchInputsOut)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	index_set_h IndexSet = GetIndexSetHandle(DataSet->Model, IndexSetName);
	
	index_t Index = GetIndex(DataSet, IndexSet, IndexName);
	
	size_t Count = DataSet->BranchInputs[IndexSet.Handle][Index].Count;
	for(size_t Idx = 0; Idx < Count; ++Idx)
	{
		index_t IdxIdx = DataSet->BranchInputs[IndexSet.Handle][Index][Idx];
		BranchInputsOut[Idx] = DataSet->IndexNames[IndexSet.Handle][IdxIdx];
	}
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllPrintResultStructure(void *DataSetPtr, char *Buf, u64 BufLen)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	const mobius_model *Model = DataSet->Model;
	
	std::stringstream Out;
	PrintResultStructure(Model, Out);
	std::string Str = Out.str();
	strncpy(Buf, Str.c_str(), BufLen);
	
	CHECK_ERROR_END
}

