


#include <Rcpp.h>

// Enable C++11 via this plugin (Rcpp 0.10.3 or later)
// [[Rcpp::plugins(cpp11)]]


#define MOBIUS_PARTIAL_ERROR(Msg) \
Rcpp::Rcout << Msg;

	
#define MOBIUS_FATAL_ERROR(Msg) \
{MOBIUS_PARTIAL_ERROR(Msg) \
Rcpp::stop("returning to R scope");}


#include "../mobius.h"

#include "../Modules/SimplyP.h"


mobius_model    *Model   = nullptr;;
mobius_data_set *DataSet = nullptr;


void BuildModel()
{
	if(Model)
	{
		//TODO: free previous model (and potentially dataset)
	}
	
	//The following has to be switched out if you want to use a different model.
	
	Model = BeginModelDefinition("SimplyP");
	
	AddSimplyPHydrologyModule(Model);
	AddSimplyPSedimentModule(Model);
	AddSimplyPPhosphorusModule(Model);
	AddSimplyPInputToWaterBodyModule(Model);
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




