


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
	
	
	#define PADDING 8   //8 probably needed for avx 512??
	
	u64 DataLen = Timesteps + PADDING; //NOTE have to add for initial value. Also, must be aligned, so we can't only add one.
	if(DataLen % PADDING) DataLen += (PADDING - DataLen % PADDING);    //NOTE! needed for correct simd alignment!
	
	double *InputData = AllocClearedArray(double, DataLen*3);
	
	double *AirTemperature = InputData;
	double *Precipitation  = InputData + DataLen;
	double *PET            = InputData + 2*DataLen;
	
	GetInputSeries(DataSet, "Air temperature", {}, AirTemperature, Timesteps, true);
	GetInputSeries(DataSet, "Precipitation", {}, Precipitation, Timesteps, true);
	GetInputSeries(DataSet, "Potential evapotranspiration", {}, PET, Timesteps, true);
	
	double *SnowResults = AllocClearedArray(double, DataLen*3);
	
	double *SnowDepth       = SnowResults                 + PADDING;
	double *Infiltration    = SnowResults + DataLen       + PADDING;
	double *InfiltrationEx  = SnowResults + 2*DataLen     + PADDING;

	
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
	
	# if 1
	{
		int LoopSteps = (int)DataLen / 4 - PADDING / 4;
		
		__m256d Zero = _mm256_set1_pd(0.0);
		__m256d One  = _mm256_set1_pd(1.0);
		__m256d DDFMelt = _mm256_set1_pd(Parameters[0]);
		__m256d Pquick = _mm256_set1_pd(Parameters[1]);
		__m256d OneMinusPquick = _mm256_set1_pd(1.0 - Parameters[1]);
	
		u64 Begin = __rdtsc();
		for(int Step = 0; Step < LoopSteps; ++Step)
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
			for(int Ts = 0; Ts < 4; ++Ts)
			{
				double PrevSnowDepth = SnowDepth[4*Step + Ts - 1];
				Melt[Ts] = PotentialMelt[Ts] > PrevSnowDepth ? PrevSnowDepth : PotentialMelt[Ts];
				SnowDepth[4*Step + Ts] = PrevSnowDepth + PrecipAsSnow[Ts] - Melt[Ts];
			}
			
			__m256d Meltd = _mm256_load_pd(Melt);
			__m256d HydrolInputSoil = _mm256_add_pd(Meltd, Prain);
			__m256d Inf = _mm256_mul_pd(HydrolInputSoil, OneMinusPquick);
			__m256d InfEx = _mm256_mul_pd(HydrolInputSoil, Pquick);
			
			_mm256_store_pd(Infiltration + 4*Step, Inf);
			_mm256_store_pd(InfiltrationEx + 4*Step, InfEx);
		}
		u64 End = __rdtsc();
		std::cout << "Cycles when using SIMD (AVX): " << (End - Begin) << std::endl;
	}
	#endif
	
	#if 0   //Not supported on my cpu :(
	{
		int LoopSteps = (int)DataLen / 8 - 1;
		
		__m512d Zero = _mm512_set1_pd(0.0);
		__m512d One  = _mm512_set1_pd(1.0);
		__m512d DDFMelt = _mm512_set1_pd(Parameters[0]);
		__m512d Pquick = _mm512_set1_pd(Parameters[1]);
		__m512d OneMinusPquick = _mm512_set1_pd(1.0 - Parameters[1]);
	
		u64 Begin = __rdtsc();
		for(int Step = 0; Step < LoopSteps; ++Step)
		{
			
			__m512d AirT        = _mm512_load_pd(AirTemperature + 8*Step);
			__m512d Precip      = _mm512_load_pd(Precipitation  + 8*Step);
			__mmask8 AirTGE0mask = _mm512_cmp_pd_mask(AirT, Zero, _CMP_GE_OQ);
			__mmask8 AirTLT0mask = _mm512_cmp_pd_mask(AirT, Zero, _CMP_LT_OQ);
			
			__m512d Prain = _mm512_mask_blend_pd(AirTGE0mask, Zero, Precip);
			__m512d Psnow = _mm512_mask_blend_pd(AirTLT0mask, Zero, Precip);
			
			__m512d PotMelt0 = _mm512_mul_pd(AirT, DDFMelt);
			__m512d PotMelt  = _mm512_max_pd(PotMelt0, Zero);
			
			double PotentialMelt[8];
			double PrecipAsRain[8];
			double PrecipAsSnow[8];
			_mm512_store_pd(PrecipAsRain, Prain);
			_mm512_store_pd(PrecipAsSnow, Psnow);
			_mm512_store_pd(PotentialMelt, PotMelt);
			
			double Melt[8];
			//Very unfortunate this can't be simdized:
			for(int Ts = 0; Ts < 8; ++Ts)
			{
				double PrevSnowDepth = SnowDepth[8*Step + Ts - 1];
				Melt[Ts] = PotentialMelt[Ts] > PrevSnowDepth ? PrevSnowDepth : PotentialMelt[Ts];
				SnowDepth[8*Step + Ts] = PrevSnowDepth + PrecipAsSnow[Ts] - Melt[Ts];
			}
			
			__m512d Meltd = _mm512_load_pd(Melt);
			__m512d HydrolInputSoil = _mm512_add_pd(Meltd, Prain);
			__m512d Inf = _mm512_mul_pd(HydrolInputSoil, OneMinusPquick);
			__m512d InfEx = _mm512_mul_pd(HydrolInputSoil, Pquick);
			
			_mm512_store_pd(Infiltration + 8*Step, Inf);
			_mm512_store_pd(InfiltrationEx + 8*Step, InfEx);
		}
		u64 End = __rdtsc();
		std::cout << "Cycles when using SIMD (AVX 512): " << (End - Begin) << std::endl;
	}
	#endif
	
	#if 1
	{
		//AVX256, but with single precision floats.
		
		float *InputData32 = AllocClearedArray(float, DataLen * 3);
		for(int Idx = 0; Idx < DataLen*3; ++Idx)
		{
			InputData32[Idx] = (float)InputData[Idx];
		}
		
		float *AirTemperature = InputData32;
		float *Precipitation  = InputData32 +   DataLen;
		float *PET            = InputData32 + 2*DataLen;
		
		float *ResultData32 = AllocClearedArray(float, DataLen * 3);
		float *SnowDepth      = ResultData32             + PADDING;
		float *Infiltration   = ResultData32 +   DataLen + PADDING;
		float *InfiltrationEx = ResultData32 + 2*DataLen + PADDING;
		
		int LoopSteps = (int)DataLen / 8 - PADDING / 8;
		
		__m256 Zero = _mm256_set1_ps(0.0f);
		__m256 One  = _mm256_set1_ps(1.0f);
		__m256 DDFMelt = _mm256_set1_ps((float)Parameters[0]);
		__m256 Pquick = _mm256_set1_ps((float)Parameters[1]);
		__m256 OneMinusPquick = _mm256_set1_ps(1.0f - (float)Parameters[1]);
	
		u64 Begin = __rdtsc();
		for(int Step = 0; Step < LoopSteps; ++Step)
		{
			__m256 AirT        = _mm256_load_ps(AirTemperature + 8*Step);
			__m256 Precip      = _mm256_load_ps(Precipitation  + 8*Step);
			__m256 AirTGE0mask = _mm256_cmp_ps(AirT, Zero, _CMP_GE_OQ);
			__m256 AirTLT0mask = _mm256_cmp_ps(AirT, Zero, _CMP_LT_OQ);
			
			__m256 Prain = _mm256_blendv_ps(Zero, Precip, AirTGE0mask);
			__m256 Psnow = _mm256_blendv_ps(Zero, Precip, AirTLT0mask);
			
			__m256 PotMelt0 = _mm256_mul_ps(AirT, DDFMelt);
			__m256 PotMelt  = _mm256_max_ps(PotMelt0, Zero);
			
			float PotentialMelt[8];
			float PrecipAsRain[8];
			float PrecipAsSnow[8];
			_mm256_store_ps(PrecipAsRain, Prain);
			_mm256_store_ps(PrecipAsSnow, Psnow);
			_mm256_store_ps(PotentialMelt, PotMelt);
			
			float Melt[8];
			//Very unfortunate this can't be simdized:
			for(int Ts = 0; Ts < 4; ++Ts)
			{
				float PrevSnowDepth = SnowDepth[8*Step + Ts - 1];
				Melt[Ts] = PotentialMelt[Ts] > PrevSnowDepth ? PrevSnowDepth : PotentialMelt[Ts];
				SnowDepth[8*Step + Ts] = PrevSnowDepth + PrecipAsSnow[Ts] - Melt[Ts];
			}
			
			__m256 Meltd = _mm256_load_ps(Melt);
			__m256 HydrolInputSoil = _mm256_add_ps(Meltd, Prain);
			__m256 Inf = _mm256_mul_ps(HydrolInputSoil, OneMinusPquick);
			__m256 InfEx = _mm256_mul_ps(HydrolInputSoil, Pquick);
			
			_mm256_store_ps(Infiltration + 8*Step, Inf);
			_mm256_store_ps(InfiltrationEx + 8*Step, InfEx);
		}
		u64 End = __rdtsc();
		std::cout << "Cycles when using SIMD (AVX), single precision: " << (End - Begin) << std::endl;
		
		//for(u64 Ts = 0; Ts < 200; ++Ts) std::cout << Infiltration[Ts] << " ";
		//std::cout << std::endl;
		//PrintResultSeries(DataSet, "Infiltration", {}, 200);
	}
	#endif
	
	#if 1
	{
		double DDFMelt = Parameters[0];
		double Pquick  = Parameters[1];
	
		u64 Begin = __rdtsc();
		
		for(int Ts = 0; Ts < (int)Timesteps; ++Ts)
		{
			double AirT = AirTemperature[Ts];
			double Precip = Precipitation[Ts];
			
			double Prain = AirT >= 0.0 ? Precip : 0.0;
			double Psnow = AirT <  0.0 ? Precip : 0.0;
			
			double PotMelt = AirT > 0.0 ? AirT * DDFMelt : 0.0;
			
			double PrevSnowDepth = SnowDepth[Ts - 1];
			double Melt = PotMelt > PrevSnowDepth ? PrevSnowDepth : PotMelt;
			SnowDepth[Ts + 4] = PrevSnowDepth + Psnow - Melt;
			
			double HydrolInputSoil = Melt + Prain;
			
			Infiltration[Ts] = HydrolInputSoil * (1.0 - Pquick);
			InfiltrationEx[Ts] = HydrolInputSoil * Pquick;
		}
		
		u64 End   = __rdtsc();
		std::cout << "Cycles when not using SIMD: " << (End - Begin) << std::endl;
	}
	#endif
	
}