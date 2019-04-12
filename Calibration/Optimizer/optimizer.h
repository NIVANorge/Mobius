
#if !defined(OPTIMIZER_H)

#include "../calibration.h"

#include <dlib/optimization.h>
#include <dlib/global_optimization.h>

#include <dlib/threads/thread_pool_extension.cpp>
#include <dlib/global_optimization/global_function_search.cpp>


typedef dlib::matrix<double,0,1> column_vector;

struct optimization_setup
{
	size_t MaxFunctionCalls;
	size_t DiscardTimesteps;
	std::vector<parameter_calibration> Calibration;
	std::vector<calibration_objective> Objectives;
};

static void
ReadOptimizationSetup(optimization_setup *Setup, const char *Filename)
{
	token_stream Stream(Filename);
	
	while(true)
	{
		token Token = Stream.PeekToken();
		if(Token.Type == TokenType_EOF)
			break;
		
		token_string Section = Stream.ExpectUnquotedString();
		Stream.ExpectToken(TokenType_Colon);
		if(Section.Equals("max_function_calls"))
		{
			Setup->MaxFunctionCalls = (size_t)Stream.ExpectUInt();
		}
		else if(Section.Equals("discard_timesteps"))
		{
			Setup->DiscardTimesteps = (size_t)Stream.ExpectUInt();
		}
		else if(Section.Equals("parameter_calibration"))
		{
			ReadParameterCalibration(Stream, Setup->Calibration);
		}
		else if(Section.Equals("objectives"))
		{
			ReadCalibrationObjectives(Stream, Setup->Objectives);
		}
	}
}

class optimization_model
{
	mobius_data_set *DataSet;
	optimization_setup *Setup;
	
public:
	optimization_model(mobius_data_set *DataSet, optimization_setup *Setup)
	{
		this->DataSet = DataSet;
		this->Setup = Setup;
		
		if(Setup->Objectives.size() != 1)
		{
			MOBIUS_FATAL_ERROR("ERROR: At the moment we only support having a single optimization objective." << std::endl);
		}
	}
	
	double operator()(const column_vector& Par)
	{
		//TODO: Allow multiple objectives
		calibration_objective &Objective = Setup->Objectives[0];
		
		double Performance = EvaluateObjective(DataSet, Setup->Calibration, Objective, Par.begin(), Setup->DiscardTimesteps);
		
		return ShouldMaximize(Objective.PerformanceMeasure) ? -Performance : Performance;
	}
};

static void
PrintOptimizationResult(optimization_setup *Setup, dlib::function_evaluation& Result)
{
	std::cout << "Optimal values: " << std::endl << std::endl;
	size_t ValIdx = 0;
	for(parameter_calibration &Cal : Setup->Calibration)
	{
		PrintParameterCalibration(Cal);
		for(size_t Dim = 0; Dim < GetDimensions(Cal); ++Dim)
		{
			std::cout << " " << Result.x(ValIdx++) << std::endl << std::endl;
		}
	}
}

static void
WriteOptimalParametersToDataSet(mobius_data_set *DataSet, optimization_setup *Setup, dlib::function_evaluation &Result)
{
	ApplyCalibrations(DataSet, Setup->Calibration, Result.x.begin());
}

static dlib::function_evaluation
RunOptimizer(mobius_data_set *DataSet, optimization_setup *Setup)
{
	optimization_model Optim(DataSet, Setup);
	
	size_t Dimensions = GetDimensions(Setup->Calibration);
	column_vector MinBound(Dimensions);
	column_vector MaxBound(Dimensions);
	size_t ValIdx = 0;
	for(parameter_calibration &Cal : Setup->Calibration)
	{
		for(size_t Dim = 0; Dim < GetDimensions(Cal); ++Dim)
		{
			MinBound(ValIdx) = Cal.Min;
			MaxBound(ValIdx) = Cal.Max;
			++ValIdx;
		}
	}
	
	std::cout << "Running optimization problem with " << Dimensions << " free variables. Max function calls: " << Setup->MaxFunctionCalls << "." << std::endl;
	
	auto Result = dlib::find_min_global(Optim, MinBound, MaxBound, dlib::max_function_calls(Setup->MaxFunctionCalls));
	
	return Result;
}

#define OPTIMIZER_H
#endif
