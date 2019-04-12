
#define MOBIUS_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 1

#include "../../mobius.h"

#include "../../boost_solvers.h"

int main()
{
	mobius_model *Model = BeginModelDefinition("Test model", "0.0");
	
	auto Days 	        = RegisterUnit(Model, "days");
	auto DynamicOptions = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, DynamicOptions, "Timesteps", Days, 1);
	RegisterParameterDate(Model, DynamicOptions, "Start date", "1981-1-1");
	

	auto Unit = RegisterUnit(Model, "unit");
	
	auto Solver = RegisterSolver(Model, "Solver", 0.001, BoostRosenbrock4, 1e-6, 1e-6);
	
	auto Pop1 = RegisterEquationODE(Model, "pop1", Unit);
	SetSolver(Model, Pop1, Solver);
	SetInitialValue(Model, Pop1, 10.0);
	
	auto Pop2 = RegisterEquationODE(Model, "pop2", Unit);
	SetSolver(Model, Pop2, Solver);
	SetInitialValue(Model, Pop2, 10.0);
	
	auto Loss = RegisterEquation(Model, "loss", Unit);
	SetSolver(Model, Loss, Solver);
	
	EQUATION(Model, Loss,
		return 0.02 * RESULT(Pop1) * RESULT(Pop2);
	)
	
	
	EQUATION(Model, Pop1,
		return 0.1 * RESULT(Pop1) - RESULT(Loss);
	)
	
	EQUATION(Model, Pop2,
		return 0.01 * RESULT(Pop1) * RESULT(Pop2) - 0.3 * RESULT(Pop2);
	)
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);

	
	RunModel(DataSet);

	
}