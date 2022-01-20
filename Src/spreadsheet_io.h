
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

#include <windows.h>
#include <oleauto.h>



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

char *
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
	return itoa(Row, Buf, 10)+1;
}

static VARIANT
OLEGetRangeMatrix(int FromRow, int ToRow, int FromCol, int ToCol, IDispatch *Sheet, IDispatch **Range)
{
	char RangeBuf[256];
	char *Range0 = &RangeBuf[0];
	Range0 = ColRowToCell(FromCol, FromRow, Range0);
	*Range0 = ':';
	ColRowToCell(ToCol, ToRow, Range0+1);
	//WarningPrint("Range is ", RangeBuf, "\n");
	
	VARIANT RangeString = OLEStringVariant(RangeBuf);
	*Range = OLEGetObject(Sheet, L"Range", &RangeString);
	if(!Range)
		FatalError("ERROR(internal, excel): Failed to select range.\n");
	
	VARIANT Matrix = OLEGetValue(*Range, L"Value");
	
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
		return Success;
	}
	return false;
}


static void
ReadInputDependenciesFromSpreadsheet(mobius_data_set *DataSet, const char *Inputfile)
{
	
}

static void
ReadInputsFromSpreadsheet(mobius_data_set *DataSet, const char *Inputfile)
{
	//TODO: This needs an absolute path for the file name, so we need to get that!
	
	OleInitialize(NULL);
	//Initialize COM for this thread
	CoInitialize(NULL);
	
	IDispatch *App = OLECreateObject(L"Excel.Application");
	
	//Don't make this open the excel application in a way that is visible to the user.
	VARIANT Visible = OLEInt4Variant(0);
	OLESetValue(App, L"Visible", &Visible);
	
	VARIANT FileVar = OLEStringVariant(Inputfile);
	
	IDispatch *Books = OLEGetObject(App, L"Workbooks");
	if(!Books)
		FatalError("ERROR(excel): Failed to initialize excel workbooks.\n");
	
	IDispatch *Book = OLEGetObject(Books, L"Open", &FileVar);
	if(!Book)
		FatalError("ERROR(excel): Failed to open file \"", Inputfile, "\".\n");
	
	IDispatch *Sheet = OLEGetObject(Book, L"ActiveSheet");
	if(!Sheet)
		FatalError("ERROR(excel): Failed to open excel sheet.\n");
	
	IDispatch *Range = nullptr;
	
	
	//NOTE: Ideally there would be a way to get the number of nonempty rows directly, but it doesn't seem so (??)
	
	
	constexpr int RowsAtATime = 1024;
	std::vector<datetime> Dates;
	Dates.reserve(RowsAtATime);
	
	int SuperRow = 2;  //TODO: Why doesn't it work with SuperRow = 2?
	bool FoundFirst = false;
	while(true)
	{
		bool BreakOut = false;
		VARIANT Matrix = OLEGetRangeMatrix(SuperRow, SuperRow + RowsAtATime - 1, 1, 1, Sheet, &Range);
		for(int R = 0; R < RowsAtATime; ++R)
		{
			int Row = 1 + R;
			VARIANT Var = OLEGetMatrixValue(&Matrix, Row, 1);
			
			datetime Date;
			bool Success = OLEGetDate(&Var, &Date);
			
			if(Success)
			{
				Dates.push_back(Date);
				WarningPrint(Date.ToString(), "\n");
			}
			
			if(!FoundFirst && Success) FoundFirst = true;
			if(FoundFirst && !Success)
			{
				BreakOut = true;
				break;
			}
		}
		
		if(BreakOut) break;
		
		if(!FoundFirst)
			FatalError("ERROR(excel): Not able to find a date among the first ", RowsAtATime, " cells of column A.\n");
		
		OLEDestroyMatrix(&Matrix);
		if(Range) Range->Release();
	}
	
	//TODO: Find first and last date and allocate accordingly.
	
	std::vector<std::string> InputNames;
	InputNames.reserve(100);
	VARIANT Matrix = OLEGetRangeMatrix(1, 1, 2, 128, Sheet, &Range); //TODO: Ideally also do this in a loop in case there are more than 128 rows!
	
	for(int Col = 1; Col <= 127; ++Col)
	{
		VARIANT Name = OLEGetMatrixValue(&Matrix, 1, Col);
		//TODO: Do a OLEGetString instead in case they are misformatted
		if(Name.vt == VT_BSTR)
		{
			char Buf[512];
			WideCharToMultiByte(CP_ACP, 0, Name.bstrVal, -1, Buf, 512, NULL, NULL);
			if(strlen(Buf) > 0)
			{
				InputNames.push_back(std::string(Buf));
				WarningPrint(Buf, "\n");
			}
			else
				break;
		}
		else
			break;
	}
	
	OLEDestroyMatrix(&Matrix);
	if(Range) Range->Release();
	
	/*
	VARIANT x = OLEInt4Variant(1);
	VARIANT y = OLEInt4Variant(1);
	
	IDispatch *Cell = OLEGetObject(Sheet, L"Cells", &x, &y);
	VARIANT Result = OLEGetValue(Cell, L"value");
	
	WarningPrint("Type of result is ", Result.vt, "\n");
	if(Result.vt == VT_BSTR)
	{
		char OutVal[512];
		WideCharToMultiByte(CP_ACP, 0, Result.bstrVal, -1, OutVal, 512, NULL, NULL);
		WarningPrint("String value is ", OutVal, "\n");
	}
	*/
	
	//TODO: This is not called if we error out, leaving the excel app running in memory!
	OLEAutoWrap(DISPATCH_METHOD, NULL, App, L"Quit", 0);
	
	
	Sheet->Release();
	Book->Release();
	Books->Release();
	App->Release();
	//VariantClear(&arr);

	OleUninitialize();
	// Uninitialize COM for this thread...
	CoUninitialize();
}

#endif //_WIN32