
//NOTE: This functionality was factored out to its own file to not clutter up mobius_model.cpp too much, but its workings are intimately tied with those of mobius_model.cpp


static void
FindSolverBatchIndirectEquationDependencies(mobius_model *Model, equation_batch &Batch, equation_h Equation, std::set<size_t> &ODEDependenciesOut)
{
	equation_spec &Spec = Model->Equations[Equation];
	for(equation_h Dependency : Spec.DirectResultDependencies)
	{
		auto It = std::find(Batch.EquationsODE.begin(), Batch.EquationsODE.end(), Dependency);   // NOTE: See if the dependency is an ODE function in the same batch.
		if(It != Batch.EquationsODE.end())
		{
			size_t Pos = (size_t)std::distance(Batch.EquationsODE.begin(), It);   //NOTE: This is just the index of the Dependency in the EquationsODE vector (yes, C++ has no standard function for this :( 
			ODEDependenciesOut.insert(Pos);
			continue;
		}
		
		auto It2 = std::find(Batch.Equations.begin(), Batch.Equations.end(), Dependency);  // NOTE: See if the dependency is a non-ODE in the same batch
		if(It2 != Batch.Equations.end())
		{
			//NOTE: We have to traverse recursively so that we get all true dependencies of the original ODE!
			FindSolverBatchIndirectEquationDependencies(Model, Batch, Dependency, ODEDependenciesOut);
		}
	}
}

static void
BuildJacobianInfo(mobius_model *Model)
{
	for(equation_batch &Batch : Model->EquationBatches)
	{
		if(IsValid(Batch.Solver) && Model->Solvers[Batch.Solver].UsesJacobian)
		{	
			size_t N = Batch.EquationsODE.Count;
			
			Batch.ODEIsDependencyOfODE.Allocate(&Model->BucketMemory, N);
			Batch.ODEIsDependencyOfNonODE.Allocate(&Model->BucketMemory, N);
			
			std::vector<std::vector<size_t>> ODEIsDependencyOfODE(N);
			
			for(size_t Idx = 0; Idx < N; ++Idx)
			{
				equation_h Equation = Batch.EquationsODE[Idx];
				
				std::set<size_t> ODEDependencies;
				
				FindSolverBatchIndirectEquationDependencies(Model, Batch, Equation, ODEDependencies);
				
				for(size_t Dep : ODEDependencies)
				{
					ODEIsDependencyOfODE[Dep].push_back(Idx);  //NOTE: Signifies that the equation with index Dep is a dependency of the equation with index Idx.
				}
			}
			
			for(size_t Idx = 0; Idx < N; ++Idx)
			{
				Batch.ODEIsDependencyOfODE[Idx].CopyFrom(&Model->BucketMemory, ODEIsDependencyOfODE[Idx]);
			}
			
			std::vector<std::vector<equation_h>> ODEIsDependencyOfNonODE(N);
			
			for(equation_h Equation : Batch.Equations)
			{
				std::set<size_t> ODEDependencies;
				
				FindSolverBatchIndirectEquationDependencies(Model, Batch, Equation, ODEDependencies);
				
				for(size_t Dep : ODEDependencies)
				{
					ODEIsDependencyOfNonODE[Dep].push_back(Equation);
				}
			}
			
			for(size_t Idx = 0; Idx < N; ++Idx)
			{
				Batch.ODEIsDependencyOfNonODE[Idx].CopyFrom(&Model->BucketMemory, ODEIsDependencyOfNonODE[Idx]);
			}
		}
	}
}


#define USE_JACOBIAN_OPTIMIZATION 1          //Ooops! Don't turn this off unless know what you are doing. It could break some solvers.

static void
EstimateJacobian(double *X, const mobius_matrix_insertion_function &MatrixInserter, model_run_state *RunState, const equation_batch *Batch)
{
	// Using a callback to insert into the matrix is not optimal, but the problem is that we can't know the implementation every solver has of their linear algebra stuff.
	
	//NOTE: This is not a very numerically accurate estimation of the Jacobian, it is mostly optimized for speed. We'll see if it is good enough..

	const mobius_model *Model = RunState->DataSet->Model;

	size_t N = Batch->EquationsODE.Count;
	
	double *FBaseVec = RunState->JacobianTempStorage;
	double *Backup   = FBaseVec + N;
	
	for(size_t Idx = 0; Idx < N; ++Idx)
	{
		equation_h Equation = Batch->EquationsODE[Idx];
		RunState->CurResults[Equation.Handle] = X[Idx];
	}
	
	//NOTE: Evaluation of the ODE system at base point. TODO: We should find a way to reuse the calculation we already do at the basepoint, however it is done by a separate callback, so that is tricky..
	for(equation_h Equation : Batch->Equations)
	{
		double ResultValue = CallEquation(Model, RunState, Equation);
		RunState->CurResults[Equation.Handle] = ResultValue;
	}
	
	for(size_t Idx = 0; Idx < N; ++Idx)
	{
		equation_h EquationToCall = Batch->EquationsODE[Idx];
		FBaseVec[Idx] = CallEquation(Model, RunState, EquationToCall);
	}

	//printf("begin matrix\n");
	
	for(size_t Col = 0; Col < N; ++Col)
	{
		equation_h EquationToPermute = Batch->EquationsODE[Col];
		
		//Hmm. here we assume we can use the same H for all Fs which may not be a good idea?? But it makes things significantly faster because then we don't have to recompute the non-odes in all cases.
		double H0 = 1e-6;  //TODO: This should definitely be sensitive to the size of the base values. But how to do that?
		volatile double Temp = X[Col] + H0;   //NOTE: volatile to make sure the optimizer doesn't optimize this away. We do it to improve numerical accuracy.
		double H = Temp - X[Col];
		RunState->CurResults[EquationToPermute.Handle] = X[Col] + H;
		
#if USE_JACOBIAN_OPTIMIZATION		
		for(equation_h Equation : Batch->ODEIsDependencyOfNonODE[Col])
#else
		for(equation_h Equation : Batch->Equations)
#endif
		{
#if USE_JACOBIAN_OPTIMIZATION
			Backup[Equation.Handle] = RunState->CurResults[Equation.Handle];
#endif
			double ResultValue = CallEquation(Model, RunState, Equation);
			RunState->CurResults[Equation.Handle] = ResultValue;
		}
		
#if USE_JACOBIAN_OPTIMIZATION
		for(size_t Row : Batch->ODEIsDependencyOfODE[Col])
#else
		for(size_t Row = 0; Row < N; ++Row)
#endif
		{
			equation_h EquationToCall = Batch->EquationsODE[Row];
		
			double FBase = FBaseVec[Row]; //NOTE: The value of the EquationToCall at the base point.
			
			double FPermute = CallEquation(Model, RunState, EquationToCall);
			
			double Derivative = (FPermute - FBase) / H;
			
			MatrixInserter(Row, Col, Derivative);
			
			//printf("%.2f\t", Derivative);
		}
		//printf("\n");
		
		RunState->CurResults[EquationToPermute.Handle] = X[Col];  //NOTE: Reset the value so that it is correct for the next column.
		
#if USE_JACOBIAN_OPTIMIZATION
		for(equation_h Equation : Batch->ODEIsDependencyOfNonODE[Col])
		{
			RunState->CurResults[Equation.Handle] = Backup[Equation.Handle];
		}
#endif
	}

	//printf("end matrix\n");
}

#undef USE_JACOBIAN_OPTIMIZATION