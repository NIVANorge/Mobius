


#include <Rcpp.h>


#define MOBIUS_PARTIAL_ERROR(Msg) \
Rcpp::Rcout << Msg;

	
#define MOBIUS_FATAL_ERROR(Msg) \
{MOBIUS_PARTIAL_ERROR(Msg) \
Rcpp::stop("returning to R scope");}


#include "../mobius.h"

#include "../Modules/SimplyP.h"

// [[Rcpp::export]]
Rcpp::NumericVector
Run()
{
	mobius_model *Model = BeginModelDefinition("SimplyP");
	
	AddSimplyPHydrologyModule(Model);
	AddSimplyPSedimentModule(Model);
	AddSimplyPPhosphorusModule(Model);
	AddSimplyPInputToWaterBodyModule(Model);
	
	ReadInputDependenciesFromFile(Model, "../Applications/SimplyP/Tarland/TarlandInputs.dat"); //NOTE: This has to happen here before EndModelDefinition
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
	
	ReadParametersFromFile(DataSet, "../Applications/SimplyP/Tarland/TarlandParameters_v0-3.dat");

	ReadInputsFromFile(DataSet, "../Applications/SimplyP/Tarland/TarlandInputs.dat");
	
	RunModel(DataSet);
	
	u64 Timesteps = GetTimesteps(DataSet);
	
	
	std::vector<double> Result(Timesteps);
	
	GetResultSeries(DataSet, "Reach flow (daily mean, cumecs)", {"Coull"}, Result.data(), Result.size());
	
	Rcpp::NumericVector Out(Result.begin(), Result.end());
	
	return Out;
}