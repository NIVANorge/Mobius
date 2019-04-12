

#define MCMC_USE_OPENMP

#include "mcmc.hpp"
#include "de.cpp"
#include "rwmh.cpp"
#include "hmc.cpp"
#include "mala.cpp"


#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>


#include <omp.h>


#include "../calibration.h"

enum mcmc_algorithm
{
	MCMCAlgorithm_DifferentialEvolution,
	MCMCAlgorithm_RandomWalkMetropolisHastings,
	MCMCAlgorithm_HamiltonianMonteCarlo,
	MCMCAlgorithm_MetropolisAdjustedLangevin,
};

struct mcmc_setup
{
	mcmc_algorithm Algorithm;
	
	size_t NumChains;
	size_t NumGenerations;   //Excluding burnin.
	size_t NumBurnin;
	size_t DiscardTimesteps; //Discard the N first timesteps.
	
	double DeBound;
	bool   DeJumps;
	double DeJumpGamma;
	
	double StepSize;
	size_t HMCLeapSteps;
	
	std::vector<parameter_calibration> Calibration;
	
	std::vector<calibration_objective> Objectives;
	
};

struct mcmc_run_data
{
	std::vector<mobius_data_set *> DataSets;
	
	std::vector<parameter_calibration> Calibration;
	
	calibration_objective Objective;
	
	size_t DiscardTimesteps;
};

struct mcmc_results
{
	double AcceptanceRate;
	arma::cube DrawsOut;
	arma::mat  DrawsOut2; //NOTE: For algs without multiple chains
};

static void
ReadMCMCSetupFromFile(mcmc_setup *Setup, const char *Filename)
{
	token_stream Stream(Filename);
	
	while(true)
	{
		token Token = Stream.PeekToken();
		
		if(Token.Type == TokenType_EOF)
		{
			break;
		}
		
		token_string Section = Stream.ExpectUnquotedString();
		Stream.ExpectToken(TokenType_Colon);

		if(Section.Equals("algorithm"))
		{
			token_string Alg = Stream.ExpectUnquotedString();
			if(Alg.Equals("differential_evolution"))
			{
				Setup->Algorithm = MCMCAlgorithm_DifferentialEvolution;
			}
			else if(Alg.Equals("metropolis_hastings"))
			{
				Setup->Algorithm = MCMCAlgorithm_RandomWalkMetropolisHastings;
			}
			else if(Alg.Equals("hamiltonian_monte_carlo"))
			{
				Setup->Algorithm = MCMCAlgorithm_HamiltonianMonteCarlo;
			}
			else if(Alg.Equals("metropolis_adjusted_langevin"))
			{
				Setup->Algorithm = MCMCAlgorithm_MetropolisAdjustedLangevin;
			}
			//else if ...
			else
			{
				Stream.PrintErrorHeader();
				MOBIUS_FATAL_ERROR("Unknown or unimplemented algorithm " << Alg << std::endl);
			}
		}
		else if(Section.Equals("chains"))
		{
			Setup->NumChains = (size_t)Stream.ExpectUInt();
		}
		else if(Section.Equals("generations"))
		{
			Setup->NumGenerations = (size_t)Stream.ExpectUInt();
		}
		else if(Section.Equals("burnin"))
		{
			Setup->NumBurnin = (size_t)Stream.ExpectUInt();
		}
		else if(Section.Equals("discard_timesteps"))
		{
			Setup->DiscardTimesteps = (size_t)Stream.ExpectUInt();
		}
		else if(Section.Equals("de_b"))
		{
			Setup->DeBound = Stream.ExpectDouble();
		}
		else if(Section.Equals("de_jumps"))
		{
			Setup->DeJumps = Stream.ExpectBool();
		}
		else if(Section.Equals("de_jump_gamma"))
		{
			Setup->DeJumpGamma = Stream.ExpectDouble();
		}
		else if(Section.Equals("step_size"))
		{
			Setup->StepSize = Stream.ExpectDouble();
		}
		else if(Section.Equals("hmc_leap_steps"))
		{
			Setup->HMCLeapSteps = Stream.ExpectUInt();
		}
		else if(Section.Equals("parameter_calibration"))
		{
			ReadParameterCalibration(Stream, Setup->Calibration, ParameterCalibrationReadInitialGuesses); //TODO: We should also read distributions here!
		}
		else if(Section.Equals("objectives"))
		{
			ReadCalibrationObjectives(Stream, Setup->Objectives, false);
		}
		else
		{
			Stream.PrintErrorHeader();
			MOBIUS_FATAL_ERROR("Unknown section name: " << Section << std::endl);
		}
	}
	
	//TODO: Check that the file contained enough data, i.e that most of the settings received sensible values?
}

double
TargetLogKernel(const arma::vec& Par, void* Data, size_t ChainIdx = 0)
{
	
	mcmc_run_data *RunData = (mcmc_run_data *)Data;
	
	mobius_data_set *DataSet = RunData->DataSets[ChainIdx];
	
	double LogLikelyhood = EvaluateObjective(DataSet, RunData->Calibration, RunData->Objective, Par.memptr(), RunData->DiscardTimesteps);
	
	//TODO: When we have bounds turned on, it looks like de algorithm adds a log_jacobian for the priors. Find out what that is for!
	double LogPriors = 0.0; //NOTE: This assumes uniformly distributed priors and that the MCMC driving algorithm discards draws outside the parameter min-max boundaries on its own.
	//TODO: implement other priors.
	
	return LogPriors + LogLikelyhood;
}

double
TargetLogKernelWithGradient(const arma::vec &Par, arma::vec* GradientOut, void *Data, size_t ChainIdx)
{
	mcmc_run_data *RunData = (mcmc_run_data *)Data;
	
	mobius_data_set *DataSet = RunData->DataSets[ChainIdx];
	
	//static int GradCalls = 0;
	//static int NonGradCalls = 0;

	double LogLikelyhood;
	if(GradientOut)
	{
		LogLikelyhood = EvaluateObjectiveAndGradientSingleForwardDifference(DataSet, RunData->Calibration, RunData->Objective, Par.memptr(), RunData->DiscardTimesteps, GradientOut->memptr());
		//GradCalls++;
	}
	else
	{
		LogLikelyhood = EvaluateObjective(DataSet, RunData->Calibration, RunData->Objective, Par.memptr(), RunData->DiscardTimesteps);
		//NonGradCalls++;
	}
	
	double LogPriors = 0.0;  //NOTE: See above!
	
	//std::cout << "Run number: " << NonGradCalls << " Gradient run number: " << GradCalls << std::endl;
	
	return LogPriors + LogLikelyhood;
}


static void RunMCMC(mobius_data_set *DataSet, mcmc_setup *Setup, mcmc_results *Results)
{
	mcmc_run_data RunData = {};
	
	RunData.Calibration = Setup->Calibration;
	
	size_t Dimensions = GetDimensions(RunData.Calibration);
	
	if(Dimensions == 0)
	{
		MOBIUS_FATAL_ERROR("ERROR: (MCMC) Need at least one parameter to calibrate." << std::endl);
	}
	
	arma::vec InitialGuess(Dimensions + 1);
	arma::vec LowerBounds(Dimensions + 1);
	arma::vec UpperBounds(Dimensions + 1);
	
	size_t ValIdx = 0;
	for(parameter_calibration &Cal : RunData.Calibration)
	{
		for(size_t Dim = 0; Dim < GetDimensions(Cal); ++Dim)
		{
			InitialGuess[ValIdx] = Cal.InitialGuess;   //TODO: This gets WRONG for partition setups.
			LowerBounds [ValIdx] = Cal.Min;
			UpperBounds [ValIdx] = Cal.Max;
			++ValIdx;
		}
	}
	
	//NOTE: The final parameter is the parameter for random perturbation.
	//TODO: Don't hard code these, they should be in the setup file!
	InitialGuess[Dimensions] = 0.5;
	LowerBounds [Dimensions] = 0.0;
	UpperBounds [Dimensions] = 1.0;
	
	for(size_t CalIdx = 0; CalIdx < Dimensions; ++CalIdx)
	{
		if(
				(LowerBounds[CalIdx] > UpperBounds[CalIdx])
			||  (LowerBounds[CalIdx] > InitialGuess[CalIdx])
			||  (InitialGuess[CalIdx] > UpperBounds[CalIdx])
		)
		{
			MOBIUS_FATAL_ERROR("The initial guess of calibration #" << CalIdx << " do not lie between the upper and lower bounds");
		}
	}
	
	mcmc::algo_settings_t Settings;
	
	Settings.vals_bound = true;				//Alternatively we could actually allow people to not have bounds here...
	Settings.lower_bounds = LowerBounds;
	Settings.upper_bounds = UpperBounds;
	
	if(Setup->Algorithm == MCMCAlgorithm_DifferentialEvolution)
	{
		Settings.de_n_pop = Setup->NumChains;
		Settings.de_n_gen = Setup->NumGenerations;
		Settings.de_n_burnin = Setup->NumBurnin;
		
		Settings.de_initial_lb = LowerBounds;
		Settings.de_initial_ub = UpperBounds;

		Settings.de_par_b = Setup->DeBound;
		
		Settings.de_jumps = Setup->DeJumps;
		Settings.de_par_gamma_jump = Setup->DeJumpGamma;
	}
	else if(Setup->Algorithm == MCMCAlgorithm_RandomWalkMetropolisHastings)
	{
		Settings.rwmh_n_draws = Setup->NumGenerations;
		Settings.rwmh_n_burnin = Setup->NumBurnin;
		Settings.rwmh_par_scale = 1.0;                 //TODO: See if we need to do anything with this.
		//arma::mat rwmh_cov_mat;
		
		//Ouch, rwmh does not support multiple chains, and so no parallelisation...
		if(Setup->NumChains > 1)
		{
			std::cout << "WARNING (MCMC): Random Walk Metropolis Hastings does not support having more than one chain." << std::endl;
		}
		Setup->NumChains = 1; // :(
	}
	else if (Setup->Algorithm == MCMCAlgorithm_HamiltonianMonteCarlo)
	{
		Settings.hmc_n_draws = Setup->NumGenerations;
		Settings.hmc_step_size = Setup->StepSize;
		Settings.hmc_leap_steps = Setup->HMCLeapSteps;
		Settings.hmc_n_burnin = Setup->NumBurnin;
		//hmc_precond_mat
		
		if(Setup->NumChains > 1)
		{
			std::cout << "WARNING (MCMC): Hamiltonian Monte Carlo does not support having more than one chain." << std::endl;
		}
		Setup->NumChains = 1;
	}
	else if(Setup->Algorithm == MCMCAlgorithm_MetropolisAdjustedLangevin)
	{
		Settings.mala_n_draws = Setup->NumGenerations;
		Settings.mala_step_size = Setup->StepSize;
		Settings.mala_n_burnin = Setup->NumBurnin;
		//mala_precond_mat
		
		if(Setup->NumChains > 1)
		{
			std::cout << "WARNING (MCMC): Metropolis-Adjusted Langevin Algorithm does not support having more than one chain." << std::endl;
		}
		Setup->NumChains = 1;
	}
	
	//NOTE: Make one copy of the dataset for each chain (so that they don't overwrite each other).
	RunData.DataSets.resize(Setup->NumChains);
	RunData.DataSets[0] = DataSet;
	for(size_t ChainIdx = 1; ChainIdx < Setup->NumChains; ++ChainIdx)
	{
		RunData.DataSets[ChainIdx] = CopyDataSet(DataSet);
	}
	
	if(Setup->Objectives.size() != 1)
	{
		MOBIUS_FATAL_ERROR("ERROR: (MCMC) We currently support having only one objective." << std::endl);
	}
	
	RunData.Objective = Setup->Objectives[0];
	
	if(!IsLogLikelyhoodMeasure(RunData.Objective.PerformanceMeasure))
	{
		MOBIUS_FATAL_ERROR("ERROR: (MCMC) A performance measure was selected that was not a log likelyhood measure." << std::endl);
	}
	
	RunData.DiscardTimesteps = Setup->DiscardTimesteps;
	
	u64 Timesteps = GetTimesteps(DataSet);
	if(RunData.DiscardTimesteps >= Timesteps)
	{
		MOBIUS_FATAL_ERROR("ERROR: (MCMC) We are told to discard the first " << RunData.DiscardTimesteps << " timesteps when evaluating the objective, but we only run the model for " << Timesteps << " timesteps." << std::endl);
	}
	
	if(Setup->NumChains > 1)
	{
		omp_set_num_threads(Setup->NumChains);
	}
	
	if(Setup->Algorithm == MCMCAlgorithm_DifferentialEvolution)
	{
		mcmc::de(InitialGuess, Results->DrawsOut, TargetLogKernel, &RunData, Settings); //NOTE: we had to make a modification to the library so that it passes the Chain index to the TargetLogKernel.
	
		Results->AcceptanceRate = Settings.de_accept_rate;
	}
	else if(Setup->Algorithm == MCMCAlgorithm_RandomWalkMetropolisHastings)
	{
		mcmc::rwmh(InitialGuess, Results->DrawsOut2, 
		[](const arma::vec& CalibrationIn, void* Data){return TargetLogKernel(CalibrationIn, Data, 0);}, 
		&RunData, Settings);
		
		Results->AcceptanceRate = Settings.rwmh_accept_rate;
	}
	else if(Setup->Algorithm == MCMCAlgorithm_HamiltonianMonteCarlo)
	{
		mcmc::hmc(InitialGuess, Results->DrawsOut2,
		[](const arma::vec& CalibrationIn, arma::vec* GradOut, void* Data){return TargetLogKernelWithGradient(CalibrationIn, GradOut, Data, 0);},
		&RunData, Settings);
		
		Results->AcceptanceRate = Settings.hmc_accept_rate;
	}
	else if(Setup->Algorithm == MCMCAlgorithm_MetropolisAdjustedLangevin)
	{
		mcmc::mala(InitialGuess, Results->DrawsOut2,
		[](const arma::vec& CalibrationIn, arma::vec* GradOut, void* Data){return TargetLogKernelWithGradient(CalibrationIn, GradOut, Data, 0);},
		&RunData, Settings);
		
		Results->AcceptanceRate = Settings.mala_accept_rate;
	}
	
	
	//NOTE: We delete every DataSet that we allocated above. We don't delete the one that was passed in since the caller may want to keep it.
	for(size_t ChainIdx = 1; ChainIdx < Setup->NumChains; ++ChainIdx)
	{
		delete RunData.DataSets[ChainIdx];
	}
}