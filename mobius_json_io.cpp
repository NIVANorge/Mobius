
#if !defined(MOBIUS_JSON_IO_CPP)

#include "json/json.hpp"    //NOTE: download at https://github.com/nlohmann/json

static void
WriteInputsToJson(mobius_data_set *DataSet, const char *Filename)
{
	using nlohmann::json;
	
	const mobius_model *Model = DataSet->Model;
	
	std::vector<std::string> AdditionalTimeseries;
	std::map<std::string, std::vector<std::string>> IndexSetDependencies;
	
	for(entity_handle InputHandle = 1; InputHandle < Model->FirstUnusedInputHandle; ++InputHandle)
	{
		const input_spec &Spec = Model->InputSpecs[InputHandle];
		
		if(Spec.IsAdditional)
		{
			AdditionalTimeseries.push_back(GetName(Model, input_h {InputHandle}));
		}
		
		std::vector<std::string> Dep;
		for(index_set_h IndexSet : Spec.IndexSetDependencies)
		{
			Dep.push_back(GetName(Model, IndexSet));
		}
		
		IndexSetDependencies[Spec.Name] = Dep;	
	}
	
	
	auto T = std::time(nullptr);
	auto TM = *std::localtime(&T);
	std::stringstream Oss;
	Oss << std::put_time(&TM, "%Y-%m-%d %H:%M:%S");
	
	std::string CreationDate = Oss.str();
	
	u64 Timesteps = DataSet->InputDataTimesteps;
	
	std::string StartDate = GetInputStartDate(DataSet).ToString();
    
	json Json = {
				{"creation_date", CreationDate},
				{"start_date", StartDate},
				{"timesteps", Timesteps},
				{"index_set_dependencies", IndexSetDependencies},
				{"additional_timeseries", AdditionalTimeseries},
				{"data", nullptr},
			};
	
	
	//index_t CurrentIndexes[256];
	for(entity_handle InputHandle = 1; InputHandle < Model->FirstUnusedInputHandle; ++InputHandle)
	{
		const char *InputName = GetName(Model, input_h {InputHandle});
		
		ForeachInputInstance(DataSet, InputName,
			[DataSet, Timesteps, InputName, &Json](const char * const *IndexNames, size_t IndexesCount)
			{
				std::vector<double> Values((size_t)Timesteps);
				GetInputSeries(DataSet, InputName, IndexNames, IndexesCount, Values.data(), Values.size());
				
				std::vector<std::string> Indices(IndexesCount);
				for(size_t Idx = 0; Idx < IndexesCount; ++Idx) Indices[Idx] = IndexNames[Idx];
				
				json JObj
				{
					{"indexes", Indices},
					{"values", Values},
				};
				
				Json["data"][InputName].push_back(JObj);
			}
		);
		
		//WriteTimeseriesToJsonRecursive(DataSet, Json, InputHandle, 1, CurrentIndexes, Timesteps);
	}
  
	std::ofstream Out(Filename);
	Out << Json.dump(1,'\t') << std::endl;
}

static void
WriteResultsToJson(mobius_data_set *DataSet, const char *Filename)
{
	using nlohmann::json;
	
	const mobius_model *Model = DataSet->Model;
	
	std::map<std::string, std::vector<std::string>> IndexSetDependencies;
	
	for(entity_handle EquationHandle = 1; EquationHandle < Model->FirstUnusedEquationHandle; ++EquationHandle)
	{
		const equation_spec &Spec = Model->EquationSpecs[EquationHandle];
		
		if(Spec.Type == EquationType_InitialValue) continue;
		
		std::vector<std::string> Dep;
	
		size_t UnitIdx = DataSet->ResultStorageStructure.UnitForHandle[EquationHandle];
		storage_unit_specifier &Unit = DataSet->ResultStorageStructure.Units[UnitIdx];
		
		for(index_set_h IndexSet : Unit.IndexSets)
		{
			Dep.push_back(GetName(Model, IndexSet));
		}
		
		IndexSetDependencies[Spec.Name] = Dep;	
	}
	
	
	auto T = std::time(nullptr);
	auto TM = *std::localtime(&T);
	std::stringstream Oss;
	Oss << std::put_time(&TM, "%Y-%m-%d %H:%M:%S");
	
	std::string CreationDate = Oss.str();
	
	u64 Timesteps = GetTimesteps(DataSet);
	
	std::string StartDate = GetStartDate(DataSet).ToString();
    
	json Json = {
				{"creation_date", CreationDate},
				{"start_date", StartDate},
				{"timesteps", Timesteps},
				{"index_set_dependencies", IndexSetDependencies},
				{"data", nullptr},
			};
   
	//index_t CurrentIndexes[256];
	for(entity_handle EquationHandle = 1; EquationHandle < Model->FirstUnusedEquationHandle; ++EquationHandle)
	{
		const equation_spec &Spec = Model->EquationSpecs[EquationHandle];
		
		if(Spec.Type == EquationType_InitialValue) continue;
		
		const char *EquationName = Spec.Name;
		
		ForeachResultInstance(DataSet, EquationName,
			[DataSet, Timesteps, EquationName, &Json](const char * const *IndexNames, size_t IndexesCount)
			{
				std::vector<double> Values((size_t)Timesteps);
				GetResultSeries(DataSet, EquationName, IndexNames, IndexesCount, Values.data(), Values.size());
				
				std::vector<std::string> Indices(IndexesCount);
				for(size_t Idx = 0; Idx < IndexesCount; ++Idx) Indices[Idx] = IndexNames[Idx];
				
				json JObj
				{
					{"indexes", Indices},
					{"values", Values},
				};
				
				Json["data"][EquationName].push_back(JObj);
			}
		);
		
		//WriteTimeseriesToJsonRecursive(DataSet, Json, InputHandle, 1, CurrentIndexes, Timesteps);
	}
  
	std::ofstream Out(Filename);
	Out << Json.dump(1,'\t') << std::endl;
	Out.close();
	if(!Out)
	{
		MOBIUS_PARTIAL_ERROR("ERROR: Could not write to file " << Filename << std::endl);
	}
}

static void
ReadInputDependenciesFromJson(mobius_model *Model, const char *Filename)
{
	std::ifstream Ifs(Filename);
	nlohmann::json JData;
	Ifs >> JData;
	
	if (JData.find("additional_timeseries") != JData.end())
    {
        std::vector<std::string> AdditionalTimeseries = JData["additional_timeseries"].get<std::vector<std::string>>();
		
		for(std::string &Str : AdditionalTimeseries)
		{
			const char *InputName = CopyString(Str.c_str());
			RegisterInput(Model, InputName, {0}, true);
		}
    }
    
    if (JData.find("index_set_dependencies") != JData.end())
    {
        std::map<std::string, std::vector<std::string>> Dep = JData["index_set_dependencies"].get<std::map<std::string,std::vector<std::string>>>();
		for(auto &D : Dep)
		{
			const std::string &Name = D.first;
			std::vector<std::string> &IndexSets = D.second;
			
			input_h Input = GetInputHandle(Model, Name.c_str());
			
			for(std::string &IndexSet : IndexSets)
			{
				index_set_h IndexSetH = GetIndexSetHandle(Model, IndexSet.c_str());
				AddInputIndexSetDependency(Model, Input, IndexSetH);
			}
		}
    }
}


static void 
ReadInputsFromJson(mobius_data_set *DataSet, const char *Filename)
{
	std::ifstream Ifs(Filename);
	nlohmann::json JData;
	Ifs >> JData;
 
	if (JData.find("timesteps") != JData.end())
	{
		DataSet->InputDataTimesteps = (u64)JData["timesteps"].get<u64>();
	}
	else
	{
		MOBIUS_FATAL_ERROR("Input file " << Filename << " does not declare the number of timesteps for the input data." << std::endl);
	}
    
	if (JData.find("start_date") != JData.end())
	{
		std::string DateStr = JData["start_date"];
		
		bool ParseSuccess;
		datetime Date(DateStr.c_str(), &ParseSuccess);
		
		if(!ParseSuccess)
		{
			MOBIUS_PARTIAL_ERROR("ERROR: In file " << Filename << ": ");
			MOBIUS_FATAL_ERROR("Unrecognized date format \"" << DateStr << "\". Supported format: Y-m-d" << std::endl);
		}
		
		DataSet->InputDataHasSeparateStartDate = true;
		DataSet->InputDataStartDate = Date;
	}

	
	if (JData.find("data") != JData.end())
	{
		for (nlohmann::json::iterator It = JData["data"].begin(); It != JData["data"].end(); ++It)
		{
			std::string Name = It.key();
			
			for(auto Itit = It->begin(); Itit != It->end(); ++Itit)
			{
				std::vector<std::string> Indices = (*Itit)["indexes"].get<std::vector<std::string>>();
				std::vector<const char *> Indices2;
				for(std::string &Str : Indices) Indices2.push_back(Str.c_str());
				
				auto &Val = (*Itit)["values"];
				std::vector<double> Values;
				Values.reserve(Val.size());
				for(auto &V : Val)
				{
					if(V.is_number_float())
					{
						double X = V;
						Values.push_back(X);
					}
					else Values.push_back(std::numeric_limits<double>::quiet_NaN());
				}

				SetInputSeries(DataSet, Name.c_str(), Indices2, Values.data(), Values.size());
			}
		}
	}
}


static void
WriteParametersToJson(mobius_data_set *DataSet, const char *Filename)
{
	using nlohmann::json;
	
	const mobius_model *Model = DataSet->Model;
	
	json Json
	{
		{"index_sets", nullptr},
		{"parameters", nullptr},
	};
	
	for(entity_handle IndexSetHandle = 1; IndexSetHandle < Model->FirstUnusedIndexSetHandle; ++IndexSetHandle)
	{
		const index_set_spec &Spec = Model->IndexSetSpecs[IndexSetHandle];
		if(Spec.Type == IndexSetType_Basic)
		{
			for(index_t Index = {IndexSetHandle, 0}; Index < DataSet->IndexCounts[IndexSetHandle]; ++Index)
			{
				std::string IndexName = DataSet->IndexNames[IndexSetHandle][Index];
				Json["index_sets"][Spec.Name].push_back(IndexName);
			}
		}
		else if(Spec.Type == IndexSetType_Branched)
		{
			std::vector<std::vector<std::string>> BranchInputs;
			for(index_t Index = {IndexSetHandle, 0}; Index < DataSet->IndexCounts[IndexSetHandle]; ++Index)
			{
				std::vector<std::string> Branches;
				Branches.push_back(DataSet->IndexNames[IndexSetHandle][Index]); //NOTE: The name of the index itself.
				for(size_t In = 0; In < DataSet->BranchInputs[IndexSetHandle][Index].Count; ++In)
				{
					index_t InIndex = DataSet->BranchInputs[IndexSetHandle][Index].Inputs[In];
					
					std::string InName = DataSet->IndexNames[IndexSetHandle][InIndex];
					Branches.push_back(InName);
				}
				
				BranchInputs.push_back(Branches);
			}
			Json["index_sets"][Spec.Name] = BranchInputs;
		}
	}
	
	for(entity_handle ParameterHandle = 1; ParameterHandle < Model->FirstUnusedParameterHandle; ++ParameterHandle)
	{
		const parameter_spec &Spec = Model->ParameterSpecs[ParameterHandle];
		const char *ParameterName = Spec.Name;
		
		if(Spec.ShouldNotBeExposed) continue;
		
		parameter_type Type = Spec.Type;
		
		std::vector<parameter_value> Values;
		ForeachParameterInstance(DataSet, ParameterName,
			[DataSet, ParameterName, Type, &Values](const char * const *IndexNames, size_t IndexCount)
			{
				parameter_value Value = GetParameterValue(DataSet, ParameterName, IndexNames, IndexCount, Type);
				Values.push_back(Value);
			}
		);
		
		if(Spec.Type == ParameterType_Double)
		{
			std::vector<double> ValuesDouble(Values.size());
			for(size_t Idx = 0; Idx < Values.size(); ++Idx) ValuesDouble[Idx] = Values[Idx].ValDouble;
			Json["parameters"][ParameterName] = ValuesDouble;
		}
		else if(Spec.Type == ParameterType_UInt)
		{
			std::vector<u64> ValuesUInt(Values.size());
			for(size_t Idx = 0; Idx < Values.size(); ++Idx) ValuesUInt[Idx] = Values[Idx].ValUInt;
			Json["parameters"][ParameterName] = ValuesUInt;
		}
		else if(Spec.Type == ParameterType_Bool)
		{
			std::vector<bool> ValuesBool(Values.size());
			for(size_t Idx = 0; Idx < Values.size(); ++Idx) ValuesBool[Idx] = Values[Idx].ValBool;
			Json["parameters"][ParameterName] = ValuesBool;
		}
		else if(Spec.Type == ParameterType_Time)
		{
			std::vector<std::string> ValuesTime(Values.size());
			for(size_t Idx = 0; Idx < Values.size(); ++Idx) ValuesTime[Idx] = Values[Idx].ValTime.ToString();
			Json["parameters"][ParameterName] = ValuesTime;
		}
	}
	
	std::ofstream Out(Filename);
	Out << Json.dump(1,'\t') << std::endl;
}

static void
ReadParametersFromJson(mobius_data_set *DataSet, const char *Filename)
{
	const mobius_model *Model = DataSet->Model;
	
	std::ifstream Ifs(Filename);
	nlohmann::json JData;
	Ifs >> JData;
	
	if(JData.find("index_sets") != JData.end())
	{
		for (nlohmann::json::iterator It = JData["index_sets"].begin(); It != JData["index_sets"].end(); ++It)
		{
			std::string IndexSetName = It.key();
			
			index_set_h IndexSet = GetIndexSetHandle(Model, IndexSetName.c_str());
			const index_set_spec &Spec = Model->IndexSetSpecs[IndexSet.Handle];
			
			if(Spec.Type == IndexSetType_Basic)
			{
				std::vector<std::string> IndexNames = It->get<std::vector<std::string>>();
				std::vector<token_string> IndexNames2;
				for(std::string &Str : IndexNames) IndexNames2.push_back(Str.c_str());
				
				SetIndexes(DataSet, IndexSetName.c_str(), IndexNames2);
			}
			else if(Spec.Type == IndexSetType_Branched)
			{
				std::vector<std::vector<std::string>> Indexes = It->get<std::vector<std::vector<std::string>>>();
				
				std::vector<std::pair<token_string, std::vector<token_string>>> Inputs;
				
				for(auto &Branch : Indexes)
				{
					std::vector<token_string> BranchInputs;
					token_string IndexName = Branch[0].c_str();
					for(size_t Idx = 1; Idx < Branch.size(); ++Idx)
					{
						BranchInputs.push_back(Branch[Idx].c_str());
					}
					Inputs.push_back({IndexName, BranchInputs});
				}
				
				SetBranchIndexes(DataSet, IndexSetName.c_str(), Inputs);
			}
		}
	}
	
	if(JData.find("parameters") != JData.end())
	{
		if(!DataSet->ParameterData) AllocateParameterStorage(DataSet);
		
		for (nlohmann::json::iterator It = JData["parameters"].begin(); It != JData["parameters"].end(); ++It)
		{
			std::string ParameterName = It.key();
			entity_handle ParameterHandle = GetParameterHandle(Model, ParameterName.c_str());
			std::vector<parameter_value> Values;
			const parameter_spec &Spec = Model->ParameterSpecs[ParameterHandle];
			
			if(Spec.ShouldNotBeExposed)
			{
				MOBIUS_FATAL_ERROR("ERROR: In file " << Filename << ". The parameter " << ParameterName << " is computed by the model, and should not be provided in a parameter file." << std::endl);
			}
			
			if(Spec.Type == ParameterType_Double)
			{
				std::vector<double> Val = It->get<std::vector<double>>();
				for(double D : Val)
				{
					parameter_value ParVal; ParVal.ValDouble = D;
					Values.push_back(ParVal);
				}
			}
			else if(Spec.Type == ParameterType_UInt)
			{
				std::vector<u64> Val = It->get<std::vector<u64>>();
				for(u64 D : Val)
				{
					parameter_value ParVal; ParVal.ValUInt = D;
					Values.push_back(ParVal);
				}
			}
			else if(Spec.Type == ParameterType_Bool)
			{
				std::vector<bool> Val = It->get<std::vector<bool>>();
				for(bool D : Val)
				{
					parameter_value ParVal; ParVal.ValBool = D;
					Values.push_back(ParVal);
				}
			}
			else if(Spec.Type == ParameterType_Time)
			{
				std::vector<std::string> Val = It->get<std::vector<std::string>>();
				for(std::string &D : Val)
				{
					bool ParseSuccess;
					parameter_value ParVal; ParVal.ValTime = datetime(D.c_str(), &ParseSuccess);
					
					//TODO: On failure..
					
					Values.push_back(ParVal);
				}
			}
			
			SetMultipleValuesForParameter(DataSet, ParameterHandle, Values.data(), Values.size());
		}
	}
}

#define MOBIUS_JSON_IO_CPP
#endif
