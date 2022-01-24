
#ifndef _WIN32

void ReadInputDependenciesFromSpreadsheet(mobius_data_set *DataSet, const char *Inputfile)
{
	FatalError("ERROR: Reading from Excel files is only supported on Windows.\n");
}

void ReadInputsFromSpreadsheet(mobius_data_set *DataSet, const char *Inputfile)
{
	FatalError("ERROR: Reading from Excel files is only supported on Windows.\n");
}

#else

/*****

MAJOR TODO:
	If it has opened an Excel application then exits with an error, it doesn't close the Excel application, and it keeps running as a separate process even if this proces exits.
	This has to be handled much better!
*****/



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
    char szName[200];


    // Convert down to ANSI
    WideCharToMultiByte(CP_ACP, 0, Name, -1, szName, 256, NULL, NULL);

    // Get DISPID for name passed...
    hr = pDisp->GetIDsOfNames(IID_NULL, (wchar_t**)&Name, 1, LOCALE_USER_DEFAULT, &dispID);
    if(FAILED(hr))
        FatalError("ERROR(internal, excel) AutoWrap() failed.\n");//IDispatch::GetIDsOfNames(\"%s\") failed w/err 0x%08lx", szName, hr);

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
		FatalError("ERROR (internal): CLSIDFromProgID() failed.\n");
		return nullptr;
	}
	IDispatch *app;
	// Start server and get IDispatch
	hr = CoCreateInstance(clsid, NULL, CLSCTX_LOCAL_SERVER, IID_IDispatch, (void **)&app);
	if(FAILED(hr)) {
		FatalError("ERROR (internal): OfficeAutomation internal error. Application not registered properly.\n");
		return NULL;
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
	Var.bstrVal = SysAllocString(Name2);   //TODO: We need to free this later, but for now we leak it.
	
	return Var;
}

static VARIANT
OLEInt4Variant(int Value)
{
	VARIANT Var = OLENewVariant();
	Var.vt = VT_I4;
	Var.lVal = Value;
	return Var;
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

static void
OLEDestroyString(VARIANT *String)
{
	SysFreeString(String->bstrVal);
	String->bstrVal = nullptr;
}

//TODO: Make OLEDestroyString, and use it where needed!!

static char *
ColRowToCell(int Col, int Row, char *Buf) {
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

static VARIANT
OLEGetRangeMatrix(int FromRow, int ToRow, int FromCol, int ToCol, IDispatch *Sheet)
{
	IDispatch *Range;
	char RangeBuf[256];
	char *Range0 = &RangeBuf[0];
	Range0 = ColRowToCell(FromCol, FromRow, Range0);
	*Range0 = ':';
	ColRowToCell(ToCol, ToRow, Range0+1);
	//WarningPrint("Range is ", RangeBuf, "\n");
	
	VARIANT RangeString = OLEStringVariant(RangeBuf);
	Range = OLEGetObject(Sheet, L"Range", &RangeString);
	if(!Range)
		FatalError("ERROR(internal, excel): Failed to select range.\n");
	
	VARIANT Matrix = OLEGetValue(Range, L"Value");
	
	OLEDestroyString(&RangeString);
	
	Range->Release();
	
	return Matrix;
}

static VARIANT
OLEGetMatrixValue(VARIANT *Matrix, int Row, int Col)
{
	VARIANT Result = OLENewVariant();
	long Indices[2] = {Row, Col};
	HRESULT hr = SafeArrayGetElement(Matrix->parray, Indices, (void *)&Result);
	if(hr != S_OK)
		FatalError("ERROR(internal, excel): Unable to read matrix value.\n");
	
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

struct ole_handles
{
	IDispatch *App = nullptr;
	IDispatch *Books = nullptr;
	IDispatch *Book = nullptr;
	IDispatch *Sheet = nullptr;
};

static void
OLEOpenSpreadsheet(const char *Filepath, ole_handles *Handles)
{
	char FullPath[_MAX_PATH];
	if(_fullpath(FullPath, Filepath, _MAX_PATH ) == NULL)
		FatalError("ERROR(excel): Can't convert the relative path \"", Filepath, "\" to a full path.\n");
	
	OleInitialize(NULL);
	//Initialize COM for this thread
	CoInitialize(NULL);
	
	Handles->App = OLECreateObject(L"Excel.Application");
	
	VARIANT FileVar = OLEStringVariant(FullPath);
	
	Handles->Books = OLEGetObject(Handles->App, L"Workbooks");
	if(!Handles->Books)
		FatalError("ERROR(excel): Failed to initialize excel workbooks.\n");
	
	Handles->Book = OLEGetObject(Handles->Books, L"Open", &FileVar);
	if(!Handles->Book)
		FatalError("ERROR(excel): Failed to open file \"", Filepath, "\".\n");
	
	Handles->Sheet = OLEGetObject(Handles->Book, L"ActiveSheet");
	if(!Handles->Sheet)
		FatalError("ERROR(excel): Failed to open excel sheet.\n");
	
	OLEDestroyString(&FileVar);
	
	//Don't make this open the excel application in a way that is visible to the user.
	VARIANT Visible = OLEInt4Variant(0);
	OLESetValue(Handles->App, L"Visible", &Visible);
}

static void
OLECloseSpreadsheet(ole_handles *Handles)
{
	//TODO: This is not called if we error out, leaving the excel app running in memory!
	OLEAutoWrap(DISPATCH_METHOD, NULL, Handles->App, L"Quit", 0);
	
	Handles->Sheet->Release();
	Handles->Book->Release();
	Handles->Books->Release();
	Handles->App->Release();

	OleUninitialize();
	// Uninitialize COM for this thread...
	CoUninitialize();
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
		FatalError("ERROR(excel, internal): Failed to get Sheet object for tab ", Tab, ".\n");
	bool Success = OLEMethod(Handles->Sheet, L"Select");
	if(!Success)
		FatalError("ERROR(excel, internal): Failed to select tab ", Tab, ".\n");
}

static void
ReadInputDependenciesFromSpreadsheet(mobius_model *Model, const char *Inputfile)
{
	//TODO: We do a lot of unnecessary work by scanning the file twice. We should find a way to keep the structure instead...
	
	ole_handles Handles = {};
	OLEOpenSpreadsheet(Inputfile, &Handles);
	
	int NTabs = OLEGetNumTabs(&Handles);
	
	
	char Buf[512];
	
	for(int Tab = 0; Tab < NTabs; ++Tab)
	{
		OLEChooseTab(&Handles, Tab);
		
		
		std::vector<index_set_h> IndexSets;
		
		VARIANT Matrix = OLEGetRangeMatrix(2, 1026, 1, 1, Handles.Sheet); //NOTE: We only search for index sets among the first 1024 rows since anything more than that would be ridiculous.
		
		for(int Row = 0; Row < 1024; ++Row)
		{
			VARIANT IdxSetName = OLEGetMatrixValue(&Matrix, Row+1, 1);

			//TODO: Check if this is not just a date that was mis-formatted as a string

			OLEGetString(&IdxSetName, Buf, 512);
			//WarningPrint("Looking at index set at tab ", Tab, " row ", Row+2, " name ", Buf, "\n");
			if(strlen(Buf) > 0)
			{
				index_set_h IndexSet = GetIndexSetHandle(Model, Buf);
				IndexSets.push_back(IndexSet);
			}
			else
				break;
			
		}
		
		OLEDestroyMatrix(&Matrix);
		
		Matrix = OLEGetRangeMatrix(1, 1+IndexSets.size(), 2, 128, Handles.Sheet); //TODO: Ideally also do this in a loop in case there are more than 128 columns!
		
		input_h CurInput = {};
		for(int Col = 0; Col < 127; ++Col)
		{
			
			//TODO: There is a discrepancy here where it could find data after several blank columns, while in the input reading later, such columns will be ignored!
			
			VARIANT Name = OLEGetMatrixValue(&Matrix, 1, Col+1);
			
			OLEGetString(&Name, Buf, 512);
			if(strlen(Buf) > 0)
			{
				if(Model->Inputs.Has(Buf)) // Register as "additional input" if it was not already registered with the model
					CurInput = GetInputHandle(Model, Buf);
				else
				{
					token_string InputName = token_string(Buf).Copy(&Model->BucketMemory);
					CurInput = RegisterInput(Model, InputName.Data, {}, true, true);   //TODO: should we allow provision of units?
				}
			}
		
			if(IsValid(CurInput))
			{
				//NOTE: We don't check subsequent columns of this input since if the format is correct, the first given column has to be indexed by all the indexes that are relevant.
				if(Model->Inputs[CurInput].IndexSetDependencies.size() > 0)
					continue;
				
				for(int Row = 0; Row < IndexSets.size(); ++Row)
				{
					VARIANT IndexName = OLEGetMatrixValue(&Matrix, Row+2, Col+1);
					
					OLEGetString(&IndexName, Buf, 512);
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
	
	for(int Tab = 0; Tab < NTabs; ++Tab)
	{
		OLEChooseTab(&Handles, Tab);
		
		Dates[Tab].reserve(RowsAtATime);
		
		int SuperRow = 2;
		
		FirstDateRow[Tab] = -1;
		bool FoundFirst = false;
		while(true)
		{
			bool BreakOut = false;
			VARIANT Matrix = OLEGetRangeMatrix(SuperRow, SuperRow + RowsAtATime - 1, 1, 1, Handles.Sheet);
			for(int R = 0; R < RowsAtATime; ++R)
			{
				int MatRow = 1 + R;
				int Row = SuperRow + R;
				VARIANT Var = OLEGetMatrixValue(&Matrix, MatRow, 1);
				
				datetime Date;
				bool Success = OLEGetDate(&Var, &Date);
				
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
				else if(FoundFirst) //We are at the end of the set of valid dates.
				{
					BreakOut = true;
					break;
				}
			}
			
			SuperRow += RowsAtATime;
			
			if(BreakOut) break;
			
			if(!FoundFirst)
				FatalError("ERROR(excel): Not able to find a date among the first ", RowsAtATime, " cells of column A.\n");
			
			OLEDestroyMatrix(&Matrix);
		}
	}
	
	if(Dates.size() == 0)
		return;
	
	//WarningPrint("Dates size ", Dates.size(), "\n");
	
	std::vector<datetime> DatesSort;
	for(int Tab = 0; Tab < NTabs; ++Tab)
		DatesSort.insert(DatesSort.end(), Dates[Tab].begin(), Dates[Tab].end());
	std::sort(DatesSort.begin(), DatesSort.end());
	
	DataSet->InputDataStartDate = DatesSort[0];
	DataSet->InputDataHasSeparateStartDate = true;

	s64 Step = FindTimestep(DataSet->InputDataStartDate, DatesSort[DatesSort.size()-1], DataSet->Model->TimestepSize);
	Step += 1;    //NOTE: Because the end date is inclusive. 
	if(Step <= 0)
		FatalError("The input data end date was set to be earlier than the input data start date.\n"); //NOTE: Should technically not be possible.
	
	u64 Timesteps = (u64)Step;
	AllocateInputStorage(DataSet, Timesteps);
	
	char Buf[512];
	for(int Tab = 0; Tab < NTabs; ++Tab)
	{
		std::vector<size_t> Offsets;
		
		OLEChooseTab(&Handles, Tab);
		
		VARIANT Matrix = OLEGetRangeMatrix(1, FirstDateRow[Tab]-1, 2, 128, Handles.Sheet); //TODO: Ideally also do this in a loop in case there are more than 128 rows!
		
		input_h CurInput = {};
		for(int Col = 0; Col < 127; ++Col)
		{
			VARIANT Name = OLEGetMatrixValue(&Matrix, 1, Col+1);
			//TODO: Do a OLEGetString instead in case they are misformatted
			
			bool GotNameThisColumn = false;
			
			OLEGetString(&Name, Buf, 512);
			if(strlen(Buf) > 0)
			{
				CurInput = GetInputHandle(DataSet->Model, Buf);
				GotNameThisColumn = true;
			}
			else if(!IsValid(CurInput))
				FatalError("ERROR(excel): In tab ", Tab, " there is no input name in cell B1.\n");
			
			size_t StorageUnitIndex = DataSet->InputStorageStructure.UnitForHandle[CurInput.Handle];
			array<index_set_h> &IndexSets = DataSet->InputStorageStructure.Units[StorageUnitIndex].IndexSets;
			
			std::vector<index_t> Indexes;
			
			int IdxSetNum = 0;
			for(int Row = 0; Row < IndexSets.size(); ++Row)
			{
				VARIANT IndexName = OLEGetMatrixValue(&Matrix, Row+2, Col+1);
				
				OLEGetString(&IndexName, Buf, 512);
				if(strlen(Buf) > 0)
				{
					//TODO: We should actually test that IndexSets[IdxSetNum] is the same as the index set given in cell A.Row.
					index_t Index = GetIndex(DataSet, IndexSets[IdxSetNum], Buf);
					Indexes.push_back(Index);
					++IdxSetNum;
				}
			}
			
			// An entirely blank column (in the header at least) signifies that we should ignore the rest of the columns;
			if(!GotNameThisColumn && Indexes.empty())
				break;
			
			size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, Indexes.data(), Indexes.size(), DataSet->IndexCounts, CurInput);
			//size_t Offset = OffsetForHandle(DataSet->InputStorageStructure, CurInput);
			Offsets.push_back(Offset);
			DataSet->InputTimeseriesWasProvided[Offset] = true;
			
			//WarningPrint(GetName(DataSet->Model, CurInput), " ", Offset, "\n");
		}
		
		OLEDestroyMatrix(&Matrix);

		// Select the rest of the data!
		Matrix = OLEGetRangeMatrix(FirstDateRow[Tab], FirstDateRow[Tab]-1 + Dates[Tab].size(), 2, 1+Offsets.size(), Handles.Sheet);
		
		double *WriteToBase = DataSet->InputData;
		for(int Row = 0; Row < Dates[Tab].size(); ++Row)
		{
			s64 Timestep = FindTimestep(DataSet->InputDataStartDate, Dates[Tab][Row], DataSet->Model->TimestepSize);
			if(Timestep < 0 || Timestep >= DataSet->InputDataTimesteps)
				FatalError("ERROR(internal): Something went wrong with setting the input data timesteps in the spreadsheet reader.\n"); //NOTE: this should not happen unless the code above is worng
			double *WriteTo = WriteToBase + Timestep*DataSet->InputStorageStructure.TotalCount;
			for(int Col = 0; Col < Offsets.size(); ++Col)
			{
				VARIANT Value = OLEGetMatrixValue(&Matrix, Row+1, Col+1);
				double Val = OLEGetDouble(&Value);
				
				*(WriteTo + Offsets[Col]) = Val;
			}
		}
		
		OLEDestroyMatrix(&Matrix);
	}
	
	
	OLECloseSpreadsheet(&Handles);
}

#endif //_WIN32