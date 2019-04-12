
//TODO: This file needs more documentation and error handling.
//NOTE: Is adaptation of https://github.com/LeahJB/SimplyP/blob/Hydrology_Model/Current_Release/v0-2A/simplyP/inputs.py


#if !defined(THORNTHWAITE_PET_H)

static void
AnnualThornthwaite(const std::vector<double> &MonthlyMeanT, const std::vector<double> &MonthlyMeanDLH, s32 Year, std::vector<double> &PETOut)
{
	PETOut.resize(12);
	//NOTE: Calculate heat index.
	double I = 0.0;
	for(double Tai : MonthlyMeanT)
	{
		if(Tai >= 0.0) I += pow(Tai / 5.0, 1.514);
	}
	
	double a = (6.75e-07 * I * I * I) - (7.71e-05 * I * I) + (1.792e-02 * I) + 0.49239;
	
	for(int M = 0; M < 12; ++M)
	{
		double T = MonthlyMeanT[M];
		T = T >= 0.0 ? T : 0.0;
		double L = MonthlyMeanDLH[M];
		double N = (double)MonthLength(Year, M);
		PETOut[M] = 16.0 * (L / 12.0) * (N / 30.0) * pow(10.0 * T / I, a);
	}
}

static void
MonthlyMeanDLH(double Latitude, s32 Year, std::vector<double> &DLHOut)
{
	DLHOut.resize(12);
	int DayOfYear = 1;
	double Ylen = 365.0 + (double)IsLeapYear(Year);
	
	for(int M = 0; M < 12; ++M)
	{
		double Dlh = 0.0;
		int Len = MonthLength(Year, M);
		for(int Day = 1; Day <= Len; ++Day)
		{
			double SD = 0.409 * sin((2.0 * Pi / Ylen)*(double)DayOfYear - 1.39); //Solar declination
			double CosSha = -tan(Latitude) * tan(SD);
			double Sha = acos(std::min(std::max(CosSha, -1.0), 1.0)); //Sunset hour angle
			Dlh += (24.0 / Pi) * Sha; //Daylight hours
			++DayOfYear;
		}
		DLHOut[M] = Dlh / (double)Len;
	}
}


static void
ComputeThornthwaitePET(mobius_data_set *DataSet)
{
	//NOTE: This should only be added as a preprocessing step if you have registered the inputs "Air temperature" and "Potential evapotranspiration" and the parameter "Latitude". Moreover, the "Air temperature" and "Potential evapotranspiration" has to index over the same index sets.
	
	bool AnyNeedProcessing = false;
	ForeachInputInstance(DataSet, "Potential evapotranspiration", 
		[DataSet, &AnyNeedProcessing](const char * const *IndexNames, size_t IndexesCount)
		{
			if(!InputSeriesWasProvided(DataSet, "Potential evapotranspiration", IndexNames, IndexesCount))
			{
				AnyNeedProcessing = true;
			}
		}
	);
	
	if(!AnyNeedProcessing) return;
	
	s32 Year, Month, Day;
	datetime Date = GetInputStartDate(DataSet);
	
	u64 Timesteps = DataSet->InputDataTimesteps;
	
	Date.YearMonthDay(&Year, &Month, &Day);
	if(Month != 1 && Day != 1)
	{
		MOBIUS_FATAL_ERROR("ERROR: To use the Thornthwaite PET module, the input data has to start at Jan. 1st." << std::endl);
	}
	s32 StartYear = Year;
	
	datetime Date2 = Date;
	Date2.AdvanceDays((s32)Timesteps - 1);
	Date2.YearMonthDay(&Year, &Month, &Day);
	if(Month != 12 && Day != 31)
	{
		MOBIUS_FATAL_ERROR("ERROR: To use the Thornthwaite PET module, the input data has to end at Dec. 31st. It ends on " << Year << "-" << Month << "-" << Day << std::endl);
	}
	s32 EndYear = Year;
	
	ForeachInputInstance(DataSet, "Potential evapotranspiration",
		[DataSet, Timesteps, StartYear, EndYear](const char * const *IndexNames, size_t IndexesCount)
		{
			double Latitude = GetParameterDouble(DataSet, "Latitude", {});
			Latitude = Latitude * Pi / 180.0; //Convert degrees to radians
			
			std::vector<double> AirTemperature(Timesteps);
	
			GetInputSeries(DataSet, "Air temperature", IndexNames, IndexesCount, AirTemperature.data(), AirTemperature.size());
			
			std::vector<double> MonthlyPET;
	
			size_t Timestep = 0;
			for(s32 Year = StartYear; Year <= EndYear; ++Year)
			{
				std::vector<double> MonthlyMeanT(12);
				for(int M = 0; M < 12; ++M)
				{
					double AirTSum = 0.0;
					int MonthLen = MonthLength(Year, M);
					for(int Day = 0; Day < MonthLen; ++Day)
					{
						AirTSum += AirTemperature[Timestep];
						++Timestep;
					}
					MonthlyMeanT[M] = AirTSum / (double)MonthLen;
				}
				
				std::vector<double> DLH;
				MonthlyMeanDLH(Latitude, Year, DLH);
				
				std::vector<double> PET;
				AnnualThornthwaite(MonthlyMeanT, DLH, Year, PET);
				
				for(int M = 0; M < 12; ++M)
				{
					int MonthLen = MonthLength(Year, M);
					PET[M] /= (double)MonthLen;         //Turn average monthly into average daily.
				}
				
				MonthlyPET.insert(MonthlyPET.end(), PET.begin(), PET.end());
			}
			
			std::vector<double> PET(Timesteps);
			
			int MonthIndex = 0;
			Timestep = 0;
			for(s32 Year = StartYear; Year <= EndYear; ++Year)
			{
				for(int M = 0; M < 12; ++M)
				{
					double MonthValue = MonthlyPET[MonthIndex];
					
					double PrevMonthValue = MonthlyPET[MonthIndex];
					if(Year >= StartYear || M > 0) PrevMonthValue = MonthlyPET[MonthIndex - 1];
					
					double NextMonthValue = MonthlyPET[MonthIndex];
					if(Year <= EndYear || M < 11) NextMonthValue = MonthlyPET[MonthIndex + 1];
					
					int MonthLen = MonthLength(Year, M);
					for(int Day = 0; Day < MonthLen; ++Day)
					{
						double Value;
						int Midpoint = MonthLen / 2;
						if(Day < Midpoint)
						{
							double t = (double)(Midpoint - Day) / (double)Midpoint;
							Value = 0.5*t*(PrevMonthValue + MonthValue) + (1.0 - t)*MonthValue;
						}
						else
						{
							double t = (double)(MonthLen - 1 - Day) / (double)(MonthLen - Midpoint - 1);
							Value = t * MonthValue + 0.5*(1.0 - t)*(NextMonthValue + MonthValue);
						}
						PET[Timestep] = Value;
						
						++Timestep;
					}
					
					++MonthIndex;
				}
			}
			
			SetInputSeries(DataSet, "Potential evapotranspiration", IndexNames, IndexesCount, PET.data(), PET.size());
		}
	);
}


#define THORNTHWAITE_PET_H
#endif