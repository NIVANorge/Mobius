

static mobius_data_set *
GenerateDataSet(mobius_model *Model)
{
	if(!Model->Finalized)
	{
		std::cout << "ERROR: Attempted to generate a data set before the model was finalized using an EndModelDefinition call." << std::endl;
		return 0;
	}
	mobius_data_set *DataSet = new mobius_data_set {};  //NOTE: The {} ensures that all pointers are set to 0. This is important.
	
	DataSet->Model = Model;
	
	DataSet->IndexCounts = AllocClearedArray(index_t, Model->IndexSets.Count());
	DataSet->IndexCounts[0] = index_t({0}, 1);
	DataSet->IndexNames = AllocClearedArray(const char **, Model->IndexSets.Count());
	DataSet->IndexNamesToHandle.resize(Model->IndexSets.Count());
	
	DataSet->BranchInputs = AllocClearedArray(branch_inputs *, Model->IndexSets.Count());
	
	if(Model->IndexSets.Count() == 1) // NOTE: In case there are no index sets, all index sets have had their indexes set.
	{
		DataSet->AllIndexesHaveBeenSet = true;
	}
	
	DataSet->ParameterStorageStructure.Model = Model;
	DataSet->ParameterStorageStructure.Type  = EntityType_Parameter;
	
	DataSet->InputStorageStructure.Model     = Model;
	DataSet->InputStorageStructure.Type      = EntityType_Input;
	
	DataSet->ResultStorageStructure.Model    = Model;
	DataSet->ResultStorageStructure.Type     = EntityType_Equation;
	
	DataSet->TimestepsLastRun = 0;
	
	return DataSet;
}

mobius_data_set::~mobius_data_set()
{
	if(ParameterData) free(ParameterData);
	if(InputData) free(InputData);
	if(ResultData) free(ResultData);
	if(InputTimeseriesWasProvided) free(InputTimeseriesWasProvided);
	
	//TODO: This destructor should not have to look up Model->IndexSets.Count(), because we want to allow people to delete the model and datasets in arbitrary order.
	
	if(IndexCounts)
	{
		if(IndexNames)
		{
			for(entity_handle IndexSetHandle = 1; IndexSetHandle < Model->IndexSets.Count(); ++IndexSetHandle)
			{
				if(IndexNames[IndexSetHandle])
				{
					for(index_t Index = {IndexSetHandle, 0}; Index < IndexCounts[IndexSetHandle]; ++Index)
					{
						if(IndexNames[IndexSetHandle][Index]) free((void *)IndexNames[IndexSetHandle][Index]); //NOTE: We free this const char * because we know that it was set using SetIndexes(), which always allocates copies of the index names it receives.
					}
					free(IndexNames[IndexSetHandle]);
				}
			}
			free(IndexNames);
		}
		
		if(BranchInputs)
		{
			for(entity_handle IndexSetHandle = 1; IndexSetHandle < Model->IndexSets.Count(); ++IndexSetHandle)
			{
				if(BranchInputs[IndexSetHandle])
				{
					for(index_t Index = {IndexSetHandle, 0}; Index < IndexCounts[IndexSetHandle]; ++Index)
					{
						if(BranchInputs[IndexSetHandle][Index].Inputs) free(BranchInputs[IndexSetHandle][Index].Inputs);
					}
					free(BranchInputs[IndexSetHandle]);
				}
			}
			free(BranchInputs);
		}
		
		free(IndexCounts);
	}
	
	if(x0) free(x0);
	if(wk) free(wk);
}

static void
CopyStorageStructure(storage_structure &Source, storage_structure &Dest, size_t FirstUnusedHandle)
{
	Dest.Units = Source.Units; //NOTE: Nested vector copy.
	
	size_t UnitCount = Dest.Units.size();
	
	//TODO: It may be ok to have these as std::vector<size_t> instead of size_t*, in which case we would not have to do all this work doing explicit copies.
	if(Source.TotalCountForUnit) Dest.TotalCountForUnit = CopyArray(size_t, UnitCount, Source.TotalCountForUnit);
	if(Source.OffsetForUnit)     Dest.OffsetForUnit     = CopyArray(size_t, UnitCount, Source.OffsetForUnit);
	if(Source.UnitForHandle)     Dest.UnitForHandle     = CopyArray(size_t, FirstUnusedHandle, Source.UnitForHandle);
	if(Source.LocationOfHandleInUnit) Dest.LocationOfHandleInUnit = CopyArray(size_t, FirstUnusedHandle, Source.LocationOfHandleInUnit);
	Dest.TotalCount = Source.TotalCount;
	Dest.HasBeenSetUp = Source.HasBeenSetUp;
	
	Dest.Model = Source.Model;
}

static mobius_data_set *
CopyDataSet(mobius_data_set *DataSet, bool CopyResults = false)
{
	const mobius_model *Model = DataSet->Model;
	
	mobius_data_set *Copy = new mobius_data_set {};
	
	Copy->Model = Model;
	
	if(DataSet->ParameterData) Copy->ParameterData = CopyArray(parameter_value, DataSet->ParameterStorageStructure.TotalCount, DataSet->ParameterData);
	CopyStorageStructure(DataSet->ParameterStorageStructure, Copy->ParameterStorageStructure, Model->Parameters.Count());
	
	if(DataSet->InputData) Copy->InputData = CopyArray(double, DataSet->InputStorageStructure.TotalCount * DataSet->InputDataTimesteps, DataSet->InputData);
	CopyStorageStructure(DataSet->InputStorageStructure, Copy->InputStorageStructure, Model->Inputs.Count());
	Copy->InputDataStartDate = DataSet->InputDataStartDate;
	Copy->InputDataHasSeparateStartDate = DataSet->InputDataHasSeparateStartDate;
	Copy->InputDataTimesteps = DataSet->InputDataTimesteps;
	
	if(DataSet->InputTimeseriesWasProvided) Copy->InputTimeseriesWasProvided = CopyArray(bool, DataSet->InputStorageStructure.TotalCount, DataSet->InputTimeseriesWasProvided);
	
	if(CopyResults)
	{
		if(DataSet->ResultData) Copy->ResultData = CopyArray(double, DataSet->ResultStorageStructure.TotalCount * (DataSet->TimestepsLastRun + 1), DataSet->ResultData);
		CopyStorageStructure(DataSet->ResultStorageStructure, Copy->ResultStorageStructure, Model->Equations.Count());
		Copy->TimestepsLastRun = DataSet->TimestepsLastRun;
		Copy->StartDateLastRun = DataSet->StartDateLastRun;
		Copy->HasBeenRun = DataSet->HasBeenRun;
	}
	else
	{
		Copy->HasBeenRun = false;
	}
	
	if(DataSet->IndexCounts) Copy->IndexCounts = CopyArray(index_t, Model->IndexSets.Count(), DataSet->IndexCounts);
	//TODO: This could probably be a std::vector<std::vector<std::string>>
	if(DataSet->IndexNames)
	{
		Copy->IndexNames = AllocClearedArray(const char **, Model->IndexSets.Count());
		for(entity_handle IndexSetHandle = 0; IndexSetHandle < Model->IndexSets.Count(); ++IndexSetHandle)
		{
			if(DataSet->IndexNames[IndexSetHandle])
			{
				Copy->IndexNames[IndexSetHandle] = AllocClearedArray(const char *, DataSet->IndexCounts[IndexSetHandle]);
				for(index_t Index = {IndexSetHandle, 0}; Index < DataSet->IndexCounts[IndexSetHandle]; ++Index)
				{
					Copy->IndexNames[IndexSetHandle][Index] = CopyString(DataSet->IndexNames[IndexSetHandle][Index]);
				}
			}
		}
	}
	Copy->IndexNamesToHandle = DataSet->IndexNamesToHandle;
	Copy->AllIndexesHaveBeenSet = DataSet->AllIndexesHaveBeenSet;
	
	if(DataSet->BranchInputs)
	{
		Copy->BranchInputs = AllocClearedArray(branch_inputs *, Model->IndexSets.Count());
		for(entity_handle IndexSetHandle = 0; IndexSetHandle < Model->IndexSets.Count(); ++IndexSetHandle)
		{
			if(DataSet->BranchInputs[IndexSetHandle])
			{
				Copy->BranchInputs[IndexSetHandle] = AllocClearedArray(branch_inputs, DataSet->IndexCounts[IndexSetHandle]);
				for(index_t Index = {IndexSetHandle, 0}; Index < DataSet->IndexCounts[IndexSetHandle]; ++Index)
				{
					//yeahh.. this could also be a std::vector
					branch_inputs &Inputs = DataSet->BranchInputs[IndexSetHandle][Index];
					size_t Count = Inputs.Count;
					Copy->BranchInputs[IndexSetHandle][Index].Count = Count;
					Copy->BranchInputs[IndexSetHandle][Index].Inputs = CopyArray(index_t, Count, Inputs.Inputs);
				}
			}
		}
	}
	
	//NOTE: We don't copy any of the following as they will be generated when you try to run the DataSet.
	//std::vector<parameter_value> FastParameterLookup;
	//std::vector<size_t> FastInputLookup;
	//std::vector<size_t> FastResultLookup;
	//std::vector<size_t> FastLastResultLookup;
	//double *x0; //NOTE: Temporary storage for use by solvers
	//double *wk; //NOTE: Temporary storage for use by solvers
	//u64 TimestepsLastRun;
	
	return Copy;
}

static void
SetupStorageStructureSpecifer(storage_structure &Structure, index_t *IndexCounts, size_t FirstUnusedHandle)
{
	size_t UnitCount = Structure.Units.size();
	Structure.TotalCountForUnit = AllocClearedArray(size_t, UnitCount);
	Structure.OffsetForUnit     = AllocClearedArray(size_t, UnitCount);
	Structure.UnitForHandle     = AllocClearedArray(size_t, FirstUnusedHandle);
	Structure.LocationOfHandleInUnit = AllocClearedArray(size_t, FirstUnusedHandle);
	Structure.TotalCount = 0;
	
	size_t UnitIndex = 0;
	size_t OffsetForUnitSoFar = 0;
	for(storage_unit_specifier &Unit : Structure.Units)
	{
		Structure.TotalCountForUnit[UnitIndex] = Unit.Handles.size();
		for(index_set_h IndexSet : Unit.IndexSets)
		{
			Structure.TotalCountForUnit[UnitIndex] *= IndexCounts[IndexSet.Handle];
		}
		
		size_t HandleIdx = 0;
		for(entity_handle Handle : Unit.Handles)
		{
			Structure.UnitForHandle[Handle] = UnitIndex;
			Structure.LocationOfHandleInUnit[Handle] = HandleIdx;
			++HandleIdx;
		}
		
		Structure.OffsetForUnit[UnitIndex] = OffsetForUnitSoFar;
		OffsetForUnitSoFar += Structure.TotalCountForUnit[UnitIndex];
		Structure.TotalCount += Structure.TotalCountForUnit[UnitIndex];
		++UnitIndex;
	}
	
	Structure.HasBeenSetUp = true;
}

storage_structure::~storage_structure()
{
	if(HasBeenSetUp)
	{
		free(TotalCountForUnit);
		free(OffsetForUnit);
		free(UnitForHandle);
		free(LocationOfHandleInUnit);
	}
}

//NOTE: The following functions are very similar, but it would incur a performance penalty to try to merge them.
//NOTE: The OffsetForHandle functions are meant to be internal functions that can be used by other wrappers that do more error checking.


// NOTE: Returns the storage index of the first instance of a value corresponding to this Handle, i.e where all indexes that this handle depends on are the first index of their index set.
inline size_t
OffsetForHandle(storage_structure &Structure, entity_handle Handle)
{
	size_t UnitIndex = Structure.UnitForHandle[Handle];
	size_t OffsetForUnit = Structure.OffsetForUnit[UnitIndex];
	size_t LocationOfHandleInUnit = Structure.LocationOfHandleInUnit[Handle];
	
	return OffsetForUnit + LocationOfHandleInUnit;
}

// NOTE: Returns the storage index of a value corresponding to this Handle with the given index set indexes.
// CurrentIndexes must be set up so that for any index set with handle IndexSetHandle, CurrentIndexes[IndexSetHandle] is the current index of that index set. (Typically ValueSet->CurrentIndexes)
// IndexCounts    must be set up so that for any index set with handle IndexSetHandle, IndexCounts[IndexSetHandle] is the index count of that index set. (Typically DataSet->IndexCounts)
inline size_t
OffsetForHandle(storage_structure &Structure, const index_t *CurrentIndexes, const index_t *IndexCounts, entity_handle Handle)
{
	std::vector<storage_unit_specifier> &Units = Structure.Units;
	
	size_t UnitIndex = Structure.UnitForHandle[Handle];
	storage_unit_specifier &Specifier = Units[UnitIndex];
	size_t NumHandlesInUnitInstance = Specifier.Handles.size();
	size_t OffsetForUnit = Structure.OffsetForUnit[UnitIndex];
	size_t LocationOfHandleInUnit = Structure.LocationOfHandleInUnit[Handle];
	
	size_t InstanceOffset = 0;
	for(index_set_h IndexSet : Specifier.IndexSets)
	{
		size_t Count = IndexCounts[IndexSet.Handle];
		index_t Index = CurrentIndexes[IndexSet.Handle];
		
#if MOBIUS_INDEX_BOUNDS_TESTS
		if(Index >= Count)
		{
			const char *EntityName = GetName(Structure.Model, Structure.Type, Handle);
			const char *TypeName   = GetEntityTypeName(Structure.Type);
			MOBIUS_PARTIAL_ERROR("ERROR: Index out of bounds for index set \"" << GetName(Structure.Model, IndexSet) << "\", got index " << Index << ", count was " << Count << std::endl);
			MOBIUS_FATAL_ERROR("This happened while looking up the value of the " << TypeName << " \"" << EntityName << "\"." << std::endl);
		}
		if(Index.IndexSetHandle != IndexSet.Handle)
		{
			const char *EntityName = GetName(Structure.Model, Structure.Type, Handle);
			const char *TypeName   = GetEntityTypeName(Structure.Type);
			MOBIUS_PARTIAL_ERROR("ERROR: Used an index addressed to the index set \"" << GetName(Structure.Model, index_set_h {Index.IndexSetHandle}) << "\" for indexing the index set \"" << GetName(Structure.Model, IndexSet) << "\"." << std::endl);
			MOBIUS_FATAL_ERROR("This happened while looking up the value of the " << TypeName << " \"" << EntityName << "\"." << std::endl);
		}
#endif
		
		InstanceOffset = InstanceOffset * Count + Index;
	}
	return OffsetForUnit + InstanceOffset * NumHandlesInUnitInstance + LocationOfHandleInUnit;
}

#if !defined(MOBIUS_INDEX_BOUNDS_TESTS)
#define MOBIUS_INDEX_BOUNDS_TESTS 0
#endif

inline void
CheckIndexErrors(const mobius_model *Model, index_set_h IndexSet, index_t Index, size_t Count, entity_type Type, entity_handle Handle)
{
	bool CountError = (Index >= Count);
	bool HandleError = (Index.IndexSetHandle != IndexSet.Handle);
	if(CountError || HandleError)
	{
		const char *EntityName = GetName(Model, Type, Handle);
		const char *TypeName   = GetEntityTypeName(Type);
		if(CountError)
		{
			MOBIUS_PARTIAL_ERROR("ERROR: Index out of bounds for index set \"" << GetName(Model, IndexSet) << "\", got index " << Index << ", count was " << Count << std::endl);
		}
		if(HandleError)
		{
			MOBIUS_PARTIAL_ERROR("ERROR: Used an index addressed to the index set \"" << GetName(Model, index_set_h {Index.IndexSetHandle}) << "\" for indexing the index set \"" << GetName(Model, IndexSet) << "\"." << std::endl);
		}
		MOBIUS_FATAL_ERROR("This happened while looking up the value of the " << TypeName << " \"" << EntityName << "\"." << std::endl);
	}
}

// NOTE: Returns the storage index of a value corresponding to this Handle with the given index set indexes.
// Indexes must be set up so that Indexes[I] is the index of the I'th index set that the entity one wishes to look up depends on.
// IndexesCount is the number of index sets the entity depends on (and so the length of the array Indexes).
// IndexCounts    must be set up so that for any index set with handle IndexSetHandle, IndexCounts[IndexSetHandle] is the index count of that index set. (Typically DataSet->IndexCounts)
inline size_t
OffsetForHandle(storage_structure &Structure, const index_t *Indexes, size_t IndexesCount, const index_t *IndexCounts, entity_handle Handle)
{
	std::vector<storage_unit_specifier> &Units = Structure.Units;
	
	size_t UnitIndex = Structure.UnitForHandle[Handle];
	storage_unit_specifier &Specifier = Units[UnitIndex];
	size_t NumHandlesInUnitInstance = Specifier.Handles.size();
	size_t OffsetForUnit = Structure.OffsetForUnit[UnitIndex];
	size_t LocationOfHandleInUnit = Structure.LocationOfHandleInUnit[Handle];
	
	size_t InstanceOffset = 0;
	size_t Level = 0;
	for(index_set_h IndexSet : Specifier.IndexSets)
	{
		size_t Count = IndexCounts[IndexSet.Handle];
		index_t Index = Indexes[Level];
		
#if MOBIUS_INDEX_BOUNDS_TESTS
		CheckIndexErrors(Structure.Model, IndexSet, Index, Count, Structure.Type, Handle);
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
inline size_t
OffsetForHandle(storage_structure &Structure, const index_t* CurrentIndexes, const index_t *IndexCounts, const index_t *OverrideIndexes, size_t OverrideCount, entity_handle Handle)
{
	std::vector<storage_unit_specifier> &Units = Structure.Units;
	
	size_t UnitIndex = Structure.UnitForHandle[Handle];
	storage_unit_specifier &Specifier = Units[UnitIndex];
	size_t NumHandlesInUnitInstance = Specifier.Handles.size();
	size_t OffsetForUnit = Structure.OffsetForUnit[UnitIndex];
	size_t LocationOfHandleInUnit = Structure.LocationOfHandleInUnit[Handle];
	
	size_t IndexSetCount = Specifier.IndexSets.size();
	size_t InstanceOffset = 0;
	size_t IndexSetLevel = 0;
	for(index_set_h IndexSet : Specifier.IndexSets)
	{
		size_t Count = IndexCounts[IndexSet.Handle];
		index_t Index;
		if(IndexSetLevel < (IndexSetCount - OverrideCount))
		{
			Index = CurrentIndexes[IndexSet.Handle];
		}
		else
		{
			Index = OverrideIndexes[IndexSetLevel + (OverrideCount - IndexSetCount)];
		}
		
#if MOBIUS_INDEX_BOUNDS_TESTS
		CheckIndexErrors(Structure.Model, IndexSet, Index, Count, Structure.Type, Handle);
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
static size_t
OffsetForHandle(storage_structure &Structure, index_t *CurrentIndexes, index_t *IndexCounts, index_set_h Skip, size_t& SubsequentOffset, entity_handle Handle)
{
	std::vector<storage_unit_specifier> &Units = Structure.Units;
	
	size_t UnitIndex = Structure.UnitForHandle[Handle];
	storage_unit_specifier &Specifier = Units[UnitIndex];
	size_t NumHandlesInUnitInstance = Specifier.Handles.size();
	size_t OffsetForUnit = Structure.OffsetForUnit[UnitIndex];
	size_t LocationOfHandleInUnit = Structure.LocationOfHandleInUnit[Handle];
	
	size_t InstanceOffset = 0;
	SubsequentOffset = 1;
	bool Skipped = false;
	for(index_set_h IndexSet : Specifier.IndexSets)
	{
		size_t Count = IndexCounts[IndexSet.Handle];
		index_t Index = CurrentIndexes[IndexSet.Handle];
		if(Skipped)
		{
			SubsequentOffset *= Count;
		}
		if(IndexSet == Skip)
		{
			Index = {IndexSet, 0};
			Skipped = true;
		}

#if MOBIUS_INDEX_BOUNDS_TESTS
		CheckIndexErrors(Structure.Model, IndexSet, Index, Count, Structure.Type, Handle);
#endif
		
		InstanceOffset = InstanceOffset * Count + Index;
	}
	SubsequentOffset *= NumHandlesInUnitInstance;
	
	return OffsetForUnit + InstanceOffset * NumHandlesInUnitInstance + LocationOfHandleInUnit;
}

static void
SetMultipleValuesForParameter(mobius_data_set *DataSet, entity_handle ParameterHandle, parameter_value *Values, size_t Count)
{
	//NOTE: There are almost no safety checks in this function. The caller of the function is responsible for the checks!
	// It was designed to be used with the default text input for parameters. If you want to use it for anything else, you should make sure you understand how it works.
	
	size_t UnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[ParameterHandle];	
	size_t Stride = DataSet->ParameterStorageStructure.Units[UnitIndex].Handles.size();
	
	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, ParameterHandle);
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
		entity_handle StartTimeHandle = FindTime->second;
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
	
	auto FindTimestep = Model->Parameters.NameToHandle.find("Timesteps");
	if(FindTimestep != Model->Parameters.NameToHandle.end())
	{
		entity_handle TimestepHandle = FindTimestep->second;
		size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, TimestepHandle);
		return DataSet->ParameterData[Offset].ValUInt; //TODO: Check that it was actually registered with the correct type and that it does not have any index set dependencies.
	}
	
	return 100;
}


static void
SetIndexes(mobius_data_set *DataSet, token_string IndexSetName, const std::vector<token_string>& IndexNames)
{
	const mobius_model *Model = DataSet->Model;
	
	entity_handle IndexSetHandle = GetIndexSetHandle(DataSet->Model, IndexSetName).Handle;
	const index_set_spec &Spec = Model->IndexSets.Specs[IndexSetHandle];
	
	if(Spec.Type != IndexSetType_Basic)
	{
		MOBIUS_FATAL_ERROR("ERROR: Can not use the method SetIndexes for the index set " << Spec.Name << ", use a method that is specific to the type of that index set instead." << std::endl);
	}
	
	if(DataSet->IndexNames[IndexSetHandle] != 0)
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried to set the indexes for the index set " << Spec.Name << " more than once." << std::endl);
	}
	
	if(IndexNames.empty())
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried to set indexes for the index set " << Spec.Name << ", but no indexes were provided" << std::endl);
	}
	
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
			MOBIUS_PARTIAL_ERROR("ERROR: The model requires the following indexes to be the first indexes for the index set " << Spec.Name << ":" << std::endl);
			for(const char *IndexName :  Spec.RequiredIndexes)
			{
				MOBIUS_PARTIAL_ERROR("\"" << IndexName << "\" ");
			}
			MOBIUS_PARTIAL_ERROR(std::endl << "in that order. We got the indexes: " << std::endl);
			for(token_string IndexName : IndexNames)
			{
				MOBIUS_PARTIAL_ERROR("\"" << IndexName << "\" ");
			}
			MOBIUS_FATAL_ERROR(std::endl);
		}
	}
	
	DataSet->IndexCounts[IndexSetHandle] = {IndexSetHandle, (u32)IndexNames.size()};
	DataSet->IndexNames[IndexSetHandle] = AllocClearedArray(const char *, IndexNames.size());
	
	for(size_t IndexIndex = 0; IndexIndex < IndexNames.size(); ++IndexIndex)
	{
		const char *IndexName = IndexNames[IndexIndex].Copy().Data; //NOTE: Leaks unless we free it.
		DataSet->IndexNames[IndexSetHandle][IndexIndex] = IndexName;
		DataSet->IndexNamesToHandle[IndexSetHandle][IndexName] = IndexIndex;
	}
	
	if(DataSet->IndexNamesToHandle[IndexSetHandle].size() != IndexNames.size())
	{
		MOBIUS_FATAL_ERROR("ERROR: Got duplicate indexes for index set " << Spec.Name << std::endl);
	}
	
	bool AllSet = true;
	for(size_t IndexSetHandle = 1; IndexSetHandle < Model->IndexSets.Count(); ++IndexSetHandle)
	{
		if(DataSet->IndexCounts[IndexSetHandle] == 0)
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
	
	entity_handle IndexSetHandle = GetIndexSetHandle(Model, IndexSetName).Handle;
	const index_set_spec &Spec = Model->IndexSets.Specs[IndexSetHandle];
	
	if(Spec.Type != IndexSetType_Branched)
	{
		MOBIUS_FATAL_ERROR("ERROR: Can not use the method SetBranchIndexes for the index set " << IndexSetName << ", use a method that is specific to the type of that index set instead." << std::endl);
	}
	
	if(DataSet->IndexNames[IndexSetHandle] != 0)
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried to set the indexes for the index set " << Spec.Name << " more than once." << std::endl);
	}
	
	if(Inputs.empty())
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried to set indexes for the index set " << Spec.Name << ", but no indexes were provided" << std::endl);
	}
	
	DataSet->IndexCounts[IndexSetHandle] = {IndexSetHandle, (u32)Inputs.size()};
	DataSet->IndexNames[IndexSetHandle] = AllocClearedArray(const char *, Inputs.size());

	DataSet->BranchInputs[IndexSetHandle] = AllocClearedArray(branch_inputs, Inputs.size());
	index_t IndexIndex = {IndexSetHandle, 0};
	for(const auto &InputData : Inputs)
	{
		const char *IndexName = InputData.first.Copy().Data; //NOTE: Leaks unless we free it.

		const std::vector<token_string> &InputNames = InputData.second;
		if(DataSet->IndexNamesToHandle[IndexSetHandle].find(IndexName) != DataSet->IndexNamesToHandle[IndexSetHandle].end())
		{
			MOBIUS_FATAL_ERROR("ERROR: Got duplicate indexes for index set " << IndexSetName << std::endl);
		}
		
		DataSet->IndexNamesToHandle[IndexSetHandle][IndexName] = IndexIndex;
		DataSet->IndexNames[IndexSetHandle][IndexIndex] = IndexName;
		
		DataSet->BranchInputs[IndexSetHandle][IndexIndex].Count = InputNames.size();
		DataSet->BranchInputs[IndexSetHandle][IndexIndex].Inputs = AllocClearedArray(index_t, InputNames.size());
		
		index_t InputIdxIdx = {IndexSetHandle, 0};
		for(token_string InputName : InputNames)
		{
			auto Find = DataSet->IndexNamesToHandle[IndexSetHandle].find(InputName);
			if(Find == DataSet->IndexNamesToHandle[IndexSetHandle].end())
			{
				MOBIUS_FATAL_ERROR("ERROR: The index \"" << InputName << "\" appears an input to the index \"" << IndexName << "\", in the index set " << IndexSetName << ", before it itself is declared." << std::endl);
			}
			index_t InputIndex = {IndexSetHandle, Find->second};
			DataSet->BranchInputs[IndexSetHandle][IndexIndex].Inputs[InputIdxIdx] = InputIndex;
			++InputIdxIdx;
		}
		
		++IndexIndex;
	}
	
	bool AllSet = true;
	for(size_t IndexSetHandle = 1; IndexSetHandle < DataSet->Model->IndexSets.Count(); ++IndexSetHandle)
	{
		if(DataSet->IndexCounts[IndexSetHandle] == 0)
		{
			AllSet = false;
			break;
		}
	}
	DataSet->AllIndexesHaveBeenSet = AllSet;
}

static void
AllocateParameterStorage(mobius_data_set *DataSet)
{
	const mobius_model *Model = DataSet->Model;
	
	if(!DataSet->AllIndexesHaveBeenSet)
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried to allocate parameter storage before all index sets were filled." << std::endl);
	}
	
	if(DataSet->ParameterData)
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried to allocate parameter storage twice." << std::endl);
	}
	
	std::map<std::vector<index_set_h>, std::vector<entity_handle>> TransposedParameterDependencies;
	for(entity_handle ParameterHandle = 1; ParameterHandle < Model->Parameters.Count(); ++ParameterHandle)
	{
		std::vector<index_set_h> Dependencies = Model->Parameters.Specs[ParameterHandle].IndexSetDependencies;
		TransposedParameterDependencies[Dependencies].push_back(ParameterHandle);
	}
	size_t ParameterStorageUnitCount = TransposedParameterDependencies.size();
	std::vector<storage_unit_specifier> &Units = DataSet->ParameterStorageStructure.Units;
	Units.resize(ParameterStorageUnitCount);
	size_t UnitIndex = 0;
	for(auto& Structure : TransposedParameterDependencies)
	{
		Units[UnitIndex].IndexSets = Structure.first; //NOTE: vector copy.
		Units[UnitIndex].Handles   = Structure.second; //NOTE: vector copy.
		
		++UnitIndex;
	}
	SetupStorageStructureSpecifer(DataSet->ParameterStorageStructure, DataSet->IndexCounts, Model->Parameters.Count());
	
	DataSet->ParameterData = AllocClearedArray(parameter_value, DataSet->ParameterStorageStructure.TotalCount);
	
	//NOTE: Setting up default values.
	UnitIndex = 0;
	for(storage_unit_specifier& Unit : Units)
	{
		size_t HandlesInInstance = Unit.Handles.size();
		size_t TotalHandlesForUnit = DataSet->ParameterStorageStructure.TotalCountForUnit[UnitIndex];
		
		size_t ParameterIndex = 0;
		for(entity_handle ParameterHandle : Unit.Handles)
		{
			parameter_value DefaultValue = Model->Parameters.Specs[ParameterHandle].Default;
			size_t At = OffsetForHandle(DataSet->ParameterStorageStructure, ParameterHandle);
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
		MOBIUS_FATAL_ERROR("ERROR: Tried to allocate input storage before all index sets were filled." << std::endl);
	}
	
	if(DataSet->InputData)
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried to allocate input storage twice." << std::endl);
	}

	std::map<std::vector<index_set_h>, std::vector<entity_handle>> TransposedInputDependencies;
	
	for(entity_handle InputHandle = 1; InputHandle < Model->Inputs.Count(); ++InputHandle)
	{
		const input_spec &Spec = Model->Inputs.Specs[InputHandle];
		TransposedInputDependencies[Spec.IndexSetDependencies].push_back(InputHandle);
	}
	
	size_t InputStorageUnitCount = TransposedInputDependencies.size();
	std::vector<storage_unit_specifier> &Units = DataSet->InputStorageStructure.Units;
	Units.resize(InputStorageUnitCount);
	size_t UnitIndex = 0;
	for(auto& Structure : TransposedInputDependencies)
	{
		Units[UnitIndex].IndexSets = Structure.first; //NOTE: vector copy.
		Units[UnitIndex].Handles   = Structure.second; //NOTE: vector copy.
		
		++UnitIndex;
	}
	SetupStorageStructureSpecifer(DataSet->InputStorageStructure, DataSet->IndexCounts, Model->Inputs.Count());

	
	DataSet->InputData = AllocClearedArray(double, DataSet->InputStorageStructure.TotalCount * Timesteps);
	DataSet->InputDataTimesteps = Timesteps;
	
	DataSet->InputTimeseriesWasProvided = AllocClearedArray(bool, DataSet->InputStorageStructure.TotalCount);
}

static void
AllocateResultStorage(mobius_data_set *DataSet, u64 Timesteps)
{
	const mobius_model *Model = DataSet->Model;
	
	if(!DataSet->AllIndexesHaveBeenSet)
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried to allocate result storage before all index sets were filled." << std::endl);
	}
	
	if(DataSet->ResultData)
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried to allocate result storage twice." << std::endl);
	}
	
	//NOTE: We set up a storage structure for results that mirrors the equation batch group structure. This simplifies things a lot in other code.
	
	if(!DataSet->HasBeenRun) //If it was run once before we don't need to set up the storage structure again. //TODO: This should be a flag on the storage structure instead, and it should be the same for all AllocateXStorage functions.
	{
		size_t ResultStorageUnitCount = Model->BatchGroups.size();
		std::vector<storage_unit_specifier> &Units = DataSet->ResultStorageStructure.Units;
		Units.resize(ResultStorageUnitCount);
		for(size_t UnitIndex = 0; UnitIndex < ResultStorageUnitCount; ++UnitIndex)
		{
			const equation_batch_group &BatchGroup = Model->BatchGroups[UnitIndex];
			storage_unit_specifier &Unit = Units[UnitIndex];
			Unit.IndexSets = BatchGroup.IndexSets;
			for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
			{
				const equation_batch &Batch = Model->EquationBatches[BatchIdx];
				
				ForAllBatchEquations(Batch,
				[&Unit](equation_h Equation)
				{
					Unit.Handles.push_back(Equation.Handle);
					return false;
				});
				
				//FOR_ALL_BATCH_EQUATIONS(Batch,
				//	Units[UnitIndex].Handles.push_back(Equation.Handle);
				//)
			}
		}
		
		SetupStorageStructureSpecifer(DataSet->ResultStorageStructure, DataSet->IndexCounts, Model->Equations.Count());
	}

	DataSet->ResultData = AllocClearedArray(double, DataSet->ResultStorageStructure.TotalCount * (Timesteps + 1)); //NOTE: We add one to timesteps since we also need space for the initial values.
}



//NOTE: Returns the numeric index corresponding to an index name and an index_set.
inline index_t
GetIndex(mobius_data_set *DataSet, index_set_h IndexSet, token_string IndexName)
{
	auto &IndexMap = DataSet->IndexNamesToHandle[IndexSet.Handle];
	auto Find = IndexMap.find(IndexName);
	if(Find != IndexMap.end())
	{
		return {IndexSet, Find->second};
	}
	else
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried the index name " << IndexName << " with the index set " << GetName(DataSet->Model, IndexSet) << ", but that index set does not contain that index." << std::endl);
	}
	return {IndexSet, 0};
}

static void
SetParameterValue(mobius_data_set *DataSet, const char *Name, const char * const *Indexes, size_t IndexCount, parameter_value Value, parameter_type Type)
{
	if(!DataSet->AllIndexesHaveBeenSet)
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried to set a parameter value before all index sets have been filled with indexes" << std::endl);
	}
	if(DataSet->ParameterData == 0)
	{
		AllocateParameterStorage(DataSet);
	}
	
	const mobius_model *Model = DataSet->Model;
	entity_handle ParameterHandle = GetParameterHandle(Model, Name);
	
	if(Model->Parameters.Specs[ParameterHandle].Type != Type)
	{
		std::cout << "WARNING: Tried to set the value of the parameter " << Name << " with a value that was of the wrong type. This can lead to undefined behaviour." << std::endl;
	}
	
	//TODO: Check that the value is in the Min-Max range. (issue warning only)
	
	size_t StorageUnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[ParameterHandle];
	std::vector<storage_unit_specifier> &Units = DataSet->ParameterStorageStructure.Units;
	std::vector<index_set_h> &IndexSetDependencies = Units[StorageUnitIndex].IndexSets;
	
	if(IndexCount != IndexSetDependencies.size())
	{
		MOBIUS_FATAL_ERROR("ERROR; Tried to set the value of the parameter " << Name << ", but an incorrect number of indexes were provided. Got " << IndexCount << ", expected " << IndexSetDependencies.size() << std::endl);
	}

	//TODO: This crashes if somebody have more than 256 index sets for a parameter, but that is highly unlikely. Still, this is not clean code...
	index_t IndexValues[256];
	for(size_t Level = 0; Level < IndexCount; ++Level)
	{
		IndexValues[Level] = GetIndex(DataSet, IndexSetDependencies[Level], Indexes[Level]);
	}
	
	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, IndexValues, IndexCount, DataSet->IndexCounts, ParameterHandle);
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
SetParameterValue(mobius_data_set *DataSet, const char *Name, const std::vector<const char *>& Indexes, const char *TimeValue)
{
	parameter_value Val;
	bool ParseSuccess;
	Val.ValTime = datetime(TimeValue, &ParseSuccess);
	
	if(!ParseSuccess)
	{
		MOBIUS_FATAL_ERROR("ERROR: Unrecognized date format when setting the value of the parameter " << Name << std::endl);
	}
	
	SetParameterValue(DataSet, Name, Indexes.data(), Indexes.size(), Val, ParameterType_Time);
}

static parameter_value
GetParameterValue(mobius_data_set *DataSet, const char *Name, const char * const *Indexes, size_t IndexCount, parameter_type Type)
{
	if(DataSet->ParameterData == 0)
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried to get a parameter value before parameter storage was allocated." << std::endl);
	}
	
	const mobius_model *Model = DataSet->Model;
	entity_handle ParameterHandle = GetParameterHandle(Model, Name);
	
	if(Model->Parameters.Specs[ParameterHandle].Type != Type)
	{
		std::cout << "WARNING: Tried to get the value of the parameter " << Name << " specifying the wrong type for the parameter. This can lead to undefined behaviour." << std::endl;
	}
	
	size_t StorageUnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[ParameterHandle];
	std::vector<storage_unit_specifier> &Units = DataSet->ParameterStorageStructure.Units;
	std::vector<index_set_h> &IndexSetDependencies = Units[StorageUnitIndex].IndexSets;
	
	if(IndexCount != IndexSetDependencies.size())
	{
		MOBIUS_FATAL_ERROR("ERROR; Tried to get the value of the parameter " << Name << ", but an incorrect number of indexes were provided. Got " << IndexCount << ", expected " << IndexSetDependencies.size() << std::endl);
	}

	//TODO: This crashes if somebody have more than 256 index sets for a parameter, but that is highly unlikely. Still, this is not clean code...
	index_t IndexValues[256];
	for(size_t Level = 0; Level < IndexCount; ++Level)
	{
		IndexValues[Level] = GetIndex(DataSet, IndexSetDependencies[Level], Indexes[Level]);
	}
	
	size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, IndexValues, IndexCount, DataSet->IndexCounts, ParameterHandle);
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
	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, CurrentIndexes, DataSet->IndexCounts, CumulateOverIndexSet, SubsequentOffset, Equation.Handle);
	
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
	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, CurrentIndexes, DataSet->IndexCounts, CumulateOverIndexSet, SubsequentOffset, Equation.Handle);
	
	size_t ParSubsequentOffset;
	size_t ParOffset = OffsetForHandle(DataSet->ParameterStorageStructure, CurrentIndexes, DataSet->IndexCounts, CumulateOverIndexSet, ParSubsequentOffset, Weight.Handle);
	
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
	{
		AllocateInputStorage(DataSet, InputSeriesSize);
	}
	
	const mobius_model *Model = DataSet->Model;
	
	input_h Input = GetInputHandle(Model, Name);
	
	size_t StorageUnitIndex = DataSet->InputStorageStructure.UnitForHandle[Input.Handle];
	std::vector<storage_unit_specifier> &Units = DataSet->InputStorageStructure.Units;
	std::vector<index_set_h> &IndexSets = Units[StorageUnitIndex].IndexSets;
	
	if(IndexCount != IndexSets.size())
	{
		MOBIUS_FATAL_ERROR("ERROR: Got the wrong amount of indexes when setting the input series for " << GetName(Model, Input) << ". Got " << IndexCount << ", expected " << IndexSets.size() << std::endl);
	}
	index_t Indexes[256];
	for(size_t IdxIdx = 0; IdxIdx < IndexSets.size(); ++IdxIdx)
	{
		Indexes[IdxIdx] = GetIndex(DataSet, IndexSets[IdxIdx], IndexNames[IdxIdx]);
	}
	
	size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, Indexes, IndexCount, DataSet->IndexCounts, Input.Handle);
	double *At = DataSet->InputData + Offset;
	s64 TimestepOffset = 0;
	
	if(AlignWithResults && DataSet->InputDataHasSeparateStartDate)
	{
		//NOTE: In case the user asked for a input timeseries that starts at the start of the modelrun rather than at the start of the input series.
		datetime DataSetStartDate = GetStartDate(DataSet);
		datetime InputStartDate   = DataSet->InputDataStartDate;
		TimestepOffset = InputStartDate.DaysUntil(DataSetStartDate); //TODO: If we later allow for different lengths of timestep we have to update this!
	}
	
	if(InputSeriesSize + TimestepOffset > DataSet->InputDataTimesteps)
	{
		MOBIUS_FATAL_ERROR("ERROR: When setting input series for " << Name << ", the lenght of the timeseries was longer than what was allocated space for in the dataset." << std::endl);
	}
	
	for(size_t Idx = 0; Idx < DataSet->InputDataTimesteps; ++Idx)
	{
		if(Idx >= TimestepOffset && Idx <= InputSeriesSize + TimestepOffset)
		{
			*At = InputSeries[Idx - TimestepOffset];
		}
		else
		{
			*At = std::numeric_limits<double>::quiet_NaN();
		}
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
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried to extract result series before the model was run at least once.")
	}
	
	const mobius_model *Model = DataSet->Model;
	
	//TODO: If we ask for more values than we could get, should there not be an error?
	u64 NumToWrite = Min(WriteSize, DataSet->TimestepsLastRun);
	
	equation_h Equation = GetEquationHandle(Model, Name);
	
	const equation_spec &Spec = Model->Equations.Specs[Equation.Handle];
	if(Spec.Type == EquationType_InitialValue)
	{
		MOBIUS_FATAL_ERROR("ERROR: Can not get the result series of the equation " << Name << ", which is an initial value equation." << std::endl);
	}
	
	size_t StorageUnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Equation.Handle];
	std::vector<storage_unit_specifier> &Units = DataSet->ResultStorageStructure.Units;
	std::vector<index_set_h> &IndexSets = Units[StorageUnitIndex].IndexSets;

	if(IndexCount != IndexSets.size())
	{
		MOBIUS_FATAL_ERROR("ERROR: Got the wrong amount of indexes when getting the result series for " << GetName(Model, Equation) << ". Got " << IndexCount << ", expected " << IndexSets.size() << std::endl);
	}
	index_t Indexes[256];
	for(size_t IdxIdx = 0; IdxIdx < IndexSets.size(); ++IdxIdx)
	{
		Indexes[IdxIdx] = GetIndex(DataSet, IndexSets[IdxIdx], IndexNames[IdxIdx]);
	}

	size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, Indexes, IndexCount, DataSet->IndexCounts, Equation.Handle);
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
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried to extract input series before input data was allocated." << std::endl);
	}
	
	const mobius_model *Model = DataSet->Model;
	
	//TODO: If we ask for more values than we could get, should there not be an error?
	u64 NumToWrite = Min(WriteSize, DataSet->InputDataTimesteps);
	
	input_h Input = GetInputHandle(Model, Name);
	
	size_t StorageUnitIndex = DataSet->InputStorageStructure.UnitForHandle[Input.Handle];
	std::vector<storage_unit_specifier> &Units = DataSet->InputStorageStructure.Units;
	std::vector<index_set_h> &IndexSets = Units[StorageUnitIndex].IndexSets;

	if(IndexCount != IndexSets.size())
	{
		MOBIUS_FATAL_ERROR("ERROR: Got the wrong amount of indexes when getting the input series for " << GetName(Model, Input) << ". Got " << IndexCount << ", expected " << IndexSets.size() << std::endl);
	}
	index_t Indexes[256];
	for(size_t IdxIdx = 0; IdxIdx < IndexSets.size(); ++IdxIdx)
	{
		Indexes[IdxIdx] = GetIndex(DataSet, IndexSets[IdxIdx], IndexNames[IdxIdx]);
	}

	size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, Indexes, IndexCount, DataSet->IndexCounts, Input.Handle);
	double *Lookup = DataSet->InputData + Offset;
	
	if(AlignWithResults && DataSet->InputDataHasSeparateStartDate)
	{
		//NOTE: In case the user asked for a input timeseries that starts at the start of the modelrun rather than at the start of the input series.
		datetime DataSetStartDate = GetStartDate(DataSet);
		datetime InputStartDate   = DataSet->InputDataStartDate;
		s64 TimestepOffset = InputStartDate.DaysUntil(DataSetStartDate); //TODO: If we later allow for different lengths of timestep we have to update this!
		Lookup += TimestepOffset * DataSet->InputStorageStructure.TotalCount;
	}
	
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
	{
		MOBIUS_FATAL_ERROR("ERROR: Tried to see if an input series was provided before input data was even allocated." << std::endl);
	}
	
	const mobius_model *Model = DataSet->Model;
	
	input_h Input = GetInputHandle(Model, Name);
	
	size_t StorageUnitIndex = DataSet->InputStorageStructure.UnitForHandle[Input.Handle];
	std::vector<storage_unit_specifier> &Units = DataSet->InputStorageStructure.Units;
	std::vector<index_set_h> &IndexSets = Units[StorageUnitIndex].IndexSets;

	if(IndexCount != IndexSets.size())
	{
		MOBIUS_FATAL_ERROR("ERROR: Got the wrong amount of indexes when checking the input series for " << GetName(Model, Input) << ". Got " << IndexCount << ", expected " << IndexSets.size() << std::endl);
	}
	index_t Indexes[256];
	for(size_t IdxIdx = 0; IdxIdx < IndexSets.size(); ++IdxIdx)
	{
		Indexes[IdxIdx] = GetIndex(DataSet, IndexSets[IdxIdx], IndexNames[IdxIdx]);
	}
	size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, Indexes, IndexCount, DataSet->IndexCounts, Input.Handle);
	
	return DataSet->InputTimeseriesWasProvided[Offset];
}

inline bool
InputSeriesWasProvided(mobius_data_set *DataSet, const char *Name, const std::vector<const char*> &IndexNames)
{
	return InputSeriesWasProvided(DataSet, Name, IndexNames.data(), IndexNames.size());
}

static void
PrintResultSeries(mobius_data_set *DataSet, const char *Name, const std::vector<const char*> &Indexes, size_t WriteSize)
{
	double *ResultSeries = AllocClearedArray(double, WriteSize);
	GetResultSeries(DataSet, Name, Indexes, ResultSeries, WriteSize);
	
	std::cout << "Result series for " << Name << " ";
	for(const char *Index : Indexes) std::cout << "[" << Index << "]";
	
	equation_h Equation = GetEquationHandle(DataSet->Model, Name);
	const equation_spec &Spec = DataSet->Model->Equations.Specs[Equation.Handle];
	if(IsValid(Spec.Unit))
	{
		std::cout << " (" << GetName(DataSet->Model, Spec.Unit) << ")";
	}
	
	std::cout << ":" << std::endl;
	for(size_t Idx = 0; Idx < WriteSize; ++Idx)
	{
		std::cout << ResultSeries[Idx] << " ";
	}
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
	const input_spec &Spec = DataSet->Model->Inputs.Specs[Input.Handle];
	if(IsValid(Spec.Unit))
	{
		std::cout << " (" << GetName(DataSet->Model, Spec.Unit) << ")";
	}
	
	std::cout << ":" << std::endl;
	for(size_t Idx = 0; Idx < WriteSize; ++Idx)
	{
		std::cout << InputSeries[Idx] << " ";
	}
	std::cout << std::endl << std::endl;
	
	free(InputSeries);
}

static void
PrintIndexes(mobius_data_set *DataSet, const char *IndexSetName)
{
	//TODO: checks
	const mobius_model *Model = DataSet->Model;
	entity_handle IndexSetHandle = GetIndexSetHandle(Model, IndexSetName).Handle;
	std::cout << "Indexes for " << IndexSetName << ": ";
	const index_set_spec &Spec = Model->IndexSets.Specs[IndexSetHandle];
	if(Spec.Type == IndexSetType_Basic)
	{
		for(size_t IndexIndex = 0; IndexIndex < DataSet->IndexCounts[IndexSetHandle]; ++IndexIndex)
		{
			std::cout << DataSet->IndexNames[IndexSetHandle][IndexIndex] << " ";
		}
	}
	else if(Spec.Type == IndexSetType_Branched)
	{
		for(size_t IndexIndex = 0; IndexIndex < DataSet->IndexCounts[IndexSetHandle]; ++IndexIndex)
		{
			std::cout <<  "(" << DataSet->IndexNames[IndexSetHandle][IndexIndex] << ": ";
			for(size_t InputIndexIndex = 0; InputIndexIndex < DataSet->BranchInputs[IndexSetHandle][IndexIndex].Count; ++InputIndexIndex)
			{
				size_t InputIndex = DataSet->BranchInputs[IndexSetHandle][IndexIndex].Inputs[InputIndexIndex];
				std::cout << DataSet->IndexNames[IndexSetHandle][InputIndex] << " ";
			}
			std::cout << ") ";
		}
	}
	std::cout << std::endl;
}

static void
ForeachRecursive(mobius_data_set *DataSet, char **CurrentIndexNames, const std::vector<index_set_h> &IndexSets, const std::function<void(const char *const *IndexNames, size_t IndexesCount)> &Do, s32 Level)
{
	if(Level + 1 == IndexSets.size())
	{
		Do(CurrentIndexNames, IndexSets.size());
	}
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
	storage_unit_specifier &Unit = DataSet->InputStorageStructure.Units[UnitIndex];
	std::vector<index_set_h> &IndexSets = Unit.IndexSets;
	
	ForeachRecursive(DataSet, CurrentIndexNames, IndexSets, Do, -1);
}

static void
ForeachResultInstance(mobius_data_set *DataSet, const char *ResultName, const std::function<void(const char *const *IndexNames, size_t IndexesCount)> &Do)
{
	const mobius_model *Model = DataSet->Model;
	
	char *CurrentIndexNames[256];
	
	equation_h Equation = GetEquationHandle(Model, ResultName);
	
	size_t UnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Equation.Handle];
	storage_unit_specifier &Unit = DataSet->ResultStorageStructure.Units[UnitIndex];
	std::vector<index_set_h> &IndexSets = Unit.IndexSets;
	
	ForeachRecursive(DataSet, CurrentIndexNames, IndexSets, Do, -1);
}

static void
ForeachParameterInstance(mobius_data_set *DataSet, const char *ParameterName, const std::function<void(const char *const *IndexNames, size_t IndexesCount)> &Do)
{
	const mobius_model *Model = DataSet->Model;
	
	char *CurrentIndexNames[256];
	
	entity_handle ParameterHandle = GetParameterHandle(Model, ParameterName);
	
	size_t UnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[ParameterHandle];
	storage_unit_specifier &Unit = DataSet->ParameterStorageStructure.Units[UnitIndex];
	std::vector<index_set_h> &IndexSets = Unit.IndexSets;
	
	ForeachRecursive(DataSet, CurrentIndexNames, IndexSets, Do, -1);
}



static void
ForeachRecursive(mobius_data_set *DataSet, index_t *CurrentIndexes, const std::vector<index_set_h> &IndexSets, const std::function<void(index_t *Indexes, size_t IndexesCount)> &Do, s32 Level)
{
	if(Level + 1 == IndexSets.size())
	{
		Do(CurrentIndexes, IndexSets.size());
	}
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
ForeachParameterInstance(mobius_data_set *DataSet, entity_handle ParameterHandle, const std::function<void(index_t *Indexes, size_t IndexesCount)> &Do)
{
	const mobius_model *Model = DataSet->Model;
	
	index_t CurrentIndexes[256];
	
	size_t UnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[ParameterHandle];
	storage_unit_specifier &Unit = DataSet->ParameterStorageStructure.Units[UnitIndex];
	std::vector<index_set_h> &IndexSets = Unit.IndexSets;
	
	ForeachRecursive(DataSet, CurrentIndexes, IndexSets, Do, -1);
}



