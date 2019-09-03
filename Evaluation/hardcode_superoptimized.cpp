


// The purpose of this is to see if one could gain even more in a hardcoded model by using SIMD extensions.
// NOTE: This one can not be compiled with MingW64 since AVX don't work correctly there. Can use MSVC.

#include "../mobius.h"

#include <immintrin.h>


#define SIMPLYQ_GROUNDWATER    //NOTE: #defining this before the inclusion of the SimplyQ.h file turns on groundwater in SimplyQ.
#include "SimplyQ_eval_copy.h"

int main()
{
	mobius_model *Model = BeginModelDefinition("SimplyQ", "0.1");
	
	AddSimplyHydrologyModule(Model);
	
	ReadInputDependenciesFromFile(Model, "tarlandinputs.dat");
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);

	ReadParametersFromFile(DataSet, "testparameters.dat");

	ReadInputsFromFile(DataSet, "tarlandinputs.dat");
	
	RunModel(DataSet);
	
	
	
	const char *Reach = "Coull"; //NOTE: Assuming only one reach
	
	u64 Timesteps = GetParameterUInt(DataSet, "Timesteps", {});
	
	u64 DataLen = Timesteps + 4; //NOTE have to add for initial value. Also, must be aligned, so we can't only add one.
	if(DataLen % 4) DataLen += (4 - DataLen % 4);    //NOTE! needed for correct simd alignment!
	
	double *InputData = AllocClearedArray(double, DataLen*3);
	
	double *AirTemperature = InputData;
	double *Precipitation  = InputData + DataLen;
	double *PET            = InputData + 2*DataLen;
	
	GetInputSeries(DataSet, "Air temperature", {}, AirTemperature, Timesteps, true);
	GetInputSeries(DataSet, "Precipitation", {}, Precipitation, Timesteps, true);
	GetInputSeries(DataSet, "Potential evapotranspiration", {}, PET, Timesteps, true);
	
	double *SnowResults = AllocClearedArray(double, DataLen*3);
	
	double *SnowDepth       = SnowResults;
	double *Infiltration    = SnowResults + DataLen;
	double *InfiltrationEx  = SnowResults + 2*DataLen;

	
	double Parameters[11];
	
	Parameters[0] = GetParameterDouble(DataSet, "Degree-day factor for snowmelt", {});
	Parameters[1] = GetParameterDouble(DataSet, "Proportion of precipitation that contributes to quick flow", {});
	Parameters[2] = GetParameterDouble(DataSet, "PET multiplication factor", {});
	Parameters[3] = GetParameterDouble(DataSet, "Soil field capacity", {});
	Parameters[4] = GetParameterDouble(DataSet, "Baseflow index", {});
	Parameters[5] = GetParameterDouble(DataSet, "Groundwater time constant", {});
	Parameters[6] = GetParameterDouble(DataSet, "Minimum groundwater flow", {}),
	Parameters[7] = GetParameterDouble(DataSet, "Manning's coefficient", {});
	Parameters[8] = GetParameterDouble(DataSet, "Catchment area", {Reach});
	//Effective reach length:
	Parameters[9] = GetParameterDouble(DataSet, "Reach length", {Reach}) * 0.5;
	Parameters[10] = GetParameterDouble(DataSet, "Reach slope", {Reach});
	
	u64 LoopSteps = DataLen / 4 - 1;
	
	
	{
		__m256d Zero = _mm256_set1_pd(0.0);
		__m256d One  = _mm256_set1_pd(1.0);
		__m256d DDFMelt = _mm256_set1_pd(Parameters[0]);
		__m256d Pquick = _mm256_set1_pd(Parameters[1]);
		__m256d OneMinusPquick = _mm256_set1_pd(1.0 - Parameters[1]);
	
		u64 Begin = __rdtsc();
		for(u64 Step = 0; Step < LoopSteps; ++Step)
		{
			__m256d AirT        = _mm256_load_pd(AirTemperature + 4*Step);
			__m256d Precip      = _mm256_load_pd(Precipitation  + 4*Step);
			__m256d AirTGE0mask = _mm256_cmp_pd(AirT, Zero, _CMP_GE_OQ);
			__m256d AirTLT0mask = _mm256_cmp_pd(AirT, Zero, _CMP_LT_OQ);
			
			__m256d Prain = _mm256_blendv_pd(Zero, Precip, AirTGE0mask);
			__m256d Psnow = _mm256_blendv_pd(Zero, Precip, AirTLT0mask);
			
			__m256d PotMelt0 = _mm256_mul_pd(AirT, DDFMelt);
			__m256d PotMelt  = _mm256_max_pd(PotMelt0, Zero);
			
			double PotentialMelt[4];
			double PrecipAsRain[4];
			double PrecipAsSnow[4];
			_mm256_store_pd(PrecipAsRain, Prain);
			_mm256_store_pd(PrecipAsSnow, Psnow);
			_mm256_store_pd(PotentialMelt, PotMelt);
			
			double Melt[4];
			//Very unfortunate this can't be simdized:
			for(u64 Ts = 0; Ts < 4; ++Ts)
			{
				double PrevSnowDepth = SnowDepth[4*(Step+1) + Ts - 1];
				Melt[Ts] = PotentialMelt[Ts] > PrevSnowDepth ? PrevSnowDepth : PotentialMelt[Ts];
				SnowDepth[4*(Step+1) + Ts] = PrevSnowDepth + PrecipAsSnow[Ts] - Melt[Ts];
			}
			
			__m256d Meltd = _mm256_load_pd(Melt);
			__m256d HydrolInputSoil = _mm256_add_pd(Meltd, Prain);
			__m256d Inf = _mm256_mul_pd(HydrolInputSoil, OneMinusPquick);
			__m256d InfEx = _mm256_mul_pd(HydrolInputSoil, Pquick);
			
			_mm256_store_pd(Infiltration + 4*(Step+1), Inf);
			_mm256_store_pd(InfiltrationEx + 4*(Step+1), InfEx);
		}
		u64 End = __rdtsc();
		std::cout << "Cycles when using SIMD: " << (End - Begin) << std::endl;
	}
	
	{
		double DDFMelt = Parameters[0];
		double Pquick  = Parameters[1];
	
		u64 Begin = __rdtsc();
		
		for(u64 Ts = 0; Ts < Timesteps; ++Ts)
		{
			double AirT = AirTemperature[Ts];
			double Precip = Precipitation[Ts];
			
			double Prain = AirT >= 0.0 ? Precip : 0.0;
			double Psnow = AirT <  0.0 ? Precip : 0.0;
			
			double PotMelt = AirT > 0.0 ? AirT * DDFMelt : 0.0;
			
			double PrevSnowDepth = SnowDepth[Ts + 4 - 1];
			double Melt = PotMelt > PrevSnowDepth ? PrevSnowDepth : PotMelt;
			SnowDepth[Ts + 4] = PrevSnowDepth + Psnow - Melt;
			
			double HydrolInputSoil = Melt + Prain;
			
			Infiltration[Ts + 4] = HydrolInputSoil * (1.0 - Pquick);
			InfiltrationEx[Ts + 4] = HydrolInputSoil * Pquick;
		}
		
		u64 End   = __rdtsc();
		std::cout << "Cycles when not using SIMD: " << (End - Begin) << std::endl;
	}
	
	/*
	for(u64 Ts = 0; Ts < 200; ++Ts) std::cout << Infiltration[Ts + 4] << " ";
	std::cout << std::endl;
	PrintResultSeries(DataSet, "Infiltration", {}, 200);
	*/
}