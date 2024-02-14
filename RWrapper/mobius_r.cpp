


#include <Rcpp.h>

// Enable C++11 via this plugin (Rcpp 0.10.3 or later)
// [[Rcpp::plugins(cpp11)]]


// Override error and warning calls to direct them to the R output channel.

void
ErrorPrint() {}

template<typename t, typename... v>
void
ErrorPrint(t Value, v... Tail)
{
	Rcpp::Rcout << Value;
	ErrorPrint(Tail...);
}

template<typename... v>
void
FatalError(v... Tail)
{
	ErrorPrint(Tail...);
	Rcpp::stop("returning to R scope");
}	

void
WarningPrint() {}

template<typename t, typename... v>
void
WarningPrint(t Value, v... Tail)
{
	Rcpp::Rcout << Value;
	WarningPrint(Tail...);
}


#define MOBIUS_ERROR_OVERRIDE
#define MOBIUS_ALLOW_OLE 0

#include "../mobius.h"

#include "../Modules/PET.h"
#include "../Modules/Simply/SimplySnow.h"
#define SIMPLYQ_GROUNDWATER
#include "../Modules/Simply/SimplyQ.h"
#include "../Modules/Simply/SimplySed.h"
#include "../Modules/Simply/SimplyP.h"


mobius_model    *Model   = nullptr;;
mobius_data_set *DataSet = nullptr;


void BuildModel()
{
	if(Model)
	{
		//TODO: free previous model (and potentially dataset)
	}
	
	//The following has to be switched out if you want to use a different model.
	
	Model = BeginModelDefinition("SimplyP", true);   //'true' signifies that we want an "End date" parameter instead of a "Timesteps" parameter
	
	AddThornthwaitePETModule(Model);
	AddSimplySnowModule(Model);
	AddSimplyHydrologyModule(Model);
	AddSimplySedimentModule(Model);
	AddSimplyPModel(Model);
}


// [[Rcpp::export]]
void
mobius_setup_from_parameter_and_input_file(std::string ParameterFileName, std::string InputFileName)
{
	BuildModel();
	
	ReadInputDependenciesFromFile(Model, InputFileName.data());
	
	EndModelDefinition(Model);
	
	DataSet = GenerateDataSet(Model);
	
	ReadParametersFromFile(DataSet, ParameterFileName.data());

	ReadInputsFromFile(DataSet, InputFileName.data());
}

// [[Rcpp::export]]
void
mobius_setup_from_parameter_file_and_input_series(std::string ParameterFileName, std::string InputDataStartDate, std::vector<double> AirTemperature, std::vector<double> Precipitation)
{
	BuildModel();
	EndModelDefinition(Model);
	
	DataSet = GenerateDataSet(Model);
	
	ReadParametersFromFile(DataSet, ParameterFileName.data());
	
	bool Success;
	DataSet->InputDataStartDate = datetime(InputDataStartDate.data(), &Success);
	if(!Success) Rcpp::stop("Erroneous date format provided. Expected yyyy-mm-dd");
	DataSet->InputDataHasSeparateStartDate = true;
	
	SetInputSeries(DataSet, "Air temperature", {}, AirTemperature.data(), AirTemperature.size());
	SetInputSeries(DataSet, "Precipitation", {}, Precipitation.data(), Precipitation.size());
}


// [[Rcpp::export]]
void
mobius_run_model()
{
	if(!DataSet) return;
	
	RunModel(DataSet);
}

// [[Rcpp::export]]
std::vector<double>
mobius_get_result_series(std::string Name, Rcpp::StringVector IndexesIn)
{
	if(!DataSet) return {};
	
	std::vector<const char *> Indexes(IndexesIn.begin(), IndexesIn.end());
	
	u64 Timesteps = GetTimesteps(DataSet);
	
	std::vector<double> Result(Timesteps);
	
	GetResultSeries(DataSet, Name.data(), Indexes, Result.data(), Result.size());
	
	return Result;
}


// [[Rcpp::export]]
void
mobius_set_parameter_double(std::string Name, Rcpp::StringVector IndexesIn, double Value)
{
	if(!DataSet) return;
	
	std::vector<const char *> Indexes(IndexesIn.begin(), IndexesIn.end());
	
	SetParameterValue(DataSet, Name.data(), Indexes, Value);
}

// [[Rcpp::export]]
void
mobius_set_parameter_uint(std::string Name, Rcpp::StringVector IndexesIn, u64 Value)
{
	if(!DataSet) return;
	
	std::vector<const char *> Indexes(IndexesIn.begin(), IndexesIn.end());
	
	SetParameterValue(DataSet, Name.data(), Indexes, Value);
}

// [[Rcpp::export]]
void
mobius_set_parameter_bool(std::string Name, Rcpp::StringVector IndexesIn, bool Value)
{
	if(!DataSet) return;
	
	std::vector<const char *> Indexes(IndexesIn.begin(), IndexesIn.end());
	
	SetParameterValue(DataSet, Name.data(), Indexes, Value);
}

// [[Rcpp::export]]
void
mobius_set_parameter_time(std::string Name, Rcpp::StringVector IndexesIn, std::string Value)
{
	if(!DataSet) return;
	
	std::vector<const char *> Indexes(IndexesIn.begin(), IndexesIn.end());
	
	SetParameterValue(DataSet, Name.data(), Indexes, Value.data());
}


//NOTE: This one is thread safe and does not modify the global dataset:

// [[Rcpp::export]]
Rcpp::DataFrame
mobius_run_with(Rcpp::StringVector ParNames, Rcpp::List ParIndexes, std::vector<double> ParValues, Rcpp::StringVector ResultNames, Rcpp::List ResultIndexes)
{
	if(!DataSet) return {};
	
	mobius_data_set *Copy = CopyDataSet(DataSet);
	
	if(ParNames.size() != ParIndexes.size() || ParNames.size() != ParValues.size())
	{
		Rcpp::stop("The number of parameter names, index tuples and values must be the same.");
	}
	
	if(ResultNames.size() != ResultIndexes.size())
	{
		Rcpp::stop("The number of result names must be the same as the number of result index tuples");
	}
	
	for(size_t Idx = 0; Idx < ParNames.size(); ++Idx)
	{
		const char *Name = ParNames[Idx];
		Rcpp::StringVector Ind = ParIndexes[Idx];
		std::vector<const char *> Indexes(Ind.begin(), Ind.end());
		double Value = ParValues[Idx];
		
		SetParameterValue(Copy, Name, Indexes, Value);
	}
	
	RunModel(Copy);
	
	u64 Timesteps = GetTimesteps(Copy);
	std::vector<double> Result(Timesteps);
	
	Rcpp::DataFrame Results;
	for(size_t Idx = 0; Idx < ResultNames.size(); ++Idx)
	{
		const char *Name = ResultNames[Idx];
		Rcpp::StringVector Ind = ResultIndexes[Idx];
		std::vector<const char *> Indexes(Ind.begin(), Ind.end());
		
		GetResultSeries(Copy, Name, Indexes, Result.data(), Result.size());
		
		Results.push_back(Result);
	}
	
	
	delete Copy;
	
	return Results;
}


// [[Rcpp::export]]
void
mobius_print_result_structure()
{
	if(!DataSet) return;
	
	PrintResultStructure(DataSet->Model, Rcpp::Rcout);
}

