

#if !defined(MOBIUS_PRINT_TIMING_INFO)
#define MOBIUS_PRINT_TIMING_INFO 0
#endif

static mobius_model *
BeginModelDefinition(const char *Name = "(unnamed model)", bool UseEndDate = false, const char *TimestepSize = "1D")
{
	mobius_model *Model = new mobius_model {};
	Model->BucketMemory.Initialize(1024*1024);
#if MOBIUS_PRINT_TIMING_INFO
	Model->DefinitionTimer = BeginTimer();
#endif
	Model->Name = Name;

	auto System       = RegisterParameterGroup(Model, "System");
	RegisterParameterDate(Model, System, "Start date", "1970-1-1", "1000-1-1", "3000-1-1", "The start date is inclusive");
	
	if(UseEndDate)
		RegisterParameterDate(Model, System, "End date", "1970-1-1", "1000-1-1", "3000-1-1", "The end date is inclusive");
	else
	{
		token_string StepUnit = TimestepSizeAsUnitName(TimestepSize, &Model->BucketMemory);
		auto Steps 	      = RegisterUnit(Model, StepUnit.Data);
		RegisterParameterUInt(Model, System, "Timesteps", Steps, 1);
	}
	
	SetTimestepSize(Model, TimestepSize);
	
	return Model;
}

mobius_model::~mobius_model()
{
	BucketMemory.DeallocateAll();
}


struct pre_batch;

struct dependency_entry
{
	int          BatchIndex;
	equation_h   WhichEquation;
	equation_h   DependsOnEquation;
	bool         ImplicitlyIndexed;
};

bool
operator==(const dependency_entry &A, const dependency_entry &B)
{
	//NOTE: This is used later when we remove duplicates. After the initial stage we are not interested in which equations caused the dependencies.
	return A.BatchIndex == B.BatchIndex && A.ImplicitlyIndexed == B.ImplicitlyIndexed;
}

struct pre_batch
{
	solver_h Solver = {0};
	conditional_h Conditional = {0};
	std::vector<dependency_entry> Dependencies;
	std::vector<equation_h>       Equations;
	std::set<index_set_h>         IndexSetDependencies;
	
	//NOTE: These are used for a graph sort algorithm:
	bool Visited        = false;
	bool TempVisited    = false;
};

struct pre_batch_group
{
	std::vector<size_t> PreBatchIndexes;
	std::set<index_set_h> IndexSetDependencies;
	
	conditional_h Conditional; //NOTE: this will in practice only be set if the only contained pre_batch has a Conditional set. This is just kept here as a convenience.
};


static void
ErrorPrintEquation(mobius_model *Model, equation_h Equation, bool CareAboutConditionals = false)
{
	equation_spec &Spec = Model->Equations[Equation];
	conditional_h Conditional = GetConditional(Model, Equation);
	
	if(CareAboutConditionals && IsValid(Conditional))
		ErrorPrint("conditional branch \"", GetName(Model, Conditional), "\" (");
	if(IsValid(Spec.Solver))
		ErrorPrint("solver \"", GetName(Model, Spec.Solver), "\" (");
	ErrorPrint("equation \"", GetName(Model, Equation), "\"");
	if(IsValid(Spec.Solver))
		ErrorPrint(")");
	if(CareAboutConditionals && IsValid(Conditional))
		ErrorPrint(")");
}

static void
PrintPartialDependencyTrace(mobius_model *Model, equation_h Equation, bool First = false)
{
	if(First) ErrorPrint("       ");
	else ErrorPrint("<----- ");
	ErrorPrintEquation(Model, Equation);
	ErrorPrint('\n');
}

static void
PrintPartialDependencyTrace(mobius_model *Model, equation_h WhichEquation, equation_h DependsOnEquation, bool CareAboutConditionals)
{
	ErrorPrintEquation(Model, DependsOnEquation, CareAboutConditionals);
	ErrorPrint(" <----- ");
	ErrorPrintEquation(Model, WhichEquation, CareAboutConditionals);
	ErrorPrint("\n");
}


static bool
TopologicalSortBatchesVisit(mobius_model *Model, int BatchIdx, std::vector<pre_batch> &Batches, std::vector<size_t> &BatchIndexesOut, bool CareAboutConditionals)
{
	if(BatchIdx < 0) return true;    //NOTE: This indicates that we registered a dependency outside the current sorting set. Just ignore it.
	
	pre_batch &Batch = Batches[BatchIdx];
	if(Batch.Visited) return true;
	if(Batch.TempVisited)
	{
		ErrorPrint("ERROR: There is a circular dependency between the equations:\n");
		return false;
	}
	Batch.TempVisited = true;
	
	for(dependency_entry &Dep : Batch.Dependencies)
	{
		if(!Dep.ImplicitlyIndexed) continue;
		bool Success = TopologicalSortBatchesVisit(Model, Dep.BatchIndex, Batches, BatchIndexesOut, CareAboutConditionals);
		if(!Success)
		{
			PrintPartialDependencyTrace(Model, Dep.WhichEquation, Dep.DependsOnEquation, CareAboutConditionals);
			return false;
		}
	}
	
	Batch.Visited = true;
	BatchIndexesOut.push_back(BatchIdx);
	return true;
}

static void
TopologicalSortBatches(mobius_model *Model, std::vector<pre_batch> &Batches, std::vector<size_t> &BatchIndexesOut, bool CareAboutConditionals)
{
	for(size_t BatchIdx = 0; BatchIdx < Batches.size(); ++BatchIdx)
	{
		bool Success = TopologicalSortBatchesVisit(Model, BatchIdx, Batches, BatchIndexesOut, CareAboutConditionals);
		if(!Success) FatalError("");
	}
}

static bool
TopologicalSortEquationsInSolverVisit(mobius_model *Model, equation_h Equation, std::vector<equation_h>& PushTo)
{
	equation_spec &Spec = Model->Equations[Equation];
	
	if(Spec.Visited) return true;
	if(Spec.TempVisited)
	{
		ErrorPrint("ERROR: There is a circular dependency between the non-ode equations within a solver :\n");
		PrintPartialDependencyTrace(Model, Equation, true);
		return false;
	}
	Spec.TempVisited = true;
	for(equation_h Dependency : Spec.DirectResultDependencies)
	{
		equation_spec &DepSpec = Model->Equations[Dependency];
		if(DepSpec.Type == EquationType_ODE || DepSpec.Solver != Spec.Solver) continue; //NOTE: Here we are only interested sorting non-ode equations belonging to this solver.
		bool Success = TopologicalSortEquationsInSolverVisit(Model, Dependency, PushTo);
		if(!Success)
		{
			PrintPartialDependencyTrace(Model, Equation);
			return false;
		}
	}
	Spec.Visited = true;
	PushTo.push_back(Equation);
	return true;
}

static bool
TopologicalSortEquationsInitialValueVisit(mobius_model *Model, equation_h Equation, std::vector<equation_h>& PushTo)
{	
	equation_h EquationToLookUp = Equation;
	equation_spec &OriginalSpec = Model->Equations[Equation];
	equation_h InitialValueEq = OriginalSpec.InitialValueEquation;
	if(IsValid(InitialValueEq))
		EquationToLookUp = InitialValueEq;
	
	equation_spec &Spec = Model->Equations[EquationToLookUp];
	
	if(Spec.Visited) return true;
	if(Spec.TempVisited)
	{
		ErrorPrint("ERROR: There is a circular dependency between the initial value of the equations :\n");
		PrintPartialDependencyTrace(Model, Equation, true);
		return false;
	}
	Spec.TempVisited = true;
	
	if(!IsValid(OriginalSpec.InitialValue) && !OriginalSpec.HasExplicitInitialValue) //NOTE: If the equation has one type of explicit initial value or other, we don't evaluate it during the initial value run, and so we don't care what its dependencies are for this sort.
	{
		for(equation_h Dependency : Spec.DirectResultDependencies)
		{
			bool Success = TopologicalSortEquationsInitialValueVisit(Model, Dependency, PushTo);
			if(!Success)
			{
				PrintPartialDependencyTrace(Model, Equation);
				return false;
			}
		}
		//NOTE: In an initial value setting, a LAST_RESULT is the same time step as a RESULT
		for(equation_h Dependency : Spec.DirectLastResultDependencies)
		{
			if(Dependency == Equation) continue; //We don't care if we depend on ourselves.
			
			bool Success = TopologicalSortEquationsInitialValueVisit(Model, Dependency, PushTo);
			if(!Success)
			{
				PrintPartialDependencyTrace(Model, Equation);
				return false;
			}
		}
	}
	
	Spec.Visited = true;
	PushTo.push_back(Equation);
	return true;
}

typedef bool topological_sort_equations_visit(mobius_model *Model, equation_h Equation, std::vector<equation_h>& PushTo);


//TODO: Resolve the issue of code duplication below. It is caused by the initial value order having to be allowed to grow.

static void
TopologicalSortEquations(mobius_model *Model, std::vector<equation_h> &Equations, topological_sort_equations_visit *Visit)
{
	std::vector<equation_h> Temporary;
	Temporary.reserve(Equations.size());
	for(equation_h Equation : Equations)
	{
		bool Success = Visit(Model, Equation, Temporary);
		if(!Success)
			FatalError("");
	}
	
	//NOTE: The following that is commented out is NOT correct since in the case of the initial value order sort, the vector can have become larger!!
	//for(size_t Idx = 0; Idx < Equations.size(); ++Idx)
	//	Equations[Idx] = Temporary[Idx];
	
	Equations = Temporary;
}

static void
TopologicalSortEquations(mobius_model *Model, array<equation_h> &Equations, topological_sort_equations_visit *Visit)
{
	std::vector<equation_h> Temporary;
	Temporary.reserve(Equations.size());
	for(equation_h Equation : Equations)
	{
		bool Success = Visit(Model, Equation, Temporary);
		if(!Success)
			FatalError("");
	}
	
	for(size_t Idx = 0; Idx < Equations.size(); ++Idx)
		Equations[Idx] = Temporary[Idx];
}

static bool
IsTopIndexSetForThisDependency(std::vector<index_set_h> &IndexSetDependencies, array<index_set_h> &BatchGroupIndexSets, size_t IndexSetLevel)
{
	index_set_h CurrentLevelIndexSet = BatchGroupIndexSets[IndexSetLevel];
	bool DependsOnCurrentLevel = (std::find(IndexSetDependencies.begin(), IndexSetDependencies.end(), CurrentLevelIndexSet) != IndexSetDependencies.end());
	if(!DependsOnCurrentLevel) return false;
	
	for(size_t LevelAbove = IndexSetLevel + 1; LevelAbove < BatchGroupIndexSets.Count; ++LevelAbove)
	{
		index_set_h IndexSetAtLevelAbove = BatchGroupIndexSets[LevelAbove];
		if(std::find(IndexSetDependencies.begin(), IndexSetDependencies.end(), IndexSetAtLevelAbove) != IndexSetDependencies.end())
			return false;
	}
	return true;
}

static void
GroupPreBatches(mobius_model *Model, std::vector<equation_h> &Equations, std::vector<pre_batch> &PreBatches, std::vector<pre_batch_group> &GroupBuild, bool CareAboutConditionals = true)
{
	bucket_allocator TemporaryBucket;
	TemporaryBucket.Initialize(1024*1024);
	
	array<int> BatchOfSolver(&TemporaryBucket, Model->Solvers.Count());
	for(size_t Idx = 0; Idx < BatchOfSolver.Count; ++Idx) BatchOfSolver[Idx] = -1;
	
	array<int> BatchOfConditional(&TemporaryBucket, Model->Conditionals.Count());
	for(size_t Idx = 0; Idx < BatchOfConditional.Count; ++Idx) BatchOfConditional[Idx] = -1;
	
	array<int> BatchOfEquation(&TemporaryBucket, Model->Equations.Count());
	for(size_t Idx = 0; Idx < BatchOfEquation.Count; ++Idx) BatchOfEquation[Idx] = -1;
	
	for(equation_h Equation : Equations)
	{
		equation_spec &Spec = Model->Equations[Equation];
		
		conditional_h Conditional = GetConditional(Model, Equation);
		solver_h      Solver      = Spec.Solver;
	
		if(!IsValid(Solver) && Spec.Type == EquationType_ODE)
			FatalError("ERROR: The equation \"", GetName(Model, Equation), "\" is registered as an ODE equation, but it has not been given a solver.\n");
	
		if(IsValid(Solver) && IsValid(Spec.Conditional))
			FatalError("The equation \"", GetName(Model, Equation), "\" was registered with both a solver and a conditional execution. For solvers, the conditional execution should be put on the solver instead of on the equation directly.\n");
		
		if(IsValid(Spec.IsComputedBy) && (IsValid(Solver) || IsValid(Conditional)))
			FatalError("The equation \"", GetName(Model, Equation), "\" was registered with an IsComputedBy and either a conditional execution or a solver. The IsComputedBy functionality can't be combined with these.\n");
		
		if(Spec.Type == EquationType_InitialValue) continue; //NOTE: initial value equations should not be a part of the result structure.
		
		pre_batch *Batch;
		bool MakeNew = true;
		
		if(CareAboutConditionals && IsValid(Conditional))
		{
			int BatchIdx = BatchOfConditional[Conditional.Handle];
			if(BatchIdx != -1)
			{
				MakeNew = false;
				Batch = &PreBatches[BatchIdx];
				BatchOfEquation[Equation.Handle] = BatchIdx;
			}
		}
		else if(IsValid(Solver))
		{
			int BatchIdx = BatchOfSolver[Solver.Handle];
			if(BatchIdx != -1)
			{
				MakeNew = false;
				Batch = &PreBatches[BatchIdx];
				BatchOfEquation[Equation.Handle] = BatchIdx;
			}
		}
		
		if(MakeNew)
		{
			PreBatches.push_back({});
			int BatchIdx = PreBatches.size()-1;
			Batch = &PreBatches[BatchIdx];
			BatchOfEquation[Equation.Handle] = BatchIdx;
			if(CareAboutConditionals && IsValid(Conditional))
			{
				Batch->Conditional = Conditional;
				BatchOfConditional[Conditional.Handle] = BatchIdx;
			}
			else if(IsValid(Solver))
			{
				Batch->Solver = Solver;
				BatchOfSolver[Solver.Handle] = BatchIdx;
			}
			
		}
		
		Batch->Equations.push_back(Equation);
		Batch->IndexSetDependencies.insert(Spec.IndexSetDependencies.begin(), Spec.IndexSetDependencies.end());
		
		for(equation_h Dependency : Spec.DirectResultDependencies)
		{
			equation_spec &DepSpec = Model->Equations[Dependency];
			if(CareAboutConditionals && IsValid(Conditional) && Conditional == GetConditional(Model, Dependency)) continue;
			else if(IsValid(Solver) && DepSpec.Solver == Solver) continue;
			
			dependency_entry Dep;
			Dep.WhichEquation = Equation;
			Dep.DependsOnEquation = Dependency;
			Dep.ImplicitlyIndexed = true;
			Dep.BatchIndex = -1; //NOTE: Can't be resolved at this point;
			Batch->Dependencies.push_back(Dep);
		}
		for(equation_h Dependency : Spec.CrossIndexResultDependencies)
		{
			equation_spec &DepSpec = Model->Equations[Dependency];
			if(CareAboutConditionals && IsValid(Conditional) && Conditional == GetConditional(Model, Dependency)) continue;
			else if(IsValid(Solver) && DepSpec.Solver == Solver) continue;
			
			dependency_entry Dep;
			Dep.WhichEquation = Equation;
			Dep.DependsOnEquation = Dependency;
			Dep.ImplicitlyIndexed = false;
			Dep.BatchIndex = -1; //NOTE: Can't be resolved at this point;
			Batch->Dependencies.push_back(Dep);
		}
	}
	
	// Resolve batch indexes in references between pre_batches in dependencies
	for(pre_batch &Batch : PreBatches)
		for(dependency_entry &Dep : Batch.Dependencies)
			Dep.BatchIndex = BatchOfEquation[Dep.DependsOnEquation.Handle];
		
	// Check if a pre_batch directly depends on another one with a mutually exclusive conditional
	if(CareAboutConditionals)
	{
		for(pre_batch &Batch : PreBatches)
		{
			if(IsValid(Batch.Conditional))
				for(dependency_entry &Dep : Batch.Dependencies)
				{
					if(!Dep.ImplicitlyIndexed) continue;
					pre_batch &OtherBatch = PreBatches[Dep.BatchIndex];
					if(IsValid(Batch.Conditional))
					{
						conditional_spec &FirstSpec = Model->Conditionals[Batch.Conditional];
						conditional_spec &SecondSpec = Model->Conditionals[OtherBatch.Conditional];
						if(FirstSpec.Switch == SecondSpec.Switch && FirstSpec.Value != SecondSpec.Value)
							FatalError("ERROR: The equation \"", GetName(Model, Dep.WhichEquation), "\" depends directly on the equation \"", GetName(Model, Dep.DependsOnEquation), "\" but they belong to conditional executions that are mutually exclusive.\n");
					}
				}
		}
	}
	
	std::vector<size_t> SortedBatchIndexes;
	SortedBatchIndexes.reserve(PreBatches.size());
	TopologicalSortBatches(Model, PreBatches, SortedBatchIndexes, CareAboutConditionals);
	//NOTE: We no longer care about what equations caused the dependencies, so we consider two dependencies between the same pre_batch a duplicate even if it was caused by different equations.
	for(pre_batch &Batch : PreBatches)
		Batch.Dependencies.erase(std::unique(Batch.Dependencies.begin(), Batch.Dependencies.end()), Batch.Dependencies.end()); // Erase duplicates
	
	for(size_t BatchIndex : SortedBatchIndexes)
	{	
		pre_batch &Batch = PreBatches[BatchIndex];
		
		s32 EarliestSuitableGroupIdx = GroupBuild.size();
		s32 EarliestSuitablePosition = GroupBuild.size();
		
		for(s32 GroupIdx = GroupBuild.size() - 1; GroupIdx >= 0; --GroupIdx)
		{
			pre_batch_group &Group = GroupBuild[GroupIdx];
			
			//NOTE: We want to put conditional execution branches in their own group as a proxy that later will be replaced by a proper group structure. This group can then only contain the single pre_batch that is the proxy.
			if(Batch.IndexSetDependencies == Group.IndexSetDependencies && !(CareAboutConditionals && (IsValid(Batch.Conditional) || IsValid(Group.Conditional))))
				EarliestSuitableGroupIdx = GroupIdx;
			
			bool DependOnGroup = false;
			for(dependency_entry Dep : Batch.Dependencies)
			{
				if(std::find(Group.PreBatchIndexes.begin(), Group.PreBatchIndexes.end(), Dep.BatchIndex) != Group.PreBatchIndexes.end())
				{
					DependOnGroup = true;
					break;
				}
			}
			
			if(DependOnGroup) break; // We can not enter a position that is earlier than this group since we depend on a pre_batch in this group.
			
			EarliestSuitablePosition = GroupIdx;
		}
		
		if(EarliestSuitableGroupIdx == (s32)GroupBuild.size())
		{
			//We did not find a suitable group to enter, make a new one.
			pre_batch_group NewGroup = {};
			NewGroup.PreBatchIndexes.push_back(BatchIndex);
			NewGroup.IndexSetDependencies = Batch.IndexSetDependencies;
			if(CareAboutConditionals && IsValid(Batch.Conditional))
				NewGroup.Conditional = Batch.Conditional;
			
			if(EarliestSuitablePosition == (s32)GroupBuild.size())
				GroupBuild.push_back(NewGroup);
			else
				GroupBuild.insert(GroupBuild.begin()+EarliestSuitablePosition, NewGroup);
		}
		else
		{
			GroupBuild[EarliestSuitableGroupIdx].PreBatchIndexes.push_back(BatchIndex);
		}
	}
	
#if 0
	std::cout << "First pass\n";
	for(pre_batch_group &Group : GroupBuild)
	{
		if(Group.IndexSetDependencies.empty()) std::cout << "[]"; 
		for(index_set_h IndexSet : Group.IndexSetDependencies)
			std::cout << "[" << GetName(Model, IndexSet) << "]";
		std::cout << "\n";
		for(size_t BatchIdx : Group.PreBatchIndexes)
		{
			pre_batch &Batch = PreBatches[BatchIdx];
			if(IsValid(Batch.Solver))
				std::cout << "\tSOLVER (" << GetName(Model, Batch.Solver) << ")\n";
			for(equation_h Eq : Batch.Equations)
			{
				if(!IsValid(Eq)) std::cout << "INVALID!";
				else std::cout << "\t\t" << GetName(Model, Eq) << "\n";
			}
			if(IsValid(Batch.Solver))
				std::cout << "\tEND SOLVER\n";
		}
	}
#endif
	
	//NOTE: We do a second pass to see if some equations can be shifted to a later batch. This may ultimately reduce the amount of batch groups and speed up execution. It will also make sure that cross indexing between results is more likely to be correct.
	// (TODO: this needs a better explanation, but for instance SimplyP gets a more fragmented run structure without this second pass).
	//TODO: Maaybe this could be done in the same pass as above, but I haven't figured out how. The problem is that while we are building the batches above, we don't know about any of the batches that will appear after the current batch we are building.
	
	//NOTE: In some models we have to do this pass twice to get a good structure.. We just have to be wary and see if more passes are needed later!
	for(size_t It = 0; It < 2; ++It)
	{
		for(size_t GroupIdx = 0; GroupIdx < GroupBuild.size(); ++GroupIdx)
		{
			pre_batch_group &Group = GroupBuild[GroupIdx];
			if(CareAboutConditionals && IsValid(Group.Conditional)) continue;
			
			s64 BatchIdxIdx = Group.PreBatchIndexes.size() - 1;
			while(BatchIdxIdx >= 0)
			{
				size_t ThisBatchIdx = Group.PreBatchIndexes[BatchIdxIdx];
				pre_batch &ThisBatch = PreBatches[ThisBatchIdx];
				
				bool Continue = false;
				for(size_t BatchBehind = BatchIdxIdx+1; BatchBehind < Group.PreBatchIndexes.size(); ++BatchBehind)
				{
					size_t BehindIdx = Group.PreBatchIndexes[BatchBehind];
					pre_batch &Behind = PreBatches[BehindIdx];
					for(dependency_entry &Dep : Behind.Dependencies)
					{
						if(Dep.BatchIndex == ThisBatchIdx)
						{
							// If another pre_batch behind us in this group depend on us, we are not allowed to move.
							Continue = true;
							break;
						}
					}
					if(Continue) break;
				}
				if(Continue)
				{
					BatchIdxIdx--;
					continue;
				}
				
				size_t LastSuitableGroup = GroupIdx;
				for(size_t GroupBehind = GroupIdx+1; GroupBehind < GroupBuild.size(); ++GroupBehind)
				{
					pre_batch_group &NextGroup = GroupBuild[GroupBehind];
					if(NextGroup.IndexSetDependencies == ThisBatch.IndexSetDependencies && !(CareAboutConditionals && IsValid(NextGroup.Conditional)))
						LastSuitableGroup = GroupBehind;
					
					bool GroupDependsOnUs = false;
					for(size_t OtherBatchIdx : NextGroup.PreBatchIndexes)
					{
						pre_batch &Other = PreBatches[OtherBatchIdx];
						for(dependency_entry &Dep : Other.Dependencies)
						{
							if(Dep.BatchIndex == ThisBatchIdx)
							{
								GroupDependsOnUs = true;
								break;
							}
						}
						if(GroupDependsOnUs) break;
					}
					
					if(GroupDependsOnUs || GroupBehind == GroupBuild.size()-1)
					{
						if(LastSuitableGroup != GroupIdx)
						{
							//Move to the front of that group
							pre_batch_group &InsertTo = GroupBuild[LastSuitableGroup];
							InsertTo.PreBatchIndexes.insert(InsertTo.PreBatchIndexes.begin(), ThisBatchIdx);
							Group.PreBatchIndexes.erase(Group.PreBatchIndexes.begin()+BatchIdxIdx);
						}
						break;
					}
				}
				BatchIdxIdx--;
			}
		}
	}
	
	{
		//NOTE: Erase batch groups that were emptied as a result of the step above
		s64 GroupIdx = GroupBuild.size()-1;
		while(GroupIdx >= 0)
		{
			pre_batch_group &Group = GroupBuild[GroupIdx];
			if(Group.PreBatchIndexes.empty())
			{
				GroupBuild.erase(GroupBuild.begin() + GroupIdx);
			}
			--GroupIdx;
		}
	}

#if 0
	std::cout << "Second pass\n\n\n";
	for(pre_batch_group &Group : GroupBuild)
	{
		if(Group.IndexSetDependencies.empty()) std::cout << "[]"; 
		for(index_set_h IndexSet : Group.IndexSetDependencies)
			std::cout << "[" << GetName(Model, IndexSet) << "]";
		std::cout << "\n";
		for(size_t BatchIdx : Group.PreBatchIndexes)
		{
			pre_batch &Batch = PreBatches[BatchIdx];
			if(IsValid(Batch.Solver))
				std::cout << "\tSOLVER (" << GetName(Model, Batch.Solver) << ")\n";
			for(equation_h Eq : Batch.Equations)
			{
				if(!IsValid(Eq)) std::cout << "INVALID!";
				else std::cout << "\t\t" << GetName(Model, Eq) << "\n";
			}
			if(IsValid(Batch.Solver))
				std::cout << "\tEND SOLVER\n";
		}
	}
#endif

	TemporaryBucket.DeallocateAll();
}

static void
AddBatchGroup(std::vector<equation_batch_group> &BatchGroups, equation_batch_group &NewGroup)
{
	if(BatchGroups.empty())
	{
		BatchGroups.push_back(NewGroup);
		return;
	}
	
	equation_batch_group &LastGroup = BatchGroups[BatchGroups.size()-1];
	assert(LastGroup.LastBatch+1 == NewGroup.FirstBatch);
	
	//Merge the new group into the last group if they have the same index set dependencies
	//TODO: There could potentially be a problem here with the index sets not being sorted the same way since we haven't applied the canonical sorting order yet? If that is a problem we have to do some kind of set compare instead of array compare.
	if(LastGroup.IndexSets == NewGroup.IndexSets)
		LastGroup.LastBatch = NewGroup.LastBatch;
	else
		BatchGroups.push_back(NewGroup);
}

static void
ProcessBatchGroups(mobius_model *Model, std::vector<pre_batch> &PreBatches, std::vector<pre_batch_group> &GroupBuild, std::vector<equation_batch> &EquationBatches, std::vector<equation_batch_group> &BatchGroups, bool CareAboutConditionals = true)
{
	size_t BatchIdx   = 0;
	for(pre_batch_group &PreGroup : GroupBuild)
	{	
		if(!(CareAboutConditionals && IsValid(PreGroup.Conditional)))
		{
			equation_batch_group NewGroup = {};
			NewGroup.IndexSets.CopyFrom(&Model->BucketMemory, PreGroup.IndexSetDependencies);
			NewGroup.FirstBatch = BatchIdx;
			
			std::vector<equation_h> BatchEquations;
			
			for(size_t PreBatchIdx : PreGroup.PreBatchIndexes)
			{
				pre_batch &PreBatch = PreBatches[PreBatchIdx];
				
				if(IsValid(PreBatch.Solver))
				{
					if(!BatchEquations.empty())
					{
						// We hit a solver, so we have to make a batch out of the single equations we hit so far.
						equation_batch Batch = {};
						Batch.Equations.CopyFrom(&Model->BucketMemory, BatchEquations);
						BatchEquations.clear();
						EquationBatches.push_back(Batch);
						++BatchIdx;
					}
					equation_batch Batch = {};
					
					Batch.Solver = PreBatch.Solver;
					
					size_t ODECount = 0;
					for(equation_h Eq: PreBatch.Equations)
						if(Model->Equations[Eq].Type == EquationType_ODE)
							++ODECount;
					
					Batch.Equations.Allocate(&Model->BucketMemory, PreBatch.Equations.size()-ODECount);
					Batch.EquationsODE.Allocate(&Model->BucketMemory, ODECount);
					
					size_t EIdx = 0, ODEIdx = 0;
					for(equation_h Eq: PreBatch.Equations)
						if(Model->Equations[Eq].Type == EquationType_ODE)
							Batch.EquationsODE[ODEIdx++] = Eq;
						else
							Batch.Equations[EIdx++] = Eq;
					
					//Sort the non-ode equations in the batch among themselves.
					TopologicalSortEquations(Model, Batch.Equations, TopologicalSortEquationsInSolverVisit);
					
					EquationBatches.push_back(Batch);
					++BatchIdx;
				}
				else
					BatchEquations.push_back(PreBatch.Equations[0]);
			}
			
			if(!BatchEquations.empty())
			{
				// Gather up any remaining single equations.
				equation_batch Batch = {};
				Batch.Equations.CopyFrom(&Model->BucketMemory, BatchEquations);
				EquationBatches.push_back(Batch);
				++BatchIdx;
			}
			
			NewGroup.LastBatch = BatchIdx-1;
			
			AddBatchGroup(BatchGroups, NewGroup);
		}
		else // if CareAboutConditionals && IsValid(PreGroup.Conditional)
		{
			//NOTE: In this case, the group should only contain one "proxy" pre_batch, that should now be replaced with its own group structure.
			assert(PreGroup.PreBatchIndexes.size() == 1);
			
			pre_batch &PreBatch = PreBatches[PreGroup.PreBatchIndexes[0]];
			
			std::vector<pre_batch> InnerPreBatches;
			std::vector<pre_batch_group> InnerGroupBuild;
			
			GroupPreBatches(Model, PreBatch.Equations, InnerPreBatches, InnerGroupBuild, false);
			
			std::vector<equation_batch> InnerEquationBatches;
			std::vector<equation_batch_group> InnerBatchGroups;
			
			ProcessBatchGroups(Model, InnerPreBatches, InnerGroupBuild, InnerEquationBatches, InnerBatchGroups, false); //The false signifies that in this new pass, we don't care about conditionals (we don't have nested conditionals)
			for(equation_batch_group &Group : InnerBatchGroups)
			{
				Group.FirstBatch += BatchIdx;
				Group.LastBatch += BatchIdx;
				
				AddBatchGroup(BatchGroups, Group);
			}
			
			for(equation_batch &Batch : InnerEquationBatches)
			{
				Batch.Conditional = PreGroup.Conditional;
				conditional_spec &ConditionalSpec = Model->Conditionals[PreGroup.Conditional];
				Batch.ConditionalSwitch = ConditionalSpec.Switch;
				Batch.ConditionalValue  = ConditionalSpec.Value;
			}
			
			EquationBatches.insert(EquationBatches.end(), InnerEquationBatches.begin(), InnerEquationBatches.end());
			
			BatchIdx += InnerEquationBatches.size();
		}
	}
}

static void
EndModelDefinition(mobius_model *Model)
{
	if(Model->Finalized)
		FatalError("ERROR: Called EndModelDefinition twice on the same model.\n");
	
	bucket_allocator TemporaryBucket;
	TemporaryBucket.Initialize(1024*1024);
	
	///////////// Find out what index sets each parameter depends on /////////////
	
	for(parameter_h Parameter : Model->Parameters)
	{
		parameter_spec &Spec = Model->Parameters[Parameter];
		parameter_group_h CurrentGroup = Spec.Group;
		const parameter_group_spec &GroupSpec = Model->ParameterGroups[CurrentGroup];
		Spec.IndexSetDependencies = GroupSpec.IndexSets;  //NOTE: vector copy.
	}
	
	/////////////////////// Find all dependencies of equations on parameters, inputs and other results /////////////////////
	
	model_run_state RunState(Model);
	for(equation_h Equation : Model->Equations)
	{
		equation_spec &Spec = Model->Equations[Equation];
		
		if(Spec.Type == EquationType_Cumulative)
		{
			//NOTE: Cumulative equations depend on the equations they cumulate (only).
			Spec.DirectResultDependencies.insert(Spec.Cumulates);
			continue;
		}
		
		if(IsValid(Spec.IsComputedBy))
		{
			//NOTE: If an equation is declared as IsComputedBy, the index set dependencies were declared in that call, and will not be found out using the method below.
			Spec.IndexSetDependencies.insert(Spec.IndexSetDependencies_ComputedBy.begin(), Spec.IndexSetDependencies_ComputedBy.end());
			continue;
		}
		
		if(!Spec.EquationIsSet)
			FatalError("ERROR: The equation body for the registered equation \"", GetName(Model, Equation), "\" has not been defined.\n");
		
		
		// Clear dependency registrations from evaluation of previous equation.
		RunState.Clear();
		
		//Call the equation. Since we are in RunState.Running==false mode, the equation will register which values it tried to access.
		Model->EquationBodies[Equation.Handle](&RunState);
		
		Spec.IndexSetDependencies.insert(RunState.DirectIndexSetDependencies.begin(), RunState.DirectIndexSetDependencies.end());
		
		for(auto &ParameterDependency : RunState.ParameterDependencies)
		{
			parameter_h Parameter = ParameterDependency.Handle;
			
			parameter_spec &ParSpec = Model->Parameters[Parameter];
			std::vector<index_set_h>& IndexSetDependencies = ParSpec.IndexSetDependencies;
			if(ParameterDependency.NumExplicitIndexes > IndexSetDependencies.size())
				FatalError("ERROR: In equation \"", Spec.Name, "\". The parameter \"", ParSpec.Name, "\" is referenced with more explicit indexes than the number of index sets this parameter depends on.\n");
			
			size_t NumImplicitIndexes = IndexSetDependencies.size() - ParameterDependency.NumExplicitIndexes;
			
			if(NumImplicitIndexes > 0)
				Spec.IndexSetDependencies.insert(IndexSetDependencies.begin(), IndexSetDependencies.begin() + NumImplicitIndexes);
			
			if(ParameterDependency.NumExplicitIndexes == 0)
			{
				//NOTE: We only store the parameters that should be hotloaded at the start of the batch in this vector: For various reasons we can't do that with parameters that are referred to by explicit indexing.
				//TODO: We should maybe store a cross-index parameter dependency list for easy referencing later (during debugging etc.).
				Spec.ParameterDependencies.insert(Parameter);
			}
		}
		
		for(auto &InputDependency : RunState.InputDependencies)
		{
			//TODO: This block has to be updated to match the parameter registration above if we later allow for explicitly indexed inputs.
			input_h Input = InputDependency.Handle;
			std::vector<index_set_h>& IndexSetDependencies = Model->Inputs[Input].IndexSetDependencies;
			Spec.IndexSetDependencies.insert(IndexSetDependencies.begin(), IndexSetDependencies.end());
			Spec.InputDependencies.insert(Input);
		}
		
		//NOTE: Every equation always depends on its initial value parameter if it has one.
		//TODO: We should have a specialized system for this, because this currently causes the initial value parameter to be loaded into the CurParameters buffer at each step (instead of just during the initial value step), which is unnecessary.
		if(IsValid(Spec.InitialValue))
		{
			std::vector<index_set_h>& IndexSetDependencies = Model->Parameters[Spec.InitialValue].IndexSetDependencies;
			Spec.IndexSetDependencies.insert(IndexSetDependencies.begin(), IndexSetDependencies.end());
			Spec.ParameterDependencies.insert(Spec.InitialValue);
		}
		
		if(IsValid(Spec.Conditional))
		{
			conditional_spec &Conditional = Model->Conditionals[Spec.Conditional];
			std::vector<index_set_h>& IndexSetDependencies = Model->Parameters[Conditional.Switch].IndexSetDependencies;
			Spec.IndexSetDependencies.insert(IndexSetDependencies.begin(), IndexSetDependencies.end());
			Spec.ParameterDependencies.insert(Conditional.Switch);
		}
		
		for(const result_dependency_registration &ResultDependency : RunState.ResultDependencies)
		{
			equation_h DepResult = equation_h {ResultDependency.Handle};
			
			if(Model->Equations[DepResult].Type == EquationType_InitialValue)
				FatalError("ERROR: The equation \"", GetName(Model, Equation), "\" depends explicitly on the result of the equation \"", GetName(Model, DepResult), "\" which is an EquationInitialValue. This is not allowed, instead it should depend on the result of the equation that \"", GetName(Model, DepResult), "\" is an initial value for.\n");
			
			if(ResultDependency.Indexes.size() == 0)
			{
				//NOTE: This is a bit hacky, since the DepResult should really depend directly on it's IsComputedBy, but that screws up the initial value system at least for SedFlex.
				//   We should really improve the initial value system...
				equation_spec &DepSpec = Model->Equations[DepResult];
				if(IsValid(DepSpec.IsComputedBy))
					Spec.DirectResultDependencies.insert(DepSpec.IsComputedBy);
				
				Spec.DirectResultDependencies.insert(DepResult);
			}
			else
			{
				Spec.CrossIndexResultDependencies.insert(DepResult); //TODO: Do we really need to keep this separately?
				Spec.IndexedResultAndLastResultDependencies.push_back(ResultDependency); //TODO: Maybe don't store these on the equation spec? They are only needed in this algorithm..
			}
		}
		
		for(const result_dependency_registration &ResultDependency : RunState.LastResultDependencies)
		{
			equation_h DepResult = equation_h {ResultDependency.Handle};
			if(Model->Equations[DepResult].Type == EquationType_InitialValue)
				FatalError("ERROR: The equation \"", GetName(Model, Equation), "\" depends explicitly on the (last) result of the equation \"", GetName(Model, DepResult), "\" which is an EquationInitialValue. This is not allowed, instead it should depend on the result of the equation that \"", GetName(Model, DepResult), "\" is an initial value for.\n");
			
			if(ResultDependency.Indexes.size() == 0)
				Spec.DirectLastResultDependencies.insert(DepResult);
			else
				Spec.IndexedResultAndLastResultDependencies.push_back(ResultDependency);
		}
		
		//NOTE: Every equation always depends on its initial value equation if it has one.
		//TODO: Right now we register it as a LastResultDependency, which is not technically correct, but it should give the desired result.
		//TODO: Figure out if this may break something, and if we need a specialized system for this?
		equation_h EqInitialValue = Spec.InitialValueEquation;
		if(IsValid(EqInitialValue))
			Spec.DirectLastResultDependencies.insert(EqInitialValue);
	}
	
	{
		//NOTE: Check computed parameters to see if their equation satisfies the requirements specified in the documentation:
		for(parameter_h Parameter : Model->Parameters)
		{
			parameter_spec &Spec = Model->Parameters[Parameter];
			equation_h Equation = Spec.IsComputedBy;
			if(IsValid(Equation))
			{
				equation_spec &EqSpec = Model->Equations[Equation];
				
				if(
					   !EqSpec.DirectResultDependencies.empty()
					|| !EqSpec.DirectLastResultDependencies.empty()
					|| !EqSpec.IndexedResultAndLastResultDependencies.empty()
					|| !EqSpec.InputDependencies.empty()
				)
					FatalError("ERROR: The initial value equation ", EqSpec.Name, " assigned to compute the parameter ", Spec.Name, " depends on either a result of an equation or an input. This is not allowed for computed parameters.\n");
			}
		}
	}
	
	///////////////////// Resolve indirect dependencies of equations on index sets.
	
	//TODO: This is probably an inefficient way to do it, we should instead use some kind of graph traversal, but it is tricky. To do it properly one would have to collapse the dependency graph (including both results and lastresults) by its strongly connected components, then resolving the dependencies between the components.  However the current implementation has proven fast enough so far.
	//NOTE: We stop the iteraton at 1000 so that if the dependencies are unresolvable, we don't go in an infinite loop. (they can probably never become unresolvable though??)
	bool DependenciesWereResolved = false;
	for(size_t It = 0; It < 1000; ++It)
	{
		bool Changed = false;
		for(equation_h Equation : Model->Equations)
		{
			equation_spec &Spec = Model->Equations[Equation];
			size_t DependencyCount = Spec.IndexSetDependencies.size();
				
			if(Spec.Type == EquationType_Cumulative)
			{
				equation_spec &DepSpec = Model->Equations[Spec.Cumulates];
				for(index_set_h IndexSet : DepSpec.IndexSetDependencies)
				{
					if(IndexSet != Spec.CumulatesOverIndexSet) Spec.IndexSetDependencies.insert(IndexSet); 
				}
				parameter_spec &WeightSpec = Model->Parameters[Spec.CumulationWeight];
				for(index_set_h IndexSet : WeightSpec.IndexSetDependencies)
				{
					if(IndexSet != Spec.CumulatesOverIndexSet) Spec.IndexSetDependencies.insert(IndexSet); 
				}
			}
			else
			{
				for(equation_h ResultDependency : Spec.DirectResultDependencies)
				{
					equation_spec &DepSpec = Model->Equations[ResultDependency];
					Spec.IndexSetDependencies.insert(DepSpec.IndexSetDependencies.begin(), DepSpec.IndexSetDependencies.end());
				}
				for(equation_h ResultDependency : Spec.DirectLastResultDependencies)
				{
					equation_spec &DepSpec = Model->Equations[ResultDependency];
					Spec.IndexSetDependencies.insert(DepSpec.IndexSetDependencies.begin(), DepSpec.IndexSetDependencies.end());
				}
				for(const result_dependency_registration &ResultDependency : Spec.IndexedResultAndLastResultDependencies)
				{
					equation_spec &DepSpec = Model->Equations[ResultDependency.Handle];
					const std::set<index_set_h> &IndexSetDependencies = DepSpec.IndexSetDependencies;
					for(index_set_h IndexSet : IndexSetDependencies)
					{
						bool ExplicitlyIndexed = false;
						for(index_t Index : ResultDependency.Indexes)
						{
							if(Index.IndexSetHandle == IndexSet.Handle)
							{
								ExplicitlyIndexed = true;
								break;
							}
						}
						if(!ExplicitlyIndexed) Spec.IndexSetDependencies.insert(IndexSet);
					}
				}
			}
			if(DependencyCount != Spec.IndexSetDependencies.size()) Changed = true;
		}
		
		if(!Changed)
		{
			DependenciesWereResolved = true;
#if MOBIUS_PRINT_TIMING_INFO
			WarningPrint("Dependencies resolved at ", It + 1, " iterations.\n");
#endif
			break;
		}
	}
	
	if(!DependenciesWereResolved)
		FatalError("ERROR: We were unable to resolve all equation dependencies!\n");
	
	/////////////// Sorting the equations into equation batches ///////////////////////////////
	
	//NOTE: The pre_batches may initially just contain one single equation (unless they come from solvers). They are grouped into larger batches later.
	std::vector<equation_h> EquationsToSort;
	EquationsToSort.reserve(Model->Equations.Count());
	
	std::vector<pre_batch> PreBatches;
	std::vector<pre_batch_group> GroupBuild;
	
	for(equation_h Equation : Model->Equations)
	{
		//NOTE: initial value equations should not be a part of the main equation structure.
		if(Model->Equations[Equation].Type == EquationType_InitialValue) continue; 
		EquationsToSort.push_back(Equation);
	}
	
	GroupPreBatches(Model, EquationsToSort, PreBatches, GroupBuild);
	
	
	/////////////// Process the batches into a finished result structure ////////////////////////
	
	std::vector<equation_batch>       EquationBatches;
	std::vector<equation_batch_group> BatchGroups;
	
	ProcessBatchGroups(Model, PreBatches, GroupBuild, EquationBatches, BatchGroups);
	
	//Sort index sets so that the ones with more batch groups depending on them are higher up. This empirically gives a better model structure.
	array<size_t> Counts(&TemporaryBucket, Model->IndexSets.Count());
	for(auto& Group : BatchGroups)
		for(index_set_h IndexSet : Group.IndexSets)
			Counts[IndexSet.Handle]++;
	
	for(equation_batch_group &Group : BatchGroups)
	{
		std::sort(Group.IndexSets.begin(), Group.IndexSets.end(),
			[Counts] (index_set_h A, index_set_h B) { return ((Counts[A.Handle] == Counts[B.Handle]) ? (A.Handle > B.Handle) : (Counts[A.Handle] > Counts[B.Handle])); }
		);
	}
		
	Model->EquationBatches.CopyFrom(&Model->BucketMemory, EquationBatches);
	Model->BatchGroups.CopyFrom(&Model->BucketMemory, BatchGroups);
	
#if 0
	std::cout << "\nSorted structure:\n";
	for(equation_batch_group &Group : Model->BatchGroups)
	{
		if(Group.IndexSets.Count == 0) std::cout << "[]"; 
		for(index_set_h IndexSet : Group.IndexSets)
			std::cout << "[" << GetName(Model, IndexSet) << "]";
		std::cout << "\n";
		for(size_t BatchIdx = Group.FirstBatch; BatchIdx <= Group.LastBatch; ++BatchIdx)
		{
			equation_batch &Batch = Model->EquationBatches[BatchIdx];
			if(IsValid(Batch.Solver))
				std::cout << "\tSOLVER (" << GetName(Model, Batch.Solver) << ")\n";
			for(equation_h Eq : Batch.Equations)
			{
				if(!IsValid(Eq)) std::cout << "INVALID!";
				else std::cout << "\t\t" << Eq.Handle << " " << GetName(Model, Eq) << "\n";
			}
			if(IsValid(Batch.Solver))
			{
				for(equation_h Eq : Batch.EquationsODE)
				{
					if(!IsValid(Eq)) std::cout << "INVALID!";
					else std::cout << "\t\t(ODE) " << Eq.Handle << " " << GetName(Model, Eq) << "\n";
				}
				std::cout << "\tEND SOLVER\n";
			}
		}
	}
#endif
	
	//NOTE: Clear flags since they may be reused later in the Initial value setup.
	for(equation_h Eq : Model->Equations)
	{
		Model->Equations[Eq].Visited = false;
		Model->Equations[Eq].TempVisited = false;
	}
	
	array<s64> EquationBelongsToBatchGroup(&TemporaryBucket, Model->Equations.Count());
	for(size_t Idx = 0; Idx < Model->Equations.Count(); ++Idx)
		EquationBelongsToBatchGroup[Idx] = -1;
	
	//NOTE: Sort the order of initial value equations in the batch group.
	//Also store references about batch group belonging for later.
	std::vector<std::vector<equation_h>> InitialValueOrders(Model->BatchGroups.Count);

	s64 BatchGroupIdx = 0;
	for(equation_batch_group &Group : Model->BatchGroups)
	{
		std::vector<equation_h> &InitialValueOrder = InitialValueOrders[BatchGroupIdx];
		for(size_t BatchIdx = Group.FirstBatch; BatchIdx <= Group.LastBatch; ++BatchIdx)
		{
			equation_batch &Batch = Model->EquationBatches[BatchIdx];
			
			ForAllBatchEquations(Batch,
			[Model, &Batch, &InitialValueOrder](equation_h Equation)
			{
				equation_spec &Spec = Model->Equations[Equation];
				if(IsValid(Spec.InitialValueEquation) || IsValid(Spec.InitialValue) || Spec.HasExplicitInitialValue) //NOTE: We only care about equations that have an initial value (or that are depended on by an initial value equation, but they are added recursively inside the topological sort below)
					InitialValueOrder.push_back(Equation);
				return false;
			});
			
			ForAllBatchEquations(Batch,
			[&EquationBelongsToBatchGroup, BatchGroupIdx](equation_h Equation)
			{
				EquationBelongsToBatchGroup[Equation.Handle] = BatchGroupIdx;
				return false;
			});
		}
		
		//NOTE: In this setup of initial values, we get a problem if an initial value of an equation in batch group A depends on the (initial) value of an equation in batch group B and batch group A != B. If we want to allow this, we need a completely separate batch structure for the initial value equations....

#if 0		
		std::cout << "***************** Initial value order pre\n";
		for(equation_h Equation : InitialValueOrder)
			std::cout << GetName(Model, Equation) << "\n";
#endif
	
		TopologicalSortEquations(Model, InitialValueOrder, TopologicalSortEquationsInitialValueVisit);
		
#if 0
		std::cout << "***************** Initial value order post\n";
		for(equation_h Equation : InitialValueOrder)
			std::cout << GetName(Model, Equation) << "\n";
#endif

	
		//TODO: There could also be problems if BelongsTo > BatchGroupIdx. This is because values may not be properly loaded into the buffers.
		for(auto Equation : InitialValueOrder)
		{
			s64 BelongsTo = EquationBelongsToBatchGroup[Equation.Handle];
			if(BelongsTo == -1)
			{
				WarningPrint("WARNING: The value of the equation \"", GetName(Model, Equation), "\" in batch group ", BelongsTo, " is depended on by an equation in the other batch group ", BatchGroupIdx, ". This may cause unreliable behaviour.\n");
			}
		}
		
		
		++BatchGroupIdx;
	}
	
	for(s64 BatchGroupIdx = 0; BatchGroupIdx < Model->BatchGroups.Count; ++BatchGroupIdx)
		Model->BatchGroups[BatchGroupIdx].InitialValueOrder.CopyFrom(&Model->BucketMemory, InitialValueOrders[BatchGroupIdx]);
	
	InitialValueOrders.clear();
	
	///////////////// Find out which parameters, results and last_results that need to be hotloaded into the CurParameters, CurInputs etc. buffers in the model_run_state at each iteration stage during model run. /////////////////
	
	{
		size_t BatchGroupIdx = 0;
		for(equation_batch_group &BatchGroup : Model->BatchGroups)
		{
			std::set<parameter_h> AllParameterDependenciesForBatchGroup;
			std::set<equation_h>  AllResultDependenciesForBatchGroup;
			std::set<equation_h>  AllLastResultDependenciesForBatchGroup;
			std::set<input_h>     AllInputDependenciesForBatchGroup;
			
			for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
			{
				equation_batch &Batch = Model->EquationBatches[BatchIdx];
				
				if(IsValid(Batch.Solver))
				{
					solver_spec &SolverSpec = Model->Solvers[Batch.Solver];
					if(IsValid(SolverSpec.hParam)) AllParameterDependenciesForBatchGroup.insert(SolverSpec.hParam);
				}
				
				if(IsValid(Batch.ConditionalSwitch))
					AllParameterDependenciesForBatchGroup.insert(Batch.ConditionalSwitch);
				
				ForAllBatchEquations(Batch,
				[Model, &AllParameterDependenciesForBatchGroup, &AllInputDependenciesForBatchGroup, &AllResultDependenciesForBatchGroup, &AllLastResultDependenciesForBatchGroup](equation_h Equation)
				{
					equation_spec &Spec = Model->Equations[Equation];
					if(Spec.Type != EquationType_Cumulative) //NOTE: We don't have to load in dependencies for cumulative equations since these get their data directly from the DataSet.
					{						
						AllParameterDependenciesForBatchGroup.insert(Spec.ParameterDependencies.begin(), Spec.ParameterDependencies.end());
						AllInputDependenciesForBatchGroup.insert(Spec.InputDependencies.begin(), Spec.InputDependencies.end());
						AllResultDependenciesForBatchGroup.insert(Spec.DirectResultDependencies.begin(), Spec.DirectResultDependencies.end());
						AllLastResultDependenciesForBatchGroup.insert(Spec.DirectLastResultDependencies.begin(), Spec.DirectLastResultDependencies.end());
						//TODO: The following is a quick fix so that initial value equations get their parameters and inputs set correctly. However it causes unneeded parameters to be loaded at each step for the main execution. We should make a separate system for initial value equations.
						
						if(IsValid(Spec.InitialValueEquation))
						{
							equation_spec &InitSpec = Model->Equations[Spec.InitialValueEquation];
							AllParameterDependenciesForBatchGroup.insert(InitSpec.ParameterDependencies.begin(), InitSpec.ParameterDependencies.end());
							AllInputDependenciesForBatchGroup.insert(InitSpec.InputDependencies.begin(), InitSpec.InputDependencies.end());
						}
					}
					return false;
				});
			}
			
			BatchGroup.IterationData.Allocate(&Model->BucketMemory, BatchGroup.IndexSets.Count);
			
			for(size_t IndexSetLevel = 0; IndexSetLevel < BatchGroup.IndexSets.Count; ++IndexSetLevel)
			{	
				std::vector<parameter_h> ParametersToRead;
				std::vector<input_h>       InputsToRead;
				std::vector<equation_h>    ResultsToRead;
				std::vector<equation_h>    LastResultsToRead;
				//NOTE: Gather up all the parameters that need to be updated at this stage of the execution tree. By updated we mean that they need to be read into the CurParameters buffer during execution.
				//TODO: We do a lot of redundant checks here. We could store temporary information to speed this up.
				for(parameter_h Parameter : AllParameterDependenciesForBatchGroup)
				{
					std::vector<index_set_h> &ThisParDependsOn = Model->Parameters[Parameter].IndexSetDependencies;
					if(IsTopIndexSetForThisDependency(ThisParDependsOn, BatchGroup.IndexSets, IndexSetLevel))
					{
						ParametersToRead.push_back(Parameter);
					}
				}
				
				BatchGroup.IterationData[IndexSetLevel].ParametersToRead.CopyFrom(&Model->BucketMemory, ParametersToRead);
				
				for(input_h Input : AllInputDependenciesForBatchGroup)
				{
					std::vector<index_set_h> &ThisInputDependsOn = Model->Inputs[Input].IndexSetDependencies;
					if(IsTopIndexSetForThisDependency(ThisInputDependsOn, BatchGroup.IndexSets, IndexSetLevel))
					{
						InputsToRead.push_back(Input);
					}
				}
				
				BatchGroup.IterationData[IndexSetLevel].InputsToRead.CopyFrom(&Model->BucketMemory, InputsToRead);
				
				for(equation_h Equation : AllResultDependenciesForBatchGroup)
				{
					equation_spec &Spec = Model->Equations[Equation];
					if(Spec.Type != EquationType_InitialValue) //NOTE: Initial values are handled separately in an initial setup run.
					{
						size_t ResultBatchGroupIndex = EquationBelongsToBatchGroup[Equation.Handle];
						if(ResultBatchGroupIndex < BatchGroupIdx) //NOTE: Results in the current batch group will be correct any way, and by definition we can not depend on any batches that are after this one.
						{
							std::vector<index_set_h> ThisResultDependsOn;
							//ugh, we should probably use array everywhere to not have to do this:
							ThisResultDependsOn.insert(ThisResultDependsOn.end(), Model->BatchGroups[ResultBatchGroupIndex].IndexSets.begin(), Model->BatchGroups[ResultBatchGroupIndex].IndexSets.end());
							
							if(IsTopIndexSetForThisDependency(ThisResultDependsOn, BatchGroup.IndexSets, IndexSetLevel))
							{
								ResultsToRead.push_back(Equation);
							}
						} 
					}
				}
				
				BatchGroup.IterationData[IndexSetLevel].ResultsToRead.CopyFrom(&Model->BucketMemory, ResultsToRead);
				
				for(equation_h Equation : AllLastResultDependenciesForBatchGroup)
				{
					equation_spec &Spec = Model->Equations[Equation];
					if(Spec.Type != EquationType_InitialValue) //NOTE: Initial values are handled separately in an initial setup run.
					{
						size_t ResultBatchGroupIndex = EquationBelongsToBatchGroup[Equation.Handle];
						if(ResultBatchGroupIndex != BatchGroupIdx) //NOTE: LAST_RESULTs in the current batch group are loaded using a different mechanism.
						{
							std::vector<index_set_h> ThisResultDependsOn;
							//ugh, we should probably use array everywhere to not have to do this:
							ThisResultDependsOn.insert(ThisResultDependsOn.end(), Model->BatchGroups[ResultBatchGroupIndex].IndexSets.begin(), Model->BatchGroups[ResultBatchGroupIndex].IndexSets.end());
							
							if(IsTopIndexSetForThisDependency(ThisResultDependsOn, BatchGroup.IndexSets, IndexSetLevel))
							{
								LastResultsToRead.push_back(Equation);
							}
						}
					}
				}
				
				BatchGroup.IterationData[IndexSetLevel].LastResultsToRead.CopyFrom(&Model->BucketMemory, LastResultsToRead);
			}
			
			std::vector<equation_h> LastResultsToReadAtBase;
			
			for(equation_h Equation : AllLastResultDependenciesForBatchGroup)    //NOTE: We need a separate system for last_results with no index set dependencies, unfortunately.
			{
				equation_spec &Spec = Model->Equations[Equation];
				if(Spec.Type != EquationType_InitialValue) //NOTE: Initial values are handled separately in an initial setup run.
				{
					size_t ResultBatchGroupIndex = EquationBelongsToBatchGroup[Equation.Handle];
					if(ResultBatchGroupIndex != BatchGroupIdx) //NOTE: LAST_RESULTs in the current batch group are loaded using a different mechanism.
					{
						array<index_set_h> &ThisResultDependsOn = Model->BatchGroups[ResultBatchGroupIndex].IndexSets;
						
						if(ThisResultDependsOn.Count == 0)
							LastResultsToReadAtBase.push_back(Equation);
					}
				}
			}
			
			BatchGroup.LastResultsToReadAtBase.CopyFrom(&Model->BucketMemory, LastResultsToReadAtBase);
			
			++BatchGroupIdx;
		}
	}
	
	//////////////////////// Gather info about (in-) direct equation dependencies to be used by the Jacobian estimation used by some implicit solvers //////////////////////////////////
	BuildJacobianInfo(Model);
	
	TemporaryBucket.DeallocateAll();

	Model->Finalized = true;
	
#if MOBIUS_PRINT_TIMING_INFO
	u64 Duration = GetTimerMilliseconds(&Model->DefinitionTimer);
	WarningPrint("Model definition time: ", Duration, " milliseconds.\n");
#endif
}

//NOTE: It is kind of superfluous to both provide the batch group and the batch group index... But it does probably not harm either?
#define INNER_LOOP_BODY(Name) void Name(mobius_data_set *DataSet, model_run_state *RunState, const equation_batch_group &BatchGroup, size_t BatchGroupIdx, s32 CurrentLevel)
typedef INNER_LOOP_BODY(mobius_inner_loop_body);

static void
ModelLoop(mobius_data_set *DataSet, model_run_state *RunState, mobius_inner_loop_body InnerLoopBody)
{
	const mobius_model *Model = DataSet->Model;
	size_t BatchGroupIdx = 0;
	for(const equation_batch_group &BatchGroup : Model->BatchGroups)
	{	
		if(BatchGroup.IndexSets.Count == 0)
		{
			InnerLoopBody(DataSet, RunState, BatchGroup, BatchGroupIdx, -1);
			BatchGroupIdx++;
			continue;
		}
		
		s32 BottomLevel = (s32)BatchGroup.IndexSets.Count - 1;
		s32 CurrentLevel = 0;
		
		while (true)
		{
			index_set_h CurrentIndexSet = BatchGroup.IndexSets[CurrentLevel];
			
			if(RunState->CurrentIndexes[CurrentIndexSet.Handle] != DataSet->IndexCounts[CurrentIndexSet.Handle])
				InnerLoopBody(DataSet, RunState, BatchGroup, BatchGroupIdx, CurrentLevel);
			
			if(CurrentLevel == BottomLevel)
				++RunState->CurrentIndexes[CurrentIndexSet.Handle];
			
			//NOTE: We need to check again because currentindex may have changed.
			if(RunState->CurrentIndexes[CurrentIndexSet.Handle] == DataSet->IndexCounts[CurrentIndexSet.Handle])
			{
				//NOTE: We are at the end of this index set
				
				RunState->CurrentIndexes[CurrentIndexSet.Handle] = {CurrentIndexSet, 0};
				//NOTE: Traverse up the tree
				if(CurrentLevel == 0) break; //NOTE: We are finished with this batch group.
				CurrentLevel--;
				CurrentIndexSet = BatchGroup.IndexSets[CurrentLevel];
				++RunState->CurrentIndexes[CurrentIndexSet.Handle]; //Advance the index set above us so that we don't walk down the same branch again.
				continue;
			}
			else if(CurrentLevel != BottomLevel)
			{
				//NOTE: If we did not reach the end index, and we are not at the bottom, we instead traverse down the tree again.
				++CurrentLevel;
			}
		}
		++BatchGroupIdx;
	}
}

#if !defined(MOBIUS_TIMESTEP_VERBOSITY)
#define MOBIUS_TIMESTEP_VERBOSITY 0
#endif

#if !defined(MOBIUS_TEST_FOR_NAN)
#define MOBIUS_TEST_FOR_NAN 0
#endif

inline void
NaNTest(const mobius_model *Model, model_run_state *RunState, double ResultValue, equation_h Equation)
{
	if(!std::isfinite(ResultValue))
	{
		ErrorPrint("ERROR: Got a NaN or Inf value as the result of the equation \"", GetName(Model, Equation), "\" at timestep ", RunState->Timestep, ", model time ", RunState->CurrentTime, " .\n");
		const equation_spec &Spec = Model->Equations[Equation];
		ErrorPrint("Indexes:\n");
		for(index_set_h IndexSet : Spec.IndexSetDependencies)
		{
			const char *IndexName = RunState->DataSet->IndexNames[IndexSet.Handle][RunState->CurrentIndexes[IndexSet.Handle]];
			ErrorPrint("\"", GetName(Model, IndexSet), "\": \"", IndexName, "\"\n");
		}
		for(parameter_h Par : Spec.ParameterDependencies )
		{
			//TODO: Make an ErrorPrintParameterValue or something like that?
			const parameter_spec &ParSpec = Model->Parameters[Par];
			parameter_type Type = ParSpec.Type;
			parameter_value Value = RunState->CurParameters[Par.Handle];
			ErrorPrint("Value of \"", GetName(Model, Par), "\" is ");
			if(Type == ParameterType_Double)
				ErrorPrint(Value.ValDouble);
			else if(Type == ParameterType_UInt)
				ErrorPrint(Value.ValUInt);
			else if(Type == ParameterType_Bool)
				ErrorPrint(Value.ValBool ? "true" : "false");
			else if(Type == ParameterType_Enum)
				ErrorPrint(ParSpec.EnumNames[Value.ValUInt]);
			
			ErrorPrint('\n');
		}
		for(input_h In : Spec.InputDependencies)
		{
			ErrorPrint("Current value of \"", GetName(Model, In), "\" is ", RunState->CurInputs[In.Handle]);
			if(!RunState->CurInputWasProvided[In.Handle])
				ErrorPrint(" (Not provided in dataset)");
			ErrorPrint('\n');
		}
		for(equation_h Res : Spec.DirectResultDependencies )
			ErrorPrint("Current value of \"", GetName(Model, Res), "\" is ", RunState->CurResults[Res.Handle], '\n');
		
		for(equation_h Res : Spec.DirectLastResultDependencies )
			if(Model->Equations[Res].Type != EquationType_InitialValue)
				ErrorPrint("Last value of \"", GetName(Model, Res), "\" was ", RunState->LastResults[Res.Handle], '\n');
		
		FatalError("");
	}
}

void ODEEquationFunction(double *x0, double *wk, model_run_state *RunState, const equation_batch *Batch)
{
	//Function that evaluates the set of ODEs once. Can be called by a solver multiple times per time step depending on the solver algorithm.
	//x0 and wk have to be pre-allocted to be large enough.
	
	const mobius_model *Model = RunState->DataSet->Model;
	size_t EquationIdx = 0;
	//NOTE: Read in initial values of the ODE equations to the CurResults buffer to be accessible from within the batch equations using RESULT(H).
	//NOTE: Values are not written to ResultData before the entire solution process is finished. So during the solver process one can ONLY read intermediary results from equations belonging to this solver using RESULT(H), never RESULT(H, Idx1,...) etc. However there is no reason one would want to do that any way.
	for(equation_h Equation : Batch->EquationsODE)
	{
		RunState->CurResults[Equation.Handle] = x0[EquationIdx];
		++EquationIdx;
	}
	
	//NOTE: Solving basic equations tied to the solver. Values should NOT be written to the working set wk. They can instead be accessed from inside other equations in the solver batch using RESULT(H)
	for(equation_h Equation : Batch->Equations)
	{
		double ResultValue = CallEquation(Model, RunState, Equation);
		RunState->CurResults[Equation.Handle] = ResultValue;
	}
	
	//NOTE: Solving ODE equations tied to the solver. These values should be written to the working set wk.
	EquationIdx = 0;
	for(equation_h Equation : Batch->EquationsODE)
	{
		double ResultValue = CallEquation(Model, RunState, Equation);
		wk[EquationIdx] = ResultValue;
		
		++EquationIdx;
	}
}

INNER_LOOP_BODY(RunInnerLoop)
{
	const mobius_model *Model = DataSet->Model;
	
	s32 BottomLevel = (s32)BatchGroup.IndexSets.Count - 1;
	
	//NOTE: Reading in to the Cur-buffers data that need to be updated at this iteration stage.
	if(CurrentLevel >= 0)
	{
		const iteration_data &IterationData = BatchGroup.IterationData[CurrentLevel];
		for(parameter_h Parameter : IterationData.ParametersToRead)
		{
			RunState->CurParameters[Parameter.Handle] = *RunState->AtParameterLookup; //NOTE: Parameter values are stored directly in the lookup since they don't change with the timestep.
			++RunState->AtParameterLookup;
		}
		for(input_h Input : IterationData.InputsToRead)
		{
			size_t Offset = *RunState->AtInputLookup;
			++RunState->AtInputLookup;
			RunState->CurInputs[Input.Handle] = RunState->AllCurInputsBase[Offset];
			RunState->CurInputWasProvided[Input.Handle] = DataSet->InputTimeseriesWasProvided[Offset];
		
		}
		for(equation_h Result : IterationData.ResultsToRead)
		{
			size_t Offset = *RunState->AtResultLookup;
			++RunState->AtResultLookup;
			RunState->CurResults[Result.Handle] = RunState->AllCurResultsBase[Offset];
		}
		for(equation_h Result : IterationData.LastResultsToRead)
		{
			size_t Offset = *RunState->AtLastResultLookup;
			++RunState->AtLastResultLookup;
			RunState->LastResults[Result.Handle] = RunState->AllLastResultsBase[Offset];
		}
	}
	else
	{
		for(equation_h Result : BatchGroup.LastResultsToReadAtBase)
		{
			size_t Offset = *RunState->AtLastResultLookup;
			++RunState->AtLastResultLookup;
			RunState->LastResults[Result.Handle] = RunState->AllLastResultsBase[Offset];
		}
	}

#if MOBIUS_TIMESTEP_VERBOSITY >= 2
	if(CurrentLevel >= 0)
	{
		index_set_h CurrentIndexSet = BatchGroup.IndexSets[CurrentLevel];
		for(size_t Lev = 0; Lev < CurrentLevel; ++Lev) WarningPrint("\t");
		index_t CurrentIndex = RunState->CurrentIndexes[CurrentIndexSet.Handle];
		WarningPrint("*** ", GetName(Model, CurrentIndexSet), ": ", DataSet->IndexNames[CurrentIndexSet.Handle][CurrentIndex], "\n");
	}
#endif
	
	if(CurrentLevel == BottomLevel)
	{
		for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
		{
			//NOTE: Write LastResult values into the RunState for fast lookup.
			const equation_batch &Batch = Model->EquationBatches[BatchIdx];
			
			//TODO: We should maybe watch this and see if it causes performance problems and in that case expand the for loops.
			ForAllBatchEquations(Batch,
			[RunState](equation_h Equation)
			{
				double LastResultValue = *RunState->AtLastResult;
				++RunState->AtLastResult;
				RunState->LastResults[Equation.Handle] = LastResultValue;
				return false;
			});
		}
		
		for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
		{
			const equation_batch &Batch = Model->EquationBatches[BatchIdx];
			
			//Check if there is a conditional execution of this batch and if it should be executed. If it shouldn't, skip it.
			if(IsValid(Batch.ConditionalSwitch) && Batch.ConditionalValue != RunState->CurParameters[Batch.ConditionalSwitch.Handle])
			{
				RunState->AtResult += Batch.Equations.Count;
				RunState->AtResult += Batch.EquationsODE.Count;
				continue;
			}
			
			if(!IsValid(Batch.Solver))
			{	
				//NOTE: Basic discrete timestep evaluation of equations.
				for(equation_h Equation : Batch.Equations) 
				{
					double ResultValue;
					//TODO: This is very inefficient, but we can't remove them from the equation batches since that would screw up the storage structure... Is there a better way to do this?
					const equation_spec &Spec = Model->Equations[Equation];
					if(!IsValid(Spec.IsComputedBy))
					{
						ResultValue = CallEquation(Model, RunState, Equation);
						*RunState->AtResult = ResultValue;
					}
					else
						ResultValue = *RunState->AtResult;
					
					RunState->CurResults[Equation.Handle] = ResultValue;
#if MOBIUS_TEST_FOR_NAN
					NaNTest(Model, RunState, ResultValue, Equation);
#endif
#if MOBIUS_TIMESTEP_VERBOSITY >= 3
					for(int Lev = 0; Lev < CurrentLevel; ++Lev) WarningPrint("\t");
					WarningPrint("\t", GetName(Model, Equation), " = ", ResultValue, "\n");
#endif
					++RunState->AtResult;
				}
			}
			else // IsValid(Batch.Solver)
			{
				//NOTE: The results from the last timestep are the initial results for this timestep.
				size_t EquationIdx = 0;
				for(equation_h Equation : Batch.EquationsODE)
				{
					//NOTE: Reading the Equations.Specs vector here may be slightly inefficient since each element of the vector is large. We could copy out an array of the ResetEveryTimestep bools instead beforehand.
					if(Model->Equations[Equation].ResetEveryTimestep)
						RunState->SolverTempX0[EquationIdx] = 0;
					else
						RunState->SolverTempX0[EquationIdx] = RunState->LastResults[Equation.Handle]; //NOTE: RunState->LastResults is filled with the correct values already, see above.
					++EquationIdx;
				}
				// NOTE: Do we need to clear DataSet->wk to 0? (Has not been needed in the solvers we have used so far...)
				
				const solver_spec &SolverSpec = Model->Solvers[Batch.Solver];
				
				// The desired solver step. (Guideline only, solver is free to correct its step during error correction).
				double h = SolverSpec.h;
				if(IsValid(SolverSpec.hParam)) h = RunState->CurParameters[SolverSpec.hParam.Handle].ValDouble;
				
				//NOTE: Solve the system using the provided solver
				SolverSpec.SolverFunction(h, Batch.EquationsODE.Count, RunState->SolverTempX0, RunState->SolverTempWorkStorage, &Batch, RunState, SolverSpec.RelErr, SolverSpec.AbsErr);
				
				//NOTE: Store out the final results from this solver to the main dataset.
				for(equation_h Equation : Batch.Equations)
				{
					double ResultValue = RunState->CurResults[Equation.Handle];
#if MOBIUS_TEST_FOR_NAN
					NaNTest(Model, RunState, ResultValue, Equation);
#endif
					*RunState->AtResult = ResultValue;
					++RunState->AtResult;
#if MOBIUS_TIMESTEP_VERBOSITY >= 3
					for(int Lev = 0; Lev < CurrentLevel; ++Lev) WarningPrint("\t");
					WarningPrint("\t", GetName(Model, Equation), " = ", ResultValue, "\n");
#endif
				}
				EquationIdx = 0;
				for(equation_h Equation : Batch.EquationsODE)
				{
					double ResultValue = RunState->SolverTempX0[EquationIdx];
#if MOBIUS_TEST_FOR_NAN
					NaNTest(Model, RunState, ResultValue, Equation);
#endif
					RunState->CurResults[Equation.Handle] = ResultValue;
					*RunState->AtResult = ResultValue;
					++RunState->AtResult;
					++EquationIdx;
#if MOBIUS_TIMESTEP_VERBOSITY >= 3
					for(int Lev = 0; Lev < CurrentLevel; ++Lev) WarningPrint("\t");
					WarningPrint("\t", GetName(Model, Equation), " = ", ResultValue, "\n");
#endif
				}
			}
		}
	}
}

INNER_LOOP_BODY(FastLookupCounter)
{
	if(CurrentLevel >= 0)
	{
		RunState->FastParameterLookup.Count  += BatchGroup.IterationData[CurrentLevel].ParametersToRead.Count;
		RunState->FastInputLookup.Count      += BatchGroup.IterationData[CurrentLevel].InputsToRead.Count;
		RunState->FastResultLookup.Count     += BatchGroup.IterationData[CurrentLevel].ResultsToRead.Count;
		RunState->FastLastResultLookup.Count += BatchGroup.IterationData[CurrentLevel].LastResultsToRead.Count;
	}
	else
	{
		RunState->FastLastResultLookup.Count += BatchGroup.LastResultsToReadAtBase.Count;
	}
}

INNER_LOOP_BODY(FastLookupSetupInnerLoop)
{
	if(CurrentLevel >= 0)
	{
		for(parameter_h Parameter : BatchGroup.IterationData[CurrentLevel].ParametersToRead)
		{
			//NOTE: Parameters are special here in that we can just store the value in the fast lookup, instead of the offset. This is because they don't change with the timestep.
			size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, Parameter);
			parameter_value Value = DataSet->ParameterData[Offset];
			RunState->FastParameterLookup[RunState->FastParameterLookup.Count++] = Value;
		}
		
		for(input_h Input : BatchGroup.IterationData[CurrentLevel].InputsToRead)
		{
			size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, Input);
			RunState->FastInputLookup[RunState->FastInputLookup.Count++] = Offset;
		}
		
		for(equation_h Equation : BatchGroup.IterationData[CurrentLevel].ResultsToRead)
		{
			size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, Equation);
			RunState->FastResultLookup[RunState->FastResultLookup.Count++] = Offset;
		}
		
		for(equation_h Equation : BatchGroup.IterationData[CurrentLevel].LastResultsToRead)
		{
			size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, Equation);
			RunState->FastLastResultLookup[RunState->FastLastResultLookup.Count++] = Offset;
		}
	}
	else
	{
		for(equation_h Equation : BatchGroup.LastResultsToReadAtBase)
		{
			size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, Equation);
			RunState->FastLastResultLookup[RunState->FastLastResultLookup.Count++] = Offset;
		}
	}
}


inline double
SetupInitialValue(mobius_data_set *DataSet, model_run_state *RunState, equation_h Equation)
{
	const mobius_model *Model = DataSet->Model;
	const equation_spec &Spec = Model->Equations[Equation];
	
	double Initial = 0.0;
	if(IsValid(Spec.InitialValue))
	{
		size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, (parameter_h)Spec.InitialValue);
		//NOTE: We should not get a type mismatch here since we only allow for registering initial values using handles of type parameter_double_h. Thus we do not need to test for which type it is.
		Initial = DataSet->ParameterData[Offset].ValDouble;
	}
	else if(Spec.HasExplicitInitialValue)
		Initial = Spec.ExplicitInitialValue;
	else if(IsValid(Spec.InitialValueEquation))
		Initial = Model->EquationBodies[Spec.InitialValueEquation.Handle](RunState);
	else
	{
		//NOTE: Equations without any type of initial value act as their own initial value equation
		if(IsValid(Spec.IsComputedBy))
		{
			//NOTE: In this case, this equation doesn't have an equation body of its own..
			WarningPrint("WARNING: The equation \"", Spec.Name, "\" is called during the initial value setup, but it does not have an equation body since it is set to be computed by a separate equation. You should set an intial value equation or parameter as the initial value of this equation.\n");
			Initial = 0.0;
		}
		else
			Initial = Model->EquationBodies[Equation.Handle](RunState);
	}
	
	size_t ResultStorageLocation = DataSet->ResultStorageStructure.LocationOfHandleInUnit[Equation.Handle];
	
#if MOBIUS_TEST_FOR_NAN
	NaNTest(Model, RunState, Initial, Equation);
#endif
	
	RunState->AtResult[ResultStorageLocation] = Initial;
	RunState->CurResults[Equation.Handle] = Initial;
	RunState->LastResults[Equation.Handle] = Initial;
	
	return Initial;
}

INNER_LOOP_BODY(InitialValueSetupInnerLoop)
{
	// TODO: IMPORTANT!! If an initial value equation depends on the result of an equation from a different batch group than itself, that is not guaranteed to work correctly.
	// TODO: Currently we don't report an error if that happens!
	
	const mobius_model *Model = DataSet->Model;
	
	if(CurrentLevel >= 0)
	{
		for(parameter_h Parameter : BatchGroup.IterationData[CurrentLevel].ParametersToRead)
		{
			RunState->CurParameters[Parameter.Handle] = *RunState->AtParameterLookup;
			++RunState->AtParameterLookup;
		}
		for(input_h Input : BatchGroup.IterationData[CurrentLevel].InputsToRead)
		{
			size_t Offset = *RunState->AtInputLookup;
			++RunState->AtInputLookup;
			RunState->CurInputs[Input.Handle] = RunState->AllCurInputsBase[Offset];
			RunState->CurInputWasProvided[Input.Handle] = DataSet->InputTimeseriesWasProvided[Offset];
		}
	}

#if MOBIUS_TIMESTEP_VERBOSITY >= 2
	if(CurrentLevel >= 0)
	{
		index_set_h CurrentIndexSet = BatchGroup.IndexSets[CurrentLevel];
		for(int Lev = 0; Lev < CurrentLevel; ++Lev) WarningPrint("\t");
		index_t CurrentIndex = RunState->CurrentIndexes[CurrentIndexSet.Handle];
		WarningPrint("*** ", GetName(Model, CurrentIndexSet), ": ", DataSet->IndexNames[CurrentIndexSet.Handle][CurrentIndex], "\n");
	}
#endif
	
	s32 BottomLevel = BatchGroup.IndexSets.Count - 1;
	if(CurrentLevel == BottomLevel)
	{	
		for(equation_h Equation : BatchGroup.InitialValueOrder)
		{
			double Initial = SetupInitialValue(DataSet, RunState, Equation);
#if MOBIUS_TIMESTEP_VERBOSITY >= 3
			for(int Lev = 0; Lev < CurrentLevel; ++Lev) WarningPrint("\t");
			WarningPrint("\t", GetName(Model, Equation), " = ", Initial, "\n");
#endif
		}
		
		RunState->AtResult += DataSet->ResultStorageStructure.Units[BatchGroupIdx].Handles.Count; //NOTE: This works because we set the storage structure up to mirror the batch group structure.
	}
}

static void
ProcessComputedParameters(mobius_data_set *DataSet, model_run_state *RunState)
{
	//NOTE: Preprocessing of computed parameters.
	for(parameter_h Parameter : DataSet->Model->Parameters)
	{
		const parameter_spec &Spec = DataSet->Model->Parameters[Parameter];
		equation_h Equation = Spec.IsComputedBy;
		if(IsValid(Spec.IsComputedBy))
		{
			const equation_spec &EqSpec = DataSet->Model->Equations[Equation];
			
			ForeachParameterInstance(DataSet, Parameter,
				[DataSet, Parameter, Equation, &EqSpec, &Spec, RunState](index_t *Indexes, size_t IndexesCount)
				{
					//NOTE: We have to set the RunState into the right state.
					
					for(size_t IdxIdx = 0; IdxIdx < IndexesCount; ++IdxIdx)
					{
						index_t Index = Indexes[IdxIdx];
						RunState->CurrentIndexes[Index.IndexSetHandle] = Index;
					}
					
					for(parameter_h DependentParameter : EqSpec.ParameterDependencies)
					{
						size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, DependentParameter);
						RunState->CurParameters[DependentParameter.Handle] = DataSet->ParameterData[Offset];
					}
					
					size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, Indexes, IndexesCount, DataSet->IndexCounts, Parameter);
					double ValD = DataSet->Model->EquationBodies[Equation.Handle](RunState);
					parameter_value Value;
					if(Spec.Type == ParameterType_Double)
						Value.ValDouble = ValD;
					else if(Spec.Type == ParameterType_UInt)
						Value.ValUInt = (u64)ValD; //Or should we do a more sophisticated rounding?
					DataSet->ParameterData[Offset] = Value;
				}
			);
		}
	}
}

static void
PrintEquationProfiles(mobius_data_set *DataSet, model_run_state *RunState);

static void
RunModel(mobius_data_set *DataSet)
{
#if MOBIUS_PRINT_TIMING_INFO
	timer SetupTimer = BeginTimer();
#endif

	const mobius_model *Model = DataSet->Model;
	
	// NOTE: in case there is an error later, these have to have been cleared.
	DataSet->HasBeenRun = false;
	DataSet->TimestepsLastRun = 0;
	
	//NOTE: Check that all the index sets have at least one index.
	for(index_set_h IndexSet : Model->IndexSets)
	{
		if(DataSet->IndexCounts[IndexSet.Handle] == 0)
			FatalError("ERROR: The index set \"", GetName(Model, IndexSet), "\" does not contain any indexes.\n");
	}
	
	//NOTE: Allocate parameter storage in case it was not allocated during setup.
	if(!DataSet->ParameterData)
	{
		AllocateParameterStorage(DataSet);
		WarningPrint("WARNING: No parameter values were specified, using default parameter values only.\n");
	}
	
	
	u64 Timesteps           = GetTimesteps(DataSet); //NOTE: Reads either the "Timesteps" or the "End date" parameter and computes accordingly.
	datetime ModelStartTime = GetStartDate(DataSet); //NOTE: This reads the "Start date" parameter.
	
	//TODO: Should we put a restriction on ModelStartTime so that it is "round" with respect to the Model->TimestepSize ?
	
#if MOBIUS_PRINT_TIMING_INFO
	WarningPrint("Running model ", Model->Name);
	if(Model->Modules.Count() > 1)
	{
		WarningPrint(" with modules (");
		for(module_h Module : Model->Modules)
		{
			const module_spec &ModuleSpec = Model->Modules[Module];
			WarningPrint(ModuleSpec.Name, " V", ModuleSpec.Version);
			if(Module.Handle != Model->Modules.Count()-1) WarningPrint(", ");
		}
		WarningPrint(")");
	}
	WarningPrint(" for ", Timesteps, " timesteps starting at ", ModelStartTime.ToString(), "\n");
#endif
	
	//NOTE: Allocate input storage in case it was not allocated during setup.
	if(!DataSet->InputData)
	{
		AllocateInputStorage(DataSet, Timesteps);
		WarningPrint("WARNING: No input values were specified, using input values of 0 only.\n");
	}
	
	datetime InputStartDate = GetInputStartDate(DataSet);
	s64 InputDataStartOffsetTimesteps = FindTimestep(InputStartDate, ModelStartTime, Model->TimestepSize);
	
	
	//NOTE: Do some sanity checks of the input and model start times that limit potential errors.

	s64 TimeOffset;
	if(Model->TimestepSize.Unit == Timestep_Second)
	{
		TimeOffset = ModelStartTime.SecondsSinceEpoch - InputStartDate.SecondsSinceEpoch;
	}
	else
	{
		assert(Model->TimestepSize.Unit == Timestep_Month);
		
		s32 IYear, IMonth, IDay;
		InputStartDate.YearMonthDay(&IYear, &IMonth, &IDay);
		
		s32 MYear, MMonth, MDay;
		ModelStartTime.YearMonthDay(&MYear, &MMonth, &MDay);
		
		if(IDay != 1 || InputStartDate.SecondsSinceEpoch % 86400 != 0)
			FatalError("ERROR: For models with timestep resolution measured in months or years, input data start dates have to be on the form yyyy-mm-01 or yyyy-mm-01 00:00:00 . In the current setup, the input start date was set to ", InputStartDate.ToString(), " .\n");
		
		if(MDay != 1 || ModelStartTime.SecondsSinceEpoch % 86400 != 0)
			FatalError("ERROR: For models with timestep resolution measured in months or years, the \"Start date\" has to be on the form yyyy-mm-01 or yyyy-mm-01 00:00:00 . In the current setup, the start date was set to ", ModelStartTime.ToString(), " .\n");
		
		TimeOffset = (MYear-IYear)*12 + MMonth-IMonth;
	}
	
	if(TimeOffset % Model->TimestepSize.Magnitude != 0)
		FatalError("ERROR: The model run \"Start date\" was not set to be a whole number of timesteps after the input start date.\n");
	
	if(ModelStartTime.SecondsSinceEpoch < InputStartDate.SecondsSinceEpoch)
		FatalError("ERROR: The input data starts at a later date than the model run.\n");
	
	if(((s64)DataSet->InputDataTimesteps - InputDataStartOffsetTimesteps) < (s64)Timesteps)
		FatalError("ERROR: The input data provided has fewer timesteps (after the model run start date) than the number of timesteps the model is running for.\n");
	
	AllocateResultStorage(DataSet, Timesteps);
	
	
	for(const mobius_preprocessing_step &PreprocessingStep : Model->PreprocessingSteps)
		PreprocessingStep(DataSet);
	
	// Check if solver step size makes sense.
	for(solver_h Solver: Model->Solvers)
	{
		const solver_spec &SolverSpec = Model->Solvers[Solver];
		double Correct = true;
		
		if(IsValid(SolverSpec.hParam))
		{
			parameter_h Par = SolverSpec.hParam;
			ForeachParameterInstance(DataSet, SolverSpec.hParam, [DataSet, Par, &Correct](index_t *Indexes, size_t IndexesCount)
			{
				size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, Indexes, IndexesCount, DataSet->IndexCounts, Par);
				double h = DataSet->ParameterData[Offset].ValDouble;
				if(h <= 0.0 || h > 1.0) Correct = false;
			});
		}
		else
		{
			double h = SolverSpec.h;
			if(h <= 0.0 || h > 1.0) Correct = false;
		}
		
		if(!Correct)
			FatalError("ERROR: The solver \"", SolverSpec.Name, "\" was given a step that is not in the range (0,1]\n");
	}
	
	//NOTE: The following have to be set here, because in case there is an error later, TimestepsLastRun must have been recorded correctly.
	//TODO: Maybe we should have a separate number that denotes the size of the ResultData allocation just to be safe.
	DataSet->HasBeenRun = true;
	DataSet->TimestepsLastRun = Timesteps;
	DataSet->StartDateLastRun = ModelStartTime;
	
	
	model_run_state RunState(DataSet);
	
	ProcessComputedParameters(DataSet, &RunState);
	
	RunState.Clear();
	
	
	///////////// Setting up fast lookup ////////////////////
	
	//NOTE: This is a hack, where we first set the Count for each array in the FastLookupCounter routine, then allocate, then set it to 0 to use it as an iterator in FastLookupSetupInnerLoop
	ModelLoop(DataSet, &RunState, FastLookupCounter);
	RunState.Clear();

	RunState.FastParameterLookup.Allocate(&RunState.BucketMemory, RunState.FastParameterLookup.Count);
	RunState.FastInputLookup.Allocate(&RunState.BucketMemory, RunState.FastInputLookup.Count);
	RunState.FastResultLookup.Allocate(&RunState.BucketMemory, RunState.FastResultLookup.Count);
	RunState.FastLastResultLookup.Allocate(&RunState.BucketMemory, RunState.FastLastResultLookup.Count);

	RunState.FastParameterLookup.Count  = 0;
	RunState.FastInputLookup.Count      = 0;
	RunState.FastResultLookup.Count     = 0;
	RunState.FastLastResultLookup.Count = 0;
	
	//Technically we really only need to rebuild the parameter lookup, not the three others, but it is very fast anyway.
	ModelLoop(DataSet, &RunState, FastLookupSetupInnerLoop);
	RunState.Clear();
	
	//NOTE: Temporary storage for use by solvers:
	//TODO: This code should probably be a member function of model_run_state or similar.
	size_t MaxODECount = 0;
	size_t MaxNonODECount = 0;
	size_t SolverTempWorkSpace = 0;
	for(const equation_batch_group& BatchGroup : Model->BatchGroups)
	{
		for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
		{
			const equation_batch &Batch = Model->EquationBatches[BatchIdx];
			if(IsValid(Batch.Solver))
			{
				size_t ODECount = Batch.EquationsODE.Count;
				MaxODECount = Max(MaxODECount, ODECount);
				MaxNonODECount = Max(MaxNonODECount, Batch.Equations.Count);
				const solver_spec &SolverSpec = Model->Solvers[Batch.Solver];
				SolverTempWorkSpace = Max(SolverTempWorkSpace, SolverSpec.SpaceRequirement(ODECount));
			}
		}
	}

	size_t JacobiTempWorkSpace = MaxODECount + MaxNonODECount;
	
	RunState.SolverTempX0          = RunState.BucketMemory.Allocate<double>(MaxODECount);
	RunState.SolverTempWorkStorage = RunState.BucketMemory.Allocate<double>(SolverTempWorkSpace);
	RunState.JacobianTempStorage   = RunState.BucketMemory.Allocate<double>(JacobiTempWorkSpace);
	
	

	//NOTE: System parameters (i.e. parameters that don't depend on index sets) are going to be the same during the entire run, so we just load them into CurParameters once and for all.
	//NOTE: If any system parameters exist, the storage units are sorted such that the system parameters have to belong to storage unit [0].
	if(DataSet->ParameterStorageStructure.Units.Count != 0 && DataSet->ParameterStorageStructure.Units[0].IndexSets.Count == 0)
	{
		for(parameter_h Parameter : DataSet->ParameterStorageStructure.Units[0].Handles)
		{
			size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, Parameter);
			RunState.CurParameters[Parameter.Handle] = DataSet->ParameterData[Offset];
		}
	}
	
	//NOTE: Similarly we have to set which of the unindexed inputs were provided.
	if(DataSet->InputStorageStructure.Units.Count != 0 && DataSet->InputStorageStructure.Units[0].IndexSets.Count == 0)
	{
		for(input_h Input : DataSet->InputStorageStructure.Units[0].Handles)
		{
			size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, Input);
			RunState.CurInputWasProvided[Input.Handle] = DataSet->InputTimeseriesWasProvided[Offset];
		}
	}
	
	RunState.AllLastResultsBase = DataSet->ResultData;
	RunState.AllCurResultsBase  = DataSet->ResultData;
	
	//TODO: We may want to set this one timestep back for it to also be correct during the initial value run.
	RunState.CurrentTime = expanded_datetime(ModelStartTime, Model->TimestepSize);
	
	//************ NOTE: Set up initial values;
	RunState.AtResult          = RunState.AllCurResultsBase;
	RunState.AtLastResult      = RunState.AllLastResultsBase;
	RunState.AtParameterLookup = RunState.FastParameterLookup.Data;
	RunState.AtInputLookup     = RunState.FastInputLookup.Data;
	//NOTE: Initial value equations can access input timeseries. The timestep they access is either the model run timestep (if the input start date is the same as the model run start date), or the timestep before that.
	size_t InitialValueInputTimestep = (size_t)InputDataStartOffsetTimesteps;
	if(InitialValueInputTimestep > 0) --InitialValueInputTimestep;
	RunState.AllCurInputsBase = DataSet->InputData + (InitialValueInputTimestep)*DataSet->InputStorageStructure.TotalCount;
	//NOTE: We have to update the inputs that don't depend on any index sets here, as that is not handled by the "fast lookup system".
	if(DataSet->InputStorageStructure.Units.Count != 0 && DataSet->InputStorageStructure.Units[0].IndexSets.Count == 0)
	{
		for(input_h Input : DataSet->InputStorageStructure.Units[0].Handles)
		{
			size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, Input);
			RunState.CurInputs[Input.Handle] = RunState.AllCurInputsBase[Offset];
		}
	}
#if MOBIUS_TIMESTEP_VERBOSITY >= 1
	WarningPrint("Initial value step:\n");
#endif
	RunState.Timestep = -1;
	ModelLoop(DataSet, &RunState, InitialValueSetupInnerLoop);
	//***********
	
#if MOBIUS_PRINT_TIMING_INFO
	u64 SetupDuration = GetTimerMilliseconds(&SetupTimer);
	timer RunTimer = BeginTimer();
	u64 BeforeC = __rdtsc();
#endif
	
	RunState.AllLastResultsBase = DataSet->ResultData;
	RunState.AllCurResultsBase = DataSet->ResultData + DataSet->ResultStorageStructure.TotalCount;
	RunState.AllCurInputsBase = DataSet->InputData + ((size_t)InputDataStartOffsetTimesteps)*DataSet->InputStorageStructure.TotalCount;
	
#if MOBIUS_EQUATION_PROFILING
	RunState.EquationHits        = RunState.BucketMemory.Allocate<size_t>(Model->Equations.Count());
	RunState.EquationTotalCycles = RunState.BucketMemory.Allocate<u64>(Model->Equations.Count());
#endif
	
	//TODO: Timesteps is u64. Can cause problems if somebody have an unrealistically high amount of timesteps. Ideally we should move every parameter from u64 to s64 anyway? There is a similar problem a little earlier in this routine.
	s64 MaxStep = (s64)Timesteps;
	
	for(RunState.Timestep = 0; RunState.Timestep < MaxStep; ++RunState.Timestep)
	{
		
#if MOBIUS_TIMESTEP_VERBOSITY >= 1
		WarningPrint("Timestep: ", RunState.Timestep, "\n");
		//std::cout << "Day of year: " << RunState.DayOfYear << std::endl;
#endif
		
		RunState.AtResult           = RunState.AllCurResultsBase;
		RunState.AtLastResult       = RunState.AllLastResultsBase;
		
		RunState.AtParameterLookup  = RunState.FastParameterLookup.Data;
		RunState.AtInputLookup      = RunState.FastInputLookup.Data;
		RunState.AtResultLookup     = RunState.FastResultLookup.Data;
		RunState.AtLastResultLookup = RunState.FastLastResultLookup.Data;
		
		//NOTE: We have to update the inputs that don't depend on any index sets here, as that is not handled by the "fast lookup system".
		if(DataSet->InputStorageStructure.Units.Count != 0 && DataSet->InputStorageStructure.Units[0].IndexSets.Count == 0)
		{
			for(input_h Input : DataSet->InputStorageStructure.Units[0].Handles)
			{
				size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, Input);
				RunState.CurInputs[Input.Handle] = RunState.AllCurInputsBase[Offset];
			}
		}
		
		ModelLoop(DataSet, &RunState, RunInnerLoop);
		
		RunState.AllLastResultsBase = RunState.AllCurResultsBase;
		RunState.AllCurResultsBase += DataSet->ResultStorageStructure.TotalCount;
		RunState.AllCurInputsBase  += DataSet->InputStorageStructure.TotalCount;
		
		RunState.CurrentTime.Advance();
	}
	
#if MOBIUS_PRINT_TIMING_INFO
	u64 AfterC = __rdtsc();
	
	u64 RunDuration = GetTimerMilliseconds(&RunTimer);
	u64 RunDurationCycles = AfterC - BeforeC;

	WarningPrint("Model execution setup time: ", SetupDuration, " milliseconds\n");
	WarningPrint("Model execution time: ", RunDuration, " milliseconds\n");
	WarningPrint("Model execution processor cycles: ", RunDurationCycles, "\n");
	WarningPrint("Average cycles per result instance including overhead: ", (RunDurationCycles / (Timesteps * DataSet->ResultStorageStructure.TotalCount)), "\n");
	WarningPrint("(Note: one instance can be the result of several equation evaluations in the case of solvers)\n");
#endif

#if MOBIUS_EQUATION_PROFILING
	PrintEquationProfiles(DataSet, &RunState);
#endif
}

static void
PrintEquationDependencies(mobius_model *Model)
{
	if(!Model->Finalized)
	{
		WarningPrint("WARNING: Tried to print equation dependencies before the model was finalized.\n");
		return;
	}
	
	//Ooops, this one may not be correct for equations that are on solvers!!
	
	WarningPrint("**** Equation Dependencies ****\n");
	for(equation_h Equation : Model->Equations)
	{
		WarningPrint(GetName(Model, Equation), "\n\t");
		for(index_set_h IndexSet : Model->Equations[Equation].IndexSetDependencies)
			WarningPrint("[", GetName(Model, IndexSet), "]");
		WarningPrint("\n");
	}
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
	std::cout << std::endl << "**** Equation profiles - Average cycles per evaluation (number of evaluations) ****" << std::endl;
	//std::cout << "Number of batches: " << Model->ResultStructure.size() << std::endl;
	u64 SumCc = 0;
	u64 TotalHits = 0;
	
	for(const equation_batch_group &BatchGroup : Model->BatchGroups)
	{	
		std::cout << std::endl;
		if(BatchGroup.IndexSets.Count == 0) std::cout << "[]";
		for(index_set_h IndexSet : BatchGroup.IndexSets)
			std::cout << "[" << GetName(Model, IndexSet) << "]";
		
		for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
		{
			const equation_batch &Batch = Model->EquationBatches[BatchIdx];
			std::cout << "\n\t-----";
			if(IsValid(Batch.Solver)) std::cout << " Solver \"" << GetName(Model, Batch.Solver) << "\"";
			
			ForAllBatchEquations(Batch,
			[Model, RunState, &TotalHits, &SumCc](equation_h Equation)
			{
				int PrintCount = 0;
				printf("\n\t");
				if(Model->Equations[Equation].Type == EquationType_Cumulative) PrintCount += printf("(Cumulative) ");
				else if(Model->Equations[Equation].Type == EquationType_ODE) PrintCount += printf("(ODE) ");
				PrintCount += printf("%s: ", GetName(Model, Equation));
				
				u64 Cc = RunState->EquationTotalCycles[Equation.Handle];
				size_t Hits = RunState->EquationHits[Equation.Handle];
				double CcPerHit = (double)Cc / (double)Hits;
				
				char FormatString[100];
				sprintf(FormatString, "%s%dlf", "%", 60-PrintCount);
				//printf("%s", FormatString);
				printf(FormatString, CcPerHit);
				printf(" (%llu)", Hits);
				
				TotalHits += (u64)Hits;
				SumCc += Cc;
				return false;
			});
			if(BatchIdx == BatchGroup.LastBatch) std::cout << "\n\t-----\n";
		}
	}
	std::cout << "\nTotal average cycles per evaluation: " << ((double)SumCc / (double)TotalHits)<< std::endl;
#endif
}
