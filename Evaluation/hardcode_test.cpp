

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
	x0[0] = LandResults[0][Timestep-1]; // Soil water volume
	x0[1] = 0.0;                        // Mean soil water flow
	
	double Pet = PET[Timestep];
	
	double MPET = Parameters[2];
	double FC   = Parameters[3];
	
	const auto &EquationFunction = [TC, Inf, Pet, MPET, FC](double *x0, double *wk)
	{
		double V = x0[0];
		double smd = FC - V;
		double Qsw = -smd * ActivationControl(V, FC, 0.01) / TC;
		
		wk[0] = Inf - MPET * Pet * (1.0 - exp(log(0.01)*V / FC)) - Qsw;
		wk[1] = Qsw;
	};
	
	IncaDascruImpl_(0.01, 2, x0, wk, EquationFunction, nullptr, 0.0, 0.0); //NOTE: this particular solver doesn't use the RelErr and AbsErr figures.
	
	LandResults[0][Timestep] = x0[0];
	LandResults[1][Timestep] = x0[1];
}

void ReachStep(int Timestep, std::vector<std::vector<double>> &ReachResults, const std::vector<double> &Parameters, double InfEx, double Qsw, double *x0, double *wk)
{
	x0[0] = ReachResults[0][Timestep - 1]; //Groundwater volume
	x0[1] = ReachResults[1][Timestep - 1]; //Reach volume
	x0[2] = 0.0;                           //Mean reach flow
	
	double BFI    = Parameters[4];
	double Tg     = Parameters[5];
	double Qg_min = Parameters[6];
	double C_M    = Parameters[7];
	double A_catch = Parameters[8];
	double Len    = Parameters[9];
	double Slope  = Parameters[10];
	
	//TODO: See if there is any speed difference in passing Parameters by reference and then unpack inside lambda.
	const auto &EquationFunction = [Tg, Qg_min, BFI, Qsw, InfEx, A_catch, Slope, C_M, Len](double *x0, double *wk)
	{
		double Vg = x0[0];
		double Vr = x0[1];
		
		double Qg0 = Vg / Tg;
		double t = ActivationControl(Qg0, Qg_min, 0.01);
		double Qg = (1.0 - t)*Qg_min + t*Qg0;
		
		wk[0] = BFI * Qsw - Qg;
		
		double Qland0 = InfEx + (1.0 - BFI) * Qsw + Qg;
		double Qland = ConvertMmPerDayToM3PerSecond(Qland0, A_catch);
		
		double Val = Vr * sqrt(Slope) / (Len * C_M);
		double Qr = 0.28 * Val * sqrt(Val);
		
		wk[1] = 86400.0 * (Qland - Qr);
		wk[2] = Qr;
	};
	
	IncaDascruImpl_(0.1, 3, x0, wk, EquationFunction, nullptr, 0.0, 0.0); //NOTE: this particular solver doesn't use the RelErr and AbsErr figures.
	
	double Vg = x0[0];
	double Qg0 = Vg / Tg;
	double t = ActivationControl(Qg0, Qg_min, 0.01);
	double Qg = (1.0 - t)*Qg_min + t*Qg0;
	ReachResults[0][Timestep] = Qg * Tg;
	
	ReachResults[1][Timestep] = x0[1];
	ReachResults[2][Timestep] = x0[2];
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
	
	
	std::vector<double> Parameters(11);
	
	Parameters[0] = GetParameterDouble(DataSet, "Degree-day factor for snowmelt", {});
	Parameters[1] = GetParameterDouble(DataSet, "Proportion of precipitation that contributes to quick flow", {});
	Parameters[2] = GetParameterDouble(DataSet, "PET multiplication factor", {});
	Parameters[3] = GetParameterDouble(DataSet, "Soil field capacity", {});
	Parameters[4] = GetParameterDouble(DataSet, "Baseflow index", {});
	Parameters[5] = GetParameterDouble(DataSet, "Groundwater time constant", {});
	Parameters[6] = GetParameterDouble(DataSet, "Minimum groundwater flow", {}),
	Parameters[7] = GetParameterDouble(DataSet, "Manning's coefficient", {});
	Parameters[8] = GetParameterDouble(DataSet, "Catchment area", {"Coull"});
	//Effective reach length:
	Parameters[9] = GetParameterDouble(DataSet, "Reach length", {"Coull"}) * 0.5;
	Parameters[10] = GetParameterDouble(DataSet, "Reach slope", {"Coull"});
	
	
	
	std::vector<double> TC(LandscapeUnits.size());
	std::vector<double> LUP(LandscapeUnits.size());
	for(int LU = 0; LU < LandscapeUnits.size(); ++LU)
	{
		TC[LU] = GetParameterDouble(DataSet, "Soil water time constant", {LandscapeUnits[LU]});
		
		LUP[LU] = GetParameterDouble(DataSet, "Land use proportions", {"Coull", LandscapeUnits[LU]});
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
	
	std::vector<std::vector<double>> ReachResults(3);
	for(auto &Ts : ReachResults) Ts.resize(Timesteps + 1);
	
	
	//Initial values
	
	SnowResults[4][0] = GetParameterDouble(DataSet, "Initial snow depth as water equivalent", {});
	
	for(int LU = 0; LU < LandscapeUnits.size(); ++LU)
	{
		LandResults[LU][0][0] = GetParameterDouble(DataSet, "Soil field capacity", {});
	}
	
	double Qr0 = GetParameterDouble(DataSet, "Initial in-stream flow", {"Coull"});
	ReachResults[0][0] = ConvertM3PerSecondToMmPerDay(Qr0, Parameters[8]) * Parameters[4] * Parameters[5];
	
	ReachResults[1][0] = 0.349 * pow(Qr0, 0.34) * 2.71 * pow(Qr0, 0.557) * Parameters[9] * 2.0;
	
	
	
	std::vector<double> x0(3);
	std::vector<double> wk(12);
	
	
	u64 Before = __rdtsc();
	for(int Timestep = 1; Timestep <= (int)Timesteps; ++Timestep)
	{
		SnowStep(Timestep, Inputs, SnowResults, Parameters);
		
		double Inf = SnowResults[6][Timestep];
		double InfEx = SnowResults[7][Timestep];
		
		double Qsw = 0.0;
		for(int LU = 0; LU < LandscapeUnits.size(); ++LU)
		{
			LandStep(Timestep, Inputs[2], LandResults[LU], Parameters, TC[LU], Inf, x0.data(), wk.data());
			
			Qsw += LandResults[LU][1][Timestep] * LUP[LU];
		}
		
		ReachStep(Timestep, ReachResults, Parameters, InfEx, Qsw, x0.data(), wk.data());
		
		//std::cout << "Vr : " << ReachResults[1][Timestep] << std::endl;
	}
	u64 After = __rdtsc();
	
	std::cout << "Hard coded model processor cycles: " << (After - Before) << std::endl;
	
	
	
	
	
	//Correctness evaluation
	
	std::vector<double> MobiusSnowDepth(Timesteps);
	GetResultSeries(DataSet, "Snow depth", {}, MobiusSnowDepth.data(), Timesteps);
	
	std::vector<double> MobiusSoilWaterVolumeArable(Timesteps);
	GetResultSeries(DataSet, "Soil water volume", {LandscapeUnits[0]}, MobiusSoilWaterVolumeArable.data(), Timesteps);
	
	std::vector<double> MobiusMeanReachFlow(Timesteps);
	GetResultSeries(DataSet, "Reach flow (daily mean, cumecs)", {"Coull"}, MobiusMeanReachFlow.data(), Timesteps);
	
	std::vector<double> &HCSnowDepth = SnowResults[4];
	
	std::vector<double> &HCSWV = LandResults[0][0];
	
	std::vector<double> &HCMeanFlow = ReachResults[2];
	
	double ErrTol = 1e-6;
	
	for(int Timestep = 0; Timestep < (int)Timesteps; ++Timestep)
	{
		if(std::abs(MobiusSnowDepth[Timestep] - HCSnowDepth[Timestep+1]) > ErrTol)
		{
			std::cout << "Something went wrong at timestep " << Timestep << ". Snow depth was: " << MobiusSnowDepth[Timestep] << " " << HCSnowDepth[Timestep+1] << std::endl;
			break;
		}
		
		
		if(std::abs(MobiusSoilWaterVolumeArable[Timestep] - HCSWV[Timestep+1]) > ErrTol)
		{
			std::cout << "Something went wrong at timestep " << Timestep << ". Soil water volume was: " << MobiusSoilWaterVolumeArable[Timestep] << " " << HCSWV[Timestep+1] << std::endl;
			break;
		}
		
		if(std::abs(MobiusMeanReachFlow[Timestep] - HCMeanFlow[Timestep+1]) > ErrTol)
		{
			std::cout << "Something went wrong at timestep " << Timestep << ". Mean reach flow was: " << MobiusMeanReachFlow[Timestep] << " " << HCMeanFlow[Timestep+1] << std::endl;
			break;
		}
		
		//std::cout << "SWV: " << MobiusSoilWaterVolumeArable[Timestep] << " " << HCSWV[Timestep+1] << std::endl;
	}
	
}