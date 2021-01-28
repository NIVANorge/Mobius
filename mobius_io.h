
static void
DlmWriteResultSeriesToFile(mobius_data_set *DataSet, const char *Filename, std::vector<const char *> ResultNames, const std::vector<std::vector<const char *>> &Indexes, char Delimiter)
{
	size_t WriteSize = DataSet->TimestepsLastRun;
	size_t NumSeries = ResultNames.size();
	
	double **ResultSeries = AllocClearedArray(double *, NumSeries);
	for(size_t Idx = 0; Idx < NumSeries; ++Idx)
	{
		ResultSeries[Idx] = AllocClearedArray(double, WriteSize);
		GetResultSeries(DataSet, ResultNames[Idx], Indexes[Idx], ResultSeries[Idx], WriteSize);
	}
	
	FILE *File = fopen(Filename, "w");
	if(!File)
		FatalError("Tried to open file \"", Filename, "\", but was not able to.\n");
	else
	{
		for(size_t Timestep = 0; Timestep < WriteSize; ++Timestep)
		{
			for(size_t Idx = 0; Idx < NumSeries; ++Idx)
			{
				fprintf(File, "%f", ResultSeries[Idx][Timestep]);
				if(Idx < NumSeries-1) fprintf(File, "%c", Delimiter);
			}
			
			fprintf(File, "\n");
		}
		fclose(File);
	}
	
	for(size_t Idx = 0; Idx < NumSeries; ++Idx) free(ResultSeries[Idx]);
	free(ResultSeries);	
}

static void
WriteParameterValue(FILE *File, parameter_value Value, const parameter_spec &Spec)
{
	switch(Spec.Type)
	{
		case ParameterType_Double:
		fprintf(File, "%.15g", Value.ValDouble);
		break;
		
		case ParameterType_Bool:
		fprintf(File, "%s", Value.ValBool ? "true" : "false");
		break;
		
		case ParameterType_UInt:
		fprintf(File, "%llu", (unsigned long long)(Value.ValUInt)); //TODO: check correctness. May depend on sizeof(long long unsigned int)==8
		break;
		
		case ParameterType_Time:
		fprintf(File, "%s", Value.ValTime.ToString());
		break;
		
		case ParameterType_Enum:
		fprintf(File, "%s", Spec.EnumNames[Value.ValUInt]);
	}
}

static void
WriteParameterValues(FILE *File, parameter_h Parameter, const parameter_spec &Spec, mobius_data_set *DataSet, const index_set_h *IndexSets, index_t *Indexes, size_t IndexSetCount, size_t Level)
{
	if(IndexSetCount == 0)
	{
		size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, Parameter);
		parameter_value Value = DataSet->ParameterData[Offset];
		WriteParameterValue(File, Value, Spec);
		return;
	}
	
	index_set_h IndexSet = IndexSets[Level];
	size_t IndexCount = DataSet->IndexCounts[IndexSet.Handle];
	
	for(index_t Index = {IndexSet, 0}; Index < IndexCount; ++Index)
	{
		Indexes[Level] = Index;
		if(Level + 1 == IndexSetCount)
		{
			size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, Indexes, IndexSetCount, DataSet->IndexCounts, Parameter);
			parameter_value Value = DataSet->ParameterData[Offset];
			WriteParameterValue(File, Value, Spec);
			if(Index + 1 != IndexCount) fprintf(File, " ");
		}
		else
		{
			WriteParameterValues(File, Parameter, Spec, DataSet, IndexSets, Indexes, IndexSetCount, Level + 1);
			if(Index + 1 != IndexCount)
			{
				if(Level + 2 == IndexSetCount) fprintf(File, "\n");
				else fprintf(File, "\n\n");
			}
		}
	}
}

static void
WriteParametersToFile(mobius_data_set *DataSet, const char *Filename)
{
	if(!DataSet->ParameterData)
		FatalError("ERROR: Tried to write parameters to a file before parameter data was allocated.\n");
	
	FILE *File;
#ifdef _WIN32
	std::u16string Filename16 = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(Filename);
	File = _wfopen((wchar_t *)Filename16.data(), L"w");
#else
	File = fopen(Filename, "w");
#endif

	if(!File)
		FatalError("ERROR: Tried to open file \"", Filename, "\", but was not able to.\"");
	
	const mobius_model *Model = DataSet->Model;
	
	fprintf(File, "# Parameter file generated for model %s, containing modules (", Model->Name);
	for(module_h Module : Model->Modules)
	{
		const module_spec &ModuleSpec = Model->Modules[Module];
		fprintf(File, "%s V%s", ModuleSpec.Name, ModuleSpec.Version);
		if(Module.Handle != Model->Modules.Count()-1) fprintf(File, ", ");
	}
	fprintf(File, ") ");
	
	//NOTE: put_time is not implemented before gcc version 5. Caused a problem for a user at one point, but I don't know if we really need to do this check anymore.
#if (defined(__GNUC__) && __GNUC__ >= 5) || !defined(__GNUC__)
	{
		auto T = std::time(nullptr);
		auto TM = *std::localtime(&T);
		std::stringstream Oss;
		Oss << std::put_time(&TM, "%Y-%m-%d %H:%M:%S");
		fprintf(File, "at %s", Oss.str().data());
	}
#endif
	fprintf(File, "\n\n");
	
	fprintf(File, "index_sets:\n");
	for(index_set_h IndexSet : Model->IndexSets)
	{
		const index_set_spec &Spec = Model->IndexSets[IndexSet];
		fprintf(File, "\"%s\" : {", Spec.Name);
		for(index_t IndexIndex = {IndexSet.Handle, 0}; IndexIndex < DataSet->IndexCounts[IndexSet.Handle]; ++IndexIndex)
		{
			if(Spec.Type == IndexSetType_Basic)
			{
				if(IndexIndex != 0) fprintf(File, " ");
				fprintf(File, "\"%s\"", DataSet->IndexNames[IndexSet.Handle][IndexIndex]);
			}
			else if(Spec.Type == IndexSetType_Branched)
			{
				if(IndexIndex != 0) fprintf(File, " ");
				size_t InputCount = DataSet->BranchInputs[IndexSet.Handle][IndexIndex].Count;
				if(InputCount > 0) fprintf(File, "{");
				fprintf(File, "\"%s\"", DataSet->IndexNames[IndexSet.Handle][IndexIndex]);
				for(size_t InputIdx = 0; InputIdx < InputCount; ++InputIdx)
				{
					index_t InputIndexIndex = DataSet->BranchInputs[IndexSet.Handle][IndexIndex][InputIdx];
					fprintf(File, " \"%s\"", DataSet->IndexNames[IndexSet.Handle][InputIndexIndex]);
				}
				if(InputCount > 0) fprintf(File, "}");
			}
			else
			{
				assert(0);
			}
		}
		fprintf(File, "}\n");
	}
	
	fprintf(File, "\nparameters:\n");

	for(entity_handle ModuleHandle = 0; ModuleHandle < Model->Modules.Count(); ++ModuleHandle)
	{
		if(ModuleHandle != 0)
		{
			//NOTE: For module-less parameters
			const module_spec &Module = Model->Modules.Specs[ModuleHandle];
			fprintf(File, "\n###################### %s V%s ######################\n", Module.Name, Module.Version);
		}
		
		for(parameter_group_h ParameterGroup : Model->ParameterGroups)
		{
			const parameter_group_spec &Group = Model->ParameterGroups[ParameterGroup];
			if(Group.Module.Handle == ModuleHandle)
			{
				fprintf(File, "\n# %s (", Group.Name);
				if(Group.IndexSets.size() == 0) fprintf(File, "no index sets");
				int Count = 0;
				for(index_set_h IndexSet : Group.IndexSets)
				{
					if(Count > 0) fprintf(File, " ");
					fprintf(File, "\"%s\"", GetName(Model, IndexSet));
					++Count;
				}
				fprintf(File, ") #\n\n");
				
				for(parameter_h Parameter : Group.Parameters)
				{
					const parameter_spec &Spec = Model->Parameters[Parameter];
					
					if(Spec.ShouldNotBeExposed) continue;
					
					fprintf(File, "\"%s\" :", Spec.Name);
					bool PrintedPnd = false;
					if(IsValid(Spec.Unit))
					{
						fprintf(File, "     #(%s)", GetName(Model, Spec.Unit));
						PrintedPnd = true;
					}
					if(Spec.Type != ParameterType_Bool && Spec.Type != ParameterType_Enum)
					{
						if(!PrintedPnd) fprintf(File, "     #");
						PrintedPnd = true;
						fprintf(File, " [");
						WriteParameterValue(File, Spec.Min, Spec);
						fprintf(File, ", ");
						WriteParameterValue(File, Spec.Max, Spec);
						fprintf(File, "]");
					}
					else if(Spec.Type == ParameterType_Enum)
					{
						if(!PrintedPnd) fprintf(File, "     #");
						PrintedPnd = true;
						fprintf(File, " [");
						int Idx = 0;
						for(token_string EnumName : Spec.EnumNames)
						{
							fprintf(File, "%s", EnumName.Data);
							if(Idx != Spec.EnumNames.size()-1) fprintf(File, ", ");
							++Idx;
						}
						fprintf(File, "]");
					}
					
					if(Spec.Description)
					{
						if(!PrintedPnd) fprintf(File, "     #");
						fprintf(File, " %s", Spec.Description);
					}
					fprintf(File, "\n");
					
					size_t IndexSetCount = Group.IndexSets.size();
					index_t *CurrentIndexes = AllocClearedArray(index_t, IndexSetCount);
					
					WriteParameterValues(File, Parameter, Spec, DataSet, Group.IndexSets.data(), CurrentIndexes, IndexSetCount, 0);
					
					fprintf(File, "\n\n");
					free(CurrentIndexes);
				}
			}
		}
		
	}
/*
	for(storage_unit_specifier &Unit : DataSet->ParameterStorageStructure.Units)
	{
		array<index_set_h> &IndexSets = Unit.IndexSets;
		fprintf(File, "######################");
		if(IndexSets.Count == 0) fprintf(File, " (no index sets)");
		for(index_set_h IndexSet : IndexSets)
		{
			fprintf(File, " \"%s\"", GetName(Model, IndexSet));
		}
		fprintf(File, " ######################\n");
		
		for(entity_handle ParameterHandle : Unit.Handles)
		{
			const parameter_spec &Spec = Model->Parameters.Specs[ParameterHandle];
			
			if(Spec.ShouldNotBeExposed) continue;
			
			fprintf(File, "\"%s\" :", Spec.Name);
			bool PrintedPnd = false;
			if(IsValid(Spec.Unit))
			{
				fprintf(File, "     #(%s)", GetName(Model, Spec.Unit));
				PrintedPnd = true;
			}
			if(Spec.Type != ParameterType_Bool)
			{
				if(!PrintedPnd) fprintf(File, "     #");
				PrintedPnd = true;
				fprintf(File, " [");
				WriteParameterValue(File, Spec.Min, Spec.Type);
				fprintf(File, ", ");
				WriteParameterValue(File, Spec.Max, Spec.Type);
				fprintf(File, "]");
			}
			if(Spec.Description)
			{
				if(!PrintedPnd) fprintf(File, "     #");
				fprintf(File, " %s", Spec.Description);
			}
			fprintf(File, "\n");
			
			size_t IndexSetCount = IndexSets.Count;
			index_t *CurrentIndexes = AllocClearedArray(index_t, IndexSetCount);
			
			WriteParameterValues(File, ParameterHandle, Spec.Type, DataSet, IndexSets.Data, CurrentIndexes, IndexSetCount, 0);
			
			fprintf(File, "\n\n");
			free(CurrentIndexes);
		}
	}
*/
	fclose(File);
}

static void
ReadParametersFromFile(mobius_data_set *DataSet, const char *Filename, bool IgnoreUnknown=false)
{
	token_stream Stream(Filename);
	
	const mobius_model *Model = DataSet->Model;
	
	while(true)
	{
		if(!DataSet->ParameterData && DataSet->AllIndexesHaveBeenSet)
		{
			AllocateParameterStorage(DataSet);
		}
		
		token Token = Stream.PeekToken();
		
		if(Token.Type == TokenType_EOF)
			break;
		
		token_string Section = Stream.ExpectUnquotedString();
		Stream.ExpectToken(TokenType_Colon);
		
		int Mode = -1;

		if(Section.Equals("index_sets"))
		{
			Mode = 0;
		}
		else if(Section.Equals("parameters"))
		{
			Mode = 1;
		}
		else
		{
			Stream.PrintErrorHeader();
			FatalError("Parameter file parser does not recognize section type: ", Section, ".\n");
		}
		
		if(Mode == 0)
		{
			while(true)
			{
				token Token = Stream.PeekToken();
				if(Token.Type != TokenType_QuotedString) break;
				
				token_string IndexSetName = Stream.ExpectQuotedString();
				bool Found;
				index_set_h IndexSet = GetIndexSetHandle(Model, IndexSetName, Found);
				if(!Found)
				{
					Stream.PrintErrorHeader();
					FatalError("The index set \"", IndexSetName, "\" was not registered with the model.\n");
				}
				Stream.ExpectToken(TokenType_Colon);
				if(Model->IndexSets[IndexSet].Type == IndexSetType_Basic)
				{
					std::vector<token_string> Indexes;
					Stream.ReadQuotedStringList(Indexes);
					SetIndexes(DataSet, IndexSetName, Indexes);
				}
				else if(Model->IndexSets[IndexSet].Type == IndexSetType_Branched)
				{
					//TODO: Make a helper function for this too!
					std::vector<std::pair<token_string, std::vector<token_string>>> Indexes;
					Stream.ExpectToken(TokenType_OpenBrace);
					while(true)
					{
						Token = Stream.ReadToken();
						if(Token.Type == TokenType_CloseBrace)
						{
							if(Indexes.empty())
							{
								Stream.PrintErrorHeader();
								FatalError("Expected one or more indexes for index set ", IndexSetName, ".\n");
							}
							else
								SetBranchIndexes(DataSet, IndexSetName, Indexes);
							break;
						}				
						else if(Token.Type == TokenType_QuotedString)
							Indexes.push_back({Token.StringValue, {}});
						else if(Token.Type == TokenType_OpenBrace)
						{
							token_string IndexName;
							std::vector<token_string> Inputs;
							while(true)
							{
								Token = Stream.ReadToken();
								if(Token.Type == TokenType_CloseBrace)
								{
									if(!IndexName.Data || Inputs.empty())
									{
										Stream.PrintErrorHeader();
										FatalError("No inputs in the braced list for one of the indexes of index set \"", IndexSetName, "\".\n");
									}
									break;
								}
								else if(Token.Type == TokenType_QuotedString)
								{
									if(!IndexName.Data) IndexName = Token.StringValue;
									else Inputs.push_back(Token.StringValue);
								}
								else
								{
									Stream.PrintErrorHeader();
									FatalError("Expected either the quoted name of an index or a }.\n");
								}
							}
							Indexes.push_back({IndexName, Inputs});
						}
						else
						{
							Stream.PrintErrorHeader();
							FatalError("Expected either the quoted name of an index or a }.\n");
						}
					}
				}
			}
		}
		else if(Mode == 1)
		{
			if(!DataSet->ParameterData) //TODO: This is probably not necessary anymore..
				AllocateParameterStorage(DataSet);
			
			while(true)
			{
				Token = Stream.PeekToken();
				if(Token.Type != TokenType_QuotedString) break;
					
				token_string ParameterName = Stream.ExpectQuotedString();
				Stream.ExpectToken(TokenType_Colon);
				
				bool Found;
				parameter_h Parameter = GetParameterHandle(Model, ParameterName, Found);
				if(!Found)
				{
					if(!IgnoreUnknown)
					{
						Stream.PrintErrorHeader();
						FatalError("The parameter \"", ParameterName, "\" was not registered with the model.\n");
					}
					else
					{
						while(true) // Just skip through
						{
							Token = Stream.PeekToken();
							if(Token.Type == TokenType_QuotedString) break;
							Stream.ReadToken();
						}
					}
				}
				else
				{
					const parameter_spec &Spec = Model->Parameters[Parameter];
					
					if(Spec.ShouldNotBeExposed)
					{
						Stream.PrintErrorHeader();
						FatalError("The parameter \"", ParameterName, "\" is computed by the model, and should not be provided in a parameter file.\n");
					}
					
					parameter_type Type = Spec.Type;
					size_t ExpectedCount = 1;
					size_t UnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[Parameter.Handle];
					for(index_set_h IndexSet : DataSet->ParameterStorageStructure.Units[UnitIndex].IndexSets)
						ExpectedCount *= DataSet->IndexCounts[IndexSet.Handle];

					std::vector<parameter_value> Values;
					Values.reserve(ExpectedCount);
					Stream.ReadParameterSeries(Values, Spec);
					if(Values.size() != ExpectedCount)                                                                   
					{                                                                                                    
						Stream.PrintErrorHeader();                                                                       
						FatalError("Did not get the expected number of values for parameter \"", ParameterName, "\". Got ", Values.size(), ", expected ", ExpectedCount, ".\n"); 
					}                                                                                                    
					SetMultipleValuesForParameter(DataSet, Parameter, Values.data(), Values.size());
				}
			}
		}
	}
}


static void
FillConstantInputValues(mobius_data_set *DataSet, double *Base, double Value, s64 BeginTimestep, s64 EndTimestep)
{
	if(BeginTimestep < 0) BeginTimestep = 0;
	if(EndTimestep >= (s64)DataSet->InputDataTimesteps) EndTimestep = (s64)DataSet->InputDataTimesteps - 1;
	
	size_t Stride = DataSet->InputStorageStructure.TotalCount;
	double *WriteTo = Base + Stride*BeginTimestep;
	for(s64 Timestep = BeginTimestep; Timestep <= EndTimestep; ++Timestep)
	{
		*WriteTo = Value;
		WriteTo += Stride;
	}
}

static void
LinearInterpolateInputValues(mobius_data_set *DataSet, double *Base, double FirstValue, double LastValue, datetime FirstDate, datetime LastDate)
{
	size_t Stride = DataSet->InputStorageStructure.TotalCount;
	
	expanded_datetime Date(FirstDate, DataSet->Model->TimestepSize);
	
	double XRange = (double)(LastDate.SecondsSinceEpoch - FirstDate.SecondsSinceEpoch);
	double YRange = LastValue - FirstValue;
	
	s64 Step     = FindTimestep(DataSet->InputDataStartDate, FirstDate, DataSet->Model->TimestepSize);
	s64 LastStep = FindTimestep(DataSet->InputDataStartDate, LastDate, DataSet->Model->TimestepSize);
	LastStep = Min(LastStep, DataSet->InputDataTimesteps);
	
	double *WriteTo = Base + Step*Stride;
	while(Step <= LastStep)
	{
		if(Step >= 0)
		{
			double XX = (double)(Date.DateTime.SecondsSinceEpoch - FirstDate.SecondsSinceEpoch) / XRange;
			double Value = FirstValue + XX*YRange;
			*WriteTo = Value;
		}
		
		Date.Advance();
		WriteTo += Stride;
		++Step;
	}
}

static void
ReadInputSeries(mobius_data_set *DataSet, token_stream &Stream)
{
	const mobius_model *Model = DataSet->Model;
	
	while(true)
	{
		token Token = Stream.ReadToken();
		if(Token.Type == TokenType_EOF)
		{
			return;
		}
		
		if(Token.Type == TokenType_UnquotedString)
		{
			if(Token.StringValue.Equals("include_file"))
			{
				token_string Filename = Stream.ExpectQuotedString();
				
				//NOTE: The file path is given relatively to the current file, so we have to add any path of the current one in front.
				
				const char *ParentPath = Stream.Filename;
				int LastSlash;
				bool AnySlashAtAll = false;
				for(LastSlash = strlen(ParentPath) - 1; LastSlash >= 0; --LastSlash)
				{
					char C = ParentPath[LastSlash];
					if(C == '\\' || C == '/')
					{
						AnySlashAtAll = true;
						break;
					}
				}
				if(!AnySlashAtAll) LastSlash = -1;
				
				char NewPath[512]; //Umm, hope this is plenty???
				sprintf(NewPath, "%.*s%.*s", LastSlash+1, ParentPath, (int)Filename.Length, Filename.Data);
				
				token_stream SubStream(NewPath);
				
				ReadInputSeries(DataSet, SubStream);
				
				continue;
			}
			else
			{
				Stream.PrintErrorHeader();
				FatalError("Unexpected command word ", Token.StringValue, ".\n");
			}
		}
		else if(Token.Type != TokenType_QuotedString)
		{
			Stream.PrintErrorHeader();
			FatalError("Expected the quoted name of an input or an include_file directive.\n");
		}
		
		token_string InputName = Token.StringValue;
		
		bool Found;
		input_h Input = GetInputHandle(Model, InputName, Found);
		if(!Found)
		{
			Stream.PrintErrorHeader();
			FatalError("The input \"", InputName, "\" was not registered with the model.\n");
		}
		
		std::vector<size_t> Offsets;
		
		const std::vector<index_set_h> &IndexSets = Model->Inputs[Input].IndexSetDependencies;
		
		while(true)
		{
			Token = Stream.PeekToken();
			
			if(Token.Type == TokenType_OpenBrace)
			{
				std::vector<token_string> IndexNames;
				
				Stream.ReadQuotedStringList(IndexNames);
				
				if(IndexNames.size() != IndexSets.size())
				{
					Stream.PrintErrorHeader();
					FatalError("Did not get the right amount of indexes for input \"", InputName, "\". Got ", IndexNames.size(), ", expected ", IndexSets.size(), ".\n");
				}
				index_t Indexes[256]; //This could cause a buffer overflow, but will not do so in practice.
				for(size_t IdxIdx = 0; IdxIdx < IndexNames.size(); ++IdxIdx)
				{
					bool Found;
					Indexes[IdxIdx] = GetIndex(DataSet, IndexSets[IdxIdx], IndexNames[IdxIdx], Found);
					if(!Found)
					{
						Stream.PrintErrorHeader();
						FatalError("The index \"", IndexNames[IdxIdx], "\" was not registered with the index set \"", GetName(Model, IndexSets[IdxIdx]), "\".\n");
					}
				}
				
				size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, Indexes, IndexSets.size(), DataSet->IndexCounts, Input);
				
				Offsets.push_back(Offset);
				
				if(DataSet->InputTimeseriesWasProvided[Offset])
				{
					WarningPrint("WARNING: The input time series \"", InputName, "\" with indexes {");
					size_t Idx = 0;
					for(token_string IndexName : IndexNames)
					{
						WarningPrint("\"", IndexName, "\"");
						if(Idx != IndexNames.size()-1) WarningPrint(", ");
						++Idx;
					}
					WarningPrint("} was provided more than once. The last provided series will override the others.\n");
				}
			}
			else
				break;
		}
		
		if(Offsets.empty())
		{
			if(IndexSets.size() > 0)
			{
				Stream.PrintErrorHeader();
				FatalError("Did not get the right amount of indexes for input \"", InputName, "\". Got 0, expected ", IndexSets.size(), ".\n");
			}
			size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, Input);
			Offsets.push_back(Offset);
			if(DataSet->InputTimeseriesWasProvided[Offset])
				WarningPrint("WARNING: The input time series \"", InputName, "\" was provided more than once. The last provided series will override the others.\n");
		}
		
		bool LinearInterpolate = false;
		bool InterpInsideOnly = false;
		bool RepeatYearly = false;
		Token = Stream.PeekToken();
		while(Token.Type == TokenType_UnquotedString)
		{
			if(Token.StringValue.Equals("linear_interpolate"))
			{
				LinearInterpolate = true;
				Stream.ReadToken(); // Consume it to position correctly for the rest of the routine
			}
			else if(Token.StringValue.Equals("linear_interpolate_inside"))
			{
				LinearInterpolate = true;
				InterpInsideOnly  = true;
				Stream.ReadToken(); // Consume it to position correctly for the rest of the routine
			}
			else if(Token.StringValue.Equals("repeat_yearly"))
			{
				RepeatYearly = true;
				Stream.ReadToken(); // Consume it to position correctly for the rest of the routine
			}
			else
			{
				Stream.PrintErrorHeader();
				FatalError("unexpected command word ", Token.StringValue, ".\n");
			}
			Token = Stream.PeekToken();
		}
		
		Stream.ExpectToken(TokenType_Colon);
		
		for(size_t Offset : Offsets)
			DataSet->InputTimeseriesWasProvided[Offset] = true;
		
		//NOTE: This reads the actual data after the header.
		
		//NOTE: For the first timestep, try to figure out what format the data was provided in.
		int FormatType = -1;
		Token = Stream.PeekToken();
		
		u64 Timesteps = DataSet->InputDataTimesteps;
		
		if(Token.Type == TokenType_Numeric)
			FormatType = 0;
		else if(Token.Type == TokenType_Date)
			FormatType = 1;
		else
		{
			Stream.PrintErrorHeader();
			FatalError("Inputs are to be provided either as a series of numbers or a series of dates (or date ranges) together with numbers.\n");
		}
		
		/*   NOTE: Clearing to NaN is now done inside AllocateInputStorage
		if(Model->Inputs[Input].ClearToNaN)
		{
			//NOTE: Additional timeseries are by default cleared to NaN instead of 0.
			//TODO: This makes us only clear the ones that are provided in the file... Whe should really also clear ones that are not provided..
			for(size_t Offset : Offsets)
			{
				double *Base = DataSet->InputData + Offset;
				FillConstantInputValues(DataSet, Base, std::numeric_limits<double>::quiet_NaN(), 0, (s64)Timesteps-1);
			}
		}
		*/
		
		if(!LinearInterpolate)
		{
			if(FormatType == 0)
			{
				for(u64 Timestep = 0; Timestep < Timesteps; ++Timestep)
				{
					//double Value = Stream.ExpectDouble();
					Token = Stream.ReadToken();
					if(Token.Type != TokenType_Numeric)
					{
						Stream.PrintErrorHeader();
						FatalError("Only got ", Timestep, " values for series. Expected ", Timesteps, ".\n");
					}
					double Value = Token.DoubleValue;
					
					for(size_t Offset : Offsets)
					{
						double *WriteTo = DataSet->InputData + Offset + Timestep*DataSet->InputStorageStructure.TotalCount;
						*WriteTo = Value;
					}
				}
			}
			else //FormatType == 1
			{
				datetime StartDate = DataSet->InputDataStartDate;
				
				while(true)
				{
					s64 CurTimestep;
					
					token Token = Stream.PeekToken();
					
					if(Token.Type == TokenType_Date)
					{
						datetime Date = Stream.ExpectDateTime();
						CurTimestep = FindTimestep(StartDate, Date, Model->TimestepSize);
					}
					else if(Token.Type == TokenType_QuotedString || Token.Type == TokenType_EOF)
						break;
					else
					{
						Stream.PrintErrorHeader();
						FatalError("Expected either a date or the beginning of a new input series.\n");
					}
					
					Token = Stream.PeekToken();
					if(Token.Type == TokenType_UnquotedString)
					{
						Stream.ReadToken();
						if(!Token.StringValue.Equals("to"))
						{
							Stream.PrintErrorHeader();
							FatalError("Expected either a 'to' or a number.");
						}
						datetime EndDateRange = Stream.ExpectDateTime();
						s64 EndTimestepRange = FindTimestep(StartDate, EndDateRange, Model->TimestepSize);
						//s64 EndTimestepRange = StartDate.DaysUntil(EndDateRange); //NOTE: Only one-day timesteps currently supported.
						
						if(EndTimestepRange < CurTimestep)
						{
							Stream.PrintErrorHeader();
							FatalError("The end of the date range is earlier than the beginning.\n");
						}
						
						double Value = Stream.ExpectDouble();
						
						for(size_t Offset : Offsets)
						{
							double *Base = DataSet->InputData + Offset;
							FillConstantInputValues(DataSet, Base, Value, CurTimestep, EndTimestepRange);
						}
					}
					else if(Token.Type == TokenType_Numeric)
					{
						double Value = Stream.ExpectDouble();
						if(CurTimestep >= 0 && CurTimestep < (s64)Timesteps)
						{
							for(size_t Offset : Offsets)
							{
								double *WriteTo = DataSet->InputData + Offset + CurTimestep*DataSet->InputStorageStructure.TotalCount;
								*WriteTo = Value;
							}
						}
					}
					else
					{
						Stream.PrintErrorHeader();
						FatalError("Expected either a 'to' or a number.");
					}
				}
			}
		}
		else  // if LinearInterpolate
		{
			if(FormatType != 1)
			{
				Stream.PrintErrorHeader();
				FatalError("when linear interpolation is specified, the input format has to be: date (time) value.\n");
			}
			
			datetime PrevDate = DataSet->InputDataStartDate;
			double PrevValue = 0.0;
			bool AtBeginning = true;
			
			while(true)
			{
				datetime CurDate;
				
				token Token = Stream.PeekToken();
				if(Token.Type == TokenType_Date)
					CurDate = Stream.ExpectDateTime();
				else if(Token.Type == TokenType_QuotedString || Token.Type == TokenType_EOF)
				{
					if(!InterpInsideOnly)
					{
						//NOTE: Fill the rest of the time series with the last value read
						for(size_t Offset : Offsets)
						{
							double *Base = DataSet->InputData + Offset;
							s64 BeginTimestep = FindTimestep(DataSet->InputDataStartDate, PrevDate, Model->TimestepSize);
							s64 EndTimestep = (s64)DataSet->InputDataTimesteps-1;
							FillConstantInputValues(DataSet, Base, PrevValue, BeginTimestep, EndTimestep);
						}
					}
					
					break;
				}
				else
				{
					Stream.PrintErrorHeader();
					FatalError("Expected either a date or the beginning of a new input series.\n");
				}
				
				double Value = Stream.ExpectDouble();
				
				if(AtBeginning)
				{
					PrevValue = Value;
					if(CurDate < PrevDate)
					{
						//NOTE: The first given date is before the start of the input series
						PrevDate = CurDate;
						AtBeginning = false;
						continue;
					}
				}
				
				if(CurDate < PrevDate)
				{
					Stream.PrintErrorHeader();
					FatalError("in linear interpolation mode, the dates have to be sequential.\n");
				}

				if(!AtBeginning || !InterpInsideOnly)    //If we interpolate inside given values only, we don't fill any values at the beginning
				{
					for(size_t Offset : Offsets)
					{
						double *Base = DataSet->InputData + Offset;
						LinearInterpolateInputValues(DataSet, Base, PrevValue, Value, PrevDate, CurDate);
					}
				}
				
				PrevValue = Value;
				PrevDate  = CurDate;
				AtBeginning = false;
			}
		}
		
		if(RepeatYearly)
		{
			//NOTE: Copy the first year of the timeseries repeatedly for the rest of the allocated space.
			//TODO: This behaves kind of weirdly when you have sub-monthly timesteps, esp. when there are leap years.. Also if the timestep does not divide a year.
			
			//Important note! This code may be volatile if we later add Timestep_Year and handle that differently to Timestep_Month!
			
			timestep_size ModelStep = Model->TimestepSize;
			if(ModelStep.Unit == Timestep_Month && (ModelStep.Magnitude >= 12  || 12 % ModelStep.Magnitude != 0))
			{
				Stream.PrintErrorHeader();
				FatalError("yearly repetition is only available for models with a step size that is less than a year, and where the number of months in each step divides 12.\n");
			}
			
			datetime BeginDate = DataSet->InputDataStartDate;
			timestep_size YearStep = {Timestep_Month, 12};
			
			for(size_t Offset : Offsets)  //TODO: It is not the most efficient to have this as an outer loop..
			{
				expanded_datetime Year(BeginDate, YearStep);
				
				double *Base = DataSet->InputData + Offset;
				
				bool Finished = false;
				while(true)
				{
					expanded_datetime CurDate(Year.DateTime, ModelStep);
					
					size_t YearStartTimestep = FindTimestep(DataSet->InputDataStartDate, Year.DateTime, ModelStep);
					double *ReadValue  = Base;
					double *WriteValue = Base + DataSet->InputStorageStructure.TotalCount*YearStartTimestep;

					Year.Advance();
					size_t YearEnd = FindTimestep(DataSet->InputDataStartDate, Year.DateTime, ModelStep);
					
					while(true)
					{
						double Value = *ReadValue;
						ReadValue += DataSet->InputStorageStructure.TotalCount;
						*WriteValue = Value;
						WriteValue += DataSet->InputStorageStructure.TotalCount;
						
						size_t Timestep = FindTimestep(DataSet->InputDataStartDate, CurDate.DateTime, ModelStep);
						Finished = (Timestep >= DataSet->InputDataTimesteps-1);
						if(Finished) break;
						if(Timestep >= YearEnd-1) break;
						
						CurDate.Advance();
					}
					if(Finished) break;
				}
			}
		}  //if RepeatYearly
	}
}

static void
ReadInputsFromFile(mobius_data_set *DataSet, const char *Filename)
{
	const mobius_model *Model = DataSet->Model;
	
	token_stream Stream(Filename);
	
	u64 Timesteps = 0;
	
	while(true)
	{
		token Token = Stream.PeekToken();
		
		if(Token.Type == TokenType_EOF)
		{
			Stream.PrintErrorHeader();
			FatalError("Expected one of the code words timesteps, start_date, inputs, additional_timeseries or index_set_dependencies.\n");
		}
		
		token_string Section = Stream.ExpectUnquotedString();

		if(Section.Equals("timesteps"))
		{
			//TODO: Guard against both 'timesteps' and 'end_date' being set?
			Stream.ExpectToken(TokenType_Colon);
			Timesteps = Stream.ExpectUInt();
		}
		else if(Section.Equals("start_date"))
		{
			Stream.ExpectToken(TokenType_Colon);
			DataSet->InputDataStartDate = Stream.ExpectDateTime();
			DataSet->InputDataHasSeparateStartDate = true;
		}
		else if(Section.Equals("end_date"))
		{
			if(!DataSet->InputDataHasSeparateStartDate)
			{
				Stream.PrintErrorHeader();
				FatalError("The start date has to be provided before the end date.\n");
			}
			Stream.ExpectToken(TokenType_Colon);
			datetime EndDate = Stream.ExpectDateTime();
			s64 Step = FindTimestep(DataSet->InputDataStartDate, EndDate, Model->TimestepSize);
			Step += 1;    //NOTE: Because the end date is inclusive. 
			if(Step <= 0)
			{
				Stream.PrintErrorHeader();
				FatalError("The input data end date was set to be earlier than the input data start date.\n");
			}
			Timesteps = (u64)Step;
		}
		else if(Section.Equals("inputs"))
		{
			Stream.ExpectToken(TokenType_Colon);
			break;
		}
		else if(Section.Equals("index_set_dependencies") || Section.Equals("additional_timeseries"))
		{
			//NOTE: These are handled in a separate call, so we have to skip through them here.
			while(true)
			{
				Token = Stream.PeekToken();
				if(Token.Type == TokenType_UnquotedString && !Token.StringValue.Equals("unit")) break; //We hit a new section;
				Stream.ReadToken(); //Otherwise consume the token and ignore it.
			}
		}
		else
		{
			Stream.PrintErrorHeader();
			FatalError("Input file parser does not recognize section type: ", Section, ".\n");
		}
	}
	
	if(Timesteps == 0)
		FatalError("ERROR: Timesteps in the input file ", Filename, " is either not provided or set to 0.\n");
	
	AllocateInputStorage(DataSet, Timesteps);
	
	if(!DataSet->InputDataHasSeparateStartDate)
		DataSet->InputDataStartDate = GetStartDate(DataSet); //NOTE: This reads the "Start date" parameter.
	
	ReadInputSeries(DataSet, Stream);
}


static void
ReadInputDependenciesFromFile(mobius_model *Model, const char *Filename)
{
	if(!Filename || strlen(Filename)==0) return;
	
	token_stream Stream(Filename);

	while(true)
	{
		token Token = Stream.PeekToken();
		if(Token.Type == TokenType_EOF)
			break;
		
		token_string Section = Stream.ExpectUnquotedString();
		Stream.ExpectToken(TokenType_Colon);

		if(Section.Equals("index_set_dependencies"))
		{
			while(true)
			{
				token Token = Stream.PeekToken();
				if(Token.Type == TokenType_QuotedString)
				{
					Token = Stream.ReadToken();
					token_string InputName = Token.StringValue;
					bool Found;
					input_h Input = GetInputHandle(Model, InputName, Found);
					if(!Found)
					{
						Stream.PrintErrorHeader();
						FatalError("The input \"", InputName, "\" was not registered with the model.\n");
					}
					
					std::vector<index_set_h> &IndexSets = Model->Inputs[Input].IndexSetDependencies;
					if(!IndexSets.empty()) //TODO: OR we could just clear it and give a warning..
					{
						Stream.PrintErrorHeader();
						FatalError("Tried to set index set dependencies for the input ", InputName, " for a second time.\n");
					}
					Stream.ExpectToken(TokenType_Colon);
					
					std::vector<token_string> IndexSetNames;
					Stream.ReadQuotedStringList(IndexSetNames);
					
					for(token_string IndexSetName : IndexSetNames)
					{
						bool Found;
						index_set_h IndexSetHandle = GetIndexSetHandle(Model, IndexSetName, Found);
						if(!Found)
						{
							Stream.PrintErrorHeader();
							FatalError("The index set \"", IndexSetName, "\" was not registered with the model.\n");
						}
						IndexSets.push_back(IndexSetHandle);
					}
				}
				else break;
			}
		}
		else if(Section.Equals("additional_timeseries"))
		{
			while(true)
			{
				token Token = Stream.PeekToken();
				if(Token.Type == TokenType_QuotedString)
				{
					Token = Stream.ReadToken();
					token_string InputName = Token.StringValue.Copy(&Model->BucketMemory);
					
					unit_h Unit = {0};
					Token = Stream.PeekToken();
					if(Token.Type == TokenType_UnquotedString)
					{
						if(Token.StringValue.Equals("unit"))
						{
							Stream.ReadToken();
							token_string UnitName = Stream.ExpectQuotedString().Copy(&Model->BucketMemory);
							Unit = RegisterUnit(Model, UnitName.Data);
						}
					}
					RegisterInput(Model, InputName.Data, Unit, true, true);
				}
				else break;
			}
		}
		else if(Section.Equals("inputs"))
		{
			//NOTE: "index_set_dependencies" and "additional_timeseries" are assumed to come before "inputs" in the file.
			// This is so that we don't have to skip through the entire inputs section on this read since it can be quite long.
			break;
		}
		else
		{
			//NOTE: We have to skip through other sections that are not relevant for this reading
			while(true)
			{
				Token = Stream.PeekToken();
				
				if(Token.Type == TokenType_EOF) return;
				
				if(Token.Type == TokenType_UnquotedString) break; //We hit a new section;
				Stream.ReadToken(); //Otherwise consume the token and ignore it.
			}
		}
	}
}
