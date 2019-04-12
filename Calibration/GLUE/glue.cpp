

#include <random>
#include <omp.h>

#include <boost/accumulators/statistics/weighted_extended_p_square.hpp>


static double
DrawRandomParameter(parameter_calibration &ParSetting, std::mt19937_64 &Generator)
{
	//TODO: Other parameter types later
	double NewValue;
	//TODO: Allow other types of distributions
	if(ParSetting.Distribution == ParameterDistribution_Uniform)
	{
		std::uniform_real_distribution<double> Distribution(ParSetting.Min, ParSetting.Max);
		NewValue = Distribution(Generator);
	}
	// else if..
	
	return NewValue;
}

typedef boost::accumulators::accumulator_set<double, boost::accumulators::stats<boost::accumulators::tag::weighted_extended_p_square>, double> quantile_accumulator;


#if !defined(GLUE_MULTITHREAD)
#define GLUE_MULTITHREAD 1
#endif

static std::pair<double, double>
ComputeWeightedPerformance(mobius_data_set *DataSet, double Performance, calibration_objective &Objective, std::vector<quantile_accumulator>& QuantileAccumulators, size_t DiscardTimesteps)
{
	using namespace boost::accumulators;
	
	double WeightedPerformance;
	if(ShouldMaximize(Objective.PerformanceMeasure))
	{
		WeightedPerformance = (Performance - Objective.Threshold) / (Objective.OptimalValue - Objective.Threshold);
	}
	else
	{
		WeightedPerformance = (Objective.Threshold - Performance) / (Objective.Threshold - Objective.OptimalValue);
	}
	
	double StatWeight = 1.0 / (1.0 - WeightedPerformance);
	//double StatWeight = exp(WeightedPerformance);
	
	size_t Timesteps = (size_t)DataSet->TimestepsLastRun;
	
	//TODO: The other EvaluateObjective we call before this already extracts this series, so it is kind of stupid to do it twice. Maybe we could make an optional version of it that gives us back the modeled series?
	std::vector<double> ModeledSeries(Timesteps);
	GetResultSeries(DataSet, Objective.ModeledName, Objective.ModeledIndexes, ModeledSeries.data(), ModeledSeries.size());
	
	//TODO: It is probably not optimal to lock the entire for loop..
#if GLUE_MULTITHREAD
	#pragma omp critical
	{
#endif
	//TODO! TODO! We should maybe also discard timesteps here too (but has to take care to do it correctly!)
	for(u64 Timestep = 0; Timestep < Timesteps; ++Timestep)
	{
		QuantileAccumulators[Timestep](ModeledSeries[Timestep], weight = StatWeight);
	}
#if GLUE_MULTITHREAD
	}
#endif
	return {Performance, WeightedPerformance};
}

static void
RunGLUE(mobius_data_set *DataSet, glue_setup *Setup, glue_results *Results)
{
	const mobius_model *Model = DataSet->Model;
	
	std::mt19937_64 Generator(42);
	
	//TODO: If we want to parallelize we have to copy the dataset for each parallell unit
	
	if(Setup->Objectives.size() != 1)
	{
		MOBIUS_FATAL_ERROR("ERROR: (GLUE) Sorry, we only support having a single objective at the moment." << std::endl);
	}
	
	if(Setup->Quantiles.empty())
	{
		MOBIUS_FATAL_ERROR("ERROR: (GLUE) Requires at least 1 quantile" << std::endl);
	}
	
	size_t Dim = GetDimensions(Setup->Calibration);
	Results->RunData.resize(Setup->NumRuns);
	for(size_t Run = 0; Run < Setup->NumRuns; ++Run)
	{
		Results->RunData[Run].RandomParameters.resize(Dim);
		Results->RunData[Run].PerformanceMeasures.resize(1);
	}
	
	
	//TODO: We have to make this compatible with partitions!
	
	//NOTE: It is important that the loops are in this order so that we don't get any weird dependence between the parameter values (I think).
	//TODO: We should probably have a better generation scheme for parameter values (such as Latin Cubes?).
	size_t ParIdx = 0;
	for(parameter_calibration &Cal : Setup->Calibration)
	{
		for(size_t Idx = 0; Idx < GetDimensions(Cal); ++Idx)
		{
			for(size_t Run = 0; Run < Setup->NumRuns; ++Run)
			{	
				Results->RunData[Run].RandomParameters[ParIdx] = DrawRandomParameter(Cal, Generator);
			}
			++ParIdx;
		}
	}
	
	u64 NumTimesteps = GetTimesteps(DataSet);
	
	std::vector<quantile_accumulator> QuantileAccumulators;
	QuantileAccumulators.reserve(NumTimesteps);
	
	for(size_t Timestep = 0; Timestep < NumTimesteps; ++Timestep)
	{
		QuantileAccumulators.push_back(quantile_accumulator(boost::accumulators::tag::weighted_extended_p_square::probabilities = Setup->Quantiles));
	}
	
#if GLUE_MULTITHREAD

	omp_set_num_threads(Setup->NumThreads);

	#pragma omp parallel for
	for(size_t RunID = 0; RunID < Setup->NumRuns; ++RunID)
	{
		mobius_data_set *DataSet0 = CopyDataSet(DataSet); //NOTE: We have to work with a copy, otherwise the various threads will overwrite each other.
		
		calibration_objective &Objective = Setup->Objectives[0];
			
		const double *ParValues = Results->RunData[RunID].RandomParameters.data();
		
		double Performance = EvaluateObjective(DataSet0, Setup->Calibration, Objective, ParValues, Setup->DiscardTimesteps);

		auto Perf = ComputeWeightedPerformance(DataSet0, Performance, Objective, QuantileAccumulators, Setup->DiscardTimesteps);

		Results->RunData[RunID].PerformanceMeasures[0] = Perf;
		
		delete DataSet0;
	}
	
#else
	
	for(size_t RunID = 0; RunID < Setup->NumRuns; ++RunID)
	{
#if CALIBRATION_PRINT_DEBUG_INFO
		std::cout << "Run number: " << RunID << std::endl;
#endif
		calibration_objective &Objective = Setup->Objectives[0];

		const double *ParValues = Results->RunData[RunID].RandomParameters.data();
			
		double Performance = EvaluateObjective(DataSet, Setup->Calibration, Objective, ParValues, Setup->DiscardTimesteps);

		auto Perf = ComputeWeightedPerformance(DataSet, Performance, Objective, QuantileAccumulators, Setup->DiscardTimesteps);

		Results->RunData[RunID].PerformanceMeasures[0] = Perf;

#if CALIBRATION_PRINT_DEBUG_INFO		
		std::cout << "Performance and weighted performance for " << Objective.ModeledName << " vs " << Objective.ObservedName << " was " << std::endl << Perf.first << ", " << Perf.second << std::endl << std::endl;
#endif
	}
#endif
	
	Results->PostDistribution.resize(Setup->Quantiles.size());
	
	for(size_t QuantileIdx = 0; QuantileIdx < Setup->Quantiles.size(); ++QuantileIdx)
	{
		Results->PostDistribution[QuantileIdx].resize(NumTimesteps);
		
		for(size_t Timestep = 0; Timestep < NumTimesteps; ++Timestep)
		{
			using namespace boost::accumulators;
			Results->PostDistribution[QuantileIdx][Timestep] = weighted_extended_p_square(QuantileAccumulators[Timestep])[QuantileIdx];
		}
	}
}