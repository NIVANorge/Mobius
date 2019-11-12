


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


// [[Rcpp::export]]
void
mobius_setup_from_parameter_and_input_file(std::string ParameterFileName, std::string InputFileName)
{
	if(DataSet)
	{
		//TODO: Free previous dataset and model
	}
	
	Model = BeginModelDefinition("SimplyP");
	
	AddSimplyPHydrologyModule(Model);
	AddSimplyPSedimentModule(Model);
	AddSimplyPPhosphorusModule(Model);
	AddSimplyPInputToWaterBodyModule(Model);
	
	ReadInputDependenciesFromFile(Model, InputFileName.data());
	
	EndModelDefinition(Model);
	
	DataSet = GenerateDataSet(Model);
	
	ReadParametersFromFile(DataSet, ParameterFileName.data());

	ReadInputsFromFile(DataSet, InputFileName.data());
	
	RunModel(DataSet);
	
	
	return;
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
	
	std::vector<const char *> Indexes;
	for(int Idx = 0; Idx < IndexesIn.size(); ++Idx)
	{
		Indexes.push_back(IndexesIn[Idx]);
	}
	
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
	
	std::vector<const char *> Indexes;
	for(int Idx = 0; Idx < IndexesIn.size(); ++Idx)
	{
		Indexes.push_back(IndexesIn[Idx]);
	}
	
	SetParameterValue(DataSet, Name.data(), Indexes, Value);
	
	return;
}