


static void
ReadGLUESetupFromFile(glue_setup *Setup, const char *Filename)
{
	token_stream Stream(Filename);
	
	while(true)
	{
		token Token = Stream.PeekToken();
		
		bool ReadObs = false;
		
		if(Token.Type == TokenType_EOF)
			break;

		token_string Section = Stream.ExpectUnquotedString();
		Stream.ExpectToken(TokenType_Colon);

		if(Section.Equals("num_runs"))
		{
			size_t NumRuns = (size_t)Stream.ExpectUInt();
			if(NumRuns == 0)
			{
				Stream.PrintErrorHeader();
				MOBIUS_FATAL_ERROR("Expected at least 1 run." << std::endl);
			}
			Setup->NumRuns = NumRuns;
		}
		else if(Section.Equals("num_threads"))
		{
			size_t NumThreads = (size_t)Stream.ExpectUInt();
			if(NumThreads == 0)
			{
				Stream.PrintErrorHeader();
				MOBIUS_FATAL_ERROR("Expected at least 1 thread." << std::endl);
			}
			Setup->NumThreads = NumThreads;
		}
		else if(Section.Equals("discard_timesteps"))
		{
			Setup->DiscardTimesteps = (size_t)Stream.ExpectUInt();
		}
		else if(Section.Equals("parameter_calibration"))
		{
			ReadParameterCalibration(Stream, Setup->Calibration, ParameterCalibrationReadDistribution);
		}
		else if(Section.Equals("objectives"))
		{
			ReadCalibrationObjectives(Stream, Setup->Objectives, true);
		}
		else if(Section.Equals("quantiles"))
		{
			Stream.ReadDoubleSeries(Setup->Quantiles);
		}
		else
		{
			Stream.PrintErrorHeader();
			MOBIUS_FATAL_ERROR("Unknown section name: " << Section << std::endl);
		}
	}
	
	//TODO: Check that the file contained enough data?
}




static void
WriteGLUEResultsToDatabase(const char *Dbname, glue_setup *Setup, glue_results *Results, mobius_data_set *DataSet)
{
	sqlite3 *Db;
	int rc = sqlite3_open_v2(Dbname, &Db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
	if(rc != SQLITE_OK)
	{
		MOBIUS_FATAL_ERROR("ERROR: Unable to open database " << Dbname << " Message: " << sqlite3_errmsg(Db) << std::endl);
	}
	
	rc = sqlite3_exec(Db, "BEGIN TRANSACTION;", 0, 0, 0);
	
	sqlite3_stmt *CreateTableStmt;
	
	const char *CreateParameterTable =
		"CREATE TABLE Parameters (ID INTEGER NOT NULL PRIMARY KEY, Name TEXT, Indexes TEXT);";
	rc = sqlite3_prepare_v2(Db, CreateParameterTable, -1, &CreateTableStmt, 0);
	rc = sqlite3_step(CreateTableStmt);
	rc = sqlite3_finalize(CreateTableStmt);
		
	const char *CreateRunTable =
		"CREATE TABLE Run (ID INTEGER NOT NULL PRIMARY KEY, Performance DOUBLE, WeightedPerformance DOUBLE);";
	rc = sqlite3_prepare_v2(Db, CreateRunTable, -1, &CreateTableStmt, 0);
	rc = sqlite3_step(CreateTableStmt);
	rc = sqlite3_finalize(CreateTableStmt);
		
	const char *CreateParameterSetsTable =
		"CREATE TABLE ParameterSets (ParameterID INTEGER, RunID INTEGER, Value DOUBLE, FOREIGN KEY(ParameterID) REFERENCES Parameters(ID), FOREIGN KEY(RunID) REFERENCES Run(ID));";
	rc = sqlite3_prepare_v2(Db, CreateParameterSetsTable, -1, &CreateTableStmt, 0);
	rc = sqlite3_step(CreateTableStmt);
	rc = sqlite3_finalize(CreateTableStmt);
	
	const char *CreateQuantileTable =
		"CREATE TABLE Quantiles (ID INTEGER NOT NULL PRIMARY KEY, Quantile DOUBLE)";
	rc = sqlite3_prepare_v2(Db, CreateQuantileTable, -1, &CreateTableStmt, 0);
	rc = sqlite3_step(CreateTableStmt);
	rc = sqlite3_finalize(CreateTableStmt);
		
	const char *CreateDistributionTable =
		"CREATE TABLE Distribution (QuantileID INTEGER, Date BIGINT, Value DOUBLE, FOREIGN KEY(QuantileID) REFERENCES Quantiles(ID));";
	rc = sqlite3_prepare_v2(Db, CreateDistributionTable, -1, &CreateTableStmt, 0);
	rc = sqlite3_step(CreateTableStmt);
	rc = sqlite3_finalize(CreateTableStmt);
	
	
	const char *InsertParameterInfo = "INSERT INTO Parameters (ID, Name, Indexes) VALUES (?, ?, ?);";
	
	sqlite3_stmt *InsertParameterInfoStmt;
	rc = sqlite3_prepare_v2(Db, InsertParameterInfo, -1, &InsertParameterInfoStmt, 0);
	int ParID = 1;
	
	for(parameter_calibration &Cal : Setup->Calibration)
	{
		for(size_t Par = 0; Par < Cal.ParameterNames.size(); ++Par)
		{
			//TODO: Instead having the indexes as text (which is difficult to parse in an automatic system later), we should maybe instead hook the parameter ID to the structure ID you get when you export it using WriteParametersToDatabase from mobius_database_io.cpp, however that is complicated..
			
			const char *ParameterName = Cal.ParameterNames[Par];
			rc = sqlite3_bind_int(InsertParameterInfoStmt, 1, ParID);
			rc = sqlite3_bind_text(InsertParameterInfoStmt, 2, ParameterName, -1, SQLITE_STATIC);
			char Indexes[4096];
			char *Pos = Indexes;
			for(const char *Index : Cal.ParameterIndexes[Par])
			{
				Pos += sprintf(Pos, " \"%s\"", Index);
			}
			rc = sqlite3_bind_text(InsertParameterInfoStmt, 3, Indexes, -1, SQLITE_STATIC);
			
			rc = sqlite3_step(InsertParameterInfoStmt);
			rc = sqlite3_reset(InsertParameterInfoStmt);
			
			++ParID;
		}
	}
	rc = sqlite3_finalize(InsertParameterInfoStmt);
	
	
	const char *InsertRunInfo = "INSERT INTO Run (ID, Performance, WeightedPerformance) VALUES (?, ?, ?);";
	
	const char *InsertParameterSetInfo = "INSERT INTO ParameterSets (ParameterID, RunID, Value) VALUES (?, ?, ?);";
	
	sqlite3_stmt *InsertRunInfoStmt;
	rc = sqlite3_prepare_v2(Db, InsertRunInfo, -1, &InsertRunInfoStmt, 0);
	
	sqlite3_stmt *InsertParameterSetInfoStmt;
	rc = sqlite3_prepare_v2(Db, InsertParameterSetInfo, -1, &InsertParameterSetInfoStmt, 0);
	
	for(size_t Run = 0; Run < Setup->NumRuns; ++Run)
	{
		int RunID = (int)Run + 1;
		double Performance = Results->RunData[Run].PerformanceMeasures[0].first;			//TODO: Support multiple objectives eventually
		double WeightedPerformance = Results->RunData[Run].PerformanceMeasures[0].second;
		rc = sqlite3_bind_int(InsertRunInfoStmt, 1, RunID);
		rc = sqlite3_bind_double(InsertRunInfoStmt, 2, Performance);
		rc = sqlite3_bind_double(InsertRunInfoStmt, 3, WeightedPerformance);
		
		rc = sqlite3_step(InsertRunInfoStmt);
		rc = sqlite3_reset(InsertRunInfoStmt);
		
		ApplyCalibrations(DataSet, Setup->Calibration, Results->RunData[Run].RandomParameters.data()); //NOTE: We just apply the calibration so that we can read the values from the dataset again without having to copy the code that assigns values to individual parameters here.
		
		int ParID = 0;
		for(size_t CalIdx = 0; CalIdx < Setup->Calibration.size(); ++CalIdx)
		{
			parameter_calibration &Cal = Setup->Calibration[CalIdx];
			for(size_t Par = 0; Par < Cal.ParameterNames.size(); ++Par)
			{
				double Value = GetParameterDouble(DataSet, Cal.ParameterNames[Par], Cal.ParameterIndexes[Par]);
				
				rc = sqlite3_bind_int(InsertParameterSetInfoStmt, 1, ParID);
				rc = sqlite3_bind_int(InsertParameterSetInfoStmt, 2, RunID);
				rc = sqlite3_bind_double(InsertParameterSetInfoStmt, 3, Value);
				
				rc = sqlite3_step(InsertParameterSetInfoStmt);
				rc = sqlite3_reset(InsertParameterSetInfoStmt);
				
				++ParID;
			}
		}
	}
	rc = sqlite3_finalize(InsertRunInfoStmt);
	rc = sqlite3_finalize(InsertParameterSetInfoStmt);
	
	
	const char *InsertQuantileInfo = "INSERT INTO Quantiles (ID, Quantile) VALUES (?, ?);";
	sqlite3_stmt *InsertQuantileInfoStmt;
	rc = sqlite3_prepare_v2(Db, InsertQuantileInfo, -1, &InsertQuantileInfoStmt, 0);
	
	const char *InsertDistributionInfo = "INSERT INTO Distribution (QuantileID, Date, Value) VALUES (?, ?, ?);";
	sqlite3_stmt *InsertDistributionInfoStmt;
	rc = sqlite3_prepare_v2(Db, InsertDistributionInfo, -1, &InsertDistributionInfoStmt, 0);
	
	s64 StartDate = GetStartDate(DataSet).SecondsSinceEpoch;
	
	for(size_t QuantileIdx = 0; QuantileIdx < Setup->Quantiles.size(); ++QuantileIdx)
	{
		int QuantileID = (int)QuantileIdx + 1;
		double Quantile = Setup->Quantiles[QuantileIdx];
		rc = sqlite3_bind_int(InsertQuantileInfoStmt, 1, QuantileID);
		rc = sqlite3_bind_double(InsertQuantileInfoStmt, 2, Quantile);
		
		rc = sqlite3_step(InsertQuantileInfoStmt);
		rc = sqlite3_reset(InsertQuantileInfoStmt);
		
		for(size_t Timestep = 0; Timestep < Results->PostDistribution[QuantileIdx].size(); ++Timestep)
		{
			s64 Date = StartDate + (s64)Timestep * 86400; //NOTE: Dependent on the size of the timestep...
			double Value = Results->PostDistribution[QuantileIdx][Timestep];
			rc = sqlite3_bind_int(InsertDistributionInfoStmt, 1, QuantileID);
			rc = sqlite3_bind_int64(InsertDistributionInfoStmt, 2, Date);
			rc = sqlite3_bind_double(InsertDistributionInfoStmt, 3, Value);
			
			rc = sqlite3_step(InsertDistributionInfoStmt);
			rc = sqlite3_reset(InsertDistributionInfoStmt);
		}
	}
	
	rc = sqlite3_finalize(InsertQuantileInfoStmt);
	rc = sqlite3_finalize(InsertDistributionInfoStmt);
	
	rc = sqlite3_exec(Db, "COMMIT;", 0, 0, 0);
	
	sqlite3_close(Db);
}
