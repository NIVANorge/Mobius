

static mobius_data_set *
GenerateDataSet(mobius_model *Model)
{
	if(!Model->Finalized)
	{
		FatalError("ERROR: Attempted to generate a data set before the model was finalized using an EndModelDefinition call.\n");
	}
	mobius_data_set *DataSet = new mobius_data_set {};  //NOTE: The {} ensures that all member pointers are initialized to 0. This is important.
	
	DataSet->Model = Model;
	
	DataSet->BucketMemory.Initialize(1024*1024);
	
	DataSet->IndexCounts = DataSet->BucketMemory.Allocate<index_t>(Model->IndexSets.Count());
	DataSet->IndexCounts[0] = index_t({0}, 1);
	DataSet->IndexNames =  DataSet->BucketMemory.Allocate<const char **>(Model->IndexSets.Count());
	DataSet->IndexNamesToHandle.resize(Model->IndexSets.Count());
	
	DataSet->BranchInputs = DataSet->BucketMemory.Allocate<array<index_t> *>(Model->IndexSets.Count());
	
	if(Model->IndexSets.Count() == 1) // NOTE: In case there are no index sets, all index sets have had their indexes set.
	{
		DataSet->AllIndexesHaveBeenSet = true;
	}
	
	DataSet->ParameterStorageStructure.Model = Model;
	DataSet->InputStorageStructure.Model     = Model;
	DataSet->ResultStorageStructure.Model    = Model;
	
	DataSet->TimestepsLastRun = 0;
	
	return DataSet;
}

mobius_data_set::~mobius_data_set()
{
	if(ParameterData) free(ParameterData);
	if(InputData) free(InputData);
	if(ResultData) free(ResultData);
	
	BucketMemory.DeallocateAll();
}

template<typename handle_type>
static void
CopyStorageStructure(const storage_structure<handle_type> *Source, storage_structure<handle_type> *Dest, bucket_allocator *BucketMemory)
{
	//NOTE: Copy is not nested, so we still have to copy each array entry.
	Dest->Units = Source->Units.Copy(BucketMemory);
	for(size_t UnitIndex = 0; UnitIndex < Dest->Units.Count; ++UnitIndex)
	{
		Dest->Units[UnitIndex].IndexSets = Source->Units[UnitIndex].IndexSets.Copy(BucketMemory);
		Dest->Units[UnitIndex].Handles   = Source->Units[UnitIndex].Handles.Copy(BucketMemory);
	}
	
	Dest->TotalCountForUnit      = Source->TotalCountForUnit.Copy(BucketMemory);
	Dest->OffsetForUnit          = Source->OffsetForUnit.Copy(BucketMemory);
	Dest->UnitForHandle          = Source->UnitForHandle.Copy(BucketMemory);
	Dest->LocationOfHandleInUnit = Source->LocationOfHandleInUnit.Copy(BucketMemory);
	Dest->TotalCount             = Source->TotalCount;
	Dest->HasBeenSetUp           = Source->HasBeenSetUp;
	
	Dest->Model                  = Source->Model;
}

static mobius_data_set *
CopyDataSet(mobius_data_set *DataSet, bool CopyResults = false)
{
	const mobius_model *Model = DataSet->Model;
	
	mobius_data_set *Copy = new mobius_data_set {};
	
	Copy->Model = Model;
	
	Copy->BucketMemory.Initialize(1024*1024);
	
	if(DataSet->ParameterData) Copy->ParameterData = CopyArray(parameter_value, DataSet->ParameterStorageStructure.TotalCount, DataSet->ParameterData);
	CopyStorageStructure(&DataSet->ParameterStorageStructure, &Copy->ParameterStorageStructure, &Copy->BucketMemory);
	
	if(DataSet->InputData)
		Copy->InputData = CopyArray(double, DataSet->InputStorageStructure.TotalCount * DataSet->InputDataTimesteps, DataSet->InputData);
	
	else Copy->InputData = nullptr; //Should not be necessary...
	CopyStorageStructure(&DataSet->InputStorageStructure, &Copy->InputStorageStructure, &Copy->BucketMemory);
	Copy->InputDataStartDate = DataSet->InputDataStartDate;
	Copy->InputDataHasSeparateStartDate = DataSet->InputDataHasSeparateStartDate;
	Copy->InputDataTimesteps = DataSet->InputDataTimesteps;
	
	if(DataSet->InputTimeseriesWasProvided) Copy->InputTimeseriesWasProvided = Copy->BucketMemory.Copy(DataSet->InputTimeseriesWasProvided, DataSet->InputStorageStructure.TotalCount);
	
	if(CopyResults)
	{
		if(DataSet->ResultData) Copy->ResultData = CopyArray(double, DataSet->ResultStorageStructure.TotalCount * (DataSet->TimestepsLastRun + 1), DataSet->ResultData);
		CopyStorageStructure(&DataSet->ResultStorageStructure, &Copy->ResultStorageStructure, &Copy->BucketMemory);
		Copy->TimestepsLastRun = DataSet->TimestepsLastRun;
		Copy->StartDateLastRun = DataSet->StartDateLastRun;
		Copy->HasBeenRun = DataSet->HasBeenRun;
	}
	else
		Copy->HasBeenRun = false;
	
	if(DataSet->IndexCounts) Copy->IndexCounts = Copy->BucketMemory.Copy(DataSet->IndexCounts, Model->IndexSets.Count());
	
	if(DataSet->IndexNames)
	{
		Copy->IndexNames = Copy->BucketMemory.Allocate<const char **>(Model->IndexSets.Count());
		for(index_set_h IndexSet : Model->IndexSets)
		{
			if(DataSet->IndexNames[IndexSet.Handle])
			{
				Copy->IndexNames[IndexSet.Handle] = Copy->BucketMemory.Allocate<const char *>(DataSet->IndexCounts[IndexSet.Handle]);
				for(index_t Index = {IndexSet.Handle, 0}; Index < DataSet->IndexCounts[IndexSet.Handle]; ++Index)
				{
					Copy->IndexNames[IndexSet.Handle][Index] = Copy->BucketMemory.CopyString(DataSet->IndexNames[IndexSet.Handle][Index]);
				}
			}
		}
	}
	Copy->IndexNamesToHandle = DataSet->IndexNamesToHandle;
	Copy->AllIndexesHaveBeenSet = DataSet->AllIndexesHaveBeenSet;
	
	if(DataSet->BranchInputs)
	{
		Copy->BranchInputs = Copy->BucketMemory.Allocate<array<index_t> *>(Model->IndexSets.Count());
		for(index_set_h IndexSet : Model->IndexSets)
		{
			if(DataSet->BranchInputs[IndexSet.Handle])
			{
				Copy->BranchInputs[IndexSet.Handle] = Copy->BucketMemory.Allocate<array<index_t>>(DataSet->IndexCounts[IndexSet.Handle]);
				for(index_t Index = {IndexSet.Handle, 0}; Index < DataSet->IndexCounts[IndexSet.Handle]; ++Index)
				{
					Copy->BranchInputs[IndexSet.Handle][Index] = DataSet->BranchInputs[IndexSet.Handle][Index].Copy(&DataSet->BucketMemory);
				}
			}
		}
	}
	
	return Copy;
}

template<typename handle_type>
static void
SetupStorageStructureSpecifer(storage_structure<handle_type> *Structure, index_t *IndexCounts, size_t TotalHandleCount, bucket_allocator *BucketMemory)
{
	size_t UnitCount = Structure->Units.Count;
	Structure->TotalCountForUnit.Allocate(BucketMemory, UnitCount);
	Structure->OffsetForUnit.Allocate(BucketMemory, UnitCount);
	Structure->UnitForHandle.Allocate(BucketMemory, TotalHandleCount);
	Structure->LocationOfHandleInUnit.Allocate(BucketMemory, TotalHandleCount);
	Structure->TotalCount = 0;
	
	size_t UnitIndex = 0;
	size_t OffsetForUnitSoFar = 0;
	for(storage_unit_specifier<handle_type> &Unit : Structure->Units)
	{
		Structure->TotalCountForUnit[UnitIndex] = Unit.Handles.Count;
		for(index_set_h IndexSet : Unit.IndexSets)
			Structure->TotalCountForUnit[UnitIndex] *= IndexCounts[IndexSet.Handle];
		
		size_t HandleIdx = 0;
		for(handle_type Handle : Unit.Handles)
		{
			Structure->UnitForHandle[Handle.Handle] = UnitIndex;
			Structure->LocationOfHandleInUnit[Handle.Handle] = HandleIdx;
			++HandleIdx;
		}
		
		Structure->OffsetForUnit[UnitIndex] = OffsetForUnitSoFar;
		OffsetForUnitSoFar += Structure->TotalCountForUnit[UnitIndex];
		Structure->TotalCount += Structure->TotalCountForUnit[UnitIndex];
		++UnitIndex;
	}
	
	Structure->HasBeenSetUp = true;
}



//NOTE: The following functions are very similar, but it would incur a performance penalty to try to merge them.
//NOTE: The OffsetForHandle functions are meant to be internal functions that can be used by other wrappers that do more error checking.


// NOTE: Returns the storage index of the first instance of a value corresponding to this Handle, i.e where all indexes that this handle depends on are the first index of their index set.
template<typename handle_type>
inline size_t
OffsetForHandle(storage_structure<handle_type> &Structure, handle_type Handle)
{
	size_t UnitIndex = Structure.UnitForHandle[Handle.Handle];
	size_t OffsetForUnit = Structure.OffsetForUnit[UnitIndex];
	size_t LocationOfHandleInUnit = Structure.LocationOfHandleInUnit[Handle.Handle];
	
	return OffsetForUnit + LocationOfHandleInUnit;
}


#if !defined(MOBIUS_INDEX_BOUNDS_TESTS)
#define MOBIUS_INDEX_BOUNDS_TESTS 0
#endif

template<typename handle_type> const char *
GetHandleTypeName() { FatalError("INTERNAL ERROR: Tried to look up the name of an unknown type\n."); return ""; }
template<> const char *
GetHandleTypeName<parameter_h>() { return "Parameter"; }
template<> const char *
GetHandleTypeName<input_h>() { return "Input"; }
template<> const char *
GetHandleTypeName<equation_h>() { return "Equation"; }



template<typename handle_type>
inline void
CheckIndexErrors(const mobius_model *Model, index_set_h IndexSet, index_t Index, size_t Count, handle_type Handle)
{
	bool CountError = (Index >= Count);
	bool HandleError = (Index.IndexSetHandle != IndexSet.Handle);
	if(CountError || HandleError)
	{
		const char *EntityName = GetName(Model, Handle);
		const char *TypeName   = GetHandleTypeName<handle_type>();
		if(CountError)
			ErrorPrint("ERROR: Index out of bounds for index set \"", GetName(Model, IndexSet), "\", got index ", Index, ", count was ", Count, '\n');
		if(HandleError)
			ErrorPrint("ERROR: Used an index addressed to the index set \"", GetName(Model, index_set_h {Index.IndexSetHandle}), "\" for indexing the index set \"", GetName(Model, IndexSet), "\".\n");
		FatalError("This happened while looking up the value of the ", TypeName, " \"", EntityName, "\".\n");
	}
}

// NOTE: Returns the storage index of a value corresponding to this Handle with the given index set indexes.
// CurrentIndexes must be set up so that for any index set with handle IndexSetHandle, CurrentIndexes[IndexSetHandle] is the current index of that index set. (Typically RunState->CurrentIndexes)
// IndexCounts    must be set up so that for any index set with handle IndexSetHandle, IndexCounts[IndexSetHandle] is the index count of that index set. (Typically DataSet->IndexCounts)
template<typename handle_type>
inline size_t
OffsetForHandle(storage_structure<handle_type> &Structure, const index_t *CurrentIndexes, const index_t *IndexCounts, handle_type Handle)
{
	size_t UnitIndex = Structure.UnitForHandle[Handle.Handle];
	storage_unit_specifier<handle_type> &Specifier = Structure.Units[UnitIndex];
	size_t NumHandlesInUnitInstance = Specifier.Handles.Count;
	size_t OffsetForUnit = Structure.OffsetForUnit[UnitIndex];
	size_t LocationOfHandleInUnit = Structure.LocationOfHandleInUnit[Handle.Handle];
	
	size_t InstanceOffset = 0;
	for(index_set_h IndexSet : Specifier.IndexSets)
	{
		size_t Count = IndexCounts[IndexSet.Handle];
		index_t Index = CurrentIndexes[IndexSet.Handle];
		
#if MOBIUS_INDEX_BOUNDS_TESTS
		CheckIndexErrors(Structure.Model, IndexSet, Index, Count, Handle);
#endif
		
		InstanceOffset = InstanceOffset * Count + Index;
	}
	return OffsetForUnit + InstanceOffset * NumHandlesInUnitInstance + LocationOfHandleInUnit;
}



// NOTE: Returns the storage index of a value corresponding to this Handle with the given index set indexes.
// Indexes must be set up so that Indexes[I] is the index of the I'th index set that the entity one wishes to look up depends on.
// IndexesCount is the number of index sets the entity depends on (and so the length of the array Indexes).
// IndexCounts    must be set up so that for any index set with handle IndexSetHandle, IndexCounts[IndexSetHandle] is the index count of that index set. (Typically DataSet->IndexCounts)
template<typename handle_type>
inline size_t
OffsetForHandle(storage_structure<handle_type> &Structure, const index_t *Indexes, size_t IndexesCount, const index_t *IndexCounts, handle_type Handle)
{
	size_t UnitIndex = Structure.UnitForHandle[Handle.Handle];
	storage_unit_specifier<handle_type> &Specifier = Structure.Units[UnitIndex];
	size_t NumHandlesInUnitInstance = Specifier.Handles.Count;
	size_t OffsetForUnit = Structure.OffsetForUnit[UnitIndex];
	size_t LocationOfHandleInUnit = Structure.LocationOfHandleInUnit[Handle.Handle];
	
	size_t InstanceOffset = 0;
	size_t Level = 0;
	for(index_set_h IndexSet : Specifier.IndexSets)
	{
		size_t Count = IndexCounts[IndexSet.Handle];
		index_t Index = Indexes[Level];
		
#if MOBIUS_INDEX_BOUNDS_TESTS
		CheckIndexErrors(Structure.Model, IndexSet, Index, Count, Handle);
#endif
		
		InstanceOffset = InstanceOffset * Count + Index;
		
		++Level;
	}
	return OffsetForUnit + InstanceOffset * NumHandlesInUnitInstance + LocationOfHandleInUnit;
}

// NOTE: Returns the storage index of a value corresponding to this Handle with the given index set indexes.
// CurrentIndexes must be set up so that for any index set with handle IndexSetHandle, CurrentIndexes[IndexSetHandle] is the current index of that index set. (Typically ValueSet->CurrentIndexes)
// IndexCounts    must be set up so that for any index set with handle IndexSetHandle, IndexCounts[IndexSetHandle] is the index count of that index set. (Typically DataSet->IndexCounts)
// OverrideCount  specifies how many of the last index sets one wants to override the indexes of
// OverrideIndexes provides indexes for the overridden index sets.
//
// Example. If Handle is a parameter handle to a parameter depending on the index sets IndexSetA, IndexSetB, IndexSetC, IndexSetD in that order, then if
//	CurrentIndexes[IndexSetA.Handle]=0, CurrentIndexes[IndexSetB.Handle]=1, CurrentIndexes[IndexSetC.Handle]=2, CurrentIndexes[IndexSetD.Handle]=3,
//  OverrideCount = 2, OverrideIndexes = {5, 6},
// then this function returns the storage index of the parameter value corresponding to this Handle, and with indexes [0, 1, 5, 6]
//
// This function is designed to be used by the system for explicit indexing of lookup values, such as when one uses access macros like "PARAMETER(MyParameter, Index1, Index2)" etc. inside equations.
template<typename handle_type>
inline size_t
OffsetForHandle(storage_structure<handle_type> &Structure, const index_t* CurrentIndexes, const index_t *IndexCounts, const index_t *OverrideIndexes, size_t OverrideCount, handle_type Handle)
{
	size_t UnitIndex = Structure.UnitForHandle[Handle.Handle];
	storage_unit_specifier<handle_type> &Specifier = Structure.Units[UnitIndex];
	size_t NumHandlesInUnitInstance = Specifier.Handles.Count;
	size_t OffsetForUnit = Structure.OffsetForUnit[UnitIndex];
	size_t LocationOfHandleInUnit = Structure.LocationOfHandleInUnit[Handle.Handle];
	
	size_t IndexSetCount = Specifier.IndexSets.Count;
	size_t InstanceOffset = 0;
	size_t IndexSetLevel = 0;
	for(index_set_h IndexSet : Specifier.IndexSets)
	{
		size_t Count = IndexCounts[IndexSet.Handle];
		index_t Index;
		if(IndexSetLevel < (IndexSetCount - OverrideCount))
			Index = CurrentIndexes[IndexSet.Handle];
		else
			Index = OverrideIndexes[IndexSetLevel + (OverrideCount - IndexSetCount)];
		
#if MOBIUS_INDEX_BOUNDS_TESTS
		CheckIndexErrors(Structure.Model, IndexSet, Index, Count, Handle);
#endif
		
		InstanceOffset = InstanceOffset * Count + Index;
		++IndexSetLevel;
	}
	return OffsetForUnit + InstanceOffset * NumHandlesInUnitInstance + LocationOfHandleInUnit;
}

//NOTE: Returns the storage index of a value corresponding to this Handle with the given index set indexes.
// CurrentIndexes must be set up so that for any index set with handle IndexSetHandle, CurrentIndexes[IndexSetHandle] is the current index of that index set. (Typically ValueSet->CurrentIndexes)
// IndexCounts    must be set up so that for any index set with handle IndexSetHandle, IndexCounts[IndexSetHandle] is the index count of that index set. (Typically DataSet->IndexCounts)
// Skip           is an index set that one wants to be set to its first index.
// SubsequentOffset is a second output value, which will contain the distance between each instance of this value if one were to change the index in Skip.
//
// Example: MyParameter depends on IndexSetA, IndexSetB
// IndexCount[IndexSetA.Handle] = 3, IndexCount[IndexSetB.Handle] = 2. CurrentIndexes[IndexSetA.Handle] = 2, CurrentIndexes[IndexSetB.Handle] = 1.
// size_t SubsequentOffset;
// size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, IndexSetA, SubsequentOffset, MyParameter.Handle);
// DataSet->ParameterData[Offset] is the value of MyParameter with indexes {0, 1};
// DataSet->ParameterData[Offset] + Idx*SubsequentOffset is the value of MyParameter with indexes {Idx, 1};
//
// This function is designed to be used with the system that evaluates cumulation equations.
template<typename handle_type>
static size_t
OffsetForHandle(storage_structure<handle_type> &Structure, index_t *CurrentIndexes, index_t *IndexCounts, index_set_h Skip, size_t& SubsequentOffset, handle_type Handle)
{
	size_t UnitIndex = Structure.UnitForHandle[Handle.Handle];
	storage_unit_specifier<handle_type> &Specifier = Structure.Units[UnitIndex];
	size_t NumHandlesInUnitInstance = Specifier.Handles.Count;
	size_t OffsetForUnit = Structure.OffsetForUnit[UnitIndex];
	size_t LocationOfHandleInUnit = Structure.LocationOfHandleInUnit[Handle.Handle];
	
	size_t InstanceOffset = 0;
	SubsequentOffset = 1;
	bool Skipped = false;
	for(index_set_h IndexSet : Specifier.IndexSets)
	{
		size_t Count = IndexCounts[IndexSet.Handle];
		index_t Index = CurrentIndexes[IndexSet.Handle];
		if(Skipped)
			SubsequentOffset *= Count;
		if(IndexSet == Skip)
		{
			Index = {IndexSet, 0};
			Skipped = true;
		}

#if MOBIUS_INDEX_BOUNDS_TESTS
		CheckIndexErrors(Structure.Model, IndexSet, Index, Count, Handle);
#endif
		
		InstanceOffset = InstanceOffset * Count + Index;
	}
	SubsequentOffset *= NumHandlesInUnitInstance;
	
	return OffsetForUnit + InstanceOffset * NumHandlesInUnitInstance + LocationOfHandleInUnit;
}

static void
SetMultipleValuesForParameter(mobius_data_set *DataSet, parameter_h Parameter, parameter_value *Values, size_t Count)
{
	//NOTE: There are almost no safety checks in this function. The caller of the function is responsible for the checks!
	// It was designed to be used with the default text input for parameters. If you want to use it for anything else, you should make sure you understand how it works.
	
	size_t UnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[Parameter.Handle];	
	size_t Stride = DataSet->ParameterStorageStructure.Units[UnitIndex].Handles.Count;
	
	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, Parameter);
	parameter_value *Base = DataSet->ParameterData + Offset;
	
	for(size_t Idx = 0; Idx < Count; ++Idx)
	{
		*Base = Values[Idx];
		Base += Stride;
	}
}



static datetime
GetStartDate(mobius_data_set *DataSet)
{
	const mobius_model *Model = DataSet->Model;
	
	auto FindTime = Model->Parameters.NameToHandle.find("Start date");
	if(FindTime != Model->Parameters.NameToHandle.end())
	{
		parameter_h StartTimeHandle = FindTime->second;
		size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, StartTimeHandle);
		return DataSet->ParameterData[Offset].ValTime; //TODO: Check that it was actually registered with the correct type and that it does not have any index set dependencies.
	}
	
	return datetime(); //I.e. 1970-1-1
}

inline datetime
GetInputStartDate(mobius_data_set *DataSet)
{
	if(DataSet->InputDataHasSeparateStartDate)
	{
		return DataSet->InputDataStartDate;
	}
	return GetStartDate(DataSet);
}

static u64
GetTimesteps(mobius_data_set *DataSet)
{
	const mobius_model *Model = DataSet->Model;
	
	//TODO: Check that the parameter was actually registered with the correct type and that it does not have any index set dependencies.
	
	auto FindEndDate = Model->Parameters.NameToHandle.find("End date");
	if(FindEndDate != Model->Parameters.NameToHandle.end())
	{
		parameter_h EndDateHandle = FindEndDate->second;
		size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, EndDateHandle);
		datetime EndDate = DataSet->ParameterData[Offset].ValTime;
		datetime StartDate = GetStartDate(DataSet);
		
		s64 Step = FindTimestep(StartDate, EndDate, DataSet->Model->TimestepSize);
		Step += 1;    //NOTE: Because the end date is inclusive.
		
		if(Step <= 0)
			FatalError("The model run start date was set to be later than the model run end date.\n");
		
		return (u64)Step;
	}
		
		
	auto FindTimestep = Model->Parameters.NameToHandle.find("Timesteps");
	if(FindTimestep != Model->Parameters.NameToHandle.end())
	{
		parameter_h TimestepHandle = FindTimestep->second;
		size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, TimestepHandle);
		return DataSet->ParameterData[Offset].ValUInt; 
	}
	
	return 1;
}


static void
SetIndexes(mobius_data_set *DataSet, token_string IndexSetName, const std::vector<token_string>& IndexNames)
{
	const mobius_model *Model = DataSet->Model;
	
	index_set_h IndexSet = GetIndexSetHandle(DataSet->Model, IndexSetName);
	const index_set_spec &Spec = Model->IndexSets[IndexSet];
	
	if(Spec.Type != IndexSetType_Basic)
		FatalError("ERROR: Can not use the method SetIndexes for the index set ", Spec.Name, ", use a method that is specific to the type of that index set instead.\n");
	
	if(DataSet->IndexNames[IndexSet.Handle] != 0)
		FatalError("ERROR: Tried to set the indexes for the index set ", Spec.Name, " more than once.\n");
	
	if(IndexNames.empty())
		FatalError("ERROR: Tried to set indexes for the index set ", Spec.Name, ", but no indexes were provided.\n");
	
	if(!Spec.RequiredIndexes.empty())
	{
		bool Correct = true;
		if(Spec.RequiredIndexes.size() > IndexNames.size()) Correct = false;
		else
		{
			for(size_t IdxIdx = 0; IdxIdx < Spec.RequiredIndexes.size(); ++IdxIdx)
			{
				if(!IndexNames[IdxIdx].Equals(Spec.RequiredIndexes[IdxIdx]))
				{
					Correct = false;
					break;
				}
			}
		}

		if(!Correct)
		{
			ErrorPrint("ERROR: The model requires the following indexes to be the first indexes for the index set ", Spec.Name, ":\n");
			for(const char *IndexName :  Spec.RequiredIndexes)
				ErrorPrint("\"", IndexName, "\" ");
			
			ErrorPrint("\nin that order. We got the indexes:\n");
			for(token_string IndexName : IndexNames)
				ErrorPrint("\"", IndexName, "\" ");
			
			FatalError('\n');
		}
	}
	
	DataSet->IndexCounts[IndexSet.Handle] = {IndexSet.Handle, (u32)IndexNames.size()};
	DataSet->IndexNames[IndexSet.Handle] = DataSet->BucketMemory.Allocate<const char *>(IndexNames.size());
	
	for(size_t IndexIndex = 0; IndexIndex < IndexNames.size(); ++IndexIndex)
	{
		if(IndexNames[IndexIndex].Length == 0)
			FatalError("ERROR: Indexes can't be empty strings.\n");
		
		const char *IndexName = IndexNames[IndexIndex].Copy(&DataSet->BucketMemory).Data;
		DataSet->IndexNames[IndexSet.Handle][IndexIndex] = IndexName;
		DataSet->IndexNamesToHandle[IndexSet.Handle][IndexName] = IndexIndex;
	}
	
	if(DataSet->IndexNamesToHandle[IndexSet.Handle].size() != IndexNames.size())
		FatalError("ERROR: Got duplicate indexes for index set ", Spec.Name, '\n');
	
	bool AllSet = true;
	for(index_set_h IndexSet : Model->IndexSets)
	{
		if(DataSet->IndexCounts[IndexSet.Handle] == 0)
		{
			AllSet = false;
			break;
		}
	}
	DataSet->AllIndexesHaveBeenSet = AllSet;
}

static void
SetBranchIndexes(mobius_data_set *DataSet, token_string IndexSetName, const std::vector<std::pair<token_string, std::vector<token_string>>>& Inputs)
{
	const mobius_model *Model = DataSet->Model;
	
	index_set_h IndexSet = GetIndexSetHandle(Model, IndexSetName);
	const index_set_spec &Spec = Model->IndexSets[IndexSet];
	
	if(Spec.Type != IndexSetType_Branched)
		FatalError("ERROR: Can not use the method SetBranchIndexes for the index set \"", IndexSetName, "\", use a method that is specific to the type of that index set instead.\n");
	
	if(DataSet->IndexNames[IndexSet.Handle] != 0)
		FatalError("ERROR: Tried to set the indexes for the index set \"", Spec.Name, "\" more than once.\n");
	
	if(Inputs.empty())
		FatalError("ERROR: Tried to set indexes for the index set \"", Spec.Name, "\", but no indexes were provided.\n");
	
	DataSet->IndexCounts[IndexSet.Handle] = {IndexSet.Handle, (u32)Inputs.size()};
	DataSet->IndexNames[IndexSet.Handle] = DataSet->BucketMemory.Allocate<const char *>(Inputs.size());

	DataSet->BranchInputs[IndexSet.Handle] = DataSet->BucketMemory.Allocate<array<index_t>>(Inputs.size());
	index_t IndexIndex = {IndexSet.Handle, 0};
	for(const auto &InputData : Inputs)
	{
		if(InputData.first.Length == 0)
			FatalError("ERROR: Indexes can't be empty strings.\n");
		
		const char *IndexName = InputData.first.Copy(&DataSet->BucketMemory).Data;

		const std::vector<token_string> &InputNames = InputData.second;
		if(DataSet->IndexNamesToHandle[IndexSet.Handle].find(IndexName) != DataSet->IndexNamesToHandle[IndexSet.Handle].end())
			FatalError("ERROR: Got duplicate indexes for index set \"", IndexSetName, "\".\n");
		
		DataSet->IndexNamesToHandle[IndexSet.Handle][IndexName] = IndexIndex;
		DataSet->IndexNames[IndexSet.Handle][IndexIndex] = IndexName;
		
		DataSet->BranchInputs[IndexSet.Handle][IndexIndex].Allocate(&DataSet->BucketMemory, InputNames.size());
		
		index_t InputIdxIdx = {IndexSet.Handle, 0};
		for(token_string InputName : InputNames)
		{
			auto Find = DataSet->IndexNamesToHandle[IndexSet.Handle].find(InputName);
			if(Find == DataSet->IndexNamesToHandle[IndexSet.Handle].end())
			{
				FatalError("ERROR: The index \"", InputName, "\" appears as an input to the index \"", IndexName, "\" in the index set \"", IndexSetName, "\" before it itself is declared.\n");
			}
			index_t InputIndex = {IndexSet.Handle, Find->second};
			DataSet->BranchInputs[IndexSet.Handle][IndexIndex][InputIdxIdx] = InputIndex;
			++InputIdxIdx;
		}
		
		++IndexIndex;
	}
	
	bool AllSet = true;
	for(index_set_h IndexSet : DataSet->Model->IndexSets)
	{
		if(DataSet->IndexCounts[IndexSet.Handle] == 0)
		{
			AllSet = false;
			break;
		}
	}
	DataSet->AllIndexesHaveBeenSet = AllSet;
}

static void
ErrorPrintUnfilledIndexSets(mobius_data_set *DataSet)
{
	const mobius_model *Model = DataSet->Model;
	
	ErrorPrint("The following index sets are empty:");
	for(index_set_h IndexSet : Model->IndexSets)
	{
		if(DataSet->IndexCounts[IndexSet.Handle] == 0)
			ErrorPrint(" \"", GetName(Model, IndexSet), "\"");
	}
	FatalError("\n");
}

static void
AllocateParameterStorage(mobius_data_set *DataSet)
{
	const mobius_model *Model = DataSet->Model;
	
	if(!DataSet->AllIndexesHaveBeenSet)
	{
		ErrorPrint("ERROR: Tried to allocate parameter storage before all index sets were filled.\n");
		ErrorPrintUnfilledIndexSets(DataSet);
	}
	
	if(DataSet->ParameterData)
		FatalError("ERROR: Tried to allocate parameter storage twice.\n");
	
	std::map<std::vector<index_set_h>, std::vector<parameter_h>> TransposedParameterDependencies;
	for(parameter_h Parameter : Model->Parameters)
	{
		std::vector<index_set_h> Dependencies = Model->Parameters[Parameter].IndexSetDependencies;
		TransposedParameterDependencies[Dependencies].push_back(Parameter);
	}
	size_t ParameterStorageUnitCount = TransposedParameterDependencies.size();
	array<storage_unit_specifier<parameter_h>> &Units = DataSet->ParameterStorageStructure.Units;
	Units.Allocate(&DataSet->BucketMemory, ParameterStorageUnitCount);
	
	size_t UnitIndex = 0;
	for(auto& Structure : TransposedParameterDependencies)
	{
		Units[UnitIndex].IndexSets.CopyFrom(&DataSet->BucketMemory, Structure.first);
		Units[UnitIndex].Handles.CopyFrom(&DataSet->BucketMemory, Structure.second);
		++UnitIndex;
	}
	
	SetupStorageStructureSpecifer(&DataSet->ParameterStorageStructure, DataSet->IndexCounts, Model->Parameters.Count(), &DataSet->BucketMemory);
	
	DataSet->ParameterData = AllocClearedArray(parameter_value, DataSet->ParameterStorageStructure.TotalCount);
	
	//NOTE: Setting up default values.
	UnitIndex = 0;
	for(storage_unit_specifier<parameter_h>& Unit : Units)
	{
		size_t HandlesInInstance = Unit.Handles.Count;
		size_t TotalHandlesForUnit = DataSet->ParameterStorageStructure.TotalCountForUnit[UnitIndex];
		
		size_t ParameterIndex = 0;
		for(parameter_h Parameter : Unit.Handles)
		{
			parameter_value DefaultValue = Model->Parameters[Parameter].Default;
			size_t At = OffsetForHandle(DataSet->ParameterStorageStructure, Parameter);
			for(size_t Instance = 0; Instance < TotalHandlesForUnit; Instance += HandlesInInstance)
			{
				DataSet->ParameterData[At] = DefaultValue;
				At += HandlesInInstance;
			}
			++ParameterIndex;
		}
		
		++UnitIndex;
	}
}

static void
AllocateInputStorage(mobius_data_set *DataSet, u64 Timesteps)
{
	const mobius_model *Model = DataSet->Model;
	
	if(!DataSet->AllIndexesHaveBeenSet)
	{
		ErrorPrint("ERROR: Tried to allocate input storage before all index sets were filled.\n");
		ErrorPrintUnfilledIndexSets(DataSet);
	}
	
	if(DataSet->InputData)
		FatalError("ERROR: Tried to allocate input storage twice.\n");

	std::map<std::vector<index_set_h>, std::vector<input_h>> TransposedInputDependencies;
	
	for(input_h Input : Model->Inputs)
	{
		const input_spec &Spec = Model->Inputs[Input];
		TransposedInputDependencies[Spec.IndexSetDependencies].push_back(Input);
	}
	
	size_t InputStorageUnitCount = TransposedInputDependencies.size();
	array<storage_unit_specifier<input_h>> &Units = DataSet->InputStorageStructure.Units;
	Units.Allocate(&DataSet->BucketMemory, InputStorageUnitCount);
	size_t UnitIndex = 0;
	for(auto& Structure : TransposedInputDependencies)
	{
		Units[UnitIndex].IndexSets.CopyFrom(&DataSet->BucketMemory, Structure.first);
		Units[UnitIndex].Handles.CopyFrom(&DataSet->BucketMemory, Structure.second);
		
		++UnitIndex;
	}
	SetupStorageStructureSpecifer(&DataSet->InputStorageStructure, DataSet->IndexCounts, Model->Inputs.Count(), &DataSet->BucketMemory);

	
	DataSet->InputData = AllocClearedArray(double, DataSet->InputStorageStructure.TotalCount * Timesteps);
	DataSet->InputDataTimesteps = Timesteps;
	
	DataSet->InputTimeseriesWasProvided = DataSet->BucketMemory.Allocate<bool>(DataSet->InputStorageStructure.TotalCount);
}

static void
AllocateResultStorage(mobius_data_set *DataSet, u64 Timesteps)
{
	const mobius_model *Model = DataSet->Model;
	
	if(!DataSet->AllIndexesHaveBeenSet)
	{
		ErrorPrint("ERROR: Tried to allocate result storage before all index sets were filled.\n");
		ErrorPrintUnfilledIndexSets(DataSet);
	}
	
	//NOTE: We set up a storage structure for results that mirrors the equation batch group structure. This simplifies things a lot in other code.
	
	if(!DataSet->ResultStorageStructure.HasBeenSetUp) //If it was set up once before we don't need to set up the storage structure again.
	{
		size_t ResultStorageUnitCount = Model->BatchGroups.Count;
		array<storage_unit_specifier<equation_h>> &Units = DataSet->ResultStorageStructure.Units;
		Units.Allocate(&DataSet->BucketMemory, ResultStorageUnitCount);
		for(size_t UnitIndex = 0; UnitIndex < ResultStorageUnitCount; ++UnitIndex)
		{
			const equation_batch_group &BatchGroup = Model->BatchGroups[UnitIndex];
			storage_unit_specifier<equation_h> &Unit = Units[UnitIndex];
			Unit.IndexSets = BatchGroup.IndexSets.Copy(&DataSet->BucketMemory);
			
			std::vector<equation_h> Handles;
			for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
			{
				const equation_batch &Batch = Model->EquationBatches[BatchIdx];
				Handles.insert(Handles.end(), Batch.Equations.begin(), Batch.Equations.end());
				Handles.insert(Handles.end(), Batch.EquationsODE.begin(), Batch.EquationsODE.end());
			}
			
			Unit.Handles.CopyFrom(&DataSet->BucketMemory, Handles);
		}
		
		SetupStorageStructureSpecifer(&DataSet->ResultStorageStructure, DataSet->IndexCounts, Model->Equations.Count(), &DataSet->BucketMemory);
	}
	
	if(DataSet->ResultData && (Timesteps != DataSet->TimestepsLastRun))
	{
		//NOTE: We could realloc, but we need to clear it to 0 anyway, so there is probably not that much of a gain.
		free(DataSet->ResultData);
		DataSet->ResultData = nullptr;
	}
	
	//NOTE: We add 1 to Timesteps since we also need space for the initial values.
	size_t AllocationSize = DataSet->ResultStorageStructure.TotalCount * (Timesteps + 1);
	
	if(!DataSet->ResultData)
		DataSet->ResultData = AllocClearedArray(double, AllocationSize);
	else
		memset(DataSet->ResultData, 0, sizeof(double)*AllocationSize);
}



//NOTE: Returns the numeric index corresponding to an index name and an index_set.
inline index_t
GetIndex(mobius_data_set *DataSet, index_set_h IndexSet, token_string IndexName)
{
	auto &IndexMap = DataSet->IndexNamesToHandle[IndexSet.Handle];
	auto Find = IndexMap.find(IndexName);
	if(Find != IndexMap.end())
		return {IndexSet, Find->second};
	
	FatalError("ERROR: Tried the index name \"", IndexName, "\" with the index set \"", GetName(DataSet->Model, IndexSet), "\", but that index set does not contain that index.\n");
	return {IndexSet, 0};
}

inline index_t
GetIndex(mobius_data_set *DataSet, index_set_h IndexSet, token_string IndexName, bool &Success)
{
	auto &IndexMap = DataSet->IndexNamesToHandle[IndexSet.Handle];
	auto Find = IndexMap.find(IndexName);
	if(Find != IndexMap.end())
	{
		Success = true;
		return {IndexSet, Find->second};
	}
	Success = false;
	return {IndexSet, 0};
}

static void
SetParameterValue(mobius_data_set *DataSet, const char *Name, const char * const *Indexes, size_t IndexCount, parameter_value Value, parameter_type Type)
{
	if(!DataSet->AllIndexesHaveBeenSet)
	{
		ErrorPrint("ERROR: Tried to set a parameter value before all index sets have been filled with indexes.\n");
		ErrorPrintUnfilledIndexSets(DataSet);
	}

	if(DataSet->ParameterData == 0)
		AllocateParameterStorage(DataSet);
	
	const mobius_model *Model = DataSet->Model;
	parameter_h Parameter = GetParameterHandle(Model, Name);
	
	if(Model->Parameters[Parameter].Type != Type)
		FatalError("ERROR: Tried to set the value of the parameter \"", Name, "\" with a value that was of the wrong type.\n");
	
	//TODO: Check that the value is in the Min-Max range. (issue warning only)
	
	size_t StorageUnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[Parameter.Handle];
	array<index_set_h> &IndexSetDependencies = DataSet->ParameterStorageStructure.Units[StorageUnitIndex].IndexSets;
	
	if(IndexCount != IndexSetDependencies.Count)
		FatalError("ERROR; Tried to set the value of the parameter \"", Name, "\", but an incorrect number of indexes were provided. Got ", IndexCount, ", expected ", IndexSetDependencies.Count, ".\n");

	//TODO: This crashes if somebody have more than 256 index sets for a parameter, but that is highly unlikely. Still, this is not clean code...
	index_t IndexValues[256];
	for(size_t Level = 0; Level < IndexCount; ++Level)
		IndexValues[Level] = GetIndex(DataSet, IndexSetDependencies[Level], Indexes[Level]);
	
	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, IndexValues, IndexCount, DataSet->IndexCounts, Parameter);
	DataSet->ParameterData[Offset] = Value;
}

inline void
SetParameterValue(mobius_data_set *DataSet, const char *Name, const std::vector<const char *>& Indexes, double Value)
{
	parameter_value Val;
	Val.ValDouble = Value;
	SetParameterValue(DataSet, Name, Indexes.data(), Indexes.size(), Val, ParameterType_Double);
}

inline void
SetParameterValue(mobius_data_set *DataSet, const char *Name, const std::vector<const char *>& Indexes, u64 Value)
{
	parameter_value Val;
	Val.ValUInt = Value;
	SetParameterValue(DataSet, Name, Indexes.data(), Indexes.size(), Val, ParameterType_UInt);
}

inline void
SetParameterValue(mobius_data_set *DataSet, const char *Name, const std::vector<const char *>& Indexes, bool Value)
{
	parameter_value Val;
	Val.ValBool = Value;
	SetParameterValue(DataSet, Name, Indexes.data(), Indexes.size(), Val, ParameterType_Bool);
}

inline void
SetParameterValue(mobius_data_set *DataSet, const char *Name, const std::vector<const char *>& Indexes, const char *StrValue)
{
	if(!StrValue || !strlen(StrValue))
		FatalError("ERROR: Got a value string for the parameter \"", Name, "\" with length 0.\n");
		
	parameter_value Val;
	if(StrValue[0] >= '0' && StrValue[0] <= '9')
	{
		//Interpret this as a time value
		bool ParseSuccess;
		Val.ValTime = datetime(StrValue, &ParseSuccess);
		
		if(!ParseSuccess)
			FatalError("ERROR: Unrecognized date format when setting the value of the parameter \"", Name, "\".\n");
	}
	else
	{
		//Interpret this as a possible enum value
		const parameter_spec &Spec = DataSet->Model->Parameters[GetParameterHandle(DataSet->Model, Name)];
		auto Find = Spec.EnumNameToValue.find(StrValue);
		if(Find != Spec.EnumNameToValue.end())
			Val.ValUInt = Find->second;
		else
			FatalError("ERROR: The parameter \"", Name, "\" does not take the value \"", StrValue, "\".\n");
	}
	
	SetParameterValue(DataSet, Name, Indexes.data(), Indexes.size(), Val, ParameterType_Time);
}

static parameter_value
GetParameterValue(mobius_data_set *DataSet, const char *Name, const char * const *Indexes, size_t IndexCount, parameter_type Type)
{
	if(DataSet->ParameterData == 0)
		FatalError("ERROR: Tried to get a parameter value before parameter storage was allocated.\n");
	
	const mobius_model *Model = DataSet->Model;
	parameter_h Parameter = GetParameterHandle(Model, Name);
	
	if(Model->Parameters[Parameter].Type != Type)
		FatalError("ERROR: Tried to get the value of the parameter \"", Name, "\" specifying the wrong value type.\n");
	
	size_t StorageUnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[Parameter.Handle];
	array<index_set_h> &IndexSetDependencies = DataSet->ParameterStorageStructure.Units[StorageUnitIndex].IndexSets;
	
	if(IndexCount != IndexSetDependencies.Count)
		FatalError("ERROR; Tried to get the value of the parameter \"", Name, "\", but an incorrect number of indexes were provided. Got ",  IndexCount, ", expected ", IndexSetDependencies.Count, ".\n");

	//TODO: This crashes if somebody have more than 256 index sets for a parameter, but that is highly unlikely. Still, this is not clean code...
	index_t IndexValues[256];
	for(size_t Level = 0; Level < IndexCount; ++Level)
		IndexValues[Level] = GetIndex(DataSet, IndexSetDependencies[Level], Indexes[Level]);
	
	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, IndexValues, IndexCount, DataSet->IndexCounts, Parameter);
	return DataSet->ParameterData[Offset];
}

inline double
GetParameterDouble(mobius_data_set *DataSet, const char *Name, const std::vector<const char *>& Indexes)
{
	return GetParameterValue(DataSet, Name, Indexes.data(), Indexes.size(), ParameterType_Double).ValDouble;
}

inline u64
GetParameterUInt(mobius_data_set *DataSet, const char *Name, const std::vector<const char *>& Indexes)
{
	return GetParameterValue(DataSet, Name, Indexes.data(), Indexes.size(), ParameterType_UInt).ValUInt;
}

inline const char *
GetParameterEnum(mobius_data_set *DataSet, const char *Name, const std::vector<const char *>& Indexes)
{
	u64 Value = GetParameterValue(DataSet, Name, Indexes.data(), Indexes.size(), ParameterType_Enum).ValUInt;
	const parameter_spec &Spec = DataSet->Model->Parameters[GetParameterHandle(DataSet->Model, Name)];
	return Spec.EnumNames[Value];
}

inline bool
GetParameterBool(mobius_data_set *DataSet, const char  *Name, const std::vector<const char *>& Indexes)
{
	return GetParameterValue(DataSet, Name, Indexes.data(), Indexes.size(), ParameterType_Bool).ValBool;
}

//TODO: GetParameterTime? But what format should it use?


//NOTE: Is used by cumulation equations when they evaluate.
static double
CumulateResult(mobius_data_set *DataSet, equation_h Equation, index_set_h CumulateOverIndexSet, index_t *CurrentIndexes, double *LookupBase)
{
	//NOTE: LookupBase should always be DataSet->ResultData + N*DataSet->ResultStorageStructure.TotalCount. I.e. it should point to the beginning of one timestep.
	
	double Total = 0.0;
	
	size_t SubsequentOffset;
	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, CurrentIndexes, DataSet->IndexCounts, CumulateOverIndexSet, SubsequentOffset, Equation);
	
	double *Lookup = LookupBase + Offset;
	for(index_t Index = {CumulateOverIndexSet, 0}; Index < DataSet->IndexCounts[CumulateOverIndexSet.Handle]; ++Index)
	{
		Total += *Lookup;
		Lookup += SubsequentOffset;
	}
	
	return Total;
}

static double
CumulateResult(mobius_data_set *DataSet, equation_h Equation, index_set_h CumulateOverIndexSet, index_t *CurrentIndexes, double *LookupBase, parameter_double_h Weight)
{
	double Total = 0.0;
	double Total0 = 0.0;
	
	size_t SubsequentOffset;
	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, CurrentIndexes, DataSet->IndexCounts, CumulateOverIndexSet, SubsequentOffset, Equation);
	
	size_t ParSubsequentOffset;
	size_t ParOffset = OffsetForHandle(DataSet->ParameterStorageStructure, CurrentIndexes, DataSet->IndexCounts, CumulateOverIndexSet, ParSubsequentOffset, (parameter_h)Weight);
	
	double *Lookup = LookupBase + Offset;
	parameter_value *ParLookup = DataSet->ParameterData + ParOffset;
	
	ParLookup = DataSet->ParameterData + ParOffset;
	for(index_t Index = {CumulateOverIndexSet, 0}; Index < DataSet->IndexCounts[CumulateOverIndexSet.Handle]; ++Index)
	{
		double EquationValue = *Lookup;
		double ParValue = (*ParLookup).ValDouble;
		Total += EquationValue * ParValue;
		Total0 += ParValue;
		Lookup += SubsequentOffset;
		ParLookup += ParSubsequentOffset;
	}
	
	return Total / Total0;
}


//TODO: There is so much code doubling between input / result access. Could it be merged?
static void
SetInputSeries(mobius_data_set *DataSet, const char *Name, const char * const *IndexNames, size_t IndexCount, const double *InputSeries, size_t InputSeriesSize, bool AlignWithResults = false)
{
	if(!DataSet->InputData)
		AllocateInputStorage(DataSet, InputSeriesSize);
	
	const mobius_model *Model = DataSet->Model;
	
	input_h Input = GetInputHandle(Model, Name);
	
	size_t StorageUnitIndex = DataSet->InputStorageStructure.UnitForHandle[Input.Handle];
	array<index_set_h> &IndexSets = DataSet->InputStorageStructure.Units[StorageUnitIndex].IndexSets;
	
	if(IndexCount != IndexSets.Count)
		FatalError("ERROR: Got the wrong amount of indexes when setting the input series for \"", GetName(Model, Input), "\". Got ", IndexCount, ", expected ", IndexSets.Count, ".\n");
	
	index_t Indexes[256];
	for(size_t IdxIdx = 0; IdxIdx < IndexSets.Count; ++IdxIdx)
		Indexes[IdxIdx] = GetIndex(DataSet, IndexSets[IdxIdx], IndexNames[IdxIdx]);
	
	size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, Indexes, IndexCount, DataSet->IndexCounts, Input);
	double *At = DataSet->InputData + Offset;
	s64 TimestepOffset = 0;
	
	if(AlignWithResults && DataSet->InputDataHasSeparateStartDate)
	{
		//NOTE: In case the user asked for a input timeseries that starts at the start of the modelrun rather than at the start of the input series.
		datetime DataSetStartDate = GetStartDate(DataSet);
		datetime InputStartDate   = DataSet->InputDataStartDate;
		TimestepOffset = FindTimestep(InputStartDate, DataSetStartDate, Model->TimestepSize);
	}
	
	if(InputSeriesSize + TimestepOffset > DataSet->InputDataTimesteps)
		FatalError("ERROR: When setting input series for \"", Name, "\", the lenght of the time series was longer than what was allocated space for in the dataset.\n");
	
	for(size_t Idx = 0; Idx < DataSet->InputDataTimesteps; ++Idx)
	{
		if(Idx >= TimestepOffset && Idx <= InputSeriesSize + TimestepOffset)
			*At = InputSeries[Idx - TimestepOffset];
		else
			*At = std::numeric_limits<double>::quiet_NaN();
		At += DataSet->InputStorageStructure.TotalCount;
	}
	
	DataSet->InputTimeseriesWasProvided[Offset] = true;
}

inline void
SetInputSeries(mobius_data_set *DataSet, const char *Name, const std::vector<const char *> &IndexNames, const double *InputSeries, size_t InputSeriesSize, bool AlignWithResults = false)
{
	SetInputSeries(DataSet, Name, IndexNames.data(), IndexNames.size(), InputSeries, InputSeriesSize, AlignWithResults);
}

// NOTE: The caller of this function has to allocate the space that the result series should be written to and pass a pointer to it as WriteTo.
// Example:
// std::vector<double> MyResults;
// MyResults.resize(DataSet->TimestepsLastRun);
// GetResultSeries(DataSet, "Percolation input", {"Reach 1", "Forest", "Groundwater"}, MyResult.data(), MyResult.size());
static void
GetResultSeries(mobius_data_set *DataSet, const char *Name, const char* const* IndexNames, size_t IndexCount, double *WriteTo, size_t WriteSize)
{	
	if(!DataSet->HasBeenRun || !DataSet->ResultData)
		FatalError("ERROR: Tried to extract result series before the model was run at least once.\n");
	
	const mobius_model *Model = DataSet->Model;
	
	//TODO: If we ask for more values than we could get, should there not be an error?
	u64 NumToWrite = Min(WriteSize, DataSet->TimestepsLastRun);
	
	equation_h Equation = GetEquationHandle(Model, Name);
	
	const equation_spec &Spec = Model->Equations[Equation];
	if(Spec.Type == EquationType_InitialValue)
		FatalError("ERROR: Can not get the result series of the equation \"", Name, "\", because it is an initial value equation.\n");
	
	
	size_t StorageUnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Equation.Handle];
	array<index_set_h> &IndexSets = DataSet->ResultStorageStructure.Units[StorageUnitIndex].IndexSets;

	if(IndexCount != IndexSets.Count)
		FatalError("ERROR: Got the wrong amount of indexes when getting the result series for \"", Name, "\". Got ", IndexCount, ", expected ", IndexSets.Count, ".\n");
	
	index_t Indexes[256];
	for(size_t IdxIdx = 0; IdxIdx < IndexSets.Count; ++IdxIdx)
		Indexes[IdxIdx] = GetIndex(DataSet, IndexSets[IdxIdx], IndexNames[IdxIdx]);

	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, Indexes, IndexCount, DataSet->IndexCounts, Equation);
	double *Lookup = DataSet->ResultData + Offset;
	
	for(size_t Idx = 0; Idx < NumToWrite; ++Idx)
	{
		Lookup += DataSet->ResultStorageStructure.TotalCount; //NOTE: We do one initial time advancement before the lookup since the first set of values are just the initial values, which we want to skip.
		WriteTo[Idx] = *Lookup;
	}
}

inline void
GetResultSeries(mobius_data_set *DataSet, const char *Name, const std::vector<const char*> &IndexNames, double *WriteTo, size_t WriteSize)
{
	GetResultSeries(DataSet, Name, IndexNames.data(), IndexNames.size(), WriteTo, WriteSize);
}

static void
GetInputSeries(mobius_data_set *DataSet, const char *Name, const char * const *IndexNames, size_t IndexCount, double *WriteTo, size_t WriteSize, bool AlignWithResults = false)
{	
	if(!DataSet->InputData)
		FatalError("ERROR: Tried to extract input series before input data was allocated.\n");
	
	const mobius_model *Model = DataSet->Model;
	
	input_h Input = GetInputHandle(Model, Name);
	
	size_t StorageUnitIndex = DataSet->InputStorageStructure.UnitForHandle[Input.Handle];
	array<index_set_h> &IndexSets = DataSet->InputStorageStructure.Units[StorageUnitIndex].IndexSets;

	if(IndexCount != IndexSets.Count)
		FatalError("ERROR: Got the wrong amount of indexes when getting the input series for \"", Name, "\". Got ", IndexCount, ", expected ", IndexSets.Count, ".\n");

	index_t Indexes[256];
	for(size_t IdxIdx = 0; IdxIdx < IndexSets.Count; ++IdxIdx)
		Indexes[IdxIdx] = GetIndex(DataSet, IndexSets[IdxIdx], IndexNames[IdxIdx]);

	size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, Indexes, IndexCount, DataSet->IndexCounts, Input);
	double *Lookup = DataSet->InputData + Offset;
	
	s64 TimestepOffset = 0;
	if(AlignWithResults && DataSet->InputDataHasSeparateStartDate)
	{
		//NOTE: In case the user asked for a input timeseries that starts at the start of the modelrun rather than at the start of the input series.
		datetime DataSetStartDate = GetStartDate(DataSet);
		datetime InputStartDate   = DataSet->InputDataStartDate;
		TimestepOffset = FindTimestep(InputStartDate, DataSetStartDate, Model->TimestepSize);
		Lookup += TimestepOffset * DataSet->InputStorageStructure.TotalCount;
	}
	
	//TODO: If we ask for more values than we could get, should there not be an error?
	s64 NumToWrite = Min((s64)WriteSize, (s64)DataSet->InputDataTimesteps - TimestepOffset);
	NumToWrite = Max(0, NumToWrite);
	
	for(size_t Idx = 0; Idx < NumToWrite; ++Idx)
	{
		WriteTo[Idx] = *Lookup;
		Lookup += DataSet->InputStorageStructure.TotalCount;
	}
}

inline void
GetInputSeries(mobius_data_set *DataSet, const char *Name, const std::vector<const char*> &IndexNames, double *WriteTo, size_t WriteSize, bool AlignWithResults = false)
{
	GetInputSeries(DataSet, Name, IndexNames.data(), IndexNames.size(), WriteTo, WriteSize, AlignWithResults);
}

static bool
InputSeriesWasProvided(mobius_data_set *DataSet, const char *Name, const char * const *IndexNames, size_t IndexCount)
{
	if(!DataSet->InputData)
		FatalError("ERROR: Tried to see if an input series was provided before input data was even allocated.\n");
	
	const mobius_model *Model = DataSet->Model;
	
	input_h Input = GetInputHandle(Model, Name);
	
	size_t StorageUnitIndex = DataSet->InputStorageStructure.UnitForHandle[Input.Handle];
	array<index_set_h> &IndexSets = DataSet->InputStorageStructure.Units[StorageUnitIndex].IndexSets;

	if(IndexCount != IndexSets.Count)
		FatalError("ERROR: Got the wrong amount of indexes when checking the input series for \"", Name, "\". Got ", IndexCount, ", expected ", IndexSets.Count, ".\n");
	
	index_t Indexes[256];
	for(size_t IdxIdx = 0; IdxIdx < IndexSets.Count; ++IdxIdx)
		Indexes[IdxIdx] = GetIndex(DataSet, IndexSets[IdxIdx], IndexNames[IdxIdx]);
	
	size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, Indexes, IndexCount, DataSet->IndexCounts, Input);
	
	return DataSet->InputTimeseriesWasProvided[Offset];
}

inline bool
InputSeriesWasProvided(mobius_data_set *DataSet, const char *Name, const std::vector<const char*> &IndexNames)
{
	return InputSeriesWasProvided(DataSet, Name, IndexNames.data(), IndexNames.size());
}

inline bool
EquationWasComputed(mobius_data_set *DataSet, const char *Name, const char * const *IndexNames, size_t IndexCount)
{
	if(!DataSet->ResultData)
		return false;
	if(!DataSet->HasBeenRun)
		return false;
	
	const mobius_model *Model = DataSet->Model;
	
	equation_h Equation = GetEquationHandle(Model, Name);
	
	size_t StorageUnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Equation.Handle]; //NOTE: is the same as the batch group index.
	
	const equation_batch_group &BatchGroup = Model->BatchGroups[StorageUnitIndex];
	size_t FoundIdx = 0;
	bool Found = false;
	for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
	{
		const equation_batch &Batch = Model->EquationBatches[BatchIdx];
		ForAllBatchEquations(Batch, [Equation, BatchIdx, &FoundIdx, &Found](equation_h Eq)
		{
			if(Equation == Eq)
			{
				FoundIdx = BatchIdx;
				Found = true;
				return true;
			}
			return false;
		});
		if(Found) break;
	}
	if(!Found)
		FatalError("INTERNAL ERROR: Could not find equation \"", Name, "\" in the batch it is supposed to be in.\n");
	
	const equation_batch &Batch = Model->EquationBatches[FoundIdx];
	
	parameter_h          Switch = Batch.ConditionalSwitch;
	parameter_value SwitchValue = Batch.ConditionalValue;
	
	if(!IsValid(Switch)) return true;
	
	array<index_set_h> &EquationIndexSets = DataSet->ResultStorageStructure.Units[StorageUnitIndex].IndexSets;

	if(IndexCount != EquationIndexSets.Count)
		FatalError("ERROR: Got the wrong amount of indexes when checking the result series for \"", Name, "\". Got ", IndexCount, ", expected ", EquationIndexSets.Count, ".\n");
	
	size_t SwitchStorageUnitIdx = DataSet->ParameterStorageStructure.UnitForHandle[Switch.Handle];
	
	array<index_set_h> &SwitchIndexSets = DataSet->ParameterStorageStructure.Units[SwitchStorageUnitIdx].IndexSets;
	
	index_t SwitchIndexes[256];
	for(size_t IdxIdx = 0; IdxIdx < SwitchIndexSets.Count; ++IdxIdx)
	{
		index_set_h IndexSet = SwitchIndexSets[IdxIdx];
		size_t EqIdxIdx = 0;
		for(index_set_h EqIdxSet : EquationIndexSets)
		{
			if(EqIdxSet == IndexSet)
				break;
			++EqIdxIdx;
		}
		if(EqIdxIdx == EquationIndexSets.Count)
			FatalError("INTERNAL ERROR: The equation \"", Name, "\" is conditionally executed on the parameter \"", GetName(Model, Switch), "\", but it does not have all of its index set dependencies!\n");
		SwitchIndexes[IdxIdx] = GetIndex(DataSet, IndexSet, IndexNames[EqIdxIdx]);
	}

	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, SwitchIndexes, SwitchIndexSets.Count, DataSet->IndexCounts, Switch);
	
	bool WasComputed = DataSet->ParameterData[Offset] == SwitchValue;
	
	return WasComputed;
}



static void
PrintResultSeries(mobius_data_set *DataSet, const char *Name, const std::vector<const char*> &Indexes, size_t WriteSize)
{
	double *ResultSeries = AllocClearedArray(double, WriteSize);
	GetResultSeries(DataSet, Name, Indexes, ResultSeries, WriteSize);
	
	std::cout << "Result series for " << Name << " ";
	for(const char *Index : Indexes) std::cout << "[" << Index << "]";
	
	equation_h Equation = GetEquationHandle(DataSet->Model, Name);
	const equation_spec &Spec = DataSet->Model->Equations[Equation];
	if(IsValid(Spec.Unit))
		std::cout << " (" << GetName(DataSet->Model, Spec.Unit) << ")";
	
	std::cout << ":" << std::endl;
	for(size_t Idx = 0; Idx < WriteSize; ++Idx)
		std::cout << ResultSeries[Idx] << " ";
	std::cout << std::endl << std::endl;
	
	free(ResultSeries);
}

static void
PrintInputSeries(mobius_data_set *DataSet, const char *Name, const std::vector<const char*> &Indexes, size_t WriteSize)
{
	double *InputSeries = AllocClearedArray(double, WriteSize);
	GetInputSeries(DataSet, Name, Indexes, InputSeries, WriteSize);
	
	std::cout << "Input series for " << Name << " ";
	for(const char *Index : Indexes) std::cout << "[" << Index << "]";
	
	input_h Input = GetInputHandle(DataSet->Model, Name);
	const input_spec &Spec = DataSet->Model->Inputs[Input];
	if(IsValid(Spec.Unit))
		std::cout << " (" << GetName(DataSet->Model, Spec.Unit) << ")";
	
	std::cout << ":" << std::endl;
	for(size_t Idx = 0; Idx < WriteSize; ++Idx)
		std::cout << InputSeries[Idx] << " ";
	std::cout << std::endl << std::endl;
	
	free(InputSeries);
}

static void
PrintIndexes(mobius_data_set *DataSet, const char *IndexSetName)
{
	//TODO: checks
	const mobius_model *Model = DataSet->Model;
	index_set_h IndexSet = GetIndexSetHandle(Model, IndexSetName);
	std::cout << "Indexes for " << IndexSetName << ": ";
	const index_set_spec &Spec = Model->IndexSets[IndexSet];
	if(Spec.Type == IndexSetType_Basic)
	{
		for(size_t IndexIndex = 0; IndexIndex < DataSet->IndexCounts[IndexSet.Handle]; ++IndexIndex)
			std::cout << DataSet->IndexNames[IndexSet.Handle][IndexIndex] << " ";
	}
	else if(Spec.Type == IndexSetType_Branched)
	{
		for(size_t IndexIndex = 0; IndexIndex < DataSet->IndexCounts[IndexSet.Handle]; ++IndexIndex)
		{
			std::cout <<  "(" << DataSet->IndexNames[IndexSet.Handle][IndexIndex] << ": ";
			for(size_t InputIndexIndex = 0; InputIndexIndex < DataSet->BranchInputs[IndexSet.Handle][IndexIndex].Count; ++InputIndexIndex)
			{
				size_t InputIndex = DataSet->BranchInputs[IndexSet.Handle][IndexIndex][InputIndexIndex];
				std::cout << DataSet->IndexNames[IndexSet.Handle][InputIndex] << " ";
			}
			std::cout << ") ";
		}
	}
	std::cout << std::endl;
}

static void
ForeachRecursive(mobius_data_set *DataSet, char **CurrentIndexNames, const array<index_set_h> &IndexSets, const std::function<void(const char *const *IndexNames, size_t IndexesCount)> &Do, s32 Level)
{
	if(Level + 1 == IndexSets.Count)
		Do(CurrentIndexNames, IndexSets.Count);
	else
	{
		index_set_h IterateOver = IndexSets[Level + 1];
		size_t IndexCount = DataSet->IndexCounts[IterateOver.Handle];
		for(index_t Index = {IterateOver, 0}; Index < IndexCount; ++Index)
		{
			CurrentIndexNames[Level + 1] = (char *)DataSet->IndexNames[IterateOver.Handle][Index]; //NOTE: Casting away constness because it is annoying, and it does not matter in this case.
			ForeachRecursive(DataSet, CurrentIndexNames, IndexSets, Do, Level + 1);
		}
	}
}

static void
ForeachInputInstance(mobius_data_set *DataSet, const char *InputName, const std::function<void(const char *const *IndexNames, size_t IndexesCount)> &Do)
{
	const mobius_model *Model = DataSet->Model;
	
	char *CurrentIndexNames[256];
	
	input_h Input = GetInputHandle(Model, InputName);
	
	size_t UnitIndex = DataSet->InputStorageStructure.UnitForHandle[Input.Handle];
	storage_unit_specifier<input_h> &Unit = DataSet->InputStorageStructure.Units[UnitIndex];
	
	ForeachRecursive(DataSet, CurrentIndexNames, Unit.IndexSets, Do, -1);
}

static void
ForeachResultInstance(mobius_data_set *DataSet, const char *ResultName, const std::function<void(const char *const *IndexNames, size_t IndexesCount)> &Do)
{
	const mobius_model *Model = DataSet->Model;
	
	char *CurrentIndexNames[256];
	
	equation_h Equation = GetEquationHandle(Model, ResultName);
	
	size_t UnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Equation.Handle];
	storage_unit_specifier<equation_h> &Unit = DataSet->ResultStorageStructure.Units[UnitIndex];
	
	ForeachRecursive(DataSet, CurrentIndexNames, Unit.IndexSets, Do, -1);
}

static void
ForeachParameterInstance(mobius_data_set *DataSet, const char *ParameterName, const std::function<void(const char *const *IndexNames, size_t IndexesCount)> &Do)
{
	const mobius_model *Model = DataSet->Model;
	
	char *CurrentIndexNames[256];
	
	parameter_h Parameter = GetParameterHandle(Model, ParameterName);
	
	size_t UnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[Parameter.Handle];
	storage_unit_specifier<parameter_h> &Unit = DataSet->ParameterStorageStructure.Units[UnitIndex];
	
	ForeachRecursive(DataSet, CurrentIndexNames, Unit.IndexSets, Do, -1);
}



static void
ForeachRecursive(mobius_data_set *DataSet, index_t *CurrentIndexes, const array<index_set_h> &IndexSets, const std::function<void(index_t *Indexes, size_t IndexesCount)> &Do, s32 Level)
{
	if(Level + 1 == IndexSets.Count)
		Do(CurrentIndexes, IndexSets.Count);
	else
	{
		index_set_h IterateOver = IndexSets[Level + 1];
		size_t IndexCount = DataSet->IndexCounts[IterateOver.Handle];
		for(index_t Index = {IterateOver, 0}; Index < IndexCount; ++Index)
		{
			CurrentIndexes[Level + 1] = Index;
			ForeachRecursive(DataSet, CurrentIndexes, IndexSets, Do, Level + 1);
		}
	}
}

static void
ForeachParameterInstance(mobius_data_set *DataSet, parameter_h Parameter, const std::function<void(index_t *Indexes, size_t IndexesCount)> &Do)
{
	const mobius_model *Model = DataSet->Model;
	
	index_t CurrentIndexes[256];
	
	size_t UnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[Parameter.Handle];
	storage_unit_specifier<parameter_h> &Unit = DataSet->ParameterStorageStructure.Units[UnitIndex];
	
	ForeachRecursive(DataSet, CurrentIndexes, Unit.IndexSets, Do, -1);
}



