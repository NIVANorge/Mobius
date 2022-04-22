

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
PrintResultStructure(const mobius_model *Model, std::ostream &Out = std::cout)
{
	if(!Model->Finalized)
	{
		WarningPrint("WARNING: Tried to print result structure before the model was finalized.\n");
		return;
	}
	
	Out << std::endl << "**** Result Structure ****" << std::endl;
	//Out << "Number of batches: " << Model->ResultStructure.size() << std::endl;
	for(const equation_batch_group &BatchGroup : Model->BatchGroups)
	{
		if(BatchGroup.IndexSets.Count == 0) Out << "[]";
		for(index_set_h IndexSet : BatchGroup.IndexSets)
			Out << "[" << GetName(Model, IndexSet) << "]";
		
		for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
		{
			const equation_batch &Batch = Model->EquationBatches[BatchIdx];
			Out << "\n\t-----";
			if(IsValid(Batch.Solver)) Out << " Solver \"" << GetName(Model, Batch.Solver) << "\"";
			if(IsValid(Batch.ConditionalSwitch))
			{
				const conditional_spec &Conditional = Model->Conditionals[Batch.Conditional];
				const parameter_spec &SwitchSpec = Model->Parameters[Batch.ConditionalSwitch];
				Out << " Conditional \"" << Conditional.Name << "\" on \"" << GetName(Model, Conditional.Switch) << "\" == ";
				if(SwitchSpec.Type == ParameterType_UInt)
					Out << Batch.ConditionalValue.ValUInt;
				else if(SwitchSpec.Type == ParameterType_Bool)
					Out << (Batch.ConditionalValue.ValBool ? "true" : "false");
				else if(SwitchSpec.Type == ParameterType_Enum)
					Out << SwitchSpec.EnumNames[Batch.ConditionalValue.ValUInt];
			}
			
			ForAllBatchEquations(Batch,
			[Model, &Out](equation_h Equation)
			{
				Out << "\n\t";
				if(Model->Equations[Equation].Type == EquationType_Cumulative) Out << "(Cumulative) ";
				else if(Model->Equations[Equation].Type == EquationType_ODE) Out << "(ODE) ";
				Out << GetName(Model, Equation);
				return false;
			});
			if(BatchIdx == BatchGroup.LastBatch) Out << "\n\t-----\n";
		}
		
		Out << std::endl;
	}
}



static void
PrintInitialValueOrder(const mobius_model *Model, std::ostream &Out = std::cout)
{
	if(!Model->Finalized)
	{
		WarningPrint("WARNING: Tried to print initial value order before the model was finalized.\n");
		return;
	}
	
	Out << std::endl << "**** Initial value order ****" << std::endl;

	for(const equation_batch_group &BatchGroup : Model->BatchGroups)
	{
		if(BatchGroup.IndexSets.Count == 0) Out << "[]";
		for(index_set_h IndexSet : BatchGroup.IndexSets)
			Out << "[" << GetName(Model, IndexSet) << "]";
		
		for(equation_h Initial : BatchGroup.InitialValueOrder)
		{
			//TODO: Provide info about how it is evaluated (initial value eq, param, etc)
			Out << "\n\t" << GetName(Model, Initial);
		}
		Out << "\n";
	}
}



static void
PrintParameterStorageStructure(mobius_data_set *DataSet)
{
	if(!DataSet->ParameterData)
	{
		WarningPrint("WARNING: Tried to print parameter storage structure before the parameter storage was allocated.\n");
		return;
	}
	
	const mobius_model *Model = DataSet->Model;
	
	WarningPrint("\n**** Parameter storage structure ****\n");
	size_t StorageCount = DataSet->ParameterStorageStructure.Units.Count;
	for(size_t StorageIdx = 0; StorageIdx < StorageCount; ++StorageIdx)
	{
		array<index_set_h> &IndexSets = DataSet->ParameterStorageStructure.Units[StorageIdx].IndexSets;
		if(IndexSets.Count == 0)
			WarningPrint("[]");
		for(index_set_h IndexSet : IndexSets)
			WarningPrint("[", GetName(Model, IndexSet), "]");
		for(parameter_h Parameter : DataSet->ParameterStorageStructure.Units[StorageIdx].Handles)
			WarningPrint("\n\t", GetName(Model, Parameter));
		WarningPrint("\n");
	}
	WarningPrint("\n");
}

static void
PrintInputStorageStructure(mobius_data_set *DataSet)
{
	if(!DataSet->InputData)
	{
		WarningPrint("WARNING: Tried to print input storage structure before the input storage was allocated.\n");
		return;
	}
	
	const mobius_model *Model = DataSet->Model;
	
	WarningPrint("\n**** Input storage structure ****\n");
	size_t StorageCount = DataSet->InputStorageStructure.Units.Count;
	for(size_t StorageIdx = 0; StorageIdx < StorageCount; ++StorageIdx)
	{
		array<index_set_h> &IndexSets = DataSet->InputStorageStructure.Units[StorageIdx].IndexSets;
		if(IndexSets.Count == 0)
			WarningPrint("[]");
		for(index_set_h IndexSet : IndexSets)
			WarningPrint("[", GetName(Model, IndexSet), "]");
		for(input_h Handle : DataSet->InputStorageStructure.Units[StorageIdx].Handles)
			WarningPrint("\n\t", GetName(Model, Handle));
		WarningPrint("\n");
	}
	WarningPrint("\n");
}

static void
PrintEquationProfiles(mobius_data_set *DataSet, model_run_state *RunState)
{
#if MOBIUS_EQUATION_PROFILING
	const mobius_model *Model = DataSet->Model;
	WarningPrint("\n**** Equation profiles - Average cycles per evaluation (number of evaluations) ****\n");

	u64 SumCc = 0;
	u64 TotalHits = 0;
	
	for(const equation_batch_group &BatchGroup : Model->BatchGroups)
	{	
		WarningPrint('\n');
		if(BatchGroup.IndexSets.Count == 0) WarningPrint("[]");
		for(index_set_h IndexSet : BatchGroup.IndexSets)
			WarningPrint("[", GetName(Model, IndexSet), "]");
		
		for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
		{
			const equation_batch &Batch = Model->EquationBatches[BatchIdx];
			WarningPrint("\n\t-----");
			if(IsValid(Batch.Solver)) WarningPrint(" Solver \"", GetName(Model, Batch.Solver), "\"");
			
			ForAllBatchEquations(Batch,
			[Model, RunState, &TotalHits, &SumCc](equation_h Equation)
			{
				int PrintCount = 0;
				WarningPrint("\n\t");
				if(Model->Equations[Equation].Type == EquationType_Cumulative)
				{
					WarningPrint("(Cumulative) ");
					PrintCount += 13;
				}
				else if(Model->Equations[Equation].Type == EquationType_ODE)
				{
					WarningPrint("(ODE) ");
					PrintCount += 6;
				}
				const char *Name = GetName(Model, Equation);
				WarningPrint(Name, ": ");
				PrintCount += (strlen(Name) + 2);
				
				u64 Cc = RunState->EquationTotalCycles[Equation.Handle];
				size_t Hits = RunState->EquationHits[Equation.Handle];
				double CcPerHit = (double)Cc / (double)Hits;
				
				char FormatString[64];
				char ResultString[128];
				sprintf(FormatString, "%s%dlf", "%", 60-PrintCount);
				sprintf(ResultString, FormatString, CcPerHit);
				WarningPrint(ResultString, " (", Hits, ")");
				
				TotalHits += (u64)Hits;
				SumCc += Cc;
				return false;
			});
			if(BatchIdx == BatchGroup.LastBatch) WarningPrint("\n\t-----\n");
		}
	}
	WarningPrint("\nTotal average cycles per evaluation: ", ((double)SumCc / (double)TotalHits), "\n");
#endif
}
