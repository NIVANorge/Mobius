

#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 1

#include "../mobius.h"


#define SIMPLYQ_GROUNDWATER    //NOTE: #defining this before the inclusion of the SimplyQ.h file turns on groundwater in SimplyQ.

#include "SimplyQ_eval_copy.h"




void SnowStep(int Timestep, const std::vector<std::vector<double>> &Inputs, std::vector<std::vector<double>> &SnowResults, const std::vector<double> &Parameters)
{
	//Parameters
	double DDFmelt = Parameters[0];
	double PQuick  = Parameters[1];
	
	//Inputs
	double Precip  = Inputs[0][Timestep];
	double AirT    = Inputs[1][Timestep];
	
	//Results
	std::vector<double> &PSnow = SnowResults[0];
	std::vector<double> &PRain = SnowResults[1];
	std::vector<double> &PotMelt = SnowResults[2];
	std::vector<double> &Melt = SnowResults[3];
	std::vector<double> &SnowDepth = SnowResults[4];
	std::vector<double> &HydrolInputSoil = SnowResults[5];
	std::vector<double> &Inf = SnowResults[6];
	std::vector<double> &InfEx = SnowResults[7];
	
	PSnow[Timestep] = (AirT < 0.0) ? Precip : 0.0;
	PRain[Timestep] = (AirT > 0.0) ? Precip : 0.0;
	PotMelt[Timestep] = Max(0.0, DDFmelt*AirT);
	Melt[Timestep] = Min(SnowDepth[Timestep-1], PotMelt[Timestep]);
	SnowDepth[Timestep] = SnowDepth[Timestep-1] + PSnow[Timestep] - Melt[Timestep];
	HydrolInputSoil[Timestep] = Melt[Timestep] + PRain[Timestep];
	Inf[Timestep] = (1.0-PQuick)*HydrolInputSoil[Timestep];
	InfEx[Timestep] = PQuick*HydrolInputSoil[Timestep];
}

// #define MOBIUS_SOLVER_FUNCTION(Name) void Name(double h, size_t n, double* x0, double* wk, const mobius_solver_equation_function &EquationFunction, const mobius_solver_equation_function &JacobiFunction, double AbsErr, double RelErr)

void LandStep(int Timestep, const std::vector<double> &PET, std::vector<std::vector<double>> &LandResults, const std::vector<double> &Parameters, double TC, double Inf, double *x0, double *wk)
{
	x0[0] = LandResults[0][Timestep-1];
	x0[1] = 0.0;
	
	double Pet = PET[Timestep];
	
	double MPET = Parameters[2];
	double FC   = Parameters[3];
	
	const auto &EquationFunction = [TC, Inf, Pet, MPET, FC](double *x0, double *wk)
	{
		double V0 = x0[0];
		double smd = FC - V0;
		double Qsw = -smd * ActivationControl(V0, FC, 0.01) / TC;
		
		wk[0] = Inf - MPET * Pet * (1.0 - exp(log(0.01)*V0 / FC)) - Qsw;
		wk[1] = Qsw;
	};
	
	IncaDascruImpl_(0.01, 2, x0, wk, EquationFunction, nullptr, 0.0, 0.0); //NOTE: this particular solver doesn't use the RelErr and AbsErr figures.
	
	LandResults[0][Timestep] = x0[0];
	LandResults[1][Timestep] = x0[1];
}


int main()
{
	//TODO: We have to do a much more rigorous timing comparison where we run each version many times.
	
	
	mobius_model *Model = BeginModelDefinition("SimplyQ", "0.1");
	
	AddSimplyHydrologyModule(Model);
	
	ReadInputDependenciesFromFile(Model, "tarlandinputs.dat");
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);

	ReadParametersFromFile(DataSet, "testparameters.dat");

	ReadInputsFromFile(DataSet, "tarlandinputs.dat");
	
	RunModel(DataSet);
	
	//TODO: These should be extracted from the DataSet instead!
	std::vector<const char *> LandscapeUnits = {"Arable", "Improved grassland", "Semi-natural"};
	
	//NOTE: Assuming only one reach! Otherwise the hard coding of the model would be more complex.
	
	
	// Hard coded model.
	
	u64 Timesteps = GetParameterUInt(DataSet, "Timesteps", {});
	
	
	std::vector<double> Parameters(4);
	
	Parameters[0] = GetParameterDouble(DataSet, "Degree-day factor for snowmelt", {});
	Parameters[1] = GetParameterDouble(DataSet, "Proportion of precipitation that contributes to quick flow", {});
	Parameters[2] = GetParameterDouble(DataSet, "PET multiplication factor", {});
	Parameters[3] = GetParameterDouble(DataSet, "Soil field capacity", {});
	
	std::vector<double> TC(LandscapeUnits.size());
	for(int LU = 0; LU < LandscapeUnits.size(); ++LU)
	{
		TC[LU] = GetParameterDouble(DataSet, "Soil water time constant", {LandscapeUnits[LU]});
	}
	
	
	
	std::vector<std::vector<double>> Inputs(3);
	for(auto &Ts : Inputs) Ts.resize(Timesteps + 1);
	
	GetInputSeries(DataSet, "Precipitation", {}, Inputs[0].data() + 1, Timesteps, true);
	GetInputSeries(DataSet, "Air temperature", {}, Inputs[1].data() + 1, Timesteps, true);
	GetInputSeries(DataSet, "Potential evapotranspiration", {}, Inputs[2].data() + 1, Timesteps, true);
	
	std::vector<std::vector<double>> SnowResults(8);
	for(auto &Ts : SnowResults) Ts.resize(Timesteps + 1);
	
	std::vector<std::vector<std::vector<double>>> LandResults(LandscapeUnits.size());
	for(auto &R : LandResults)
	{
		R.resize(2);
		for(auto &Ts : R)
		{
			Ts.resize(Timesteps + 1);
		}
	}
	
	
	
	
	SnowResults[4][0] = GetParameterDouble(DataSet, "Initial snow depth as water equivalent", {});
	
	for(int LU = 0; LU < LandscapeUnits.size(); ++LU)
	{
		LandResults[LU][0][0] = GetParameterDouble(DataSet, "Soil field capacity", {});
	}
	
	
	
	std::vector<double> x0(3);
	std::vector<double> wk(12);
	
	
	u64 Before = __rdtsc();
	for(int Timestep = 1; Timestep <= (int)Timesteps; ++Timestep)
	{
		SnowStep(Timestep, Inputs, SnowResults, Parameters);
		
		for(int LU = 0; LU < LandscapeUnits.size(); ++LU)
		{
			double Inf = SnowResults[6][Timestep];
			
			LandStep(Timestep, Inputs[2], LandResults[LU], Parameters, TC[LU], Inf, x0.data(), wk.data());
		}
	}
	u64 After = __rdtsc();
	
	std::cout << "Hard coded model processor cycles: " << (After - Before) << std::endl;
	
	
	
	
	
	//Correctness evaluation
	
	std::vector<double> MobiusSnowDepth(Timesteps);
	GetResultSeries(DataSet, "Snow depth", {}, MobiusSnowDepth.data(), Timesteps);
	
	std::vector<double> MobiusSoilWaterVolumeArable(Timesteps);
	GetResultSeries(DataSet, "Soil water volume", {LandscapeUnits[0]}, MobiusSoilWaterVolumeArable.data(), Timesteps);
	
	std::vector<double> &HCSnowDepth = SnowResults[4];
	
	std::vector<double> &HCSWV = LandResults[0][0];
	
	for(int Timestep = 0; Timestep < (int)Timesteps; ++Timestep)
	{
		if(std::abs(MobiusSnowDepth[Timestep] - HCSnowDepth[Timestep+1]) > 1e-20)
		{
			std::cout << "Something went wrong at timestep " << Timestep << ". Snow depth was: " << MobiusSnowDepth[Timestep] << " " << HCSnowDepth[Timestep+1] << std::endl;
			break;
		}
		
		
		if(std::abs(MobiusSoilWaterVolumeArable[Timestep] - HCSWV[Timestep+1]) > 1e-20)
		{
			std::cout << "Something went wrong at timestep " << Timestep << ". Soil water volume was: " << MobiusSoilWaterVolumeArable[Timestep] << " " << HCSWV[Timestep+1] << std::endl;
			break;
		}
		
		//std::cout << "SWV: " << MobiusSoilWaterVolumeArable[Timestep] << " " << HCSWV[Timestep+1] << std::endl;
	}
	
}