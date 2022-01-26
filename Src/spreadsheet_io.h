
#ifndef _WIN32

void ReadInputDependenciesFromSpreadsheet(mobius_model *Model, const char *Inputfile)
{
	FatalError("ERROR: Reading from Excel files is only supported on Windows.\n");
}

void ReadInputsFromSpreadsheet(mobius_data_set *DataSet, const char *Inputfile)
{
	FatalError("ERROR: Reading from Excel files is only supported on Windows.\n");
}

#else



static bool
OLEAutoWrap(int autoType, VARIANT *pvResult, IDispatch *pDisp, const wchar_t *Name, int cArgs, ...)
{
    // Begin variable-argument list...
    va_list marker;
    va_start(marker, cArgs);

    if(!pDisp)
	{
        ErrorPrint("ERROR(internal): NULL IDispatch passed to AutoWrap().\n");
		return false;
	}

    // Variables used...
    DISPPARAMS dp = { NULL, NULL, 0, 0 };
    DISPID dispidNamed = DISPID_PROPERTYPUT;
    DISPID dispID;
    HRESULT hr;
    //char buf[200];
    //char szName[200];


    // Convert down to ANSI
    //WideCharToMultiByte(CP_ACP, 0, Name, -1, szName, 256, NULL, NULL);

    // Get DISPID for name passed...
    hr = pDisp->GetIDsOfNames(IID_NULL, (wchar_t**)&Name, 1, LOCALE_USER_DEFAULT, &dispID);
    if(FAILED(hr))
	{
        ErrorPrint("ERROR(internal, excel) AutoWrap() failed.\n");//IDispatch::GetIDsOfNames(\"%s\") failed w/err 0x%08lx", szName, hr);
		return false;
	}

    // Allocate memory for arguments...
    VARIANT *pArgs = new VARIANT[cArgs+1];
    // Extract arguments...
    for(int i=0; i<cArgs; i++) {
        pArgs[i] = va_arg(marker, VARIANT);
    }

    // Build DISPPARAMS
    dp.cArgs = cArgs;
    dp.rgvarg = pArgs;

    // Handle special-case for property-puts!
    if(autoType & DISPATCH_PROPERTYPUT) {
        dp.cNamedArgs = 1;
        dp.rgdispidNamedArgs = &dispidNamed;
    }

    // Make the call!
    hr = pDisp->Invoke(dispID, IID_NULL, LOCALE_SYSTEM_DEFAULT, autoType, &dp, pvResult, NULL, NULL);
    if(FAILED(hr))
	{
        ErrorPrint("ERROR(internal, excel) AutoWrap() failed.\n");//sprintf(buf, "IDispatch::Invoke(\"%s\"=%08lx) failed w/err 0x%08lx", szName, dispID, hr);
		return false;
	}
	
    // End variable-argument section...
    va_end(marker);

    delete [] pArgs;

    return true;
}

static VARIANT
OLENewVariant()
{
	VARIANT Result;
	VariantInit(&Result);
	VariantClear(&Result);
	return Result;
}


static IDispatch *
OLECreateObject(const wchar_t *Name)
{
	CLSID clsid;
	HRESULT hr = CLSIDFromProgID(Name, &clsid);	// Get CLSID for our server
	if(FAILED(hr)) {
		ErrorPrint("ERROR (excel, internal): CLSIDFromProgID() failed.\n");
		return nullptr;
	}
	IDispatch *app;
	// Start server and get IDispatch
	hr = CoCreateInstance(clsid, NULL, CLSCTX_LOCAL_SERVER, IID_IDispatch, (void **)&app);
	if(FAILED(hr)) {
		ErrorPrint("ERROR (excel, internal): Application Excel is not registered properly.\n");
		return nullptr;
	}
	return app;
}

static IDispatch *
OLEGetObject(IDispatch *From, const wchar_t *Which)
{
	VARIANT Result = OLENewVariant();
	bool Success = OLEAutoWrap(DISPATCH_PROPERTYGET, &Result, From, Which, 0);
	if(!Success)
		return nullptr;
	return Result.pdispVal;
}

static IDispatch *
OLEGetObject(IDispatch *From, const wchar_t *Which, VARIANT *Value)
{
	VARIANT Result = OLENewVariant();
	bool Success = OLEAutoWrap(DISPATCH_PROPERTYGET, &Result, From, Which, 1, *Value);
	if(!Success)
		return nullptr;
	return Result.pdispVal;
}

static IDispatch *
OLEGetObject(IDispatch *From, const wchar_t *Which, VARIANT *Value1, VARIANT *Value2)
{
	VARIANT Result = OLENewVariant();
	bool Success = OLEAutoWrap(DISPATCH_PROPERTYGET, &Result, From, Which, 2, *Value1, *Value2);
	if(!Success)
		return nullptr;
	return Result.pdispVal;
}

static VARIANT
OLEGetValue(IDispatch *From, const wchar_t *Which)
{
	VARIANT Result = OLENewVariant();
	bool Success = OLEAutoWrap(DISPATCH_PROPERTYGET|DISPATCH_METHOD, &Result, From, Which, 0);
	if(!Success)
		Result.vt = VT_EMPTY;
	
	return Result;
}

static bool
OLEMethod(IDispatch *From, const wchar_t *Which) {
	VARIANT Result;
	VariantInit(&Result);
	return OLEAutoWrap(DISPATCH_METHOD, &Result, From, Which, 0);
}

static bool
OLESetValue(IDispatch *From, const wchar_t *Which, VARIANT *Value)
{
	return OLEAutoWrap(DISPATCH_PROPERTYPUT, NULL, From, Which, 1, *Value);
}

static VARIANT
OLEStringVariant(const char *Name)
{
	VARIANT Var = OLENewVariant();
	
	WCHAR Name2[1024*sizeof(WCHAR)];

	MultiByteToWideChar(CP_UTF8, 0, Name, -1, Name2, sizeof(Name2)/sizeof(Name2[0]));
	Var.vt = VT_BSTR;
	Var.bstrVal = SysAllocString(Name2);
	
	return Var;
}

static void
OLEDestroyString(VARIANT *String)
{
	SysFreeString(String->bstrVal);
	String->bstrVal = nullptr;
}

static VARIANT
OLEInt4Variant(int Value)
{
	VARIANT Var = OLENewVariant();
	Var.vt = VT_I4;
	Var.lVal = Value;
	return Var;
}

static void
OLEGetString(VARIANT *Var, char *BufOut, size_t BufLen)
{
	BufOut[0] = 0;
	if(Var->vt == VT_BSTR)
	{
		WideCharToMultiByte(CP_ACP, 0, Var->bstrVal, -1, BufOut, BufLen, NULL, NULL);
		OLEDestroyString(Var); //Hmm, do we want to do this here?
	}
}

static char *
ColRowToCell(int Col, int Row, char *Buf)
{
	int NumAZ = 'Z' - 'A' + 1;
	int NCol = Col;
	while (NCol > 0)
	{
		int Letter = NCol/NumAZ;
		if (Letter == 0)
		{
			Letter = NCol;
			*Buf = char('A' + Letter - 1);
			Buf++;
			break;
		}
		else
		{
			NCol -= Letter*NumAZ;
			*Buf = char('A' + Letter - 1);
			Buf++;
		}
	}
	itoa(Row, Buf, 10)+1;
	while(*Buf != 0) ++Buf;
	return Buf;
}

struct ole_handles
{
	IDispatch *App = nullptr;
	IDispatch *Books = nullptr;
	IDispatch *Book = nullptr;
	IDispatch *Sheet = nullptr;
	const char *Filepath;
};

static void
OLECloseSpreadsheet(ole_handles *Handles)
{
	if(Handles->App)
		OLEAutoWrap(DISPATCH_METHOD, NULL, Handles->App, L"Quit", 0);
	
	if(Handles->Sheet) Handles->Sheet->Release();
	Handles->Sheet = nullptr;
	if(Handles->Book) Handles->Book->Release();
	Handles->Book = nullptr;
	if(Handles->Books) Handles->Books->Release();
	Handles->Books = nullptr;
	if(Handles->App) Handles->App->Release();
	Handles->App = nullptr;

	OleUninitialize();
	// Uninitialize COM for this thread...
	CoUninitialize();
}

static void
OLECloseDueToError(ole_handles *Handles, int Tab = -1, int Col=-1, int Row=-1)
{
	ErrorPrint("ERROR(excel): in file \"", Handles->Filepath, "\"");
	if(Tab >= 0)
	{
		VARIANT VarID = OLEInt4Variant(Tab + 1);
		IDispatch *Sheet = OLEGetObject(Handles->App, L"Sheets", &VarID);
		if(Sheet)
		{
			VARIANT Name = OLEGetValue(Sheet, L"Name");
			char Buf[512];
			OLEGetString(&Name, Buf, 512);
			ErrorPrint(" tab \"", Buf, "\"");
			Sheet->Release();
		}
	}
	if(Col >= 1 && Row >= 1)
	{
		char Buf[32];
		ColRowToCell(Col, Row, &Buf[0]);
		ErrorPrint(" cell ", Buf);
	}
	ErrorPrint("\n");
	OLECloseSpreadsheet(Handles);
}

static void
OLEOpenSpreadsheet(const char *Filepath, ole_handles *Handles)
{
	Handles->Filepath = Filepath;
	
	char FullPath[_MAX_PATH];
	if(_fullpath(FullPath, Filepath, _MAX_PATH ) == NULL)
		FatalError("ERROR(excel): Can't convert the relative path \"", Filepath, "\" to a full path.\n");
	
	OleInitialize(NULL);
	//Initialize COM for this thread
	CoInitialize(NULL);
	
	Handles->App = OLECreateObject(L"Excel.Application");
	if(!Handles->App)
	{
		OLECloseDueToError(Handles);
		FatalError("Failed to open Excel application.\n");
	}
	
	VARIANT FileVar = OLEStringVariant(FullPath);
	
	Handles->Books = OLEGetObject(Handles->App, L"Workbooks");
	if(!Handles->Books)
	{
		OLECloseDueToError(Handles);
		FatalError("Failed to initialize excel workbooks.\n");
	}
	
	Handles->Book = OLEGetObject(Handles->Books, L"Open", &FileVar);
	if(!Handles->Book)
	{
		OLECloseDueToError(Handles);
		FatalError("Failed to open file.\n");
	}
	
	Handles->Sheet = OLEGetObject(Handles->Book, L"ActiveSheet");
	if(!Handles->Sheet)
	{
		OLECloseDueToError(Handles);
		FatalError("Failed to open excel active sheet.\n");
	}
	
	OLEDestroyString(&FileVar);
	
	//Don't make this open the excel application in a way that is visible to the user.
	VARIANT Visible = OLEInt4Variant(0);
	OLESetValue(Handles->App, L"Visible", &Visible);
}

static VARIANT
OLECreateMatrix(int DimX, int DimY)
{
	VARIANT Matrix = OLENewVariant();
	Matrix.vt = VT_ARRAY | VT_VARIANT;
    SAFEARRAYBOUND Sab[2];
	Sab[0].lLbound = 1; Sab[0].cElements = DimY;
	Sab[1].lLbound = 1; Sab[1].cElements = DimX;
	Matrix.parray = SafeArrayCreate(VT_VARIANT, 2, Sab);
	
	return Matrix;
}

static bool
OLEDestroyMatrix(VARIANT *Matrix)
{
	bool Success = (SafeArrayDestroy(Matrix->parray) == S_OK);
	Matrix->parray = nullptr;
	return Success;
}


static VARIANT
OLEGetRangeMatrix(int FromRow, int ToRow, int FromCol, int ToCol, ole_handles *Handles)
{
	IDispatch *Range;
	char RangeBuf[256];
	char *Range0 = &RangeBuf[0];
	Range0 = ColRowToCell(FromCol, FromRow, Range0);
	*Range0 = ':';
	ColRowToCell(ToCol, ToRow, Range0+1);
	//WarningPrint("Range is ", RangeBuf, "\n");
	
	VARIANT RangeString = OLEStringVariant(RangeBuf);
	Range = OLEGetObject(Handles->Sheet, L"Range", &RangeString);
	if(!Range)
	{
		OLECloseDueToError(Handles);
		FatalError("Failed to select range ", RangeBuf, ".\n");
	}
	
	VARIANT Matrix = OLEGetValue(Range, L"Value");
	
	OLEDestroyString(&RangeString);
	
	Range->Release();
	
	return Matrix;
}

static VARIANT
OLEGetMatrixValue(VARIANT *Matrix, int Row, int Col, ole_handles *Handles)
{
	VARIANT Result = OLENewVariant();
	long Indices[2] = {Row, Col};
	HRESULT hr = SafeArrayGetElement(Matrix->parray, Indices, (void *)&Result);
	if(hr != S_OK)
	{
		OLECloseDueToError(Handles);
		FatalError("Internal error when indexing matrix. Row: ", Row, " Col: ", Col, " (relative to matrix).\n");
	}
	
	return Result;
}

static bool
OLEGetDate(VARIANT *Var, datetime *DateOut)
{
	if(Var->vt == VT_DATE)
	{
		SYSTEMTIME stime;
     	VariantTimeToSystemTime(Var->date, &stime);
		
		bool Success;
     	*DateOut = datetime(stime.wYear, stime.wMonth, stime.wDay, &Success);
		Success = Success && DateOut->AddHourMinuteSecond(stime.wHour, stime.wMinute, stime.wSecond);
		return Success;
	}
	else if(Var->vt == VT_BSTR)
	{
		char Buf[256];
		WideCharToMultiByte(CP_ACP, 0, Var->bstrVal, -1, Buf, 256, NULL, NULL);
		bool Success;
		*DateOut = datetime(Buf, &Success);
		OLEDestroyString(Var);
		return Success;
	}
	return false;
}

static double
OLEGetDouble(VARIANT *Var)
{
	double Result = std::numeric_limits<double>::quiet_NaN();
	
	//TODO: Do we need to handle other types?
	if(Var->vt == VT_I2)
		Result = (double) Var->iVal;
	else if(Var->vt == VT_I4)
		Result = (double) Var->lVal;
	else if(Var->vt == VT_R4)
		Result = (double) Var->fltVal;
	else if(Var->vt == VT_R8)
		Result = Var->dblVal;
	else if(Var->vt == VT_BSTR)
	{
		char Buf[256];
		WideCharToMultiByte(CP_ACP, 0, Var->bstrVal, -1, Buf, 256, NULL, NULL);
		char *C = &Buf[0];
		while(*C != 0)
		{
			if(*C == ',') *C = '.';   //NOTE: Replace ',' with '.' in case of nonstandard formats.
			++C;
		}
		int Count = sscanf(Buf, "%f", &Result);
		if(Count != 1) Result = std::numeric_limits<double>::quiet_NaN();
		OLEDestroyString(Var);
	}
	
	return Result;
}

static int
OLEGetNumTabs(ole_handles *Handles)
{
	IDispatch *Sheets = OLEGetObject(Handles->App, L"Sheets");
	VARIANT Count = OLEGetValue(Sheets, L"Count");
	Sheets->Release();
	return Count.lVal;
}

static void
OLEChooseTab(ole_handles *Handles, int Tab)
{
	VARIANT VarID = OLEInt4Variant(Tab + 1);
	Handles->Sheet = OLEGetObject(Handles->App, L"Sheets", &VarID);
	if(!Handles->Sheet)
	{
		OLECloseDueToError(Handles);
		FatalError("Failed to get Sheet object for tab ", Tab, ".\n");
	}
	bool Success = OLEMethod(Handles->Sheet, L"Select");
	if(!Success)
	{
		OLECloseDueToError(Handles);
		FatalError("Failed to select tab ", Tab, ".\n");
	}
}

static VARIANT
OLEGetCellValue(ole_handles *Handles, int Col, int Row)
{
	VARIANT X = OLEInt4Variant(Col);
	VARIANT Y = OLEInt4Variant(Row);
	
	IDispatch *Cell = OLEGetObject(Handles->Sheet, L"Cells", &X, &Y);
	
	VARIANT Var = OLEGetValue(Cell, L"Value");
	
	Cell->Release();
	
	return Var;
}

token_string GetTokenWord(char **In)
{
	char *At = *In;
	token_string Result = {};
	while(std::isspace(*At) && *At != 0) ++At;
	Result.Data = At;
	while(!std::isspace(*At) && *At != 0)
	{
		++At;
		++Result.Length;
	}
	*In = At;
	return Result;
}


static void
ReadInputDependenciesFromSpreadsheet(mobius_model *Model, const char *Inputfile)
{
	//TODO: We do a lot of unnecessary work by scanning the file twice. We should find a way to keep the structure instead...
	
	ole_handles Handles = {};
	OLEOpenSpreadsheet(Inputfile, &Handles);
	
	int NTabs = OLEGetNumTabs(&Handles);
	
	constexpr size_t Bufsize = 512;
	char Buf[Bufsize];
	
	for(int Tab = 0; Tab < NTabs; ++Tab)
	{
		OLEChooseTab(&Handles, Tab);
		
		VARIANT A1 = OLEGetCellValue(&Handles, 1, 1);
		OLEGetString(&A1, Buf, Bufsize);
		bool SkipTab = (strcmp(Buf, "NOREAD") == 0);
		
		if(SkipTab) continue;
		
		std::vector<index_set_h> IndexSets;
		
		VARIANT Matrix = OLEGetRangeMatrix(2, 1026, 1, 1, &Handles); //NOTE: We only search for index sets among the first 1024 rows since anything more than that would be ridiculous.
		
		int PotentialFlagRow = -1;
		
		for(int Row = 0; Row < 1024; ++Row)
		{
			VARIANT IdxSetName = OLEGetMatrixValue(&Matrix, Row+1, 1, &Handles);

			if(IdxSetName.vt == VT_DATE)
				break;
			
			OLEGetString(&IdxSetName, Buf, Bufsize);
			
			//WarningPrint("Looking at index set at tab ", Tab, " row ", Row+2, " name ", Buf, "\n");
			if(strlen(Buf) > 0)
			{
				// Check if this is a date that was formatted as a string (especially important since Excel can't format dates before 1900 as other than strings).
				bool IsDate;
				datetime TestDate(Buf, &IsDate);
				if(IsDate)
					break;
				
				bool GetSuccess;
				index_set_h IndexSet = GetIndexSetHandle(Model, Buf, GetSuccess);
				if(!GetSuccess)
				{
					OLECloseDueToError(&Handles, Tab, 1, 2+Row);
					FatalError("The index set ", Buf, " is not registered with the model.\n");
				}
				IndexSets.push_back(IndexSet);
			}
			else
			{
				if(PotentialFlagRow > 0)
				{
					OLECloseDueToError(&Handles, Tab, 1, 2+Row);
					FatalError("You should not have empty cells in column A except in row 1, potentially in the row right above the dates, or at the end.\n");
				}
				PotentialFlagRow = Row+2;
				break;
			}
		}
		
		OLEDestroyMatrix(&Matrix);
		
		int RangeMax = std::max((int)(1+IndexSets.size()), PotentialFlagRow);
		Matrix = OLEGetRangeMatrix(1, RangeMax, 2, 128, &Handles); //TODO: Ideally also do this in a loop in case there are more than 128 columns!
		
		input_h CurInput = {};
		for(int Col = 0; Col < 127; ++Col)
		{
			VARIANT Name = OLEGetMatrixValue(&Matrix, 1, Col+1, &Handles);
			
			OLEGetString(&Name, Buf, Bufsize);
			if(strlen(Buf) > 0)
			{
				if(Model->Inputs.Has(Buf)) // Register as "additional input" if it was not already registered with the model
				{
					bool GetSuccess;
					CurInput = GetInputHandle(Model, Buf, GetSuccess);
					if(!GetSuccess)
					{
						OLECloseDueToError(&Handles, Tab, Col+2, 1);
						FatalError("The input ", Buf, " is not registered with the model.\n");
					}
				}
				else
				{
					token_string InputName = token_string(Buf).Copy(&Model->BucketMemory);
					unit_h Unit = {};
					//Look for a unit among the flags
					if(PotentialFlagRow > 0)
					{
						VARIANT FlagsVar = OLEGetMatrixValue(&Matrix, PotentialFlagRow, Col+1, &Handles);
						OLEGetString(&FlagsVar, Buf, Bufsize);
						if(strlen(Buf) > 0)
						{
							char *Buff = &Buf[0];
							while(true)
							{
								token_string FlagStr = GetTokenWord(&Buff);
								if(FlagStr.Length == 0) break;
								if((FlagStr.Length >= 3) && (FlagStr.Data[0] == '[') && (FlagStr.Data[FlagStr.Length-1] == ']'))
								{
									FlagStr.Data++;
									FlagStr.Length -= 2;
									Unit = RegisterUnit(Model, FlagStr.Copy(&Model->BucketMemory).Data);
								}
							}
						}
					}
					
					CurInput = RegisterInput(Model, InputName.Data, Unit, true, true);
				}
			}
		
			if(IsValid(CurInput))
			{
				//NOTE: We don't check subsequent columns of this input since if the format is correct, the first given column has to be indexed by all the indexes that are relevant.
				if(Model->Inputs[CurInput].IndexSetDependencies.size() > 0)
					continue;
				
				for(int Row = 0; Row < IndexSets.size(); ++Row)
				{
					VARIANT IndexName = OLEGetMatrixValue(&Matrix, Row+2, Col+1, &Handles);
					
					OLEGetString(&IndexName, Buf, Bufsize);
					if(strlen(Buf) > 0)
						AddInputIndexSetDependency(Model, CurInput, IndexSets[Row]);
				}
			}
		}
		
		OLEDestroyMatrix(&Matrix);
	}
	
	OLECloseSpreadsheet(&Handles);
}

static void
ReadInputsFromSpreadsheet(mobius_data_set *DataSet, const char *Inputfile)
{
	ole_handles Handles = {};
	OLEOpenSpreadsheet(Inputfile, &Handles);
	
	//NOTE: Ideally there would be a way to get the number of nonempty rows directly, but it doesn't seem so (??)
	
	int NTabs = OLEGetNumTabs(&Handles);
	
	constexpr int RowsAtATime = 1024;
	std::vector<std::vector<datetime>> Dates(NTabs);
	std::vector<int> FirstDateRow(NTabs);
	
	std::vector<bool> SkipTab(NTabs);
	
	constexpr size_t Bufsize = 512;
	char Buf[Bufsize];
	
	std::vector<std::vector<int>>         IndexSetRow(NTabs);
	std::vector<bool>        LookForFlags(NTabs);
	
	for(std::vector<int> &IndexSetMap : IndexSetRow) IndexSetMap.resize(DataSet->Model->IndexSets.Count(), -1);
	
	for(int Tab = 0; Tab < NTabs; ++Tab)
	{
		OLEChooseTab(&Handles, Tab);
		
		VARIANT A1 = OLEGetCellValue(&Handles, 1, 1);
		OLEGetString(&A1, Buf, Bufsize);
		SkipTab[Tab] = (strcmp(Buf, "NOREAD") == 0);
		
		if(SkipTab[Tab]) continue;
		
		Dates[Tab].reserve(RowsAtATime);
		
		int SuperRow = 2;
		
		FirstDateRow[Tab] = -1;
		bool FoundFirst = false;
		while(true)
		{
			bool BreakOut = false;
			VARIANT Matrix = OLEGetRangeMatrix(SuperRow, SuperRow + RowsAtATime - 1, 1, 1, &Handles);
			for(int R = 0; R < RowsAtATime; ++R)
			{
				int MatRow = 1 + R;
				int Row = SuperRow + R;
				VARIANT Var = OLEGetMatrixValue(&Matrix, MatRow, 1, &Handles);
				
				OLEGetString(&Var, Buf, Bufsize);
				
				bool StringDate;
				datetime TestDate(Buf, &StringDate);
				
				if(!StringDate && (strlen(Buf) > 0))
				{
					bool GetSuccess;
					index_set_h IndexSet = GetIndexSetHandle(DataSet->Model, Buf, GetSuccess);
					if(!GetSuccess)
					{
						OLECloseDueToError(&Handles, 1, Row);
						FatalError("The index set ", Buf, " was not registered with the model.\n");
					}
					IndexSetRow[Tab][IndexSet.Handle] = Row;
					
					continue;
				}
				
				bool Success;
				datetime Date;
				if(StringDate)
				{
					Date = TestDate;
					Success = true;
				}
				else
					Success = OLEGetDate(&Var, &Date);
				
				if(Success)
				{
					Dates[Tab].push_back(Date);
					//WarningPrint(Date.ToString(), "\n");
					
					if(!FoundFirst)
					{
						FoundFirst = true;
						FirstDateRow[Tab] = Row;
					}
				}
				else 
				{
					if(!FoundFirst)
						LookForFlags[Tab] = true;   //NOTE:  We hit an empty cell before there are any dates. This means that there could be modifier flags in this row to the right, and we should look for them later.
					
					if(FoundFirst) //We are at the end of the set of valid dates.
					{
						BreakOut = true;
						break;
					}
				}
			}
			
			SuperRow += RowsAtATime;
			
			if(BreakOut) break;
			
			if(!FoundFirst)
			{
				OLECloseDueToError(&Handles, Tab);
				FatalError("Not able to find a date among the first ", RowsAtATime, " cells of column A in. Put \"NOREAD\" in cell A1 if you want this tab to be ignored.\n");
			}
			
			OLEDestroyMatrix(&Matrix);
		}
	}
	
	if(Dates.size() == 0)
		return;
	
	//WarningPrint("Dates size ", Dates.size(), "\n");
	
	//TODO: This is unnecessary work to just fid the min and max date...
	std::vector<datetime> DatesSort;
	for(int Tab = 0; Tab < NTabs; ++Tab)
		DatesSort.insert(DatesSort.end(), Dates[Tab].begin(), Dates[Tab].end());
	std::sort(DatesSort.begin(), DatesSort.end());
	
	DataSet->InputDataStartDate = DatesSort[0];
	DataSet->InputDataHasSeparateStartDate = true;

	s64 Step = FindTimestep(DataSet->InputDataStartDate, DatesSort[DatesSort.size()-1], DataSet->Model->TimestepSize);
	Step += 1;    //NOTE: Because the end date is inclusive. 
	if(Step <= 0)
	{
		OLECloseDueToError(&Handles);
		FatalError("The input data end date was set to be earlier than the input data start date.\n"); //NOTE: Should technically not be possible.
	}
	
	u64 Timesteps = (u64)Step;
	AllocateInputStorage(DataSet, Timesteps);
	
	for(int Tab = 0; Tab < NTabs; ++Tab)
	{
		if(SkipTab[Tab]) continue;
		
		std::vector<size_t> Offsets;
		
		std::vector<input_series_flags> Flags;
		
		OLEChooseTab(&Handles, Tab);
		
		VARIANT Matrix = OLEGetRangeMatrix(1, FirstDateRow[Tab]-1, 2, 128, &Handles); //TODO: Ideally also do this in a loop in case there are more than 128 rows!
		
		input_h CurInput = {};
		for(int Col = 0; Col < 127; ++Col)
		{
			VARIANT Name = OLEGetMatrixValue(&Matrix, 1, Col+1, &Handles);
			//TODO: Do a OLEGetString instead in case they are misformatted
			
			bool GotNameThisColumn = false;
			
			OLEGetString(&Name, Buf, Bufsize);
			if(strlen(Buf) > 0)
			{
				bool GetSuccess;
				CurInput = GetInputHandle(DataSet->Model, Buf, GetSuccess);
				if(!GetSuccess)
				{
					OLECloseDueToError(&Handles, Tab, Col+1, 1);
					FatalError("The input ", Buf, " is not registered with the model.\n");
				}
				GotNameThisColumn = true;
			}
			else if(!IsValid(CurInput))
			{
				OLECloseDueToError(&Handles, Tab, 2, 1);
				FatalError("Missing an input name.\n");
			}
			
			size_t StorageUnitIndex = DataSet->InputStorageStructure.UnitForHandle[CurInput.Handle];
			array<index_set_h> &InputIndexSets = DataSet->InputStorageStructure.Units[StorageUnitIndex].IndexSets;
			
			std::vector<index_t> Indexes;
			
			int IdxSetNum = 0;
			for(int Row = 0; Row < InputIndexSets.size(); ++Row)
			{
				int SheetRow = Row+2;
				
				VARIANT IndexName = OLEGetMatrixValue(&Matrix, Row+2, Col+1, &Handles);
				
				OLEGetString(&IndexName, Buf, Bufsize);
				if(strlen(Buf) > 0)
				{
					index_set_h IndexSet = InputIndexSets[IdxSetNum];
					int ExpectedRow = IndexSetRow[Tab][IndexSet.Handle];
					
					if(ExpectedRow != Row+2)
					{
						OLECloseDueToError(&Handles, Tab, Col+2, Row+2);
						FatalError("There is a mismatch in the positioning of indexes and index sets. Empty cells in column A except at row 1, the first row before the dates, or at the end of the column.\n");
					}

					bool GetSuccess;
					index_t Index = GetIndex(DataSet, IndexSet, Buf, GetSuccess);
					if(!GetSuccess)
					{
						OLECloseDueToError(&Handles, Tab, Col+2, Row+2);
						FatalError("The index name \"", Buf, "\" does not belong to the index set \"", GetName(DataSet->Model, IndexSet), "\".\n");
					}
					Indexes.push_back(Index);
					++IdxSetNum;
				}
			}
			
			// An entirely blank column (in the header at least) signifies that we should ignore the rest of the columns;
			if(!GotNameThisColumn && Indexes.empty())
				break;
			
			size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, Indexes.data(), Indexes.size(), DataSet->IndexCounts, CurInput);
			Offsets.push_back(Offset);
			DataSet->InputTimeseriesWasProvided[Offset] = true;
			
			input_series_flags Flag = {};
			if(LookForFlags[Tab])
			{
				// We look for flags in the last row before the dates (and values) begin.
				VARIANT FlagsVar = OLEGetMatrixValue(&Matrix, FirstDateRow[Tab]-1, Col+1, &Handles);
				OLEGetString(&FlagsVar, Buf, Bufsize);
				
				//WarningPrint("Flag string \"", Buf, "\" tab ", Tab, "\n");
				if(strlen(Buf) > 0)
				{
					char *Buff = &Buf[0];
					while(true)
					{
						token_string FlagStr = GetTokenWord(&Buff);
						if(FlagStr.Length == 0) break;
						bool FlagExists = PutFlag(&Flag, FlagStr);
						//The condition checked is if this is a unit indicator on the form [unit]. In that case it was already processed in ReadInputDependenciesFromSpreadsheet
						if(!FlagExists && !(FlagStr.Length >= 2 && FlagStr.Data[0] == '[' && FlagStr.Data[FlagStr.Length-1] == ']'))
						{
							OLECloseDueToError(&Handles, Tab, Col+2, FirstDateRow[Tab]-1);
							FatalError("Unrecognized flag \"", FlagStr, "\".\n");
						}
						//WarningPrint("Found flag ", FlagStr, "\n");
					}
				}
			}
			
			Flags.push_back(Flag);
			
			//WarningPrint(GetName(DataSet->Model, CurInput), " ", Offset, "\n");
		}
		
		OLEDestroyMatrix(&Matrix);

		std::vector<mobius_input_reader> Readers;
		Readers.reserve(Offsets.size());
		
		int ErrorRow = 0;
		int ErrorCol = 0;
		auto HandleError = [&Tab, &ErrorRow, &ErrorCol, &Handles]() { OLECloseDueToError(&Handles, Tab, ErrorCol, ErrorRow); };
		
		for(int Idx = 0; Idx < Offsets.size(); ++Idx)
		{
			//NOTE: We currently don't support setting multiple dataset series per excel column. Hence only one offset is passed to each Reader.
			Readers.push_back(
				mobius_input_reader	(
					DataSet->InputData, DataSet->InputStorageStructure.TotalCount, Flags[Idx],
					DataSet->Model->TimestepSize, {Offsets[Idx]}, GetInputStartDate(DataSet), DataSet->InputDataTimesteps, HandleError)
			);
		}
		//static void
		//InterpolateInputValues(mobius_data_set *DataSet, double *Base, double FirstValue, double LastValue, datetime FirstDate, datetime LastDate, interpolation_type Type)

		// Select the rest of the data!
		Matrix = OLEGetRangeMatrix(FirstDateRow[Tab], FirstDateRow[Tab]-1 + Dates[Tab].size(), 2, 1+Offsets.size(), &Handles);
		
		for(int Row = 0; Row < Dates[Tab].size(); ++Row)
		{
			ErrorRow = Row + FirstDateRow[Tab];
			
			for(int Col = 0; Col < Offsets.size(); ++Col)
			{
				ErrorCol = 2 + Col;
				
				VARIANT Value = OLEGetMatrixValue(&Matrix, Row+1, Col+1, &Handles);
				double Val = OLEGetDouble(&Value);
				
				if(std::isfinite(Val))
					Readers[Col].AddValue(Dates[Tab][Row], Val);
			}
		}
		for(int Col = 0; Col < Offsets.size(); ++Col)
			Readers[Col].Finish();
		
		OLEDestroyMatrix(&Matrix);
	}
	
	
	OLECloseSpreadsheet(&Handles);
}

#endif //_WIN32