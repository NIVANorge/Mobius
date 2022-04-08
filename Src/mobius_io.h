
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
	
	FILE *File = OpenFile(Filename, "w");
	if(!File)
		FatalError("ERROR: Tried to open file \"", Filename, "\", but was not able to.\n");
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
WriteParameterValues(FILE *File, parameter_h Parameter, const parameter_spec &Spec, mobius_data_set *DataSet, const index_set_h *IndexSets, index_t *Indexes, size_t IndexSetCount, size_t Level, const char *Indent)
{
	
	if(IndexSetCount == 0)
	{
		fprintf(File, "%s", Indent);
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
			if(Index == 0) fprintf(File, "%s", Indent);
			size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, Indexes, IndexSetCount, DataSet->IndexCounts, Parameter);
			parameter_value Value = DataSet->ParameterData[Offset];
			WriteParameterValue(File, Value, Spec);
			if(Index + 1 != IndexCount) fprintf(File, " ");
		}
		else
		{
			WriteParameterValues(File, Parameter, Spec, DataSet, IndexSets, Indexes, IndexSetCount, Level + 1, Indent);
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
	
	FILE *File = OpenFile(Filename, "w");

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
		const char *Indent = "";
		
		if(ModuleHandle != 0)
		{
			const module_spec &Module = Model->Modules.Specs[ModuleHandle];
			fprintf(File, "\n{ module \"%s\" version \"%s\"\n", Module.Name, Module.Version);
			
			Indent = "\t";
		}
		
		for(parameter_group_h ParameterGroup : Model->ParameterGroups)
		{
			const parameter_group_spec &Group = Model->ParameterGroups[ParameterGroup];
			if(Group.Module.Handle == ModuleHandle)
			{
				fprintf(File, "\n%s# %s (", Indent, Group.Name);
				if(Group.IndexSets.size() == 0) fprintf(File, "no index sets");
				int Count = 0;
				for(index_set_h IndexSet : Group.IndexSets)
				{
					if(Count > 0) fprintf(File, " ");
					fprintf(File, "\"%s\"", GetName(Model, IndexSet));
					++Count;
				}
				fprintf(File, ") #\n\n");
				
				for(parameter_h Parameter : Model->Parameters)
				{
					const parameter_spec &Spec = Model->Parameters[Parameter];
					
					if(Spec.Group != ParameterGroup) continue;
					if(Spec.ShouldNotBeExposed) continue;
					
					fprintf(File, "%s\"%s\" :", Indent, Spec.Name);
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
					
					WriteParameterValues(File, Parameter, Spec, DataSet, Group.IndexSets.data(), CurrentIndexes, IndexSetCount, 0, Indent);
					
					fprintf(File, "\n\n");
					free(CurrentIndexes);
				}
			}
		}
		
		if(ModuleHandle != 0)
		{
			const module_spec &Module = Model->Modules.Specs[ModuleHandle];
			fprintf(File, "\n} # end of module \"%s\"\n", Module.Name);
		}
		
	}

	fclose(File);
}

static bool
CouldBeParameterValue(token_type Type)
{
	return IsNumeric(Type) || (Type == TokenType_Bool) || (Type == TokenType_Date) || (Type == TokenType_Identifier);
}

static void
ReadParametersFromFile(mobius_data_set *DataSet, const char *Filename, bool IgnoreUnknown=false)
{
	token_stream Stream(Filename);
	
	const mobius_model *Model = DataSet->Model;
	
	while(true)
	{
		if(!DataSet->ParameterData && DataSet->AllIndexesHaveBeenSet)
			AllocateParameterStorage(DataSet);
		
		token Token = Stream.PeekToken();
		
		if(Token.Type == TokenType_EOF)
			break;
		
		token_string Section = Stream.ExpectIdentifier();
		
		int Mode = -1;

		if(Section.Equals("index_sets"))
			Mode = 0;
		else if(Section.Equals("parameters"))
			Mode = 1;
		else
		{
			Stream.PrintErrorHeader();
			FatalError("The parameter file parser does not recognize section type: ", Section, ". Accepted sections are index_sets and parameters\n");
		}
		
		Stream.ExpectToken(':');
		
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
				Stream.ExpectToken(':');
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
					Stream.ExpectToken('{');
					while(true)
					{
						Token = Stream.ReadToken();
						if(Token.Type == '}')
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
						else if(Token.Type == '{')
						{
							token_string IndexName;
							std::vector<token_string> Inputs;
							while(true)
							{
								Token = Stream.ReadToken();
								if(Token.Type == '}')
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
			
			module_h CurrentModule = {};
			bool ValidModule = true;
			bool ValidVersion = true;
			token_string ModuleName;
			token_string ModuleVersion;
			
			while(true)
			{
				Token = Stream.PeekToken();
				
				if(Token.Type == '{')
				{
					Stream.ReadToken(); // Consume the open brace
					token_string ModuleDecl = Stream.ExpectIdentifier();
					if(!ModuleDecl.Equals("module"))
					{
						Stream.PrintErrorHeader();
						FatalError("Expected an identifier saying \"module\".\n");
					}
					ModuleName = Stream.ExpectQuotedString();
					if(Model->Modules.Has(ModuleName))
					{
						CurrentModule = GetModuleHandle(Model, ModuleName);
						ValidModule = true;
					}
					else
					{
						// Issue warning?
						ValidModule = false;
						ValidVersion = true; //Just to not confuse error checking below
					}
					token_string VersionDecl = Stream.ExpectIdentifier();
					if(!VersionDecl.Equals("version"))
					{
						Stream.PrintErrorHeader();
						FatalError("Expected an identifier saying \"version\".\n");
					}
					ModuleVersion = Stream.ExpectQuotedString();
					if(ValidModule)
					{
						const module_spec &ModuleSpec = Model->Modules[CurrentModule];
						if (ModuleVersion.Equals(ModuleSpec.Version))
							ValidVersion = true;
						else
							ValidVersion = false;
					}
				}
				else if(Token.Type == '}')
				{
					Stream.ReadToken(); // Consume the close brace
					CurrentModule = {};
					ValidModule = true;
					ValidVersion = true;
					continue;
				}
				else if(Token.Type != TokenType_QuotedString) break;
					
				token_string ParameterName = Stream.ExpectQuotedString();
				Stream.ExpectToken(':');
				
				bool Found;
				parameter_h Parameter = GetParameterHandle(Model, ParameterName, Found);
				if(!Found)
				{
					//TODO: Is this the best way to do warnings. What if a valid parameter name is grouped in the file under an invalid module?
					if(!ValidModule)
						WarningPrint("WARNING: The parameter \"", ParameterName, "\" is marked as belonging to the module \"", ModuleName, "\", which is not currently loaded. Since this parameter does not exist in any other loaded modules it will be ignored, and will be deleted if the file is overwritten.\n\n");
					else if(!ValidVersion)
					{
						const module_spec &ModuleSpec = Model->Modules[CurrentModule];
						WarningPrint("WARNING: The parameter \"", ParameterName, "\" is marked as belonging to version \"", ModuleVersion, "\" of module \"", GetName(Model, CurrentModule), "\". The version of this module in the current loaded model is \"", ModuleSpec.Version, "\", and does not have this parameter. Since this parameter does not exist in any other loaded modules it will be ignored, and will be deleted if the file is overwritten.\n\n");
					}
					
					if(IgnoreUnknown || !(ValidModule && ValidVersion))
					{
						while(true) // Just skip through the values
						{
							Token = Stream.PeekToken();
							if(!CouldBeParameterValue(Token.Type)) break;
							Stream.ReadToken();
						}
					}
					else
					{
						Stream.PrintErrorHeader();
						FatalError("The parameter \"", ParameterName, "\" was not registered with the model.\n");
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
						//TODO: The patching we do here may not be the best way to do it, but we'll see.
						if(!(ValidModule && ValidVersion) || IgnoreUnknown)
						{
							WarningPrint("WARNING: Did not get the expected number of values for parameter \"", ParameterName, "\" Got ", Values.size(), ", expected ", ExpectedCount, 
								". Parameter values have been deleted or filled in by default. Check if they are correct.\n");
							parameter_value Default = Spec.Default;
							if(Values.size() > 0) Default = Values[0];
							Values.resize(ExpectedCount, Default);
							SetMultipleValuesForParameter(DataSet, Parameter, Values.data(), Values.size());
						}
						else
						{
							Stream.PrintErrorHeader();
							FatalError("Did not get the expected number of values for parameter \"", ParameterName, "\". Got ", Values.size(), ", expected ", ExpectedCount, ".\n"); 
						}
					}
					else
						SetMultipleValuesForParameter(DataSet, Parameter, Values.data(), Values.size());
				}
			}
		}
	}
}

enum interpolation_type
{
	InterpolationType_None = 0,
	InterpolationType_Step,
	InterpolationType_Linear,
	InterpolationType_Spline,
};

struct input_series_flags
{
	interpolation_type InterpolationType;
	bool InterpolateInsideOnly;
	bool RepeatYearly;
};

inline bool
PutFlag(input_series_flags *Flags, token_string Str)
{
	if(Str.Equals("linear_interpolate"))
	{
		Flags->InterpolationType = InterpolationType_Linear;
		return true;
	}
	else if(Str.Equals("step_interpolate"))
	{
		Flags->InterpolationType = InterpolationType_Step;
		return true;
	}
	else if(Str.Equals("spline_interpolate"))
	{
		Flags->InterpolationType = InterpolationType_Spline;
		return true;
	}
	else if(Str.Equals("inside"))
	{
		Flags->InterpolateInsideOnly = true;
		return true;
	}
	else if(Str.Equals("repeat_yearly"))
	{
		Flags->RepeatYearly = true;
		return true;
	}
	return false;
}

//#include "spline_example.h"

struct mobius_input_reader
{
	//TODO: Need some error printing context
	double *InputBase;
	std::vector<size_t> Offsets;
	size_t StepStride;
	input_series_flags Flags;
	timestep_size TimestepSize;
	datetime InputStartDate;
	s64      InputDataTimesteps;
	std::function<void(void)> ErrorCleanup;
	
	// For interpolation -- store values, then fill at end time.
	std::vector<datetime> XVals;
	std::vector<double>   YVals;
	
	
	mobius_input_reader(double *InputBase, size_t StepStride, input_series_flags Flags, timestep_size TimestepSize, 
		const std::vector<size_t> &Offsets, datetime InputStartDate, s64 InputDataTimesteps, const std::function<void(void)> &ErrorCleanup) 
		: InputBase(InputBase), StepStride(StepStride), Flags(Flags), TimestepSize(TimestepSize), Offsets(Offsets), InputStartDate(InputStartDate), InputDataTimesteps(InputDataTimesteps),
		  ErrorCleanup(ErrorCleanup)
	{
	}
	
	void AddValue(s64 Timestep, double Value)
	{
		if(Timestep >= 0 && Timestep < InputDataTimesteps)
		{
			for(size_t Offset : Offsets)
			{
				double *WriteTo = InputBase + Offset + Timestep*StepStride;
				*WriteTo = Value;
			}
		}
	}
	
	void AddValue(datetime Date, double Value)
	{
		if(Flags.InterpolationType == InterpolationType_None)
		{
			s64 Timestep = FindTimestep(InputStartDate, Date, TimestepSize);
			AddValue(Timestep, Value);
		}
		else
		{
			if(!std::isfinite(Value)) return;
			
			if(XVals.size() > 0 && Date <= XVals[XVals.size()-1])
			{
				ErrorCleanup();
				FatalError("In interpolation mode, the dates have to be in sequential order and non-overlapping.\n");
			}
			
			/*
			//NOTE: For spline interpolation, eliminate colinear points
			if(Flags.InterpolationType == InterpolationType_Spline)
			{
				if(XVals.size() > 2)
				{
					int N = XVals.size()-1;
					double X0 = (double)XVals[N-1].SecondsSinceEpoch - (double)XVals[N-2].SecondsSinceEpoch;
					double X1 = (double)XVals[N].SecondsSinceEpoch - (double)XVals[N-1].SecondsSinceEpoch;
					double X2 = (double)Date.SecondsSinceEpoch - (double)XVals[N].SecondsSinceEpoch;
					double Y0 = YVals[N-1] - YVals[N-2];
					double Y1 = YVals[N] - YVals[N-1];
					double Y2 = Value - YVals[N];
					
					double Delta = (Y2-Y1)*(X1-X0) - (Y1-Y0)*(X2-X1);
					if(std::abs(Delta) < 1e-12)
					{
						//WarningPrint("WARNING: Interpolation: Culled point due to colinearity\n");
						XVals.pop_back();
						YVals.pop_back();
					}
				}
			}
			*/
			
			XVals.push_back(Date);
			YVals.push_back(Value);
		}
	}
	
	void FillConstantRange(s64 FirstStep, s64 LastStep, double Value)
	{
		FirstStep = std::max(FirstStep, (s64)0);
		LastStep  = std::min(LastStep, InputDataTimesteps-1);
		for(s64 Timestep = FirstStep; Timestep <= LastStep; ++Timestep)
			AddValue(Timestep, Value);
	}
	
	void FillConstantRange(datetime First, datetime Last, double Value)
	{
		if(Last < First)
		{
			ErrorCleanup();
			FatalError("The end of the date range is earlier than the beginning.\n");
		}
		
		s64 FirstStep = FindTimestep(InputStartDate, First, TimestepSize);
		s64 LastStep  = FindTimestep(InputStartDate, Last,  TimestepSize);
		FillConstantRange(FirstStep, LastStep, Value);
	}
	
	void CopyValue(s64 FromStep, s64 ToStep)
	{
		for(size_t Offset : Offsets)
		{
			double *ReadFrom = InputBase + Offset + FromStep*StepStride;
			double *WriteTo  = InputBase + Offset + ToStep*StepStride;
			*WriteTo = *ReadFrom;
		}
	}
	
	void Finish()
	{
		if(Flags.InterpolationType != InterpolationType_None)
			FillInterpolation();
		
		if(Flags.RepeatYearly)
		{
			if(TimestepSize.Unit == Timestep_Month && (TimestepSize.Magnitude >= 12  || 12 % TimestepSize.Magnitude != 0))
			{
				ErrorCleanup();
				FatalError("yearly repetition is only available for models with a step size that is less than a year, and where the number of months in each step divides 12.\n");
			}

			expanded_datetime Year(InputStartDate, {Timestep_Month, 12});
			Year.Advance();

			bool Finished = false;
			while(true)
			{
				s64 FromStep = 0;
				s64 ToStep = FindTimestep(InputStartDate, Year.DateTime, TimestepSize);
				
				Year.Advance();
				s64 YearEnd = FindTimestep(InputStartDate, Year.DateTime, TimestepSize);
				
				while(true)
				{
					CopyValue(FromStep, ToStep);
					
					++ToStep;
					++FromStep;
					
					Finished = (ToStep >= InputDataTimesteps-1);
					if(Finished) break;
					if(ToStep >= YearEnd-1) break;
				}
				if(Finished) break;
			}
		}
	}
	
	void FillInterpolation()
	{
		if(!Flags.InterpolateInsideOnly)
		{
			// Tell it to fill constant values from first input date to first given date
			if(XVals[0] > InputStartDate)
			{
				XVals.insert(XVals.begin(), InputStartDate);
				YVals.insert(YVals.begin(), YVals[0]);
			}
			
			// Similarly fill in at the end
			//TODO: There is something wrong with the positioning of the last step here. Only important for spline interpolation though, so not a big deal before we fix that.
			s64 EndStep = FindTimestep(InputStartDate, XVals[XVals.size()-1], TimestepSize);
			if(EndStep < InputDataTimesteps)
			{
				expanded_datetime InputEnd(InputStartDate, TimestepSize);
				for(s64 Step = 0; Step < InputDataTimesteps; ++Step) InputEnd.Advance();    //TODO: Has to be a better way to do it than this!!
				
				XVals.push_back(InputEnd.DateTime);
				YVals.push_back(YVals[YVals.size()-1]);
			}
		}
		
		interpolation_type Type = Flags.InterpolationType;
		
		if(Type == InterpolationType_Spline && XVals.size() <= 2)
			Type = InterpolationType_Linear;
		
		if(Type == InterpolationType_Step)
		{
			for(int Pt = 0; Pt < XVals.size()-1; ++Pt)
				FillConstantRange(XVals[Pt], XVals[Pt+1], YVals[Pt]);
		}
		else if(Type == InterpolationType_Linear)
		{
			for(int Pt = 0; Pt < XVals.size()-1; ++Pt)
			{
				expanded_datetime Date(XVals[Pt], TimestepSize);
			
				double XRange = (double)(XVals[Pt+1].SecondsSinceEpoch - XVals[Pt].SecondsSinceEpoch);
				
				s64 Step     = FindTimestep(InputStartDate, XVals[Pt],   TimestepSize);
				s64 LastStep = FindTimestep(InputStartDate, XVals[Pt+1], TimestepSize);
				LastStep = std::min(LastStep, InputDataTimesteps-1);
				
				while(Step <= LastStep)
				{
					if(Step >= 0)
					{
						double TT = (double)(Date.DateTime.SecondsSinceEpoch - XVals[Pt].SecondsSinceEpoch) / XRange;
						double YY     = TT*YVals[Pt+1] + (1.0 - TT)*YVals[Pt];
						AddValue(Step, YY);
					}
					
					Date.Advance();
					++Step;
				}
			}
		}
		else if(Type == InterpolationType_Spline)
		{
			/******
			
				Oooops, this is currently incorrect. It produces discontinuous first derivatives!
				Switch to Bezier spline instead, or fix the cubic spline?
			
			******/
			
			
			//TODO: We should allow the user to specify that the value should never be negative (or just have this as a default?)
			
			//NOTE: Implementation inspired by https://stackoverflow.com/questions/23258711/c-spline-interpolation-from-an-array-of-points
			
			if(XVals.size() != YVals.size())
			{
				ErrorCleanup();
				FatalError("Interpolation (internal error), not the same amount of x and y values");
			}
			
			/*
			BezierSpline Spline;
			for(int Seg = 0; Seg < XVals.size(); ++Seg)
			{
				Spline.AddPoint(Vec2((double)XVals[Seg].SecondsSinceEpoch, YVals[Seg]));
				WarningPrint("x: ", (double)XVals[Seg].SecondsSinceEpoch, " y: ", YVals[Seg], "\n");
			}
			Spline.ComputeSpline();
			*/
			
			int NSeg = (int)XVals.size()-1;
			
			std::vector<double> BCol(NSeg+1);
			std::vector<double> XCol(NSeg+1);
			std::vector<double> Diag(NSeg+1);
			std::vector<double> OffDiag(NSeg);
			
			for(int Seg = 1; Seg < NSeg; ++Seg)
				Diag[Seg] = 2.0 * ((double)(XVals[Seg+1].SecondsSinceEpoch - XVals[Seg-1].SecondsSinceEpoch));
			for(int Seg = 0; Seg < NSeg; ++Seg)
				OffDiag[Seg] = (double)(XVals[Seg+1].SecondsSinceEpoch - XVals[Seg].SecondsSinceEpoch);
			for(int Seg = 1; Seg < NSeg; ++Seg)
				BCol[Seg] = 6.0*((YVals[Seg+1]-YVals[Seg])/OffDiag[Seg] - (YVals[Seg]-YVals[Seg-1])/OffDiag[Seg-1]);
			XCol[0] = 0.0;
			XCol[NSeg] = 0.0;
			
			for(int Seg = 1; Seg < NSeg; ++Seg)
			{
				BCol[Seg+1] = BCol[Seg+1] - BCol[Seg]*OffDiag[Seg]/Diag[Seg];
				Diag[Seg+1] = Diag[Seg+1] - OffDiag[Seg]*OffDiag[Seg]/Diag[Seg];
			}
			for(int Seg = NSeg-1; Seg > 0; --Seg)
				XCol[Seg] = (BCol[Seg] - OffDiag[Seg]*XCol[Seg+1])/Diag[Seg];
			
			for(int Seg = 0; Seg < NSeg; ++Seg)
			{
				expanded_datetime Date(XVals[Seg], TimestepSize);
			
				double XRange = (double)(XVals[Seg+1].SecondsSinceEpoch - XVals[Seg].SecondsSinceEpoch);
				
				s64 Step     = FindTimestep(InputStartDate, XVals[Seg],   TimestepSize);
				s64 LastStep = FindTimestep(InputStartDate, XVals[Seg+1], TimestepSize);
				LastStep = std::min(LastStep, InputDataTimesteps-1);
				
				while(Step <= LastStep)
				{
					if(Step >= 0)
					{
						
						double TT = (double)(Date.DateTime.SecondsSinceEpoch - XVals[Seg].SecondsSinceEpoch) / XRange;
						
						double OmT = 1.0 - TT;
						double TCmT   = TT*TT*TT - TT;
						double OmTCmT = OmT*OmT*OmT - OmT;
						double YY     = TT*YVals[Seg+1] + OmT*YVals[Seg] + (1.0/6.0)*XRange*XRange*(TCmT*XCol[Seg+1] - OmTCmT*XCol[Seg]);
						
						
						//Vec2 Result = Spline.Eval(Seg, TT);
						//double YY = Result.y;
						
						AddValue(Step, YY);
					}
					
					Date.Advance();
					++Step;
				}
			}
		}
		else
		{
			ErrorCleanup();
			FatalError("ERROR(internal): Got unimplemented interpolation type in input reading.\n");
		}
		
	}
};


static void
ReadInputSeries(mobius_data_set *DataSet, token_stream &Stream)
{
	const mobius_model *Model = DataSet->Model;
	
	while(true)
	{
		token Token = Stream.ReadToken();
		if(Token.Type == TokenType_EOF)	return;
		
		if(Token.Type == TokenType_Identifier)
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
			
			if(Token.Type == '{')
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
					WarningPrint("} was provided more than once. The last provided series will overwrite the earlier ones.\n");
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
				WarningPrint("WARNING: The input time series \"", InputName, "\" was provided more than once. The latest provided series will overwrite the earlier one.\n");
		}
		
		input_series_flags Flags = {};
		
		Token = Stream.PeekToken();
		while(Token.Type == TokenType_Identifier)
		{
			bool Found = PutFlag(&Flags, Token.StringValue);
			if(!Found)
			{
				Stream.PrintErrorHeader();
				FatalError("Unexpected command word ", Token.StringValue, ".\n");
			}
			Stream.ReadToken(); // Consume the current token to position for the next one.
			Token = Stream.PeekToken();
		}
		
		Stream.ExpectToken(':');
		
		for(size_t Offset : Offsets)
			DataSet->InputTimeseriesWasProvided[Offset] = true;
		
		
		u64 Timesteps = DataSet->InputDataTimesteps;
		
		auto HandleError = [&Stream]() { Stream.PrintErrorHeader(); };
		
		mobius_input_reader Reader(
			DataSet->InputData, DataSet->InputStorageStructure.TotalCount, Flags,
			Model->TimestepSize, Offsets, GetInputStartDate(DataSet), Timesteps, HandleError);
		
		//NOTE: This reads the actual data after the header.
		
		//NOTE: For the first timestep, try to figure out what format the data was provided in.
		int FormatType = -1;
		Token = Stream.PeekToken();
		
		if(IsNumeric(Token.Type))
			FormatType = 0;
		else if(Token.Type == TokenType_Date)
			FormatType = 1;
		else
		{
			Stream.PrintErrorHeader();
			FatalError("Inputs are to be provided either as a series of numbers or a series of dates (or date ranges) together with numbers.\n");
		}
		

		if(FormatType == 0)
		{
			for(u64 Timestep = 0; Timestep < Timesteps; ++Timestep)
			{
				Token = Stream.ReadToken();
				if(!IsNumeric(Token.Type))
				{
					Stream.PrintErrorHeader();
					FatalError("Only got ", Timestep, " values for series. Expected ", Timesteps, ".\n");
				}
				double Value = Token.GetDoubleValue();
				Reader.AddValue(Timestep, Value);
			}
		}
		else //FormatType == 1
		{
			datetime StartDate = DataSet->InputDataStartDate;
			
			while(true)
			{
				datetime Date;
				token Token = Stream.PeekToken();
				
				if(Token.Type == TokenType_Date)
					Date = Stream.ExpectDateTime();
				else if(Token.Type == TokenType_QuotedString || Token.Type == TokenType_EOF)
					break;
				else
				{
					Stream.PrintErrorHeader();
					FatalError("Expected either a date or the beginning of a new input series.\n");
				}
				
				Token = Stream.PeekToken();
				if(Token.Type == TokenType_Identifier)
				{
					if(Flags.InterpolationType != InterpolationType_None)
					{
						Stream.PrintErrorHeader();
						FatalError("Expected a number.\n");
					}
					
					Stream.ReadToken();
					if(!Token.StringValue.Equals("to"))
					{
						Stream.PrintErrorHeader();
						FatalError("Expected either a 'to' or a number.\n");
					}
					datetime EndDateRange = Stream.ExpectDateTime();
					
					double Value = Stream.ExpectDouble();
					Reader.FillConstantRange(Date, EndDateRange, Value);
				}
				else if(IsNumeric(Token.Type))
				{
					double Value = Stream.ExpectDouble();
					Reader.AddValue(Date, Value);
				}
				else
				{
					Stream.PrintErrorHeader();
					FatalError("Expected either a 'to' or a number.\n");
				}
			}
		}
		
		Reader.Finish();
	}
}

static void
ReadInputDependenciesFromSpreadsheet(mobius_model *Model, const char *Inputfile);

static void
ReadInputsFromSpreadsheet(mobius_data_set *DataSet, const char *Inputfile);

static const char *
GetExtension(const char *Filename, bool *Success)
{
	int Len = strlen(Filename);
	const char *C = Filename+(Len-1);
	while(*C != '.' && C != Filename) --C;
	*Success = !(C == Filename && *C != '.');
	return C;
}

static void
ReadInputsFromFile(mobius_data_set *DataSet, const char *Filename)
{
#ifdef _WIN32
	bool FoundExtension;
	const char *Extension = GetExtension(Filename, &FoundExtension);
	if(FoundExtension && ((strcmp(Extension, ".xls") == 0) || (strcmp(Extension, ".xlsx") == 0)))
	{
		ReadInputsFromSpreadsheet(DataSet, Filename);
		return;
	} //Otherwise, assume we use the .dat format
#endif
	
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
		
		token_string Section = Stream.ExpectIdentifier();

		if(Section.Equals("timesteps"))
		{
			//TODO: Guard against both 'timesteps' and 'end_date' being set?
			Stream.ExpectToken(':');
			Timesteps = Stream.ExpectUInt();
		}
		else if(Section.Equals("start_date"))
		{
			Stream.ExpectToken(':');
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
			Stream.ExpectToken(':');
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
			Stream.ExpectToken(':');
			break;
		}
		else if(Section.Equals("index_set_dependencies") || Section.Equals("additional_timeseries"))
		{
			//NOTE: These are handled in a separate call, so we have to skip through them here.
			while(true)
			{
				Token = Stream.PeekToken();
				if(Token.Type == TokenType_Identifier && !Token.StringValue.Equals("unit")) break; //We hit a new section;
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
#ifdef _WIN32
	bool FoundExtension;
	const char *Extension = GetExtension(Filename, &FoundExtension);
	if(FoundExtension && ((strcmp(Extension, ".xls") == 0) || (strcmp(Extension, ".xlsx") == 0)))
	{
		ReadInputDependenciesFromSpreadsheet(Model, Filename);
		return;
	} //Otherwise, assume we use the .dat format
#endif
	
	if(!Filename || strlen(Filename)==0) return;
	
	token_stream Stream(Filename);

	while(true)
	{
		token Token = Stream.PeekToken();
		if(Token.Type == TokenType_EOF)
			break;
		
		token_string Section = Stream.ExpectIdentifier();
		Stream.ExpectToken(':');

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
					Stream.ExpectToken(':');
					
					std::vector<token_string> IndexSetNames;
					Stream.ReadQuotedStringList(IndexSetNames);
					
					//TODO: Why not use AddInputIndexSetDependency(mobius_model *Model, input_h Input, index_set_h IndexSet) ?
					
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
					if(Token.Type == TokenType_Identifier)
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
				
				if(Token.Type == TokenType_Identifier) break; //We hit a new section;
				Stream.ReadToken(); //Otherwise consume the token and ignore it.
			}
		}
	}
}



static void
WriteInputsToFile(mobius_data_set *DataSet, const char *Filename)
{
	if(!DataSet->InputData)
		FatalError("ERROR: Tried to write inputs to a file before input data was allocated.\n");
	
	FILE *File = OpenFile(Filename, "w");

	if(!File)
		FatalError("ERROR: Tried to open file \"", Filename, "\", but was not able to.\"");
	
	const mobius_model *Model = DataSet->Model;
	
	datetime StartDate = GetInputStartDate(DataSet);
	u64 Timesteps      = DataSet->InputDataTimesteps;
	
	fprintf(File, "start_date : %s\n", StartDate.ToString());
	fprintf(File, "timesteps  : %llu\n", (unsigned long long)Timesteps);
	
	fprintf(File, "\n");
	fprintf(File, "additional_timeseries :\n");
	for(input_h Input : Model->Inputs)
	{
		const input_spec &Spec = Model->Inputs[Input];
		if(Spec.IsAdditional)
			fprintf(File, "\"%s\"\n", Spec.Name);
	}
	
	fprintf(File, "\n");
	fprintf(File, "index_set_dependencies :\n");
	for(input_h Input : Model->Inputs)
	{
		size_t UnitIndex = DataSet->InputStorageStructure.UnitForHandle[Input.Handle];
		array<index_set_h> &IndexSets = DataSet->InputStorageStructure.Units[UnitIndex].IndexSets;
		if(IndexSets.Count > 0)
		{
			fprintf(File, "\"%s\" : {", GetName(Model, Input));
			for(index_set_h IndexSet : IndexSets)
				fprintf(File, "\"%s\" ", GetName(Model, IndexSet));
			fprintf(File, "}\n");
		}
	}
	
	fprintf(File, "inputs :\n");
	
	std::vector<double> Buffer(Timesteps);
	
	for(input_h Input : Model->Inputs)
	{
		const char *Name = GetName(Model, Input);
		
		ForeachInputInstance(DataSet, Name, [&](const char *const *IndexNames, size_t IndexesCount)
		{
			bool Provided = InputSeriesWasProvided(DataSet, Name, IndexNames, IndexesCount);
			if(Provided)
			{
				fprintf(File, "\n\"%s\" ", Name);
				if(IndexesCount > 0)
				{
					fprintf(File, "{");
					for(size_t IdxIdx = 0; IdxIdx < IndexesCount; ++IdxIdx)
						fprintf(File, "\"%s\" ", IndexNames[IdxIdx]); 
					fprintf(File, "}");
				}
				fprintf(File, ":\n");
				
				GetInputSeries(DataSet, Name, IndexNames, IndexesCount, Buffer.data(), Buffer.size(), false);
				
				u64 NaNCount = 0;
				for(double Value : Buffer)
					if(std::isnan(Value)) NaNCount++;
				
				double Ratio = (double)NaNCount / (double)Timesteps;
				
				//If more than 20% of the values are NaN, use the sparse format. 
				if(Ratio >= 0.2)
				{
					if(Timesteps == NaNCount)
						fprintf(File, "%s NaN", StartDate.ToString()); //NOTE: The format requires at least one value entry.
					else
					{	
						expanded_datetime Date(StartDate, Model->TimestepSize);
						for(double Value : Buffer)
						{
							if(!std::isnan(Value))
								fprintf(File, "%s %f\n", Date.DateTime.ToString(), Value);
							Date.Advance();
						}
					}
				}
				else
				{
					for(double Value : Buffer)
					{
						if(std::isnan(Value)) fprintf(File, "NaN\n");
						else fprintf(File, "%f\n", Value);
					}
				}
			}
		});
	}
	
	fclose(File);
}
