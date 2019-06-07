
static mobius_model *
BeginModelDefinition(const char *Name = "(unnamed model)", const char *Version = "0.0")
{
	mobius_model *Model = new mobius_model {};
	
	Model->DefinitionTimer = BeginTimer();
	
	Model->Name = Name;
	Model->Version = Version;
	
	auto Days 	      = RegisterUnit(Model, "days");
	auto System       = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 100);
	RegisterParameterDate(Model, System, "Start date", "1999-1-1");
	
	return Model;
}


static void
PrintPartialDependencyTrace(mobius_model *Model, equation_h Equation, bool First = false)
{
	if(!First)
	{
		MOBIUS_PARTIAL_ERROR("<- ");
	}
	else
	{
		MOBIUS_PARTIAL_ERROR("   ");
	}
	
	equation_spec &Spec = Model->Equations.Specs[Equation.Handle];
	if(IsValid(Spec.Solver))
	{
		MOBIUS_PARTIAL_ERROR("\"" << GetName(Model, Spec.Solver) << "\" (");
	}
	MOBIUS_PARTIAL_ERROR("\"" << GetName(Model, Equation) << "\"");
	if(IsValid(Spec.Solver))
	{
		MOBIUS_PARTIAL_ERROR(")")
	}
	MOBIUS_PARTIAL_ERROR(std::endl);
}

static bool
TopologicalSortEquationsVisit(mobius_model *Model, equation_h Equation, std::vector<equation_h>& PushTo)
{
	equation_spec &Spec = Model->Equations.Specs[Equation.Handle];
	solver_spec &SolverSpec = Model->Solvers.Specs[Spec.Solver.Handle];
	
	//NOTE: We have to short-circuit out all equations that belong to a particular solver since they always have to be grouped together in a batch, and ODE equations within a batch are allowed to have circular references since they work differently. They have to be sorted as a block later.
	bool &Visited = IsValid(Spec.Solver) ? SolverSpec.Visited : Spec.Visited;
	bool &TempVisited = IsValid(Spec.Solver) ? SolverSpec.TempVisited : Spec.TempVisited;
	
	if(Visited) return true;
	if(TempVisited)
	{
		MOBIUS_PARTIAL_ERROR("ERROR: There is a circular dependency between the equations :" << std::endl);
		PrintPartialDependencyTrace(Model, Equation, true);
		return false;
	}
	TempVisited = true;
	std::set<equation_h> &DirectResultDependencies = IsValid(Spec.Solver) ? SolverSpec.DirectResultDependencies : Spec.DirectResultDependencies;
	for(equation_h Dependency : DirectResultDependencies)
	{
		equation_spec &DepSpec = Model->Equations.Specs[Dependency.Handle];
		if(IsValid(Spec.Solver) && IsValid(DepSpec.Solver) && Spec.Solver == DepSpec.Solver) continue; //NOTE: We ignore dependencies of the solver on itself for now.
		bool Success = TopologicalSortEquationsVisit(Model, Dependency, PushTo);
		if(!Success)
		{
			PrintPartialDependencyTrace(Model, Equation);
			return false;
		}
	}
	Visited = true;
	PushTo.push_back(Equation);
	return true;
}

static bool
TopologicalSortEquationsInSolverVisit(mobius_model *Model, equation_h Equation, std::vector<equation_h>& PushTo)
{
	equation_spec &Spec = Model->Equations.Specs[Equation.Handle];
	
	if(Spec.Visited) return true;
	if(Spec.TempVisited)
	{
		MOBIUS_PARTIAL_ERROR("ERROR: There is a circular dependency between the non-ode equations within a solver :" << std::endl);
		PrintPartialDependencyTrace(Model, Equation, true);
		return false;
	}
	Spec.TempVisited = true;
	for(equation_h Dependency : Spec.DirectResultDependencies)
	{
		equation_spec &DepSpec = Model->Equations.Specs[Dependency.Handle];
		if(DepSpec.Type == EquationType_ODE || DepSpec.Solver != Spec.Solver) continue; //NOTE: We are only interested sorting non-ode equations belonging to this solver.
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
	equation_spec &OriginalSpec = Model->Equations.Specs[Equation.Handle];
	equation_h InitialValueEq = OriginalSpec.InitialValueEquation;
	if(IsValid(InitialValueEq))
	{
		EquationToLookUp = InitialValueEq;
	}
	
	equation_spec &Spec = Model->Equations.Specs[EquationToLookUp.Handle];
	
	if(Spec.Visited) return true;
	if(Spec.TempVisited)
	{
		MOBIUS_PARTIAL_ERROR("ERROR: There is a circular dependency between the initial value of the equations :" << std::endl);
		PrintPartialDependencyTrace(Model, Equation, true);
		return false;
	}
	Spec.TempVisited = true;
	
	//std::cout << "Initial value sort for " << GetName(Model, EquationToLookUp) << std::endl;
	
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
	}
	
	Spec.Visited = true;
	PushTo.push_back(Equation);
	return true;
}

typedef bool topological_sort_equations_visit(mobius_model *Model, equation_h Equation, std::vector<equation_h>& PushTo);

static void
TopologicalSortEquations(mobius_model *Model, std::vector<equation_h> &Equations, topological_sort_equations_visit *Visit)
{
	std::vector<equation_h> Temporary;
	Temporary.reserve(Equations.size());
	for(equation_h Equation : Equations)
	{
		bool Success = Visit(Model, Equation, Temporary);
		if(!Success)
		{
			MOBIUS_FATAL_ERROR("");
		}
	}
	
	for(size_t Idx = 0; Idx < Equations.size(); ++Idx)
	{
		Equations[Idx] = Temporary[Idx];
	}
}

struct equation_batch_template
{
	equation_batch_type Type;
	std::vector<equation_h> Equations;
	std::vector<equation_h> EquationsODE;
	
	std::set<index_set_h> IndexSetDependencies;
	solver_h Solver;
};

static bool
IsTopIndexSetForThisDependency(std::vector<index_set_h> &IndexSetDependencies, std::vector<index_set_h> &BatchGroupIndexSets, size_t IndexSetLevel)
{
	index_set_h CurrentLevelIndexSet = BatchGroupIndexSets[IndexSetLevel];
	bool DependsOnCurrentLevel = (std::find(IndexSetDependencies.begin(), IndexSetDependencies.end(), CurrentLevelIndexSet) != IndexSetDependencies.end());
	if(!DependsOnCurrentLevel) return false;
	
	for(size_t LevelAbove = IndexSetLevel + 1; LevelAbove < BatchGroupIndexSets.size(); ++LevelAbove)
	{
		index_set_h IndexSetAtLevelAbove = BatchGroupIndexSets[LevelAbove];
		if(std::find(IndexSetDependencies.begin(), IndexSetDependencies.end(), IndexSetAtLevelAbove) != IndexSetDependencies.end())
		{
			return false;
		}
	}
	
	return true;
}

static bool
WeDependOnBatch(std::set<equation_h> &Dependencies, std::set<equation_h> &CrossDependencies, equation_batch_template &Batch)
{
	bool DependOnBatch = false;
	for(equation_h EquationInBatch : Batch.Equations)
	{
		if(!Dependencies.empty() && std::find(Dependencies.begin(), Dependencies.end(), EquationInBatch) != Dependencies.end())
		{
			DependOnBatch = true;
			break;
		}
		if(!CrossDependencies.empty() && std::find(CrossDependencies.begin(), CrossDependencies.end(), EquationInBatch) != CrossDependencies.end())
		{
			DependOnBatch = true;
			break;
		}
	}
	if(Batch.Type == BatchType_Solver && !DependOnBatch)
	{
		for(equation_h EquationInBatch : Batch.EquationsODE)
		{
			if(!Dependencies.empty() && std::find(Dependencies.begin(), Dependencies.end(), EquationInBatch) != Dependencies.end())
			{
				DependOnBatch = true;
				break;
			}
			if(!CrossDependencies.empty() && std::find(CrossDependencies.begin(), CrossDependencies.end(), EquationInBatch) != CrossDependencies.end())
			{
				DependOnBatch = true;
				break;
			}
		}
	}
	return DependOnBatch;
}


#if !defined(MOBIUS_PRINT_TIMING_INFO)
#define MOBIUS_PRINT_TIMING_INFO 0
#endif

static void
EndModelDefinition(mobius_model *Model)
{
	if(Model->Finalized)
	{
		MOBIUS_FATAL_ERROR("ERROR: Called EndModelDefinition twice on the same model." << std::endl);
	}
	
	///////////// Find out what index sets each parameter depends on /////////////
	
	for(entity_handle ParameterHandle = 1; ParameterHandle < Model->Parameters.Count(); ++ParameterHandle)
	{
		const parameter_spec &Spec = Model->Parameters.Specs[ParameterHandle];
		parameter_group_h CurrentGroup = Spec.Group;
		std::vector<index_set_h>& Dependencies = Model->Parameters.Specs[ParameterHandle].IndexSetDependencies;
		while(IsValid(CurrentGroup))
		{
			parameter_group_spec *GroupSpec = &Model->ParameterGroups.Specs[CurrentGroup.Handle];
			if(IsValid(GroupSpec->IndexSet)) //NOTE: We never insert index set 0 as a dependency. If this is a global parameter, we want to register it as having no dependencies.
			{
				Dependencies.insert(Dependencies.begin(), GroupSpec->IndexSet); 
			}
			
			CurrentGroup = GroupSpec->ParentGroup;
		}
	}
	
	/////////////////////// Find all dependencies of equations on parameters, inputs and other results /////////////////////
	
	value_set_accessor ValueSet(Model);
	for(entity_handle EquationHandle = 1; EquationHandle < Model->Equations.Count(); ++EquationHandle)
	{
		equation_spec &Spec = Model->Equations.Specs[EquationHandle];
		
		if(Spec.Type == EquationType_Cumulative)
		{
			//NOTE: Cumulative equations depend on the equations they cumulate.
			Spec.DirectResultDependencies.insert(Spec.Cumulates);
			continue;
		}
		
		if(!Model->Equations.Specs[EquationHandle].EquationIsSet)
		{
			MOBIUS_FATAL_ERROR("ERROR: The equation body for the registered equation " << GetName(Model, equation_h {EquationHandle}) << " has not been defined." << std::endl);
		}
		
		// Clear dependency registrations from evaluation of previous equation.
		ValueSet.Clear();
		
		//Call the equation. Since we are in ValueSet.Running==false mode, the equation will register which values it tried to access.
		Model->EquationBodies[EquationHandle](&ValueSet);
		
		//std::cout << GetName(Model, equation {EquationHandle}) << std::endl;
		
		Spec.IndexSetDependencies.insert(ValueSet.DirectIndexSetDependencies.begin(), ValueSet.DirectIndexSetDependencies.end());
		
		for(dependency_registration ParameterDependency : ValueSet.ParameterDependencies)
		{
			entity_handle ParameterHandle = ParameterDependency.Handle;
			
			parameter_spec &ParSpec = Model->Parameters.Specs[ParameterHandle];
			std::vector<index_set_h>& IndexSetDependencies = ParSpec.IndexSetDependencies;
			if(ParameterDependency.NumExplicitIndexes > IndexSetDependencies.size())
			{
				MOBIUS_FATAL_ERROR("ERROR: In equation " << Spec.Name << ". The parameter " << ParSpec.Name << " is referenced with more explicit indexes than the number of index sets this parameter depends on." << std::endl);
			}
			size_t NumImplicitIndexes = IndexSetDependencies.size() - ParameterDependency.NumExplicitIndexes;
			
			if(NumImplicitIndexes > 0)
			{
				Spec.IndexSetDependencies.insert(IndexSetDependencies.begin(), IndexSetDependencies.begin() + NumImplicitIndexes);
			}
			
			if(ParameterDependency.NumExplicitIndexes == 0)
			{
				//NOTE: We only store the parameters that should be hotloaded at the start of the batch in this vector: For various reasons we can't do that with parameters that are referred to by explicit indexing.
				//TODO: We should maybe store a cross-index parameter dependency list for easy referencing later (during debugging etc.).
				Spec.ParameterDependencies.insert(ParameterHandle);
			}
		}
		
		for(dependency_registration InputDependency : ValueSet.InputDependencies)
		{
			//TODO: This block has to be updated to match the parameter registration above if we later allow for explicitly indexed inputs.
			entity_handle InputHandle = InputDependency.Handle;
			std::vector<index_set_h>& IndexSetDependencies = Model->Inputs.Specs[InputHandle].IndexSetDependencies;
			Spec.IndexSetDependencies.insert(IndexSetDependencies.begin(), IndexSetDependencies.end());
			Spec.InputDependencies.insert(input_h {InputHandle});
		}
		
		//NOTE: Every equation always depends on its initial value parameter if it has one.
		//TODO: We should have a specialized system for this, because this currently causes the initial value parameter to be loaded into the CurParameters buffer at each step (instead of just during the initial value step), which is unnecessary.
		if(IsValid(Spec.InitialValue))
		{
			std::vector<index_set_h>& IndexSetDependencies = Model->Parameters.Specs[Spec.InitialValue.Handle].IndexSetDependencies;
			Spec.IndexSetDependencies.insert(IndexSetDependencies.begin(), IndexSetDependencies.end());
			Spec.ParameterDependencies.insert(Spec.InitialValue.Handle);
		}
		
		for(const result_dependency_registration &ResultDependency : ValueSet.ResultDependencies)
		{
			entity_handle DepResultHandle = ResultDependency.Handle;
			
			if(Model->Equations.Specs[DepResultHandle].Type == EquationType_InitialValue)
			{
				std::cout << "ERROR: The equation " << GetName(Model, equation_h {EquationHandle}) << " depends explicitly on the result of the equation " << GetName(Model, equation_h {DepResultHandle}) << " which is an EquationInitialValue. This is not allowed, instead it should depend on the result of the equation that " << GetName(Model, equation_h {DepResultHandle}) << " is an initial value for." << std::endl;
			}
			
			if(ResultDependency.Indexes.size() == 0)
			{
				Spec.DirectResultDependencies.insert(equation_h {DepResultHandle});
			}
			else
			{
				Spec.CrossIndexResultDependencies.insert(equation_h {DepResultHandle}); //TODO: Do we really need to keep this separately?
				Spec.IndexedResultAndLastResultDependencies.push_back(ResultDependency); //TODO: Maybe don't store these on the result spec? They are only needed in this algorithm..
			}
		}
		
		for(const result_dependency_registration &ResultDependency : ValueSet.LastResultDependencies)
		{
			entity_handle DepResultHandle = ResultDependency.Handle;
			if(Model->Equations.Specs[DepResultHandle].Type == EquationType_InitialValue)
			{
				std::cout << "ERROR: The equation " << GetName(Model, equation_h {EquationHandle}) << " depends explicitly on the result of the equation " << GetName(Model, equation_h {DepResultHandle}) << " which is an EquationInitialValue. This is not allowed, instead it should depend on the result of the equation that " << GetName(Model, equation_h {DepResultHandle}) << " is an initial value for." << std::endl;
			}
			
			if(ResultDependency.Indexes.size() == 0)
			{
				Spec.DirectLastResultDependencies.insert(equation_h {DepResultHandle});
			}
			else
			{
				Spec.IndexedResultAndLastResultDependencies.push_back(ResultDependency);
			}
		}
		
		//NOTE: Every equation always depends on its initial value equation if it has one.
		//TODO: Right now we register it as a LastResultDependency, which is not technically correct, but it should give the desired result.
		//TODO: Figure out if this may break somehting, and if we need a specialized system for this?
		equation_h EqInitialValue = Model->Equations.Specs[EquationHandle].InitialValueEquation;
		if(IsValid(EqInitialValue))
		{
			Spec.DirectLastResultDependencies.insert(EqInitialValue);
		}
	}
	
	{
		//NOTE: Check for computed parameters if their equation satisfies the requirements:
		for(entity_handle ParameterHandle = 1; ParameterHandle < Model->Parameters.Count(); ++ParameterHandle)
		{
			parameter_spec &Spec = Model->Parameters.Specs[ParameterHandle];
			equation_h Equation = Spec.IsComputedBy;
			if(IsValid(Equation))
			{
				equation_spec &EqSpec = Model->Equations.Specs[Equation.Handle];
				
				if(
					   !EqSpec.DirectResultDependencies.empty()
					|| !EqSpec.DirectLastResultDependencies.empty()
					|| !EqSpec.IndexedResultAndLastResultDependencies.empty()
					|| !EqSpec.InputDependencies.empty()
				)
				{
					MOBIUS_FATAL_ERROR("ERROR: The initial value equation " << EqSpec.Name << " assigned to compute the parameter " << Spec.Name << " depends on either a result of an equation or an input. This is not allowed for computed parameters." << std::endl);
				}
			}
		}
	}
	
	///////////////////// Resolve indirect dependencies of equations on index sets.
	
	//TODO: This is probably an inefficient way to do it, we should instead use some kind of graph traversal, but it is tricky. We need a way to do it properly with collapsing the dependency graph (including both results and lastresults) by its strongly connected components, then resolving the dependencies between the components.
	//NOTE: We stop the iteraton at 1000 so that if the dependencies are unresolvable, we don't go in an infinite loop. (they can probably never become unresolvable though??)
	bool DependenciesWereResolved = false;
	for(size_t It = 0; It < 1000; ++It)
	{
		bool Changed = false;
		for(entity_handle EquationHandle = 1; EquationHandle < Model->Equations.Count(); ++EquationHandle)
		{
			equation_spec &Spec = Model->Equations.Specs[EquationHandle];
			size_t DependencyCount = Spec.IndexSetDependencies.size();
				
			if(Spec.Type == EquationType_Cumulative)
			{
				equation_spec &DepSpec = Model->Equations.Specs[Spec.Cumulates.Handle];
				for(index_set_h IndexSet : DepSpec.IndexSetDependencies)
				{
					if(IndexSet != Spec.CumulatesOverIndexSet) Spec.IndexSetDependencies.insert(IndexSet); 
				}
				parameter_spec &WeightSpec = Model->Parameters.Specs[Spec.CumulationWeight.Handle];
				for(index_set_h IndexSet : WeightSpec.IndexSetDependencies)
				{
					if(IndexSet != Spec.CumulatesOverIndexSet) Spec.IndexSetDependencies.insert(IndexSet); 
				}
			}
			else
			{
				for(equation_h ResultDependency : Spec.DirectResultDependencies)
				{
					equation_spec &DepSpec = Model->Equations.Specs[ResultDependency.Handle];
					Spec.IndexSetDependencies.insert(DepSpec.IndexSetDependencies.begin(), DepSpec.IndexSetDependencies.end());
				}
				for(equation_h ResultDependency : Spec.DirectLastResultDependencies)
				{
					equation_spec &DepSpec = Model->Equations.Specs[ResultDependency.Handle];
					Spec.IndexSetDependencies.insert(DepSpec.IndexSetDependencies.begin(), DepSpec.IndexSetDependencies.end());
				}
				for(const result_dependency_registration &ResultDependency : Spec.IndexedResultAndLastResultDependencies)
				{
					equation_spec &DepSpec = Model->Equations.Specs[ResultDependency.Handle];
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
			//std::cout << "Dependencies resolved at " << It + 1 << " iterations." << std::endl;
			break;
		}
	}
	
	if(!DependenciesWereResolved)
	{
		MOBIUS_FATAL_ERROR("ERROR: We were unable to resolve all equation dependencies!" << std::endl);
	}
	
	/////////////// Sorting the equations into equation batches ///////////////////////////////
	
	std::vector<equation_h> EquationsToSort;
	
	bool *SolverHasBeenHitOnce = AllocClearedArray(bool, Model->Solvers.Count());
	
	for(entity_handle EquationHandle = 1; EquationHandle < Model->Equations.Count(); ++EquationHandle)
	{
		equation_spec &Spec = Model->Equations.Specs[EquationHandle];
		solver_h Solver = Spec.Solver;
		
		if(Spec.Type == EquationType_InitialValue) continue; //NOTE: initial value equations should not be a part of the result structure.
		
		if(IsValid(Solver))
		{			
			solver_spec &SolverSpec = Model->Solvers.Specs[Solver.Handle];
			SolverSpec.IndexSetDependencies.insert(Spec.IndexSetDependencies.begin(), Spec.IndexSetDependencies.end());
			SolverSpec.DirectResultDependencies.insert(Spec.DirectResultDependencies.begin(), Spec.DirectResultDependencies.end());
			SolverSpec.CrossIndexResultDependencies.insert(Spec.CrossIndexResultDependencies.begin(), Spec.CrossIndexResultDependencies.end());
			SolverSpec.EquationsToSolve.push_back(equation_h {EquationHandle});
			
			if(!SolverHasBeenHitOnce[Solver.Handle])
			{
				EquationsToSort.push_back(equation_h {EquationHandle}); //NOTE: We only put one equation into the sorting vector as a stand-in for the solver. This is because all equations belonging to a solver have to be solved together.
			}
			SolverHasBeenHitOnce[Solver.Handle] = true;
		}
		else
		{
			if(Spec.Type == EquationType_ODE)
			{
				MOBIUS_FATAL_ERROR("ERROR: The equation " << GetName(Model, equation_h {EquationHandle}) << " is registered as an ODE equation, but it has not been given a solver." << std::endl);
			}
			
			EquationsToSort.push_back(equation_h {EquationHandle});
		}
	}
	
	free(SolverHasBeenHitOnce);
	
	TopologicalSortEquations(Model, EquationsToSort, TopologicalSortEquationsVisit);
	
	std::vector<equation_batch_template> BatchBuild;
	
	for(equation_h Equation : EquationsToSort)
	{
		equation_spec &Spec = Model->Equations.Specs[Equation.Handle];
		
		if(IsValid(Spec.Solver))
		{
			//TODO: Instead of always pushing this at the end, could we try to insert it earlier if it is possible to place it next to another batch with the same index set dependencies?
			
			solver_spec &SolverSpec = Model->Solvers.Specs[Spec.Solver.Handle];
			//NOTE: There is only one stand-in equation for the whole solver in the EquationsToSort vector. We create a new batch containing all of the equations and put it at the end of the batch build list.
			equation_batch_template Batch = {};
			Batch.Type = BatchType_Solver;
			Batch.Solver = Spec.Solver;
			Batch.IndexSetDependencies = SolverSpec.IndexSetDependencies; //Important: Should be the dependencies of the solver, not of the one equation.
			//NOTE: Add all equations and sort the non-ode equations among themselves.
			//NOTE: We don't sort the ODE equations since they ARE allowed to have circular dependencies on each other.
			for(equation_h SolverEquation : SolverSpec.EquationsToSolve)
			{
				equation_spec &SolverResultSpec = Model->Equations.Specs[SolverEquation.Handle];
				if(SolverResultSpec.Type == EquationType_Basic) Batch.Equations.push_back(SolverEquation);
				else if(SolverResultSpec.Type == EquationType_ODE) Batch.EquationsODE.push_back(SolverEquation);
				else assert(0);
			}
			TopologicalSortEquations(Model, Batch.Equations, TopologicalSortEquationsInSolverVisit);
			
			
			s32 EarliestSuitableBatchIdx = BatchBuild.size();
			//size_t FirstBatchWeDependOn     = BatchBuild.size();
			for(s32 BatchIdx = BatchBuild.size() - 1; BatchIdx >= 0; --BatchIdx)
			{
				equation_batch_template &NeighborBatch = BatchBuild[BatchIdx];
				if(NeighborBatch.IndexSetDependencies == Batch.IndexSetDependencies)
				{
					EarliestSuitableBatchIdx = BatchIdx;
				}
				
				//FirstBatchWeDependOn = BatchIdx;
				bool DependOnBatch = WeDependOnBatch(SolverSpec.DirectResultDependencies, SolverSpec.CrossIndexResultDependencies, NeighborBatch);
				//if(DependOnBatch) std::cout << SolverSpec.Name << " depend on " << BatchIdx << std::endl;
				if(DependOnBatch) break; //We have to enter after this batch.
			}
			
			if(EarliestSuitableBatchIdx == (s32)BatchBuild.size())
			{
				BatchBuild.push_back(Batch);
			}
			else
			{
				//NOTE: This inserts the new batch right after the earliest suitable one.
				BatchBuild.insert(BatchBuild.begin() + EarliestSuitableBatchIdx + 1, Batch);
			}
		}
		else
		{
			//NOTE: put non-solver equations in the first batch that suits it. A batch is suitable if it has the same index set dependencies as the equation and if no batch after it have equations that this equation depends on.
			
			s32 EarliestSuitableBatchIdx = BatchBuild.size();
			bool EarliestSuitableIsSolver = false;
			for(s32 BatchIdx = (s32)BatchBuild.size() - 1; BatchIdx >= 0; --BatchIdx)
			{
				equation_batch_template &Batch = BatchBuild[BatchIdx];
				if(Batch.IndexSetDependencies == Spec.IndexSetDependencies)
				{
					EarliestSuitableBatchIdx = BatchIdx;
					EarliestSuitableIsSolver = (Batch.Type == BatchType_Solver);
				}
				
				bool DependOnBatch = WeDependOnBatch(Spec.DirectResultDependencies, Spec.CrossIndexResultDependencies, Batch);
				if(DependOnBatch) break; // We can not enter a batch that is earlier than this
				
				//NOTE: We don't have to check equations in the batch depends on us, because all equations that depend on us have been sorted so that they are processed after us, and so have not been put in the batches yet.
			}
			
			bool PushNewBatch = false;
			
			if(EarliestSuitableBatchIdx == (s32)BatchBuild.size())
			{
				PushNewBatch = true;
			}
			else if(EarliestSuitableIsSolver)
			{
				//This equation does not belong to a solver, so we can not add it to the solver batch. Try to add it immediately after, either by adding it to the next batch if it is suitable or by creating a new batch.
				bool CouldInsertInNext = false;
				if(EarliestSuitableBatchIdx + 1 != (s32)BatchBuild.size())
				{
					equation_batch_template &NextBatch = BatchBuild[EarliestSuitableBatchIdx + 1];
					if(NextBatch.IndexSetDependencies == Spec.IndexSetDependencies)
					{
						NextBatch.Equations.push_back(Equation);
						CouldInsertInNext = true;
					}
				}
				
				if(!CouldInsertInNext)
				{
					equation_batch_template Batch = {};
					Batch.Type = BatchType_Regular;
					Batch.Equations.push_back(Equation);
					Batch.IndexSetDependencies = Spec.IndexSetDependencies;
					BatchBuild.insert(BatchBuild.begin() + EarliestSuitableBatchIdx + 1, Batch);
				}
			}
			else
			{
				BatchBuild[EarliestSuitableBatchIdx].Equations.push_back(Equation);
			}
			
			if(PushNewBatch)
			{
				equation_batch_template Batch = {};
				Batch.Type = BatchType_Regular;
				Batch.Equations.push_back(Equation);
				Batch.IndexSetDependencies = Spec.IndexSetDependencies;
				
				if(Spec.DirectResultDependencies.empty() && Spec.CrossIndexResultDependencies.empty())
				{
					BatchBuild.insert(BatchBuild.begin(), Batch); //NOTE: If we had no dependencies at all, just insert us at the beginning rather than at the end. This makes it less likely to screw up later structure, and is for instance good for INCA-N.
				}
				else
				{
					BatchBuild.push_back(Batch);
				}
			}
		}
	}
	
#if 0
	for(auto& BatchTemplate : BatchBuild)
	{
		for(index_set IndexSet : BatchTemplate.IndexSetDependencies)
		{
			std::cout << "[" << GetName(Model, IndexSet) << "]";
		}
		std::cout << "Type: " << BatchTemplate.Type << " Solver: " << BatchTemplate.Solver.Handle << std::endl;
		FOR_ALL_BATCH_EQUATIONS(BatchTemplate,
			std::cout << "\t" << GetName(Model, Equation) << std::endl;
		)
	}
#endif
	
	//NOTE: We do a second pass to see if some equations can be shifted to a later batch. This may ultimately reduce the amount of batch groups and speed up execution. It will also make sure that cross indexing between results is more likely to be correct.
	// (TODO: this needs a better explanation, but for instance SimplyP gets a more fragmented run structure without this second pass).
	//TODO: Maaybe this could be done in the same pass as above, but I haven't figured out how. The problem is that while we are building the batches above, we don't know about any of the batches that will appear after the current batch we are building.

#if 1
	for(size_t It = 0; It < 2; ++It)    //NOTE: In the latest version of INCA-N we have to do this twice to get a good structure.
	{
		{	
			size_t BatchIdx = 0;
			for(equation_batch_template &Batch : BatchBuild)
			{
				if(Batch.Type != BatchType_Solver)
				{
					s64 EquationIdx = Batch.Equations.size() - 1;
					while(EquationIdx >= 0)
					{
						equation_h ThisEquation = Batch.Equations[EquationIdx];
						equation_spec &Spec = Model->Equations.Specs[ThisEquation.Handle];
						
						bool Continue = false;
						for(size_t EquationBehind = EquationIdx + 1; EquationBehind < Batch.Equations.size(); ++EquationBehind)
						{
							equation_h ResultBehind = Batch.Equations[EquationBehind];
							equation_spec &SpecBehind = Model->Equations.Specs[ResultBehind.Handle];
							//If somebody behind us in this batch depend on us, we are not allowed to move.
							if(std::find(SpecBehind.DirectResultDependencies.begin(), SpecBehind.DirectResultDependencies.end(), ThisEquation) != SpecBehind.DirectResultDependencies.end())
							{
								Continue = true;
								break;
							}
						}
						if(Continue)
						{
							EquationIdx--;
							continue;
						}
						
						size_t LastSuitableBatch = BatchIdx;
						for(size_t BatchBehind = BatchIdx + 1; BatchBehind < BatchBuild.size(); ++BatchBehind)
						{
							equation_batch_template &NextBatch = BatchBuild[BatchBehind];
							if(NextBatch.Type != BatchType_Solver && Spec.IndexSetDependencies == NextBatch.IndexSetDependencies) //This batch suits us
							{
								LastSuitableBatch = BatchBehind;
							}
							
							bool BatchDependsOnUs = false;
							ForAllBatchEquations(NextBatch,
							[Model, ThisEquation, &BatchDependsOnUs](equation_h Equation)
							{
								equation_spec &CheckSpec = Model->Equations.Specs[Equation.Handle];
								if(std::find(CheckSpec.DirectResultDependencies.begin(), CheckSpec.DirectResultDependencies.end(), ThisEquation) != CheckSpec.DirectResultDependencies.end())
								{
									BatchDependsOnUs = true;
									return true;
								}
								return false;
							});

							if(BatchBehind == BatchBuild.size() - 1 || BatchDependsOnUs)
							{
								if(LastSuitableBatch != BatchIdx)
								{
									//Move to the front of that batch.
									equation_batch_template &InsertToBatch = BatchBuild[LastSuitableBatch];
									InsertToBatch.Equations.insert(InsertToBatch.Equations.begin(), ThisEquation);
									Batch.Equations.erase(Batch.Equations.begin() + EquationIdx);
								}
								break;
							}
						}
						
						EquationIdx--;
					}
				}
				
				++BatchIdx;
			}
		}
		
		//NOTE: Erase Batch templates that got all their equations removed
		{
			s64 BatchIdx = BatchBuild.size()-1;
			while(BatchIdx >= 0)
			{
				equation_batch_template &Batch = BatchBuild[BatchIdx];
				if(Batch.Type != BatchType_Solver && Batch.Equations.empty())
				{
					BatchBuild.erase(BatchBuild.begin() + BatchIdx);
				}
				--BatchIdx;
			}
		}
	}
#endif
	
	/////////////// Process the batches into a finished result structure ////////////////////////
	
	size_t *EquationBelongsToBatchGroup = AllocClearedArray(size_t, Model->Equations.Count());
	
	{
		//NOTE: We have to clear sorting flags from previous sortings since we have to do a new sorting for the initial value equations.
		for(entity_handle EquationHandle = 1; EquationHandle < Model->Equations.Count(); ++EquationHandle)
		{
			Model->Equations.Specs[EquationHandle].TempVisited = false;
			Model->Equations.Specs[EquationHandle].Visited = false;
		}
		
		size_t *Counts = AllocClearedArray(size_t, Model->IndexSets.Count());
		for(auto& BatchTemplate : BatchBuild)
		{
			for(index_set_h IndexSet : BatchTemplate.IndexSetDependencies)
			{
				Counts[IndexSet.Handle]++;
			}
		}
		
		Model->EquationBatches.resize(BatchBuild.size(), {});
		
		size_t BatchIdx = 0;
		size_t BatchGroupIdx = 0;
		while(BatchIdx != BatchBuild.size())
		{
			Model->BatchGroups.push_back({});
			equation_batch_group &BatchGroup = Model->BatchGroups[Model->BatchGroups.size() - 1];
			
			equation_batch_template &FirstBatchOfGroup = BatchBuild[BatchIdx];
			std::set<index_set_h> IndexSets = FirstBatchOfGroup.IndexSetDependencies; //NOTE: set copy.
			
			BatchGroup.IndexSets.insert(BatchGroup.IndexSets.end(), IndexSets.begin(), IndexSets.end());
			std::sort(BatchGroup.IndexSets.begin(), BatchGroup.IndexSets.end(),
				[Counts] (index_set_h	A, index_set_h B) { return ((Counts[A.Handle] == Counts[B.Handle]) ? (A.Handle > B.Handle) : (Counts[A.Handle] > Counts[B.Handle])); }
			);
			
			BatchGroup.FirstBatch = BatchIdx;
			
			while(BatchIdx != BatchBuild.size() && BatchBuild[BatchIdx].IndexSetDependencies == IndexSets)
			{
				equation_batch_template &BatchTemplate = BatchBuild[BatchIdx];
				equation_batch &Batch = Model->EquationBatches[BatchIdx];
				
				Batch.Type = BatchTemplate.Type;
				Batch.Equations = BatchTemplate.Equations; //NOTE: vector copy
				if(Batch.Type == BatchType_Solver)
				{
					Batch.EquationsODE = BatchTemplate.EquationsODE;
					Batch.Solver = BatchTemplate.Solver;
				}
				
				ForAllBatchEquations(Batch,
				[&EquationBelongsToBatchGroup, BatchGroupIdx](equation_h Equation)
				{
					EquationBelongsToBatchGroup[Equation.Handle] = BatchGroupIdx;
					return false;
				});

				
				//NOTE: In this setup of initial values, we get a problem if an initial value of an equation in batch A depends on the (initial) value of an equation in batch B and batch A is before batch B. If we want to allow this, we need a completely separate batch structure for the initial value equations.... Hopefully that won't be necessary.
				//TODO: We should report an error if that happens!
				//NOTE: Currently we only allow for initial value equations to be sorted differently than their parent equations within the same batch (this was needed in some of the example models).
				ForAllBatchEquations(Batch,
				[Model, &Batch](equation_h Equation)
				{
					equation_spec &Spec = Model->Equations.Specs[Equation.Handle];
					if(IsValid(Spec.InitialValueEquation) || IsValid(Spec.InitialValue) || Spec.HasExplicitInitialValue) //NOTE: We only care about equations that have an initial value (or that are depended on by an initial value equation, but they are added recursively inside the visit)
					{
						TopologicalSortEquationsInitialValueVisit(Model, Equation, Batch.InitialValueOrder);
					}
					return false;
				});
				
				++BatchIdx;
			}
			
			BatchGroup.LastBatch = BatchIdx - 1;
			
			++BatchGroupIdx;
		}
		
		free(Counts);
	}
	
	///////////////// Find out which parameters, results and last_results that need to be hotloaded into the CurParameters, CurInputs etc. buffers in the value_set_accessor at each iteration stage during model run. /////////////////
	
	{
		size_t BatchGroupIdx = 0;
		for(equation_batch_group &BatchGroup : Model->BatchGroups)
		{
			std::set<entity_handle>     AllParameterDependenciesForBatchGroup;
			std::set<equation_h>   AllResultDependenciesForBatchGroup;
			std::set<equation_h>   AllLastResultDependenciesForBatchGroup;
			std::set<input_h>      AllInputDependenciesForBatchGroup;
			
			for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
			{
				equation_batch &Batch = Model->EquationBatches[BatchIdx];
				
				ForAllBatchEquations(Batch,
				[Model, &AllParameterDependenciesForBatchGroup, &AllInputDependenciesForBatchGroup, &AllResultDependenciesForBatchGroup, &AllLastResultDependenciesForBatchGroup](equation_h Equation)
				{
					equation_spec &Spec = Model->Equations.Specs[Equation.Handle];
					if(Spec.Type != EquationType_Cumulative) //NOTE: We don't have to load in dependencies for cumulative equations since these get their data directly from the DataSet.
					{
						AllParameterDependenciesForBatchGroup.insert(Spec.ParameterDependencies.begin(), Spec.ParameterDependencies.end());
						AllInputDependenciesForBatchGroup.insert(Spec.InputDependencies.begin(), Spec.InputDependencies.end());
						AllResultDependenciesForBatchGroup.insert(Spec.DirectResultDependencies.begin(), Spec.DirectResultDependencies.end());
						AllLastResultDependenciesForBatchGroup.insert(Spec.DirectLastResultDependencies.begin(), Spec.DirectLastResultDependencies.end());
						//TODO: The following is a quick fix so that initial value equations get their parameters set correctly. However it causes unneeded parameters to be loaded at each step for the main execution. We should make a separate system for initial value equations.
						if(IsValid(Spec.InitialValueEquation))
						{
							equation_spec &InitSpec = Model->Equations.Specs[Spec.InitialValueEquation.Handle];
							AllParameterDependenciesForBatchGroup.insert(InitSpec.ParameterDependencies.begin(), InitSpec.ParameterDependencies.end());
						}
					}
					return false;
				});
			}
			
			BatchGroup.IterationData.resize(BatchGroup.IndexSets.size(), {});
			
			for(size_t IndexSetLevel = 0; IndexSetLevel < BatchGroup.IndexSets.size(); ++IndexSetLevel)
			{
				//NOTE: Gather up all the parameters that need to be updated at this stage of the execution tree. By updated we mean that they need to be read into the CurParameters buffer during execution.
				//TODO: We do a lot of redundant checks here. We could store temporary information to speed this up.
				for(entity_handle ParameterHandle : AllParameterDependenciesForBatchGroup)
				{
					std::vector<index_set_h> &ThisParDependsOn = Model->Parameters.Specs[ParameterHandle].IndexSetDependencies;
					if(IsTopIndexSetForThisDependency(ThisParDependsOn, BatchGroup.IndexSets, IndexSetLevel))
					{
						BatchGroup.IterationData[IndexSetLevel].ParametersToRead.push_back(ParameterHandle);
					}
				}
				
				for(input_h Input : AllInputDependenciesForBatchGroup)
				{
					std::vector<index_set_h> &ThisInputDependsOn = Model->Inputs.Specs[Input.Handle].IndexSetDependencies;
					if(IsTopIndexSetForThisDependency(ThisInputDependsOn, BatchGroup.IndexSets, IndexSetLevel))
					{
						BatchGroup.IterationData[IndexSetLevel].InputsToRead.push_back(Input);
					}
				}
				
				for(equation_h Equation : AllResultDependenciesForBatchGroup)
				{
					equation_spec &Spec = Model->Equations.Specs[Equation.Handle];
					if(Spec.Type != EquationType_InitialValue) //NOTE: Initial values are handled separately in an initial setup run.
					{
						size_t ResultBatchGroupIndex = EquationBelongsToBatchGroup[Equation.Handle];
						if(ResultBatchGroupIndex < BatchGroupIdx) //NOTE: Results in the current batch group will be correct any way, and by definition we can not depend on any batches that are after this one.
						{
							std::vector<index_set_h> &ThisResultDependsOn = Model->BatchGroups[ResultBatchGroupIndex].IndexSets;
							
							if(IsTopIndexSetForThisDependency(ThisResultDependsOn, BatchGroup.IndexSets, IndexSetLevel))
							{
								BatchGroup.IterationData[IndexSetLevel].ResultsToRead.push_back(Equation);
							}
						} 
					}
				}
				
				for(equation_h Equation : AllLastResultDependenciesForBatchGroup)
				{
					equation_spec &Spec = Model->Equations.Specs[Equation.Handle];
					if(Spec.Type != EquationType_InitialValue) //NOTE: Initial values are handled separately in an initial setup run.
					{
						size_t ResultBatchGroupIndex = EquationBelongsToBatchGroup[Equation.Handle];
						if(ResultBatchGroupIndex != BatchGroupIdx) //NOTE: LAST_RESULTs in the current batch group are loaded using a different mechanism.
						{
							std::vector<index_set_h> &ThisResultDependsOn = Model->BatchGroups[ResultBatchGroupIndex].IndexSets;
							
							if(IsTopIndexSetForThisDependency(ThisResultDependsOn, BatchGroup.IndexSets, IndexSetLevel))
							{
								BatchGroup.IterationData[IndexSetLevel].LastResultsToRead.push_back(Equation);
							}
						}
					}
				}
			}
			
			for(equation_h Equation : AllLastResultDependenciesForBatchGroup)    //NOTE: We need a separate system for last_results with no index set dependencies, unfortunately.
			{
				equation_spec &Spec = Model->Equations.Specs[Equation.Handle];
				if(Spec.Type != EquationType_InitialValue) //NOTE: Initial values are handled separately in an initial setup run.
				{
					size_t ResultBatchGroupIndex = EquationBelongsToBatchGroup[Equation.Handle];
					if(ResultBatchGroupIndex != BatchGroupIdx) //NOTE: LAST_RESULTs in the current batch group are loaded using a different mechanism.
					{
						std::vector<index_set_h> &ThisResultDependsOn = Model->BatchGroups[ResultBatchGroupIndex].IndexSets;
						
						if(ThisResultDependsOn.empty())
						{
							BatchGroup.LastResultsToReadAtBase.push_back(Equation);
						}
					}
				}
			}
			
			++BatchGroupIdx;
		}
	}
	
	free(EquationBelongsToBatchGroup);
	
	
	//////////////////////// Gather about (in-) direct equation dependencies to be used by the Jacobian estimation used by some implicit solvers //////////////////////////////////
	BuildJacobianInfo(Model);
	

	Model->Finalized = true;
	
#if MOBIUS_PRINT_TIMING_INFO
	u64 Duration = GetTimerMilliseconds(&Model->DefinitionTimer);
	std::cout << "Model definition time: " << Duration << " milliseconds." << std::endl;
#endif
}

//NOTE: It is kind of superfluous to both provide the batch group and the batch group index... But it does probably not harm either?
#define INNER_LOOP_BODY(Name) void Name(mobius_data_set *DataSet, value_set_accessor *ValueSet, const equation_batch_group &BatchGroup, size_t BatchGroupIdx, s32 CurrentLevel)
typedef INNER_LOOP_BODY(mobius_inner_loop_body);

static void
ModelLoop(mobius_data_set *DataSet, value_set_accessor *ValueSet, mobius_inner_loop_body InnerLoopBody)
{
	const mobius_model *Model = DataSet->Model;
	size_t BatchGroupIdx = 0;
	for(const equation_batch_group &BatchGroup : Model->BatchGroups)
	{	
		if(BatchGroup.IndexSets.empty())
		{
			InnerLoopBody(DataSet, ValueSet, BatchGroup, BatchGroupIdx, -1);
			BatchGroupIdx++;
			continue;
		}
		
		s32 BottomLevel = (s32)BatchGroup.IndexSets.size() - 1;
		s32 CurrentLevel = 0;
		
		while (true)
		{
			index_set_h CurrentIndexSet = BatchGroup.IndexSets[CurrentLevel];
			
			if(ValueSet->CurrentIndexes[CurrentIndexSet.Handle] != DataSet->IndexCounts[CurrentIndexSet.Handle])
				InnerLoopBody(DataSet, ValueSet, BatchGroup, BatchGroupIdx, CurrentLevel);
			
			if(CurrentLevel == BottomLevel)
				++ValueSet->CurrentIndexes[CurrentIndexSet.Handle];
			
			//NOTE: We need to check again because currentindex may have changed.
			if(ValueSet->CurrentIndexes[CurrentIndexSet.Handle] == DataSet->IndexCounts[CurrentIndexSet.Handle])
			{
				//NOTE: We are at the end of this index set
				
				ValueSet->CurrentIndexes[CurrentIndexSet.Handle] = {CurrentIndexSet, 0};
				//NOTE: Traverse up the tree
				if(CurrentLevel == 0) break; //NOTE: We are finished with this batch group.
				CurrentLevel--;
				CurrentIndexSet = BatchGroup.IndexSets[CurrentLevel];
				++ValueSet->CurrentIndexes[CurrentIndexSet.Handle]; //Advance the index set above us so that we don't walk down the same branch again.
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
NaNTest(const mobius_model *Model, value_set_accessor *ValueSet, double ResultValue, equation_h Equation)
{
	if(!std::isfinite(ResultValue))
	{
		//TODO: We should be able to report the timestep here.
		MOBIUS_PARTIAL_ERROR("ERROR: Got a NaN or Inf value as the result of the equation " << GetName(Model, Equation) << " at timestep " << ValueSet->Timestep << std::endl);
		const equation_spec &Spec = Model->Equations.Specs[Equation.Handle];
		MOBIUS_PARTIAL_ERROR("Indexes:" << std::endl);
		for(index_set_h IndexSet : Spec.IndexSetDependencies)
		{
			const char *IndexName = ValueSet->DataSet->IndexNames[IndexSet.Handle][ValueSet->CurrentIndexes[IndexSet.Handle]];
			MOBIUS_PARTIAL_ERROR(GetName(Model, IndexSet) << ": " << IndexName << std::endl);
		}
		for(entity_handle Par : Spec.ParameterDependencies )
		{
			//Ugh, it is cumbersome to print parameter values when we don't know the type a priori....
			const parameter_spec &ParSpec = Model->Parameters.Specs[Par];
			if(ParSpec.Type == ParameterType_Double)
			{
				MOBIUS_PARTIAL_ERROR("Value of " << GetParameterName(Model, Par) << " was " << ValueSet->CurParameters[Par].ValDouble << std::endl);
			}
			else if(ParSpec.Type == ParameterType_UInt)
			{
				MOBIUS_PARTIAL_ERROR("Value of " << GetParameterName(Model, Par) << " was " << ValueSet->CurParameters[Par].ValUInt << std::endl);
			}
			else if(ParSpec.Type == ParameterType_Bool)
			{
				MOBIUS_PARTIAL_ERROR("Value of " << GetParameterName(Model, Par) << " was " << ValueSet->CurParameters[Par].ValBool << std::endl);
			}
		}
		for(input_h In : Spec.InputDependencies)
		{
			//TODO: Maaybe only report this if the input was provided?
			MOBIUS_PARTIAL_ERROR("Current value of " << GetName(Model, In) << " was " << ValueSet->CurInputs[In.Handle] << std::endl);
		}
		for(equation_h Res : Spec.DirectResultDependencies )
		{
			MOBIUS_PARTIAL_ERROR("Current value of " << GetName(Model, Res) << " was " << ValueSet->CurResults[Res.Handle] << std::endl);
		}
		for(equation_h Res : Spec.DirectLastResultDependencies )
		{
			MOBIUS_PARTIAL_ERROR("Last value of " << GetName(Model, Res) << " was " << ValueSet->LastResults[Res.Handle] << std::endl);
		}
		MOBIUS_FATAL_ERROR("");
	}
}

INNER_LOOP_BODY(RunInnerLoop)
{
	const mobius_model *Model = DataSet->Model;
	
	s32 BottomLevel = (s32)BatchGroup.IndexSets.size() - 1;
	
	//NOTE: Reading in to the Cur-buffers data that need to be updated at this iteration stage.
	if(CurrentLevel >= 0)
	{
		const iteration_data &IterationData = BatchGroup.IterationData[CurrentLevel];
		for(entity_handle ParameterHandle : IterationData.ParametersToRead)
		{
			ValueSet->CurParameters[ParameterHandle] = *ValueSet->AtParameterLookup; //NOTE: Parameter values are stored directly in the lookup since they don't change with the timestep.
			++ValueSet->AtParameterLookup;
		}
		for(input_h Input : IterationData.InputsToRead)
		{
			size_t Offset = *ValueSet->AtInputLookup;
			++ValueSet->AtInputLookup;
			ValueSet->CurInputs[Input.Handle] = ValueSet->AllCurInputsBase[Offset];
			ValueSet->CurInputWasProvided[Input.Handle] = DataSet->InputTimeseriesWasProvided[Offset];
		
		}
		for(equation_h Result : IterationData.ResultsToRead)
		{
			size_t Offset = *ValueSet->AtResultLookup;
			++ValueSet->AtResultLookup;
			ValueSet->CurResults[Result.Handle] = ValueSet->AllCurResultsBase[Offset];
		}
		for(equation_h Result : IterationData.LastResultsToRead)
		{
			size_t Offset = *ValueSet->AtLastResultLookup;
			++ValueSet->AtLastResultLookup;
			ValueSet->LastResults[Result.Handle] = ValueSet->AllLastResultsBase[Offset];
		}
	}
	else
	{
		for(equation_h Result : BatchGroup.LastResultsToReadAtBase)
		{
			size_t Offset = *ValueSet->AtLastResultLookup;
			++ValueSet->AtLastResultLookup;
			ValueSet->LastResults[Result.Handle] = ValueSet->AllLastResultsBase[Offset];
		}
	}

#if MOBIUS_TIMESTEP_VERBOSITY >= 2
	index_set_h CurrentIndexSet = BatchGroup.IndexSets[CurrentLevel];
	for(size_t Lev = 0; Lev < CurrentLevel; ++Lev) std::cout << "\t";
	index_t CurrentIndex = ValueSet->CurrentIndexes[CurrentIndexSet.Handle];
	std::cout << "*** " << GetName(Model, CurrentIndexSet) << ": " << DataSet->IndexNames[CurrentIndexSet.Handle][CurrentIndex] << std::endl;
#endif
	
	if(CurrentLevel == BottomLevel)
	{
		for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
		{
			//NOTE: Write LastResult values into the ValueSet for fast lookup.
			const equation_batch &Batch = Model->EquationBatches[BatchIdx];
			
			//TODO: We should maybe watch this and see if it causes performance problems and in that case expand the for loops.
			ForAllBatchEquations(Batch,
			[ValueSet](equation_h Equation)
			{
				double LastResultValue = *ValueSet->AtLastResult;
				++ValueSet->AtLastResult;
				ValueSet->LastResults[Equation.Handle] = LastResultValue;
				return false;
			});
		}
		
		for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
		{
			const equation_batch &Batch = Model->EquationBatches[BatchIdx];
			if(Batch.Type == BatchType_Regular)
			{
				//NOTE: Basic discrete timestep evaluation of equations.
				for(equation_h Equation : Batch.Equations) 
				{	
					double ResultValue = CallEquation(Model, ValueSet, Equation);
#if MOBIUS_TEST_FOR_NAN
					NaNTest(Model, ValueSet, ResultValue, Equation);
#endif
					*ValueSet->AtResult = ResultValue;
					++ValueSet->AtResult;
					ValueSet->CurResults[Equation.Handle] = ResultValue;
				
#if MOBIUS_TIMESTEP_VERBOSITY >= 3
					for(size_t Lev = 0; Lev < CurrentLevel; ++Lev) std::cout << "\t";
					std::cout << "\t" << GetName(Model, Equation) << " = " << ResultValue << std::endl;
#endif
				}
			}
			else if(Batch.Type == BatchType_Solver)
			{
				//NOTE: The results from the last timestep are the initial results for this timestep.
				size_t EquationIdx = 0;
				for(equation_h Equation : Batch.EquationsODE)
				{
					//NOTE: Reading the Equations.Specs vector here may be slightly inefficient since each element of the vector is large. We could copy out an array of the ResetEveryTimestep bools instead beforehand.
					if(Model->Equations.Specs[Equation.Handle].ResetEveryTimestep)
					{
						DataSet->x0[EquationIdx] = 0;
					}
					else
					{
						DataSet->x0[EquationIdx] = ValueSet->LastResults[Equation.Handle]; //NOTE: ValueSet.LastResult is set up above already.
					}
					++EquationIdx;
				}
				// NOTE: Do we need to clear DataSet->wk to 0? (Has not been needed in the solvers we have used so far...)
				
				const solver_spec &SolverSpec = Model->Solvers.Specs[Batch.Solver.Handle];
				
				//NOTE: This lambda is the "equation function" of the solver. It solves the set of equations once given the values in the working sets x0 and wk. It can be run by the SolverFunction many times.
				auto EquationFunction =
				[ValueSet, Model, &Batch](double *x0, double* wk)
				{	
					size_t EquationIdx = 0;
					//NOTE: Read in initial values of the ODE equations to the CurResults buffer to be accessible from within the batch equations using RESULT(H).
					//NOTE: Values are not written to ResultData before the entire solution process is finished. So during the solver process one can ONLY read intermediary results from equations belonging to this solver using RESULT(H), never RESULT(H, Idx1,...) etc. However there is no reason one would want to do that any way.
					for(equation_h Equation : Batch.EquationsODE)
					{
#if MOBIUS_TEST_FOR_NAN
						NaNTest(Model, ValueSet, x0[EquationIdx], Equation);
#endif
						ValueSet->CurResults[Equation.Handle] = x0[EquationIdx];
						++EquationIdx;
					}
					
					//NOTE: Solving basic equations tied to the solver. Values should NOT be written to the working set. They can instead be accessed from inside other equations in the solver batch using RESULT(H)
					for(equation_h Equation : Batch.Equations)
					{
						double ResultValue = CallEquation(Model, ValueSet, Equation);
#if MOBIUS_TEST_FOR_NAN
						NaNTest(Model, ValueSet, ResultValue, Equation);
#endif
						ValueSet->CurResults[Equation.Handle] = ResultValue;
					}
					
					//NOTE: Solving ODE equations tied to the solver. These values should be written to the working set.
					EquationIdx = 0;
					for(equation_h Equation : Batch.EquationsODE)
					{
						double ResultValue = CallEquation(Model, ValueSet, Equation);
						wk[EquationIdx] = ResultValue;
						
						++EquationIdx;
					}
				};
				
				mobius_solver_jacobi_function JacobiFunction = nullptr;
				if(SolverSpec.UsesJacobian)
				{
					JacobiFunction =
					[ValueSet, Model, DataSet, &Batch](double *X, mobius_matrix_insertion_function & MatrixInserter)
					{
						//TODO: Have to see if it is safe to use DataSet->wk here. It is ok for the boost solvers since they use their own working memory.
						// It is not really safe design to do this, and so we should instead preallocate different working memory for the Jacobian estimation..
						EstimateJacobian(X, MatrixInserter, DataSet->wk, Model, ValueSet, Batch);
					};
				}
				
				//NOTE: Solve the system using the provided solver
				SolverSpec.SolverFunction(SolverSpec.h, Batch.EquationsODE.size(), DataSet->x0, DataSet->wk, EquationFunction, JacobiFunction, SolverSpec.RelErr, SolverSpec.AbsErr);
				
				//NOTE: Store out the final results from this solver to the ResultData set.
				for(equation_h Equation : Batch.Equations)
				{
					double ResultValue = ValueSet->CurResults[Equation.Handle];
					*ValueSet->AtResult = ResultValue;
					++ValueSet->AtResult;
#if MOBIUS_TIMESTEP_VERBOSITY >= 3
					for(size_t Lev = 0; Lev < CurrentLevel; ++Lev) std::cout << "\t";
					std::cout << "\t" << GetName(Model, Equation) << " = " << ResultValue << std::endl;
#endif
				}
				EquationIdx = 0;
				for(equation_h Equation : Batch.EquationsODE)
				{
					double ResultValue = DataSet->x0[EquationIdx];
					ValueSet->CurResults[Equation.Handle] = ResultValue;
					*ValueSet->AtResult = ResultValue;
					++ValueSet->AtResult;
					++EquationIdx;
#if MOBIUS_TIMESTEP_VERBOSITY >= 3
					for(size_t Lev = 0; Lev < CurrentLevel; ++Lev) std::cout << "\t";
					std::cout << "\t" << GetName(Model, Equation) << " = " << ResultValue << std::endl;
#endif
				}
			}
		}
	}
}

INNER_LOOP_BODY(FastLookupSetupInnerLoop)
{
	if(CurrentLevel >= 0)
	{
		for(entity_handle ParameterHandle : BatchGroup.IterationData[CurrentLevel].ParametersToRead)
		{
			//NOTE: Parameters are special here in that we can just store the value in the fast lookup, instead of the offset. This is because they don't change with the timestep.
			size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, ParameterHandle);
			parameter_value Value = DataSet->ParameterData[Offset];
			DataSet->FastParameterLookup.push_back(Value);
		}
		
		for(input_h Input : BatchGroup.IterationData[CurrentLevel].InputsToRead)
		{
			size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, Input.Handle);
			DataSet->FastInputLookup.push_back(Offset);
		}
		
		for(equation_h Equation : BatchGroup.IterationData[CurrentLevel].ResultsToRead)
		{
			size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, Equation.Handle);
			DataSet->FastResultLookup.push_back(Offset);
		}
		
		for(equation_h Equation : BatchGroup.IterationData[CurrentLevel].LastResultsToRead)
		{
			size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, Equation.Handle);
			DataSet->FastLastResultLookup.push_back(Offset);
		}
	}
	else
	{
		for(equation_h Equation : BatchGroup.LastResultsToReadAtBase)
		{
			size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, Equation.Handle);
			DataSet->FastLastResultLookup.push_back(Offset);
		}
	}
}


inline void
SetupInitialValue(mobius_data_set *DataSet, value_set_accessor *ValueSet, equation_h Equation)
{
	
	const mobius_model *Model = DataSet->Model;
	const equation_spec &Spec = Model->Equations.Specs[Equation.Handle];
	
	double Initial = 0.0;
	if(IsValid(Spec.InitialValue))
	{
		size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, Spec.InitialValue.Handle);
		//NOTE: We should not get a type mismatch here since we only allow for registering initial values using handles of type parameter_double_h. Thus we do not need to test for which type it is.
		Initial = DataSet->ParameterData[Offset].ValDouble;
	}
	else if(Spec.HasExplicitInitialValue)
	{
		Initial = Spec.ExplicitInitialValue;
	}
	else if(IsValid(Spec.InitialValueEquation))
	{
		Initial = Model->EquationBodies[Spec.InitialValueEquation.Handle](ValueSet);
	}
	else
	{
		//NOTE: Equations without any type of initial value act as their own initial value equation
		Initial = Model->EquationBodies[Equation.Handle](ValueSet);
	}
	
	//std::cout << "Initial value of " << GetName(Model, Equation) << " is " << Initial << std::endl;
 	
	size_t ResultStorageLocation = DataSet->ResultStorageStructure.LocationOfHandleInUnit[Equation.Handle];
	
	ValueSet->AtResult[ResultStorageLocation] = Initial;
	ValueSet->CurResults[Equation.Handle] = Initial;
	ValueSet->LastResults[Equation.Handle] = Initial;
}

INNER_LOOP_BODY(InitialValueSetupInnerLoop)
{
	// TODO: IMPORTANT!! If an initial value equation depends on the result of an equation from a different batch than itself, that is not guaranteed to work correctly.
	// TODO: Currently we don't report an error if that happens!
	
	const mobius_model *Model = DataSet->Model;
	
	if(CurrentLevel >= 0)
	{
		for(entity_handle ParameterHandle : BatchGroup.IterationData[CurrentLevel].ParametersToRead)
		{
			ValueSet->CurParameters[ParameterHandle] = *ValueSet->AtParameterLookup;
			++ValueSet->AtParameterLookup;
		}
	}
	
	s32 BottomLevel = BatchGroup.IndexSets.size() - 1;
	if(CurrentLevel == BottomLevel)
	{
		for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
		{
			const equation_batch &Batch = Model->EquationBatches[BatchIdx];
			for(equation_h Equation : Batch.InitialValueOrder)
			{
				SetupInitialValue(DataSet, ValueSet, Equation);
			}
		}
		
		ValueSet->AtResult += DataSet->ResultStorageStructure.Units[BatchGroupIdx].Handles.size(); //NOTE: This works because we set the storage structure up to mirror the batch group structure.
	}
}

static void
PrintEquationProfiles(mobius_data_set *DataSet, value_set_accessor *ValueSet);

static void
ProcessComputedParameters(mobius_data_set *DataSet, value_set_accessor *ValueSet)
{
	//NOTE: Preprocessing of computed parameters.
	for(entity_handle ParameterHandle = 1; ParameterHandle < DataSet->Model->Parameters.Count(); ++ParameterHandle)
	{
		const parameter_spec &Spec = DataSet->Model->Parameters.Specs[ParameterHandle];
		equation_h Equation = Spec.IsComputedBy;
		if(IsValid(Spec.IsComputedBy))
		{
			const equation_spec &EqSpec = DataSet->Model->Equations.Specs[Equation.Handle];
			
			ForeachParameterInstance(DataSet, ParameterHandle,
				[DataSet, ParameterHandle, Equation, &EqSpec, &Spec, ValueSet](index_t *Indexes, size_t IndexesCount)
				{
					//NOTE: We have to set the value set accessor into the right state.
					
					for(size_t IdxIdx = 0; IdxIdx < IndexesCount; ++IdxIdx)
					{
						index_t Index = Indexes[IdxIdx];
						ValueSet->CurrentIndexes[Index.IndexSetHandle] = Index;
					}
					
					for(entity_handle DependentParameter : EqSpec.ParameterDependencies)
					{
						size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, ValueSet->CurrentIndexes, DataSet->IndexCounts, DependentParameter);
						ValueSet->CurParameters[DependentParameter] = DataSet->ParameterData[Offset];
					}
					
					size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, Indexes, IndexesCount, DataSet->IndexCounts, ParameterHandle);
					double ValD = DataSet->Model->EquationBodies[Equation.Handle](ValueSet);
					parameter_value Value;
					if(Spec.Type == ParameterType_Double)
					{
						Value.ValDouble = ValD;
					}
					else if(Spec.Type == ParameterType_UInt)
					{
						Value.ValUInt = (u64)ValD; //Or should we do a more sophisticated rounding?
					}
					DataSet->ParameterData[Offset] = Value;
				}
			);
		}
	}
}

static void
RunModel(mobius_data_set *DataSet)
{
#if MOBIUS_PRINT_TIMING_INFO
	timer SetupTimer = BeginTimer();
#endif

	const mobius_model *Model = DataSet->Model;
	
	//NOTE: Check that all the index sets have at least one index.
	for(entity_handle IndexSetHandle = 1; IndexSetHandle < Model->IndexSets.Count(); ++IndexSetHandle)
	{
		if(DataSet->IndexCounts[IndexSetHandle] == 0)
		{
			MOBIUS_FATAL_ERROR("ERROR: The index set " << GetName(Model, index_set_h {IndexSetHandle}) << " does not contain any indexes." << std::endl);
		}
	}
	
	//NOTE: Allocate parameter storage in case it was not allocated during setup.
	if(!DataSet->ParameterData)
	{
		AllocateParameterStorage(DataSet);
		std::cout << "WARNING: No parameter values were specified, using default parameter values only." << std::endl;
	}
	
	
	u64 Timesteps      = GetTimesteps(DataSet);
	datetime ModelStartTime = GetStartDate(DataSet); //NOTE: This reads the "Start date" parameter.
	
#if MOBIUS_PRINT_TIMING_INFO
	std::cout << "Running model " << Model->Name << " V" << Model->Version << " for " << Timesteps << " timesteps, starting at " << ModelStartTime.ToString() << std::endl;
#endif
	
	//NOTE: Allocate input storage in case it was not allocated during setup.
	if(!DataSet->InputData)
	{
		AllocateInputStorage(DataSet, Timesteps);
		std::cout << "WARNING: No input values were specified, using input values of 0 only." << std::endl;
	}
	
	datetime InputStartDate = GetInputStartDate(DataSet);
	s64 InputDataStartOffsetTimesteps = InputStartDate.DaysUntil(ModelStartTime); //NOTE: Only one-day timesteps currently supported.
	if(InputDataStartOffsetTimesteps < 0)
	{
		MOBIUS_FATAL_ERROR("ERROR: The input data starts at a later date than the model run." << std::endl);
	}
	
	if(((s64)DataSet->InputDataTimesteps - InputDataStartOffsetTimesteps) < (s64)Timesteps)
	{
		MOBIUS_FATAL_ERROR("ERROR: The input data provided has fewer timesteps than the number of timesteps the model is running for." << std::endl);
	}
	
	//std::cout << "Input data start offset timesteps was " << InputDataStartOffsetTimesteps << std::endl;
	
	if(DataSet->HasBeenRun)
	{
		//NOTE: This is in case somebody wants to re-run the same dataset after e.g. changing a few parameters.
		free(DataSet->ResultData);
		DataSet->ResultData = 0;
		
		DataSet->FastParameterLookup.clear();
		//TODO: We may not need to clear the last ones, but in that case we should not rebuild them below.
		DataSet->FastInputLookup.clear();
		DataSet->FastResultLookup.clear();
		DataSet->FastLastResultLookup.clear();
	}
	
	AllocateResultStorage(DataSet, Timesteps);
	
	value_set_accessor ValueSet(DataSet);
	
	ProcessComputedParameters(DataSet, &ValueSet);
	
	ValueSet.Clear();
	
	
	for(const mobius_preprocessing_step &PreprocessingStep : Model->PreprocessingSteps)
	{
		PreprocessingStep(DataSet);
	}
	
	///////////// Setting up fast lookup ////////////////////
	
	ModelLoop(DataSet, &ValueSet, FastLookupSetupInnerLoop);
	ValueSet.Clear();
	
	//NOTE: Temporary storage for use by solvers:
	size_t MaxODECount = 0;
	for(const equation_batch_group& BatchGroup : Model->BatchGroups)
	{
		for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
		{
			const equation_batch &Batch = Model->EquationBatches[BatchIdx];
			if(Batch.Type == BatchType_Solver)
			{
				size_t ODECount = Batch.EquationsODE.size();
				MaxODECount = Max(MaxODECount, ODECount);
			}
		}
	}
	if(!DataSet->x0) DataSet->x0 = AllocClearedArray(double, MaxODECount);
	if(!DataSet->wk) DataSet->wk = AllocClearedArray(double, 4*MaxODECount); //TODO: This size is specifically for IncaDascru. Other solvers may have other needs for storage, so this 4 should not be hard coded. Note however that the Boost solvers use their own storage, so this is not an issue in that case.
	
	

	//NOTE: System parameters (i.e. parameters that don't depend on index sets) are going to be the same during the entire run, so we just load them into CurParameters once and for all.
	//NOTE: If any system parameters exist, the storage units are sorted such that the system parameters have to belong to storage unit [0].
	if(!DataSet->ParameterStorageStructure.Units.empty() && DataSet->ParameterStorageStructure.Units[0].IndexSets.empty())
	{
		for(entity_handle ParameterHandle : DataSet->ParameterStorageStructure.Units[0].Handles)
		{
			size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, ParameterHandle);
			ValueSet.CurParameters[ParameterHandle] = DataSet->ParameterData[Offset];
		}
	}
	
	//NOTE: Similarly we have to set which of the unindexed inputs were provided.
	if(!DataSet->InputStorageStructure.Units.empty() && DataSet->InputStorageStructure.Units[0].IndexSets.empty())
	{
		for(entity_handle InputHandle : DataSet->InputStorageStructure.Units[0].Handles)
		{
			size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, InputHandle);
			ValueSet.CurInputWasProvided[InputHandle] = DataSet->InputTimeseriesWasProvided[Offset];
		}
	}
	
	ValueSet.AllLastResultsBase = DataSet->ResultData;
	ValueSet.AllCurResultsBase = DataSet->ResultData;
	
	s32 Year;
	ModelStartTime.DayOfYear(&ValueSet.DayOfYear, &Year);
	ValueSet.DaysThisYear = YearLength(Year);
	
	//NOTE: Set up initial values;
	ValueSet.AtResult = ValueSet.AllCurResultsBase;
	ValueSet.AtLastResult = ValueSet.AllLastResultsBase;
	ValueSet.AtParameterLookup = DataSet->FastParameterLookup.data();
	ValueSet.Timestep = -1;
	ModelLoop(DataSet, &ValueSet, InitialValueSetupInnerLoop);
	
#if MOBIUS_PRINT_TIMING_INFO
	u64 SetupDuration = GetTimerMilliseconds(&SetupTimer);
	timer RunTimer = BeginTimer();
	u64 BeforeC = __rdtsc();
#endif
	
	ValueSet.AllLastResultsBase = DataSet->ResultData;
	ValueSet.AllCurResultsBase = DataSet->ResultData + DataSet->ResultStorageStructure.TotalCount;
	ValueSet.AllCurInputsBase = DataSet->InputData + ((size_t)InputDataStartOffsetTimesteps)*DataSet->InputStorageStructure.TotalCount;
	
#if MOBIUS_EQUATION_PROFILING
	ValueSet.EquationHits        = AllocClearedArray(size_t, Model->Equations.Count());
	ValueSet.EquationTotalCycles = AllocClearedArray(u64, Model->Equations.Count());
#endif
	
	for(u64 Timestep = 0; Timestep < Timesteps; ++Timestep)
	{
		
#if MOBIUS_TIMESTEP_VERBOSITY >= 1
		std::cout << "Timestep: " << Timestep << std::endl;
		//std::cout << "Day of year: " << ValueSet.DayOfYear << std::endl;
#endif
		
		ValueSet.Timestep = (s64)Timestep;
		ValueSet.AtResult = ValueSet.AllCurResultsBase;
		ValueSet.AtLastResult = ValueSet.AllLastResultsBase;
		
		ValueSet.AtParameterLookup  = DataSet->FastParameterLookup.data();
		ValueSet.AtInputLookup      = DataSet->FastInputLookup.data();
		ValueSet.AtResultLookup     = DataSet->FastResultLookup.data();
		ValueSet.AtLastResultLookup = DataSet->FastLastResultLookup.data();
		
		//NOTE: We have to update the inputs that don't depend on any index sets here, as that is not handled by the "fast lookup system".
		if(!DataSet->InputStorageStructure.Units.empty() && DataSet->InputStorageStructure.Units[0].IndexSets.empty())
		{
			for(entity_handle InputHandle : DataSet->InputStorageStructure.Units[0].Handles)
			{
				size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, InputHandle);
				ValueSet.CurInputs[InputHandle] = ValueSet.AllCurInputsBase[Offset];
			}
		}
		
		ModelLoop(DataSet, &ValueSet, RunInnerLoop);
		
		ValueSet.AllLastResultsBase = ValueSet.AllCurResultsBase;
		ValueSet.AllCurResultsBase += DataSet->ResultStorageStructure.TotalCount;
		ValueSet.AllCurInputsBase  += DataSet->InputStorageStructure.TotalCount;
		
		ValueSet.DayOfYear++;
		if(ValueSet.DayOfYear == (365 + IsLeapYear(Year) + 1))
		{
			ValueSet.DayOfYear = 1;
			Year++;
			ValueSet.DaysThisYear = 365 + IsLeapYear(Year);
		}
	}
	
#if MOBIUS_PRINT_TIMING_INFO
	u64 AfterC = __rdtsc();
	
	u64 RunDuration = GetTimerMilliseconds(&RunTimer);
	u64 RunDurationCycles = AfterC - BeforeC;

	std::cout << "Model execution setup time: " << SetupDuration << " milliseconds" << std::endl;
	std::cout << "Model execution time: " << RunDuration << " milliseconds" << std::endl;
	std::cout << "Model execution processor cycles: " << RunDurationCycles << std::endl;
	std::cout << "Average cycles per result instance including overhead: " << (RunDurationCycles / (Timesteps * DataSet->ResultStorageStructure.TotalCount)) << std::endl;
	std::cout << "(Note: one instance can be the result of several equation evaluations in the case of solvers)" << std::endl;
#endif

#if MOBIUS_EQUATION_PROFILING
	PrintEquationProfiles(DataSet, &ValueSet);
	free(ValueSet.EquationHits);
	free(ValueSet.EquationTotalCycles);
#endif
	
	DataSet->HasBeenRun = true;
	DataSet->TimestepsLastRun = Timesteps;
	DataSet->StartDateLastRun = ModelStartTime;
}

static void
PrintEquationDependencies(mobius_model *Model)
{
	std::cout << std::endl << "**** Equation Dependencies ****" << std::endl;
	if(Model->Finalized)
	{	
		for(entity_handle EquationHandle = 1; EquationHandle < Model->Equations.Count(); ++EquationHandle)
		{
			std::cout << GetName(Model, equation_h {EquationHandle}) << "\n\t";
			for(index_set_h IndexSet : Model->Equations.Specs[EquationHandle].IndexSetDependencies)
			{
				std::cout << "[" << GetName(Model, IndexSet) << "]";
			}
			std::cout << std::endl;
		}
	}
	else
	{
		std::cout << "WARNING: Tried to print equation dependencies before the model was finalized" << std::endl;
	}
}

static void
PrintResultStructure(mobius_model *Model)
{
	std::cout << std::endl << "**** Result Structure ****" << std::endl;
	//std::cout << "Number of batches: " << Model->ResultStructure.size() << std::endl;
	for(equation_batch_group &BatchGroup : Model->BatchGroups)
	{
		if(BatchGroup.IndexSets.empty()) std::cout << "[]";
		for(index_set_h IndexSet : BatchGroup.IndexSets)
		{
			std::cout << "[" << GetName(Model, IndexSet) << "]";
		}
		
		for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
		{
			equation_batch &Batch = Model->EquationBatches[BatchIdx];
			std::cout << "\n\t-----";
			if(Batch.Type == BatchType_Solver) std::cout << " (SOLVER: " << GetName(Model, Batch.Solver) << ")";
			
			ForAllBatchEquations(Batch,
			[Model](equation_h Equation)
			{
				std::cout << "\n\t";
				if(Model->Equations.Specs[Equation.Handle].Type == EquationType_Cumulative) std::cout << "(Cumulative) ";
				else if(Model->Equations.Specs[Equation.Handle].Type == EquationType_ODE) std::cout << "(ODE) ";
				std::cout << GetName(Model, Equation);
				return false;
			});
			if(BatchIdx == BatchGroup.LastBatch) std::cout << "\n\t-----\n";
		}
		
		std::cout << std::endl;
	}
}

static void
PrintParameterStorageStructure(mobius_data_set *DataSet)
{
	if(!DataSet->ParameterData)
	{
		std::cout << "WARNING: Tried to print parameter storage structure before the parameter storage was allocated" << std::endl;
		return;
	}
	
	const mobius_model *Model = DataSet->Model;
	
	std::cout << std::endl << "**** Parameter storage structure ****" << std::endl;
	size_t StorageCount = DataSet->ParameterStorageStructure.Units.size();
	for(size_t StorageIdx = 0; StorageIdx < StorageCount; ++StorageIdx)
	{
		std::vector<index_set_h> &IndexSets = DataSet->ParameterStorageStructure.Units[StorageIdx].IndexSets;
		if(IndexSets.empty())
		{
			std::cout << "[]";
		}
		for(index_set_h IndexSet : IndexSets)
		{
			std::cout << "[" << GetName(Model, IndexSet) << "]";
		}
		for(entity_handle ParameterHandle : DataSet->ParameterStorageStructure.Units[StorageIdx].Handles)
		{
			std::cout << "\n\t" << GetParameterName(Model, ParameterHandle);
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

static void
PrintInputStorageStructure(mobius_data_set *DataSet)
{
	if(!DataSet->InputData)
	{
		std::cout << "WARNING: Tried to print input storage structure before the input storage was allocated" << std::endl;
		return;
	}
	
	const mobius_model *Model = DataSet->Model;
	
	std::cout << std::endl << "**** Input storage structure ****" << std::endl;
	size_t StorageCount = DataSet->InputStorageStructure.Units.size();
	for(size_t StorageIdx = 0; StorageIdx < StorageCount; ++StorageIdx)
	{
		std::vector<index_set_h> &IndexSets = DataSet->InputStorageStructure.Units[StorageIdx].IndexSets;
		if(IndexSets.empty())
		{
			std::cout << "[]";
		}
		for(index_set_h IndexSet : IndexSets)
		{
			std::cout << "[" << GetName(Model, IndexSet) << "]";
		}
		for(entity_handle Handle : DataSet->InputStorageStructure.Units[StorageIdx].Handles)
		{
			std::cout << "\n\t" << GetName(Model, input_h {Handle});
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

static void
PrintEquationProfiles(mobius_data_set *DataSet, value_set_accessor *ValueSet)
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
		if(BatchGroup.IndexSets.empty()) std::cout << "[]";
		for(index_set_h IndexSet : BatchGroup.IndexSets)
		{
			std::cout << "[" << GetName(Model, IndexSet) << "]";
		}
		
		for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
		{
			const equation_batch &Batch = Model->EquationBatches[BatchIdx];
			std::cout << "\n\t-----";
			if(Batch.Type == BatchType_Solver) std::cout << " (SOLVER: " << GetName(Model, Batch.Solver) << ")";
			
			ForAllBatchEquations(Batch,
			[Model, ValueSet, &TotalHits, &SumCc](equation_h Equation)
			{
				int PrintCount = 0;
				printf("\n\t");
				if(Model->Equations.Specs[Equation.Handle].Type == EquationType_Cumulative) PrintCount += printf("(Cumulative) ");
				else if(Model->Equations.Specs[Equation.Handle].Type == EquationType_ODE) PrintCount += printf("(ODE) ");
				PrintCount += printf("%s: ", GetName(Model, Equation));
				
				u64 Cc = ValueSet->EquationTotalCycles[Equation.Handle];
				size_t Hits = ValueSet->EquationHits[Equation.Handle];
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
