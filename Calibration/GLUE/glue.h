
#if !defined(INCA_GLUE_H)


#include "../calibration.h"

struct glue_setup
{
	size_t NumRuns;
	
	size_t NumThreads;
	
	size_t DiscardTimesteps;
	
	std::vector<parameter_calibration> Calibration;
	
	std::vector<calibration_objective> Objectives;
	
	std::vector<double> Quantiles;
};

struct glue_run_data
{
	std::vector<double> RandomParameters;                         //NOTE: Values for parameters that we want to vary only.
	std::vector<std::pair<double, double>> PerformanceMeasures;   //NOTE: One per objective.
};

struct glue_results
{
	std::vector<glue_run_data> RunData;
	std::vector<std::vector<double>> PostDistribution;
};


#include "../../sqlite3/sqlite3.h"

#include "glue_io.cpp"
#include "glue.cpp"




#define INCA_GLUE_H
#endif