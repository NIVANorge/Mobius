



#if !defined(CALIBRATION_H)

//NOTE: This file contains common functionality between all calibration/uncertainty analysis algorithms that want to work with our models.

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/accumulators/statistics/sum.hpp>



#if !defined(CALIBRATION_PRINT_DEBUG_INFO)
#define CALIBRATION_PRINT_DEBUG_INFO 0
#endif



enum parameter_distribution_type
{
	ParameterDistribution_Uniform,
	//TODO: Support a few more!!!!
};


enum parameter_link_type
{
	LinkType_Link,
	LinkType_Partition,
};

//NOTE: Specification of calibration of one (or several linked) parameter(s).
struct parameter_calibration
{
	parameter_link_type LinkType;
	std::vector<const char *> ParameterNames;
	std::vector<std::vector<const char *>> ParameterIndexes;
	parameter_distribution_type Distribution;
	double Min;
	double Max;
	double InitialGuess;
	//TODO: Mean, StandardDeviation etc. in case of other distributions.
};

enum performance_measure_type
{
	PerformanceMeasure_MeanAbsoluteError,
	PerformanceMeasure_MeanSquareError,
	PerformanceMeasure_NashSutcliffe,
	PerformanceMeasure_LogLikelyhood_ProportionalNormal,
};

//IMPORTANT: Remember to keep the following functions updated as new objectives are added in!
inline bool
ShouldMaximize(performance_measure_type Type) //Otherwise it should be minimized
{
	return Type == PerformanceMeasure_NashSutcliffe;
}

inline bool
IsLogLikelyhoodMeasure(performance_measure_type Type)
{
	return Type == PerformanceMeasure_LogLikelyhood_ProportionalNormal;
}



struct calibration_objective
{
	performance_measure_type PerformanceMeasure;
	
	const char *ModeledName;
	std::vector<const char *> ModeledIndexes;
	const char *ObservedName;
	std::vector<const char *> ObservedIndexes;
	
	//NOTE: These are only used if the calibration system is weighting the performance measure:
	double Threshold;
	double OptimalValue;
};

const u64 ParameterCalibrationReadDistribution   = 0x1;
const u64 ParameterCalibrationReadInitialGuesses = 0x2;

static void
ReadParameterCalibration(token_stream &Stream, std::vector<parameter_calibration> &CalibOut, u64 Flags = 0)
{
	//NOTE: This function assumes that the Stream has been opened and has already consumed the tokens 'parameter_calibration' and ':'
	
	while(true)
	{
		bool WeAreInLink = false;
		parameter_calibration Calib = {};
		
		while(true)
		{
			token Token = Stream.PeekToken();
			if(Token.Type == TokenType_QuotedString)
			{
				const char *ParameterName = Token.StringValue.Copy().Data; //NOTE: The copied string leaks unless somebody frees it later
				
				Calib.ParameterNames.push_back(ParameterName);  
				Stream.ReadToken(); //NOTE: Consumes the token we peeked.
				
				std::vector<const char *> Indexes;
				Stream.ReadQuotedStringList(Indexes); //NOTE: The copied strings leak unless we free them later
				Calib.ParameterIndexes.push_back(Indexes);
				
				if(!WeAreInLink) break;
			}
			else if(Token.Type == TokenType_UnquotedString)
			{
				if(WeAreInLink)
				{
					Stream.PrintErrorHeader();
					MOBIUS_FATAL_ERROR("Unexpected token inside link." << std::endl);
				}
				
				if(Token.StringValue.Equals("link"))
				{
					WeAreInLink = true;
					Stream.ReadToken(); //NOTE: Consumes the token we peeked.
					Stream.ExpectToken(TokenType_OpenBrace);
					Calib.LinkType = LinkType_Link;
				}
				else if(Token.StringValue.Equals("partition"))
				{
					WeAreInLink = true;
					Stream.ReadToken(); //NOTE: Consumes the token we peeked.
					Stream.ExpectToken(TokenType_OpenBrace);
					Calib.LinkType = LinkType_Partition;
				}
				else
				{
					//NOTE: We hit another code word that signifies a new section of the file.
					//NOTE: Here we should not consume the peeked token since the caller needs to be able to read it.
					return;
				}
			}
			else if(WeAreInLink && Token.Type == TokenType_CloseBrace)
			{
				Stream.ReadToken(); //NOTE: Consumes the token we peeked.
				break;
			}
			else if(Token.Type == TokenType_EOF)
			{
				if(WeAreInLink)
				{
					Stream.PrintErrorHeader();
					MOBIUS_FATAL_ERROR("File ended unexpectedly." << std::endl);
				}
				
				return;
			}
			else
			{
				Stream.PrintErrorHeader();
				MOBIUS_FATAL_ERROR("Unexpected token." << std::endl);
			}
		}
		WeAreInLink = false;
		
		if(Flags & ParameterCalibrationReadDistribution)
		{
			token_string DistrName = Stream.ExpectUnquotedString();
			if(DistrName.Equals("uniform"))
			{
				Calib.Distribution = ParameterDistribution_Uniform;
			}
			//else if ...
			else
			{
				Stream.PrintErrorHeader();
				MOBIUS_FATAL_ERROR("Unsupported distribution: " << DistrName << std::endl);
			}
		}
		
		Calib.Min = Stream.ExpectDouble();
		Calib.Max = Stream.ExpectDouble();
		if(Flags & ParameterCalibrationReadInitialGuesses)
		{
			Calib.InitialGuess = Stream.ExpectDouble();
		}
		//TODO: Optional distribution?
		
		CalibOut.push_back(Calib);
		
	}
}

inline void
PrintParameterCalibration(parameter_calibration &Cal)
{
	size_t Count = Cal.ParameterNames.size();
	if(Count > 1)
	{
		if(Cal.LinkType == LinkType_Link) std::cout << "link {" << std::endl;
		else if(Cal.LinkType == LinkType_Partition) std::cout << "partition {" << std::endl;
	}
	for(size_t Idx = 0; Idx < Count; ++Idx)
	{
		if(Count > 1) std::cout << "\t";
		std::cout << "\"" << Cal.ParameterNames[Idx] << "\" {";
		for(const char *Index : Cal.ParameterIndexes[Idx])
		{
			std::cout << "\"" << Index << "\" ";
		}
		std::cout << "}" << std::endl;
	}
	if(Count > 1) std::cout << "}";
}

static void
ReadCalibrationObjectives(token_stream &Stream, std::vector<calibration_objective> &ObjectivesOut, bool ReadWeightingInfo = false)
{
	//NOTE: This function assumes that the Stream has been opened and has already consumed the tokens 'parameter_calibration' and ':'
	
	while(true)
	{
		calibration_objective Objective = {};
		
		token Token = Stream.PeekToken();
		if(Token.Type == TokenType_QuotedString)
		{
			Objective.ModeledName = Stream.ExpectQuotedString().Copy().Data;
			Stream.ReadQuotedStringList(Objective.ModeledIndexes);
			
			Objective.ObservedName = Stream.ExpectQuotedString().Copy().Data;
			Stream.ReadQuotedStringList(Objective.ObservedIndexes);
			
			token_string PerformanceMeasure = Stream.ExpectUnquotedString();
			if(PerformanceMeasure.Equals("mean_absolute_error"))
			{
				Objective.PerformanceMeasure = PerformanceMeasure_MeanAbsoluteError;
			}
			else if(PerformanceMeasure.Equals("mean_square_error"))
			{
				Objective.PerformanceMeasure = PerformanceMeasure_MeanSquareError;
			}
			else if(PerformanceMeasure.Equals("nash_sutcliffe"))
			{
				Objective.PerformanceMeasure = PerformanceMeasure_NashSutcliffe;
			}
			else if(PerformanceMeasure.Equals("ll_proportional_normal"))
			{
				Objective.PerformanceMeasure = PerformanceMeasure_LogLikelyhood_ProportionalNormal;
			}
			else assert(0);
			
			if(ReadWeightingInfo)
			{
				Objective.Threshold = Stream.ExpectDouble();
				Objective.OptimalValue = Stream.ExpectDouble();
			}
			
			ObjectivesOut.push_back(Objective);
		}
		else break;
	}
}

static size_t
GetDimensions(parameter_calibration &Cal)
{
	if(Cal.ParameterNames.size() == 1)
	{
		return 1;
	}
	else if(Cal.LinkType == LinkType_Link)
	{
		return 1;
	}
	else if(Cal.LinkType == LinkType_Partition)
	{
		return Cal.ParameterNames.size() - 1;
	}
	
	assert(0);
	return 0;
}

static size_t
GetDimensions(std::vector<parameter_calibration> &Calibrations)
{
	size_t Dimensions = 0;
	for(parameter_calibration &Cal : Calibrations)
	{
		Dimensions += GetDimensions(Cal);
	}
	return Dimensions;
}


static void
ApplyCalibrations(mobius_data_set *DataSet, const char *Name, std::vector<const char *> &Indexes, double Value)
{
	SetParameterValue(DataSet, Name, Indexes, Value);
#if CALIBRATION_PRINT_DEBUG_INFO
	std::cout << "Setting \"" << Name << "\" {";
	for(const char *Index : Indexes) std::cout << " \"" << Index << "\"";
	std::cout << " } to " << Value << std::endl;
#endif
}


static void
ApplyCalibrations(mobius_data_set *DataSet, std::vector<parameter_calibration> &Calibrations, const double *ParameterValues)
{
	size_t AtParValue = 0;
	
	for(parameter_calibration &Cal : Calibrations)
	{
		if(Cal.ParameterNames.size() == 1)
		{
			double Value = ParameterValues[AtParValue];
			ApplyCalibrations(DataSet, Cal.ParameterNames[0], Cal.ParameterIndexes[0], Value);
			++AtParValue;

		}
		else if(Cal.LinkType == LinkType_Link)
		{
			double Value = ParameterValues[AtParValue];
			++AtParValue;
			
			for(size_t ParIdx = 0; ParIdx < Cal.ParameterNames.size(); ++ParIdx)
			{
				ApplyCalibrations(DataSet, Cal.ParameterNames[ParIdx], Cal.ParameterIndexes[ParIdx], Value);				
			}
		}
		else if(Cal.LinkType == LinkType_Partition)
		{
			size_t Dim = GetDimensions(Cal);

			std::vector<double> Values(Dim);
			for(size_t Idx = 0; Idx < Dim; ++Idx)
			{
				double Value = ParameterValues[AtParValue];
				Values[Idx] = Value;
				++AtParValue;
			}
			
			std::sort(Values.begin(), Values.end());
			Values.push_back(Cal.Max);
			
			ApplyCalibrations(DataSet, Cal.ParameterNames[0], Cal.ParameterIndexes[0], Values[0]);

			for(size_t ParIdx = 1; ParIdx < Cal.ParameterNames.size(); ++ParIdx)
			{
				double Value = Values[ParIdx] - Values[ParIdx - 1];
				ApplyCalibrations(DataSet, Cal.ParameterNames[ParIdx], Cal.ParameterIndexes[ParIdx], Value);	
			}
		}
		else assert(0);
	}
}


static double
EvaluateObjective(mobius_data_set *DataSet, std::vector<parameter_calibration> &Calibrations, calibration_objective &Objective, const double *ParameterValues, size_t DiscardTimesteps = 0)
{
	//TODO: Evaluate multiple objectives?
	
#if CALIBRATION_PRINT_DEBUG_INFO
	std::cout << "Starting an objective evaluation" << std::endl;
#endif
	
	ApplyCalibrations(DataSet, Calibrations, ParameterValues);

#if CALIBRATION_PRINT_DEBUG_INFO
	timer Timer = BeginTimer();
	RunModel(DataSet);
	u64 Ms = GetTimerMilliseconds(&Timer);
	std::cout << "Running the model took " << Ms << " milliseconds" << std::endl;
#else
	RunModel(DataSet);
#endif
	
	size_t Timesteps = (size_t)DataSet->TimestepsLastRun;
	std::vector<double> ModeledSeries(Timesteps);
	std::vector<double> ObservedSeries(Timesteps);
	
	GetResultSeries(DataSet, Objective.ModeledName, Objective.ModeledIndexes, ModeledSeries.data(), ModeledSeries.size());
	GetInputSeries(DataSet, Objective.ObservedName, Objective.ObservedIndexes, ObservedSeries.data(), ObservedSeries.size(), true); //NOTE: It is a little wasteful reading a copy of this at every evaluation since it will be the same every time...
	
	std::vector<double> Residuals(Timesteps);
	
	for(size_t Timestep = DiscardTimesteps; Timestep < Timesteps; ++Timestep)
	{
		Residuals[Timestep] = ModeledSeries[Timestep] - ObservedSeries[Timestep];
	}
	
	double Performance;
	
	using namespace boost::accumulators;
	
	if(Objective.PerformanceMeasure == PerformanceMeasure_MeanAbsoluteError)
	{
		accumulator_set<double, features<tag::mean>> Accumulator;
		
		for(u64 Timestep = DiscardTimesteps; Timestep < Timesteps; ++Timestep)
		{
			if(!std::isnan(Residuals[Timestep]))
				Accumulator(std::abs(Residuals[Timestep]));
		}
		
		Performance = mean(Accumulator);
	}
	else if(Objective.PerformanceMeasure == PerformanceMeasure_MeanSquareError)
	{
		accumulator_set<double, features<tag::mean>> Accumulator;
		
		for(u64 Timestep = DiscardTimesteps; Timestep < Timesteps; ++Timestep)
		{
			double Res = Residuals[Timestep];
			if(!std::isnan(Res))
				Accumulator(Res*Res);
		}
		
		Performance = mean(Accumulator);
	}
	else if(Objective.PerformanceMeasure == PerformanceMeasure_NashSutcliffe)
	{
		accumulator_set<double, features<tag::variance>> ObsAccum;
		accumulator_set<double, features<tag::mean>> ResAccum;

		for(u64 Timestep = DiscardTimesteps; Timestep < Timesteps; ++Timestep)
		{
			double Obs = ObservedSeries[Timestep];
			double Res = Residuals[Timestep];
			if(!std::isnan(Res))
			{
				ObsAccum(Obs);	
				ResAccum(Res*Res);
			}
		}
		
		double MeanSquaresResidual = mean(ResAccum);
		double ObservedVariance = variance(ObsAccum);
		
		Performance = 1.0 - MeanSquaresResidual / ObservedVariance;
	}
	else if(Objective.PerformanceMeasure == PerformanceMeasure_LogLikelyhood_ProportionalNormal)
	{
		size_t Dimensions = GetDimensions(Calibrations);
		//NOTE: M is an extra parameter that is not a model parameter, so it is placed at the end of the ParameterValues vector. It is important that the caller of the function sets this up correctly..
		double M = ParameterValues[Dimensions];
#if CALIBRATION_PRINT_DEBUG_INFO
		std::cout << "M was set to " << M << std::endl;
#endif
		
		accumulator_set<double, stats<tag::sum>> LogLikelyhoodAccum;
	
		for(size_t Timestep = DiscardTimesteps; Timestep < Timesteps; ++Timestep)
		{
			double Sim = ModeledSeries[Timestep];
			double Res = Residuals[Timestep];
			if(!std::isnan(Res)) //TODO: We should do something else upon NaN in the sim (return -inf?). NaN in obs just means that we don't have a value for that timestep, and so we skip it.
			{
				double Sigma = M*Sim;
				double Like = -0.5*std::log(2.0*Pi*Sigma*Sigma) - Res*Res / (2*Sigma*Sigma);
				LogLikelyhoodAccum(Like);
			}
		}
		
		Performance = sum(LogLikelyhoodAccum);
	}
	else assert(0);
	
#if CALIBRATION_PRINT_DEBUG_INFO
	std::cout << "Performance: " << Performance << std::endl << std::endl;
#endif
	
	return Performance;
}


static double
EvaluateObjectiveAndGradientSingleForwardDifference(mobius_data_set *DataSet, std::vector<parameter_calibration> &Calibrations, calibration_objective &Objective, const double *ParameterValues, size_t DiscardTimesteps, double *GradientOut)
{	
	
	//NOTE: This is a very cheap and probably not that good estimation of the gradient. It should only be used if you need the estimation to be very fast (such as if you are going to use it for each step of an MCMC run).
	size_t Dimensions = GetDimensions(Calibrations);    //IMPORTANT!! This is just for the particular LL function we have now. Should find a way to generalize this.
	
	double F0 = EvaluateObjective(DataSet, Calibrations, Objective, ParameterValues, DiscardTimesteps);
	
	double *XD = (double *)malloc(sizeof(double) * Dimensions);
	for(size_t Dim = 0; Dim < Dimensions; ++Dim) XD[Dim] = ParameterValues[Dim];
	
	const double Epsilon = 1e-6;
	
	for(size_t Dim = 0; Dim < Dimensions; ++Dim)
	{	
		//for(size_t Idx = 0; Idx < Dimensions; ++Idx) std::cout << XD[Idx] << std::endl;
		
		double H0;
		if(abs(XD[Dim]) > 1e-6)
			H0 = sqrt(XD[Dim])*Epsilon;
		else
			H0 = 1e-9; //TODO: This was just completely arbitrary, it should be done properly
		
		//double H0 = 1e-3;
		
		volatile double Temp = XD[Dim] + H0;  //Volatile so that the compiler does not optimize it away
		double H = Temp - XD[Dim];
		
		XD[Dim] += H;
		
		double FD = EvaluateObjective(DataSet, Calibrations, Objective, XD, DiscardTimesteps);
		
		double Grad = (FD - F0) / H;
		//double Grad = (F0 - FD) / H;
		
		//std::cout << "H: " << H << " F0: " << F0 << " XD: " << XD[Dim] << " FD: " << FD << " Grad: " << Grad << std::endl;
		
		GradientOut[Dim] = Grad;
		
		XD[Dim] = ParameterValues[Dim]; // NOTE: Reset for the next evaluation
	}
#if 0
	std::cout << "F0: " << F0 << " Gradient: ";
	for(size_t Dim = 0; Dim < Dimensions; ++Dim) std::cout << GradientOut[Dim] << " ";
	std::cout << std::endl;
#endif
	free(XD);
	
	return F0;
}

#define CALIBRATION_H
#endif