
static mobius_model *
BeginModelDefinition(const char *Name = "(unnamed model)")
{
	mobius_model *Model = new mobius_model {};
	
	Model->BucketMemory.Initialize(1024*1024);
	
	Model->DefinitionTimer = BeginTimer();
	
	Model->Name = Name;
	
	auto Days 	      = RegisterUnit(Model, "days");
	auto System       = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 100);
	RegisterParameterDate(Model, System, "Start date", "1970-1-1");
	
	SetTimestepSize(Model, "1D");   // One day as default
	
	return Model;
}

mobius_model::~mobius_model()
{
	BucketMemory.DeallocateAll();
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
IsTopIndexSetForThisDependency(std::vector<index_set_h> &IndexSetDependencies, array<index_set_h> &BatchGroupIndexSets, size_t IndexSetLevel)
{
	index_set_h CurrentLevelIndexSet = BatchGroupIndexSets[IndexSetLevel];
	bool DependsOnCurrentLevel = (std::find(IndexSetDependencies.begin(), IndexSetDependencies.end(), CurrentLevelIndexSet) != IndexSetDependencies.end());
	if(!DependsOnCurrentLevel) return false;
	
	for(size_t LevelAbove = IndexSetLevel + 1; LevelAbove < BatchGroupIndexSets.Count; ++LevelAbove)
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
	
	bucket_allocator TemporaryBucket;
	TemporaryBucket.Initialize(1024*1024);  //NOTE: It would be very strange if we need more space than this for each temporary array.
	
	///////////// Find out what index sets each parameter depends on /////////////
	
	for(entity_handle ParameterHandle = 1; ParameterHandle < Model->Parameters.Count(); ++ParameterHandle)
	{
		parameter_spec &Spec = Model->Parameters.Specs[ParameterHandle];
		parameter_group_h CurrentGroup = Spec.Group;
		const parameter_group_spec &GroupSpec = Model->ParameterGroups.Specs[CurrentGroup.Handle];
		
		Spec.IndexSetDependencies = GroupSpec.IndexSets;  //NOTE: vector copy.
	}
	
	/////////////////////// Find all dependencies of equations on parameters, inputs and other results /////////////////////
	
	model_run_state RunState(Model);
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
		RunState.Clear();
		
		//Call the equation. Since we are in RunState.Running==false mode, the equation will register which values it tried to access.
		Model->EquationBodies[EquationHandle](&RunState);
		
		Spec.IndexSetDependencies.insert(RunState.DirectIndexSetDependencies.begin(), RunState.DirectIndexSetDependencies.end());
		
		for(dependency_registration ParameterDependency : RunState.ParameterDependencies)
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
		
		for(dependency_registration InputDependency : RunState.InputDependencies)
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
		
		for(const result_dependency_registration &ResultDependency : RunState.ResultDependencies)
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
				Spec.IndexedResultAndLastResultDependencies.push_back(ResultDependency); //TODO: Maybe don't store these on the equation spec? They are only needed in this algorithm..
			}
		}
		
		for(const result_dependency_registration &ResultDependency : RunState.LastResultDependencies)
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
		//TODO: Figure out if this may break something, and if we need a specialized system for this?
		equation_h EqInitialValue = Model->Equations.Specs[EquationHandle].InitialValueEquation;
		if(IsValid(EqInitialValue))
		{
			Spec.DirectLastResultDependencies.insert(EqInitialValue);
		}
	}
	
	{
		//NOTE: Check computed parameters to see if their equation satisfies the requirements specified in the documentation:
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
	
	//TODO: This is probably an inefficient way to do it, we should instead use some kind of graph traversal, but it is tricky. To do it properly one would have to collapse the dependency graph (including both results and lastresults) by its strongly connected components, then resolving the dependencies between the components.  However the current implementation has proven fast enough so far.
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
	
	bool *SolverHasBeenHitOnce = TemporaryBucket.Allocate<bool>(Model->Solvers.Count());
	
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
	
	size_t *EquationBelongsToBatchGroup = TemporaryBucket.Allocate<size_t>(Model->Equations.Count());
	
	{
		//NOTE: We have to clear sorting flags from previous sortings since we have to do a new sorting for the initial value equations.
		for(entity_handle EquationHandle = 1; EquationHandle < Model->Equations.Count(); ++EquationHandle)
		{
			Model->Equations.Specs[EquationHandle].TempVisited = false;
			Model->Equations.Specs[EquationHandle].Visited = false;
		}
		
		size_t *Counts = TemporaryBucket.Allocate<size_t>(Model->IndexSets.Count());
		for(auto& BatchTemplate : BatchBuild)
		{
			for(index_set_h IndexSet : BatchTemplate.IndexSetDependencies)
			{
				Counts[IndexSet.Handle]++;
			}
		}
		
		//Model->EquationBatches.resize(BatchBuild.size(), {});
		Model->EquationBatches.Allocate(&Model->BucketMemory, BatchBuild.size());
		//NOTE: Determine the number of batch groups
		size_t BatchGroupCount = 0;
		size_t BatchIdx = 0;
		while(BatchIdx != BatchBuild.size())
		{
			BatchGroupCount++;
			std::set<index_set_h> IndexSets = BatchBuild[BatchIdx].IndexSetDependencies; //NOTE: set copy.
			while(BatchIdx != BatchBuild.size() && BatchBuild[BatchIdx].IndexSetDependencies == IndexSets) ++BatchIdx;
		}
		
		Model->BatchGroups.Allocate(&Model->BucketMemory, BatchGroupCount);
		
		BatchIdx = 0;
		size_t BatchGroupIdx = 0;
		while(BatchIdx != BatchBuild.size())
		{
			//Model->BatchGroups.push_back({});
			equation_batch_group &BatchGroup = Model->BatchGroups[BatchGroupIdx];
			
			std::set<index_set_h> IndexSets = BatchBuild[BatchIdx].IndexSetDependencies; //NOTE: set copy.

			BatchGroup.IndexSets.Allocate(&Model->BucketMemory, IndexSets.size());
			size_t Idx = 0;
			for(index_set_h IndexSet: IndexSets)
			{
				BatchGroup.IndexSets[Idx] = IndexSet;
				++Idx;
			}
			
			std::sort(BatchGroup.IndexSets.begin(), BatchGroup.IndexSets.end(),
				[Counts] (index_set_h A, index_set_h B) { return ((Counts[A.Handle] == Counts[B.Handle]) ? (A.Handle > B.Handle) : (Counts[A.Handle] > Counts[B.Handle])); }
			);    //Sort index sets so that the ones with more equations depending on them are higher up. This empirically gives a better batch structure.
			
			BatchGroup.FirstBatch = BatchIdx;
			
			while(BatchIdx != BatchBuild.size() && BatchBuild[BatchIdx].IndexSetDependencies == IndexSets)
			{
				equation_batch_template &BatchTemplate = BatchBuild[BatchIdx];
				equation_batch &Batch = Model->EquationBatches[BatchIdx];
				
				Batch.Type = BatchTemplate.Type;
				Batch.Equations = CopyDataToArray(&Model->BucketMemory, BatchTemplate.Equations.data(), BatchTemplate.Equations.size());
				if(Batch.Type == BatchType_Solver)
				{
					Batch.EquationsODE = CopyDataToArray(&Model->BucketMemory, BatchTemplate.EquationsODE.data(), BatchTemplate.EquationsODE.size());
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
				std::vector<equation_h> InitialValueOrder;
				
				ForAllBatchEquations(Batch,
				[Model, &Batch, &InitialValueOrder](equation_h Equation)
				{
					equation_spec &Spec = Model->Equations.Specs[Equation.Handle];
					if(IsValid(Spec.InitialValueEquation) || IsValid(Spec.InitialValue) || Spec.HasExplicitInitialValue) //NOTE: We only care about equations that have an initial value (or that are depended on by an initial value equation, but they are added recursively inside the visit)
					{
						TopologicalSortEquationsInitialValueVisit(Model, Equation, InitialValueOrder);
					}
					return false;
				});
				
				Batch.InitialValueOrder = CopyDataToArray(&Model->BucketMemory, InitialValueOrder.data(), InitialValueOrder.size());
				
				++BatchIdx;
			}
			
			BatchGroup.LastBatch = BatchIdx - 1;
			
			++BatchGroupIdx;
		}
	}
	
	///////////////// Find out which parameters, results and last_results that need to be hotloaded into the CurParameters, CurInputs etc. buffers in the model_run_state at each iteration stage during model run. /////////////////
	
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
			
			BatchGroup.IterationData.Allocate(&Model->BucketMemory, BatchGroup.IndexSets.Count);
			
			for(size_t IndexSetLevel = 0; IndexSetLevel < BatchGroup.IndexSets.Count; ++IndexSetLevel)
			{
				std::vector<entity_handle> ParametersToRead;
				std::vector<input_h>       InputsToRead;
				std::vector<equation_h>    ResultsToRead;
				std::vector<equation_h>    LastResultsToRead;
				//NOTE: Gather up all the parameters that need to be updated at this stage of the execution tree. By updated we mean that they need to be read into the CurParameters buffer during execution.
				//TODO: We do a lot of redundant checks here. We could store temporary information to speed this up.
				for(entity_handle ParameterHandle : AllParameterDependenciesForBatchGroup)
				{
					std::vector<index_set_h> &ThisParDependsOn = Model->Parameters.Specs[ParameterHandle].IndexSetDependencies;
					if(IsTopIndexSetForThisDependency(ThisParDependsOn, BatchGroup.IndexSets, IndexSetLevel))
					{
						ParametersToRead.push_back(ParameterHandle);
					}
				}
				
				BatchGroup.IterationData[IndexSetLevel].ParametersToRead = CopyDataToArray(&Model->BucketMemory, ParametersToRead.data(), ParametersToRead.size());
				
				for(input_h Input : AllInputDependenciesForBatchGroup)
				{
					std::vector<index_set_h> &ThisInputDependsOn = Model->Inputs.Specs[Input.Handle].IndexSetDependencies;
					if(IsTopIndexSetForThisDependency(ThisInputDependsOn, BatchGroup.IndexSets, IndexSetLevel))
					{
						InputsToRead.push_back(Input);
					}
				}
				
				BatchGroup.IterationData[IndexSetLevel].InputsToRead = CopyDataToArray(&Model->BucketMemory, InputsToRead.data(), InputsToRead.size());
				
				for(equation_h Equation : AllResultDependenciesForBatchGroup)
				{
					equation_spec &Spec = Model->Equations.Specs[Equation.Handle];
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
				
				BatchGroup.IterationData[IndexSetLevel].ResultsToRead = CopyDataToArray(&Model->BucketMemory, ResultsToRead.data(), ResultsToRead.size());
				
				for(equation_h Equation : AllLastResultDependenciesForBatchGroup)
				{
					equation_spec &Spec = Model->Equations.Specs[Equation.Handle];
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
				
				BatchGroup.IterationData[IndexSetLevel].LastResultsToRead = CopyDataToArray(&Model->BucketMemory, LastResultsToRead.data(), LastResultsToRead.size());
			}
			
			std::vector<equation_h> LastResultsToReadAtBase;
			
			for(equation_h Equation : AllLastResultDependenciesForBatchGroup)    //NOTE: We need a separate system for last_results with no index set dependencies, unfortunately.
			{
				equation_spec &Spec = Model->Equations.Specs[Equation.Handle];
				if(Spec.Type != EquationType_InitialValue) //NOTE: Initial values are handled separately in an initial setup run.
				{
					size_t ResultBatchGroupIndex = EquationBelongsToBatchGroup[Equation.Handle];
					if(ResultBatchGroupIndex != BatchGroupIdx) //NOTE: LAST_RESULTs in the current batch group are loaded using a different mechanism.
					{
						array<index_set_h> &ThisResultDependsOn = Model->BatchGroups[ResultBatchGroupIndex].IndexSets;
						
						if(ThisResultDependsOn.Count == 0)
						{
							LastResultsToReadAtBase.push_back(Equation);
						}
					}
				}
			}
			
			BatchGroup.LastResultsToReadAtBase = CopyDataToArray(&Model->BucketMemory, LastResultsToReadAtBase.data(), LastResultsToReadAtBase.size());
			
			++BatchGroupIdx;
		}
	}
	
	
	//////////////////////// Gather about (in-) direct equation dependencies to be used by the Jacobian estimation used by some implicit solvers //////////////////////////////////
	BuildJacobianInfo(Model);
	
	TemporaryBucket.DeallocateAll();

	Model->Finalized = true;
	
#if MOBIUS_PRINT_TIMING_INFO
	u64 Duration = GetTimerMilliseconds(&Model->DefinitionTimer);
	std::cout << "Model definition time: " << Duration << " milliseconds." << std::endl;
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
		//TODO: We should be able to report the timestep here.
		MOBIUS_PARTIAL_ERROR("ERROR: Got a NaN or Inf value as the result of the equation " << GetName(Model, Equation) << " at timestep " << RunState->Timestep << std::endl);
		const equation_spec &Spec = Model->Equations.Specs[Equation.Handle];
		MOBIUS_PARTIAL_ERROR("Indexes:" << std::endl);
		for(index_set_h IndexSet : Spec.IndexSetDependencies)
		{
			const char *IndexName = RunState->DataSet->IndexNames[IndexSet.Handle][RunState->CurrentIndexes[IndexSet.Handle]];
			MOBIUS_PARTIAL_ERROR(GetName(Model, IndexSet) << ": " << IndexName << std::endl);
		}
		for(entity_handle Par : Spec.ParameterDependencies )
		{
			//Ugh, it is cumbersome to print parameter values when we don't know the type a priori....
			const parameter_spec &ParSpec = Model->Parameters.Specs[Par];
			if(ParSpec.Type == ParameterType_Double)
			{
				MOBIUS_PARTIAL_ERROR("Value of " << GetParameterName(Model, Par) << " was " << RunState->CurParameters[Par].ValDouble << std::endl);
			}
			else if(ParSpec.Type == ParameterType_UInt)
			{
				MOBIUS_PARTIAL_ERROR("Value of " << GetParameterName(Model, Par) << " was " << RunState->CurParameters[Par].ValUInt << std::endl);
			}
			else if(ParSpec.Type == ParameterType_Bool)
			{
				MOBIUS_PARTIAL_ERROR("Value of " << GetParameterName(Model, Par) << " was " << RunState->CurParameters[Par].ValBool << std::endl);
			}
		}
		for(input_h In : Spec.InputDependencies)
		{
			MOBIUS_PARTIAL_ERROR("Current value of " << GetName(Model, In) << " was " << RunState->CurInputs[In.Handle]);
			if(!RunState->CurInputWasProvided[In.Handle])
			{
				MOBIUS_PARTIAL_ERROR(" (Not provided in dataset)");
			}
			MOBIUS_PARTIAL_ERROR(std::endl);
		}
		for(equation_h Res : Spec.DirectResultDependencies )
		{
			MOBIUS_PARTIAL_ERROR("Current value of " << GetName(Model, Res) << " was " << RunState->CurResults[Res.Handle] << std::endl);
		}
		for(equation_h Res : Spec.DirectLastResultDependencies )
		{
			MOBIUS_PARTIAL_ERROR("Last value of " << GetName(Model, Res) << " was " << RunState->LastResults[Res.Handle] << std::endl);
		}
		MOBIUS_FATAL_ERROR("");
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
		for(entity_handle ParameterHandle : IterationData.ParametersToRead)
		{
			RunState->CurParameters[ParameterHandle] = *RunState->AtParameterLookup; //NOTE: Parameter values are stored directly in the lookup since they don't change with the timestep.
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
	index_set_h CurrentIndexSet = BatchGroup.IndexSets[CurrentLevel];
	for(size_t Lev = 0; Lev < CurrentLevel; ++Lev) std::cout << "\t";
	index_t CurrentIndex = RunState->CurrentIndexes[CurrentIndexSet.Handle];
	std::cout << "*** " << GetName(Model, CurrentIndexSet) << ": " << DataSet->IndexNames[CurrentIndexSet.Handle][CurrentIndex] << std::endl;
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
			if(Batch.Type == BatchType_Regular)
			{
				//NOTE: Basic discrete timestep evaluation of equations.
				for(equation_h Equation : Batch.Equations) 
				{	
					double ResultValue = CallEquation(Model, RunState, Equation);
#if MOBIUS_TEST_FOR_NAN
					NaNTest(Model, RunState, ResultValue, Equation);
#endif
					*RunState->AtResult = ResultValue;
					++RunState->AtResult;
					RunState->CurResults[Equation.Handle] = ResultValue;
				
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
						RunState->SolverTempX0[EquationIdx] = 0;
					}
					else
					{
						RunState->SolverTempX0[EquationIdx] = RunState->LastResults[Equation.Handle]; //NOTE: RunState.LastResult is set up above already.
					}
					++EquationIdx;
				}
				// NOTE: Do we need to clear DataSet->wk to 0? (Has not been needed in the solvers we have used so far...)
				
				const solver_spec &SolverSpec = Model->Solvers.Specs[Batch.Solver.Handle];
				
				//NOTE: This lambda is the "equation function" of the solver. It solves the set of equations once given the values in the working sets x0 and wk. It can be run by the SolverFunction many times.
				auto EquationFunction =
				[RunState, Model, &Batch](double *x0, double* wk)
				{	
					size_t EquationIdx = 0;
					//NOTE: Read in initial values of the ODE equations to the CurResults buffer to be accessible from within the batch equations using RESULT(H).
					//NOTE: Values are not written to ResultData before the entire solution process is finished. So during the solver process one can ONLY read intermediary results from equations belonging to this solver using RESULT(H), never RESULT(H, Idx1,...) etc. However there is no reason one would want to do that any way.
					for(equation_h Equation : Batch.EquationsODE)
					{
#if MOBIUS_TEST_FOR_NAN
						NaNTest(Model, RunState, x0[EquationIdx], Equation);
#endif
						RunState->CurResults[Equation.Handle] = x0[EquationIdx];
						++EquationIdx;
					}
					
					//NOTE: Solving basic equations tied to the solver. Values should NOT be written to the working set. They can instead be accessed from inside other equations in the solver batch using RESULT(H)
					for(equation_h Equation : Batch.Equations)
					{
						double ResultValue = CallEquation(Model, RunState, Equation);
#if MOBIUS_TEST_FOR_NAN
						NaNTest(Model, RunState, ResultValue, Equation);
#endif
						RunState->CurResults[Equation.Handle] = ResultValue;
					}
					
					//NOTE: Solving ODE equations tied to the solver. These values should be written to the working set.
					EquationIdx = 0;
					for(equation_h Equation : Batch.EquationsODE)
					{
						double ResultValue = CallEquation(Model, RunState, Equation);
						wk[EquationIdx] = ResultValue;
						
						++EquationIdx;
					}
				};
				
				mobius_solver_jacobi_function JacobiFunction = nullptr;
				if(SolverSpec.UsesJacobian)
				{
					JacobiFunction =
					[RunState, Model, DataSet, &Batch](double *X, mobius_matrix_insertion_function & MatrixInserter)
					{
						EstimateJacobian(X, MatrixInserter, Model, RunState, Batch);
					};
				}
				
				double h = DataSet->hSolver[Batch.Solver.Handle]; // The desired solver step. (Guideline only, solver is free to its step during error correction).
				
				//NOTE: Solve the system using the provided solver
				SolverSpec.SolverFunction(h, Batch.EquationsODE.Count, RunState->SolverTempX0, RunState->SolverTempWorkStorage, EquationFunction, JacobiFunction, SolverSpec.RelErr, SolverSpec.AbsErr);
				
				//NOTE: Store out the final results from this solver to the ResultData set.
				for(equation_h Equation : Batch.Equations)
				{
					double ResultValue = RunState->CurResults[Equation.Handle];
					*RunState->AtResult = ResultValue;
					++RunState->AtResult;
#if MOBIUS_TIMESTEP_VERBOSITY >= 3
					for(size_t Lev = 0; Lev < CurrentLevel; ++Lev) std::cout << "\t";
					std::cout << "\t" << GetName(Model, Equation) << " = " << ResultValue << std::endl;
#endif
				}
				EquationIdx = 0;
				for(equation_h Equation : Batch.EquationsODE)
				{
					double ResultValue = RunState->SolverTempX0[EquationIdx];
					RunState->CurResults[Equation.Handle] = ResultValue;
					*RunState->AtResult = ResultValue;
					++RunState->AtResult;
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
		for(entity_handle ParameterHandle : BatchGroup.IterationData[CurrentLevel].ParametersToRead)
		{
			//NOTE: Parameters are special here in that we can just store the value in the fast lookup, instead of the offset. This is because they don't change with the timestep.
			size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, ParameterHandle);
			parameter_value Value = DataSet->ParameterData[Offset];
			RunState->FastParameterLookup[RunState->FastParameterLookup.Count++] = Value;
		}
		
		for(input_h Input : BatchGroup.IterationData[CurrentLevel].InputsToRead)
		{
			size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, Input.Handle);
			RunState->FastInputLookup[RunState->FastInputLookup.Count++] = Offset;
		}
		
		for(equation_h Equation : BatchGroup.IterationData[CurrentLevel].ResultsToRead)
		{
			size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, Equation.Handle);
			RunState->FastResultLookup[RunState->FastResultLookup.Count++] = Offset;
		}
		
		for(equation_h Equation : BatchGroup.IterationData[CurrentLevel].LastResultsToRead)
		{
			size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, Equation.Handle);
			RunState->FastLastResultLookup[RunState->FastLastResultLookup.Count++] = Offset;
		}
	}
	else
	{
		for(equation_h Equation : BatchGroup.LastResultsToReadAtBase)
		{
			size_t Offset = OffsetForHandle(DataSet->ResultStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, Equation.Handle);
			RunState->FastLastResultLookup[RunState->FastLastResultLookup.Count++] = Offset;
		}
	}
}


inline void
SetupInitialValue(mobius_data_set *DataSet, model_run_state *RunState, equation_h Equation)
{
	
	const mobius_model *Model = DataSet->Model;
	const equation_spec &Spec = Model->Equations.Specs[Equation.Handle];
	
	double Initial = 0.0;
	if(IsValid(Spec.InitialValue))
	{
		size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, Spec.InitialValue.Handle);
		//NOTE: We should not get a type mismatch here since we only allow for registering initial values using handles of type parameter_double_h. Thus we do not need to test for which type it is.
		Initial = DataSet->ParameterData[Offset].ValDouble;
	}
	else if(Spec.HasExplicitInitialValue)
	{
		Initial = Spec.ExplicitInitialValue;
	}
	else if(IsValid(Spec.InitialValueEquation))
	{
		Initial = Model->EquationBodies[Spec.InitialValueEquation.Handle](RunState);
	}
	else
	{
		//NOTE: Equations without any type of initial value act as their own initial value equation
		Initial = Model->EquationBodies[Equation.Handle](RunState);
	}
	
	//std::cout << "Initial value of " << GetName(Model, Equation) << " is " << Initial << std::endl;
 	
	size_t ResultStorageLocation = DataSet->ResultStorageStructure.LocationOfHandleInUnit[Equation.Handle];
	
	RunState->AtResult[ResultStorageLocation] = Initial;
	RunState->CurResults[Equation.Handle] = Initial;
	RunState->LastResults[Equation.Handle] = Initial;
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
			RunState->CurParameters[ParameterHandle] = *RunState->AtParameterLookup;
			++RunState->AtParameterLookup;
		}
	}
	
	s32 BottomLevel = BatchGroup.IndexSets.Count - 1;
	if(CurrentLevel == BottomLevel)
	{
		for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
		{
			const equation_batch &Batch = Model->EquationBatches[BatchIdx];
			for(equation_h Equation : Batch.InitialValueOrder)
			{
				SetupInitialValue(DataSet, RunState, Equation);
			}
		}
		
		RunState->AtResult += DataSet->ResultStorageStructure.Units[BatchGroupIdx].Handles.Count; //NOTE: This works because we set the storage structure up to mirror the batch group structure.
	}
}

static void
ProcessComputedParameters(mobius_data_set *DataSet, model_run_state *RunState)
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
				[DataSet, ParameterHandle, Equation, &EqSpec, &Spec, RunState](index_t *Indexes, size_t IndexesCount)
				{
					//NOTE: We have to set the RunState into the right state.
					
					for(size_t IdxIdx = 0; IdxIdx < IndexesCount; ++IdxIdx)
					{
						index_t Index = Indexes[IdxIdx];
						RunState->CurrentIndexes[Index.IndexSetHandle] = Index;
					}
					
					for(entity_handle DependentParameter : EqSpec.ParameterDependencies)
					{
						size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, RunState->CurrentIndexes, DataSet->IndexCounts, DependentParameter);
						RunState->CurParameters[DependentParameter] = DataSet->ParameterData[Offset];
					}
					
					size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, Indexes, IndexesCount, DataSet->IndexCounts, ParameterHandle);
					double ValD = DataSet->Model->EquationBodies[Equation.Handle](RunState);
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
PrintEquationProfiles(mobius_data_set *DataSet, model_run_state *RunState);

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
	
	
	u64 Timesteps           = GetTimesteps(DataSet);
	datetime ModelStartTime = GetStartDate(DataSet); //NOTE: This reads the "Start date" parameter.
	
	//TODO: Should we put a restriction on ModelStartTime so that it is "round" with respect to the Model->TimestepSize ?
	
#if MOBIUS_PRINT_TIMING_INFO
	std::cout << "Running model " << Model->Name;
	if(Model->Modules.Count() > 1)
	{
		std::cout << " with modules (";
		for(entity_handle ModuleHandle = 1; ModuleHandle < Model->Modules.Count(); ++ModuleHandle)
		{
			const module_spec &Module = Model->Modules.Specs[ModuleHandle];
			std::cout << Module.Name << " V" << Module.Version;
			if(ModuleHandle != Model->Modules.Count()-1) std::cout << ", ";
		}
		std::cout << ")";
	}
	std::cout << " for " << Timesteps << " timesteps starting at " << ModelStartTime.ToString() << std::endl;
#endif
	
	//NOTE: Allocate input storage in case it was not allocated during setup.
	if(!DataSet->InputData)
	{
		AllocateInputStorage(DataSet, Timesteps);
		std::cout << "WARNING: No input values were specified, using input values of 0 only." << std::endl;
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
		{
			MOBIUS_FATAL_ERROR("ERROR: For models with timestep resolution measured in months or years, input data start dates have to be on the form yyyy-mm-01 or yyyy-mm-01 00:00:00 . In the current setup, the input start date was set to " << InputStartDate.ToString() << " ." << std::endl);
		}
		
		if(MDay != 1 || ModelStartTime.SecondsSinceEpoch % 86400 != 0)
		{
			MOBIUS_FATAL_ERROR("ERROR: For models with timestep resolution measured in months or years, the \"Start date\" has to be on the form yyyy-mm-01 or yyyy-mm-01 00:00:00 . In the current setup, the start date was set to " << ModelStartTime.ToString() << " ." << std::endl);
		}
		
		TimeOffset = (MYear-IYear)*12 + MMonth-IMonth;
	}
	
	if(TimeOffset % Model->TimestepSize.Magnitude != 0)
	{
		MOBIUS_FATAL_ERROR("ERROR: The model run \"Start date\" was not set to be a whole number of timesteps after the input start date." << std::endl);
	}
	
	
	if(ModelStartTime.SecondsSinceEpoch < InputStartDate.SecondsSinceEpoch)
	{
		MOBIUS_FATAL_ERROR("ERROR: The input data starts at a later date than the model run." << std::endl);
	}
	
	if(((s64)DataSet->InputDataTimesteps - InputDataStartOffsetTimesteps) < (s64)Timesteps)
	{
		MOBIUS_FATAL_ERROR("ERROR: The input data provided has fewer timesteps (after the model run start date) than the number of timesteps the model is running for." << std::endl);
	}
	
	//std::cout << "Input data start offset timesteps was " << InputDataStartOffsetTimesteps << std::endl;
	
	if(DataSet->ResultData)
	{
		//NOTE: This is in case somebody wants to re-run the same dataset after e.g. changing a few parameters.
		free(DataSet->ResultData);
		DataSet->ResultData = nullptr;
	}
	
	AllocateResultStorage(DataSet, Timesteps);
	
	model_run_state RunState(DataSet);
	
	ProcessComputedParameters(DataSet, &RunState);
	
	RunState.Clear();
	
	
	for(const mobius_preprocessing_step &PreprocessingStep : Model->PreprocessingSteps)
	{
		PreprocessingStep(DataSet);
	}
	
	
	// If some solvers have parametrized step size, read in the actual value of the parameter from the parameter data, then store it for use when running the model.
	if(!DataSet->hSolver)
	{
		DataSet->hSolver = DataSet->BucketMemory.Allocate<double>(Model->Solvers.Count());
	}
	
	for(entity_handle SolverHandle = 1; SolverHandle < Model->Solvers.Count(); ++SolverHandle)
	{
		const solver_spec &Spec = Model->Solvers.Specs[SolverHandle];
		
		double hValue;
		
		if(IsValid(Spec.hParam))
		{
			if(Model->Parameters.Specs[Spec.hParam.Handle].IndexSetDependencies.size() > 0)
			{
				std::cout << "WARNING: The parameter " << GetName(Model, Spec.hParam) << " is used to control the step size of the solver " << Spec.Name << ". The parameter has one or more index set dependencies, but only the first value will be used." << std::endl;
			}
			size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, Spec.hParam.Handle);
			hValue = DataSet->ParameterData[Offset].ValDouble;
		}
		else
		{
			hValue = Spec.h;
		}
		
		if(hValue <= 0.0 || hValue > 1.0)
		{
			MOBIUS_FATAL_ERROR("The solver " << Spec.Name << " was given a step size that is smaller than 0 or larger than 1.");
		}
		
		DataSet->hSolver[SolverHandle] = hValue;
	}
	
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
	for(const equation_batch_group& BatchGroup : Model->BatchGroups)
	{
		for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
		{
			const equation_batch &Batch = Model->EquationBatches[BatchIdx];
			if(Batch.Type == BatchType_Solver)
			{
				MaxODECount = Max(MaxODECount, Batch.EquationsODE.Count);
			}
		}
	}
	size_t SolverTempWorkSpace = 4*MaxODECount; //TODO: 4*MaxODECount is specifically for IncaDascru. Other solvers may have other needs for storage, so this 4 should not be hard coded. Note however that the Boost solvers use their own storage, so this is not an issue in that case.
	size_t JacobiTempWorkSpace = MaxODECount + Model->Equations.Count();//MaxNonODECount;
	
	RunState.SolverTempX0          = RunState.BucketMemory.Allocate<double>(MaxODECount);
	RunState.SolverTempWorkStorage = RunState.BucketMemory.Allocate<double>(SolverTempWorkSpace);
	RunState.JacobianTempStorage   = RunState.BucketMemory.Allocate<double>(JacobiTempWorkSpace);
	
	

	//NOTE: System parameters (i.e. parameters that don't depend on index sets) are going to be the same during the entire run, so we just load them into CurParameters once and for all.
	//NOTE: If any system parameters exist, the storage units are sorted such that the system parameters have to belong to storage unit [0].
	if(DataSet->ParameterStorageStructure.Units.Count != 0 && DataSet->ParameterStorageStructure.Units[0].IndexSets.Count == 0)
	{
		for(entity_handle ParameterHandle : DataSet->ParameterStorageStructure.Units[0].Handles)
		{
			size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, ParameterHandle);
			RunState.CurParameters[ParameterHandle] = DataSet->ParameterData[Offset];
		}
	}
	
	//NOTE: Similarly we have to set which of the unindexed inputs were provided.
	if(DataSet->InputStorageStructure.Units.Count != 0 && DataSet->InputStorageStructure.Units[0].IndexSets.Count == 0)
	{
		for(entity_handle InputHandle : DataSet->InputStorageStructure.Units[0].Handles)
		{
			size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, InputHandle);
			RunState.CurInputWasProvided[InputHandle] = DataSet->InputTimeseriesWasProvided[Offset];
		}
	}
	
	RunState.AllLastResultsBase = DataSet->ResultData;
	RunState.AllCurResultsBase  = DataSet->ResultData;
	
	//TODO: We may want to set this one timestep back for it to also be correct during the initial value run.
	RunState.CurrentTime = expanded_datetime(ModelStartTime, Model->TimestepSize);
	
	//NOTE: Set up initial values;
	RunState.AtResult          = RunState.AllCurResultsBase;
	RunState.AtLastResult      = RunState.AllLastResultsBase;
	RunState.AtParameterLookup = RunState.FastParameterLookup.Data;
	RunState.Timestep = -1;
	ModelLoop(DataSet, &RunState, InitialValueSetupInnerLoop);
	
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
		std::cout << "Timestep: " << RunState.Timestep << std::endl;
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
			for(entity_handle InputHandle : DataSet->InputStorageStructure.Units[0].Handles)
			{
				size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, InputHandle);
				RunState.CurInputs[InputHandle] = RunState.AllCurInputsBase[Offset];
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

	std::cout << "Model execution setup time: " << SetupDuration << " milliseconds" << std::endl;
	std::cout << "Model execution time: " << RunDuration << " milliseconds" << std::endl;
	std::cout << "Model execution processor cycles: " << RunDurationCycles << std::endl;
	std::cout << "Average cycles per result instance including overhead: " << (RunDurationCycles / (Timesteps * DataSet->ResultStorageStructure.TotalCount)) << std::endl;
	std::cout << "(Note: one instance can be the result of several equation evaluations in the case of solvers)" << std::endl;
#endif

#if MOBIUS_EQUATION_PROFILING
	PrintEquationProfiles(DataSet, &RunState);
#endif
	
	DataSet->HasBeenRun = true;
	DataSet->TimestepsLastRun = Timesteps;
	DataSet->StartDateLastRun = ModelStartTime;
}

static void
PrintEquationDependencies(mobius_model *Model)
{
	if(!Model->Finalized)
	{
		std::cout << "WARNING: Tried to print equation dependencies before the model was finalized" << std::endl;
		return;
	}
	
	//Ooops, this one may not be correct for equations that are on solvers!!
	
	std::cout << std::endl << "**** Equation Dependencies ****" << std::endl;
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

static void
PrintResultStructure(const mobius_model *Model, std::ostream &Out = std::cout)
{
	if(!Model->Finalized)
	{
		std::cout << "WARNING: Tried to print result structure before the model was finalized" << std::endl;
		return;
	}
	
	Out << std::endl << "**** Result Structure ****" << std::endl;
	//Out << "Number of batches: " << Model->ResultStructure.size() << std::endl;
	for(const equation_batch_group &BatchGroup : Model->BatchGroups)
	{
		if(BatchGroup.IndexSets.Count == 0) Out << "[]";
		for(index_set_h IndexSet : BatchGroup.IndexSets)
		{
			Out << "[" << GetName(Model, IndexSet) << "]";
		}
		
		for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
		{
			const equation_batch &Batch = Model->EquationBatches[BatchIdx];
			Out << "\n\t-----";
			if(Batch.Type == BatchType_Solver) Out << " (SOLVER: " << GetName(Model, Batch.Solver) << ")";
			
			ForAllBatchEquations(Batch,
			[Model, &Out](equation_h Equation)
			{
				Out << "\n\t";
				if(Model->Equations.Specs[Equation.Handle].Type == EquationType_Cumulative) Out << "(Cumulative) ";
				else if(Model->Equations.Specs[Equation.Handle].Type == EquationType_ODE) Out << "(ODE) ";
				Out << GetName(Model, Equation);
				return false;
			});
			if(BatchIdx == BatchGroup.LastBatch) Out << "\n\t-----\n";
		}
		
		Out << std::endl;
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
	size_t StorageCount = DataSet->ParameterStorageStructure.Units.Count;
	for(size_t StorageIdx = 0; StorageIdx < StorageCount; ++StorageIdx)
	{
		array<index_set_h> &IndexSets = DataSet->ParameterStorageStructure.Units[StorageIdx].IndexSets;
		if(IndexSets.Count == 0)
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
	size_t StorageCount = DataSet->InputStorageStructure.Units.Count;
	for(size_t StorageIdx = 0; StorageIdx < StorageCount; ++StorageIdx)
	{
		array<index_set_h> &IndexSets = DataSet->InputStorageStructure.Units[StorageIdx].IndexSets;
		if(IndexSets.Count == 0)
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
		{
			std::cout << "[" << GetName(Model, IndexSet) << "]";
		}
		
		for(size_t BatchIdx = BatchGroup.FirstBatch; BatchIdx <= BatchGroup.LastBatch; ++BatchIdx)
		{
			const equation_batch &Batch = Model->EquationBatches[BatchIdx];
			std::cout << "\n\t-----";
			if(Batch.Type == BatchType_Solver) std::cout << " (SOLVER: " << GetName(Model, Batch.Solver) << ")";
			
			ForAllBatchEquations(Batch,
			[Model, RunState, &TotalHits, &SumCc](equation_h Equation)
			{
				int PrintCount = 0;
				printf("\n\t");
				if(Model->Equations.Specs[Equation.Handle].Type == EquationType_Cumulative) PrintCount += printf("(Cumulative) ");
				else if(Model->Equations.Specs[Equation.Handle].Type == EquationType_ODE) PrintCount += printf("(ODE) ");
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
