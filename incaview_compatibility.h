
#if !defined(INCAVIEW_COMPATIBILITY_H)


#if !defined(INCAVIEW_INCLUDE_OPTIMIZER)
#define INCAVIEW_INCLUDE_OPTIMIZER 0
#endif

#if INCAVIEW_INCLUDE_OPTIMIZER
#include "Calibration/Optimizer/optimizer.h"
#endif

#if !defined(INCAVIEW_INCLUDE_GLUE)
#define INCAVIEW_INCLUDE_GLUE 0
#endif

#if INCAVIEW_INCLUDE_GLUE
#include "Calibration/GLUE/glue.h"
#endif

#if !defined(INCAVIEW_INCLUDE_MCMC)
#define INCAVIEW_INCLUDE_MCMC 0
#endif

#if INCAVIEW_INCLUDE_MCMC
#include "Calibration/MCMC/mobius_mcmc.h"
#endif



#include "mobius_json_io.cpp"
#include "mobius_sqlite3_io.cpp"

enum incaview_run_mode
{
	IncaviewRunMode_Run,
	IncaviewRunMode_ConvertParameters,
	IncaviewRunMode_FillParameterFile,
	IncaviewRunMode_RunOptimization,
	IncaviewRunMode_RunGLUE,
	IncaviewRunMode_RunMCMC,
};

struct incaview_commandline_arguments
{
	incaview_run_mode Mode;
	const char *InputFileName;
	const char *ParameterInFileName;
	const char *ParameterOutFileName;
	const char *CalibrationScriptName;
	const char *Exename;
};

static void
ParseIncaviewCommandline(int argc, char **argv, incaview_commandline_arguments *Args)
{
	bool CorrectUse = false;
	char *Exename = argv[0];
	
	//NOTE: Very rudimentary attempt to trim off any leading directories. In case somebody called the exe from another directory while creating the database.
	int LastSlashPos = -1;
	int Pos = 0;
	for(char *C = Exename; *C != 0; ++C)
	{
		if(*C == '\\' || *C == '/') LastSlashPos = Pos;
		++Pos;
	}
	Exename = Exename + (LastSlashPos + 1);
	
	Args->Exename = Exename;
	if(argc == 4)
	{
		if(strcmp(argv[1], "run") == 0)
		{
			Args->Mode = IncaviewRunMode_Run;
			Args->InputFileName = argv[2];
			Args->ParameterInFileName = argv[3];
			CorrectUse = true;
		}
		else if(strcmp(argv[1], "convert_parameters") == 0)
		{
			Args->Mode = IncaviewRunMode_ConvertParameters;
			Args->ParameterInFileName   = argv[2];
			Args->ParameterOutFileName  = argv[3];
			CorrectUse = true;
		}
		else if(strcmp(argv[1], "fill_parameter_file") == 0)
		{
			Args->Mode = IncaviewRunMode_FillParameterFile;
			Args->ParameterInFileName  = argv[2];
			Args->ParameterOutFileName = argv[3];
			CorrectUse = true;
		}
	}
	else if(argc == 6)
	{
#if INCAVIEW_INCLUDE_OPTIMIZER
		if(strcmp(argv[1], "run_optimizer") == 0)
		{
			Args->Mode = IncaviewRunMode_RunOptimization;
			Args->InputFileName       = argv[2];
			Args->ParameterInFileName = argv[3];
			Args->CalibrationScriptName = argv[4];
			Args->ParameterOutFileName = argv[5];
			CorrectUse = true;
		}
#endif
#if INCAVIEW_INCLUDE_GLUE
		if(strcmp(argv[1], "run_glue") == 0)
		{
			Args->Mode = IncaviewRunMode_RunGLUE;
			Args->InputFileName       = argv[2];
			Args->ParameterInFileName = argv[3];
			Args->CalibrationScriptName = argv[4];
			Args->ParameterOutFileName = argv[5];
			CorrectUse = true;
		}
#endif
#if INCAVIEW_INCLUDE_GLUE
		if(strcmp(argv[1], "run_mcmc") == 0)
		{
			Args->Mode = IncaviewRunMode_RunMCMC;
			Args->InputFileName       = argv[2];
			Args->ParameterInFileName = argv[3];
			Args->CalibrationScriptName = argv[4];
			Args->ParameterOutFileName = argv[5];
			CorrectUse = true;
		}
#endif
	}
	
	if(!CorrectUse)
	{
		MOBIUS_PARTIAL_ERROR("Incorrect use of the executable. Correct use is one of: " << std::endl);
		MOBIUS_PARTIAL_ERROR(" " << Args->Exename << " run <inputfile(.dat or .json)> <parameterfile(.db or .dat or .json)>" << std::endl);
		MOBIUS_PARTIAL_ERROR(" " << Args->Exename << " convert_parameters <parameterfilein(.db or .dat or .json)> <parameterfileout(.db or .dat or .json)>" << std::endl);
		MOBIUS_PARTIAL_ERROR(" " << Args->Exename << " fill_parameter_file <parameterfilein(.dat)> <parameterfileout(.dat)>" << std::endl);
#if INCAVIEW_INCLUDE_OPTIMIZER
		MOBIUS_PARTIAL_ERROR(" " << Args->Exename << " run_optimizer <inputfile(.dat or .json)> <parameterfile(.db or .dat or .json)> <calibrationscript(.dat)> <parameterfileout(.dat or .db or .json)>" << std::endl);
#endif
#if INCAVIEW_INCLUDE_GLUE
		MOBIUS_PARTIAL_ERROR(" " << Args->Exename << " run_glue <inputfile(.dat or .json)> <parameterfile(.db or .dat or .json)> <calibrationscript(.dat)> <calibrationresults(.db)>" << std::endl);
#endif
#if INCAVIEW_INCLUDE_MCMC
		MOBIUS_PARTIAL_ERROR(" " << Args->Exename << " run_mcmc <inputfile(.dat or .json)> <parameterfile(.db or .dat or .json)> <calibrationscript(.dat)> <calibrationresults(.dat)>" << std::endl);
#endif
		MOBIUS_FATAL_ERROR("");
	}
}

static int
IncaviewParseFileType(const char *Filename)
{
	int Len = strlen(Filename);
	int At = Len - 1;
	while(At >= 0)
	{
		char C = Filename[At];
		if(C == '.') break;
		At--;
	}
	//NOTE: At now points to the last '.' in the file name
	const char *Extension = Filename + At + 1;
	
	if(strcmp(Extension, "db") == 0) return 0;
	if(strcmp(Extension, "dat") == 0) return 1;
	if(strcmp(Extension, "json") == 0) return 2;
	
	MOBIUS_FATAL_ERROR("ERROR: Unsupported file extension: " << Extension << " for file " << Filename << std::endl);
}

static void
ReadInputDependenciesFromFile_Ext(mobius_model *Model, const char *Filename)
{
	int FileType = IncaviewParseFileType(Filename);
	if(FileType == 0)
	{
		MOBIUS_FATAL_ERROR("ERROR: We do not currently support loading inputs from databases" << std::endl);
	}
	else if(FileType == 1)
	{
		ReadInputDependenciesFromFile(Model, Filename);
	}
	else if(FileType == 2)
	{
		ReadInputDependenciesFromJson(Model, Filename);
	}	
}

static void
ReadInputsFromFile_Ext(mobius_data_set *DataSet, const char *Filename)
{
	int FileType = IncaviewParseFileType(Filename);
	if(FileType == 0)
	{
		MOBIUS_FATAL_ERROR("ERROR: We do not currently support loading inputs from databases" << std::endl);
	}
	else if(FileType == 1)
	{
		ReadInputsFromFile(DataSet, Filename);
	}
	else if(FileType == 2)
	{
		ReadInputsFromJson(DataSet, Filename);
	}
}

static void
ReadParametersFromFile_Ext(mobius_data_set *DataSet, const char *Filename)
{
	int FileType = IncaviewParseFileType(Filename);
	if(FileType == 0)
	{
		ReadParametersFromDatabase(DataSet, Filename);
	}
	else if(FileType == 1)
	{
		ReadParametersFromFile(DataSet, Filename);
	}
	else if(FileType == 2)
	{
		ReadParametersFromJson(DataSet, Filename);
	}
}

static void
WriteParametersToFile_Ext(mobius_data_set *DataSet, const char *Filename, const char *Exename)
{
	int FileType = IncaviewParseFileType(Filename);
	if(FileType == 0)
	{
		WriteParametersToDatabase(DataSet, Filename, Exename);
	}
	else if(FileType == 1)
	{
		WriteParametersToFile(DataSet, Filename);
	}
	else if(FileType == 2)
	{
		WriteParametersToJson(DataSet, Filename);
	}
}

static void
EnsureModelComplianceWithIncaviewCommandline(mobius_model *Model, incaview_commandline_arguments *Args)
{
	if(Args->Mode == IncaviewRunMode_Run || Args->Mode == IncaviewRunMode_RunOptimization || Args->Mode == IncaviewRunMode_RunGLUE || Args->Mode == IncaviewRunMode_RunMCMC)
	{
		ReadInputDependenciesFromFile_Ext(Model, Args->InputFileName);
	}
}

static void
RunDatasetAsSpecifiedByIncaviewCommandline(mobius_data_set *DataSet, incaview_commandline_arguments *Args)
{
	if(Args->Mode == IncaviewRunMode_Run)
	{
		//TODO: would it be better to provide these as arguments too?
		const char *ResultDbFileName    = "results.db";
		const char *InputDbFileName     = "inputs.db"; //NOTE: This is only for writing inputs TO so that they can be read by INCAView. Inputs are always read in from the provided .dat file.
		const char *ResultJsonFileName  = "results.json";
		const char *InputJsonFileName   = "inputs.json";
		
		int Type = IncaviewParseFileType(Args->ParameterInFileName);
		
		ReadParametersFromFile_Ext(DataSet, Args->ParameterInFileName);
		ReadInputsFromFile_Ext(DataSet, Args->InputFileName);
		
		RunModel(DataSet);
		
		//TODO: Delete existing databases if they exist? (right now it is handled by incaview, but it could be confusing if somebody runs the exe manually)
		if(Type==0)
		{
			std::cout << "Model run finished. Writing result data to " << ResultDbFileName << std::endl;
			WriteResultsToDatabase(DataSet, ResultDbFileName);
			WriteInputsToDatabase(DataSet, InputDbFileName);
		}
		else
		{
			//TODO: This should be a command line option instead maybe.
			std::cout << "Model run finished. Writing result data to " << ResultJsonFileName << std::endl;
			WriteResultsToJson(DataSet, ResultJsonFileName);
			WriteInputsToJson(DataSet, InputJsonFileName);
		}
	}
#if INCAVIEW_INCLUDE_OPTIMIZER
	else if(Args->Mode == IncaviewRunMode_RunOptimization)
	{
		ReadParametersFromFile_Ext(DataSet, Args->ParameterInFileName);
		ReadInputsFromFile_Ext(DataSet, Args->InputFileName);
		
		optimization_setup Setup;
	
		ReadOptimizationSetup(&Setup, Args->CalibrationScriptName);
		
		auto Result = RunOptimizer(DataSet, &Setup);
		
		std::cout << std::endl;
		PrintOptimizationResult(&Setup, Result);
		
		WriteOptimalParametersToDataSet(DataSet, &Setup, Result);
		
		WriteParametersToFile_Ext(DataSet, Args->ParameterOutFileName, Args->Exename);
	}
#endif
#if INCAVIEW_INCLUDE_GLUE
	else if(Args->Mode == IncaviewRunMode_RunGLUE)
	{
		ReadParametersFromFile_Ext(DataSet, Args->ParameterInFileName);
		ReadInputsFromFile_Ext(DataSet, Args->InputFileName);
		
		glue_setup Setup;
		glue_results Results;
		
		ReadGLUESetupFromFile(&Setup, Args->CalibrationScriptName);
		
		timer RunGlueTimer = BeginTimer();
		RunGLUE(DataSet, &Setup, &Results);
		u64 Ms = GetTimerMilliseconds(&RunGlueTimer);
		
		std::cout << "GLUE finished. Running the model " << Setup.NumRuns << " times with " << Setup.NumThreads << " threads took " << Ms << " milliseconds." << std::endl;
		
		WriteGLUEResultsToDatabase(Args->ParameterOutFileName, &Setup, &Results, DataSet); //NOTE: misleading name, it is not a parameter file, it is a GLUE results database.
	}
#endif
#if INCAVIEW_INCLUDE_MCMC
	else if(Args->Mode == IncaviewRunMode_RunMCMC)
	{
		ReadParametersFromFile_Ext(DataSet, Args->ParameterInFileName);
		ReadInputsFromFile_Ext(DataSet, Args->InputFileName);
		
		mcmc_setup Setup = {};
	
		ReadMCMCSetupFromFile(&Setup, Args->CalibrationScriptName);

		mcmc_results Results;
		
		timer MCMCTimer = BeginTimer();
		RunMCMC(DataSet, &Setup, &Results);
		
		u64 Ms = GetTimerMilliseconds(&MCMCTimer);
		
		std::cout << "Total MCMC run time : " << Ms << " milliseconds." << std::endl;
		
		std::cout << "Acceptance rate: " << Results.AcceptanceRate << std::endl;
		
		if(Setup.Algorithm == MCMCAlgorithm_DifferentialEvolution)
		{
			arma::cube& Draws = Results.DrawsOut;
			Draws.save(Args->ParameterOutFileName, arma::arma_ascii);
		}
		else
		{
			arma::mat& Draws2 = Results.DrawsOut2;
			Draws2.save(Args->ParameterOutFileName, arma::arma_ascii);
		}
	}
#endif
	else if(Args->Mode == IncaviewRunMode_ConvertParameters)
	{
		ReadParametersFromFile_Ext(DataSet, Args->ParameterInFileName);
		WriteParametersToFile_Ext(DataSet, Args->ParameterOutFileName, Args->Exename);
	}
	else if(Args->Mode == IncaviewRunMode_FillParameterFile)
	{
		ReadParametersFromFile(DataSet, Args->ParameterInFileName);
		if(!DataSet->ParameterData)
		{
			AllocateParameterStorage(DataSet);
		}
		WriteParametersToFile(DataSet, Args->ParameterOutFileName);
	}
}

#define INCAVIEW_COMPATIBILITY_H
#endif
