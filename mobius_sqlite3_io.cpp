
#if !defined(MOBIUS_SQLITE3_IO_CPP)

//TODO: There is almost no error handling in this file..

#include <chrono>
#include "sqlite3/sqlite3.h"

static void
WriteStructureEntryToDatabase(sqlite3 *Db, int ID, const char *Name, int Lft, int Rgt, int Dpt, const char *Type, const char *Unit, const char *Description, bool IsIndexer, bool IsIndex)
{
	const char *InsertParameterStructureEntry =
		"INSERT INTO ParameterStructure (ID, name, lft, rgt, dpt, type, unit, description, isIndexer, isIndex) "
		"VALUES "
		"(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
	
	sqlite3_stmt *Statement;
	int rc = sqlite3_prepare_v2(Db, InsertParameterStructureEntry, -1, &Statement, 0);
	
	rc = sqlite3_bind_int(Statement, 1, ID);
	rc = sqlite3_bind_text(Statement, 2, Name, -1, SQLITE_STATIC);
	rc = sqlite3_bind_int(Statement, 3, Lft);
	rc = sqlite3_bind_int(Statement, 4, Rgt);
	rc = sqlite3_bind_int(Statement, 5, Dpt);
	if(Type) rc = sqlite3_bind_text(Statement, 6, Type, -1, SQLITE_STATIC);
	else     rc = sqlite3_bind_null(Statement, 6);
	if(Unit) rc = sqlite3_bind_text(Statement, 7, Unit, -1, SQLITE_STATIC);
	else     rc = sqlite3_bind_null(Statement, 7);
	if(Description) rc = sqlite3_bind_text(Statement, 8, Description, -1, SQLITE_STATIC);
	else            rc = sqlite3_bind_null(Statement, 8);
	rc = sqlite3_bind_int(Statement, 9, IsIndexer);
	rc = sqlite3_bind_int(Statement, 10, IsIndex);
	
	rc = sqlite3_step(Statement);
	sqlite3_finalize(Statement);
}

static void
WriteParameterValueToDatabase(sqlite3 *Db, int ID, parameter_value Min, parameter_value Max, parameter_value Value, parameter_type Type)
{
	const char *Typenames[4] = { "double", "int", "bool", "ptime" }; //NOTE: Volatile! Depends on these being in the same order as in the definition of parameter_type.
	char InsertValues[512];
	sprintf(InsertValues,
		"INSERT INTO ParameterValues_%s (ID, minimum, maximum, value) "
		"VALUES "
		"(?, ?, ?, ?)",
		Typenames[Type]);
		
	sqlite3_stmt *Statement;
	int rc = sqlite3_prepare_v2(Db, InsertValues, -1, &Statement, 0);
	
	rc = sqlite3_bind_int(Statement, 1, ID);
	if(Type == ParameterType_Double)
	{
		rc = sqlite3_bind_double(Statement, 2, Min.ValDouble);
		rc = sqlite3_bind_double(Statement, 3, Max.ValDouble);
		rc = sqlite3_bind_double(Statement, 4, Value.ValDouble);
	}
	else if(Type == ParameterType_UInt)
	{
		rc = sqlite3_bind_int64(Statement, 2, (s64)Min.ValUInt);
		rc = sqlite3_bind_int64(Statement, 3, (s64)Max.ValUInt);
		rc = sqlite3_bind_int64(Statement, 4, (s64)Value.ValUInt);
	}
	else if(Type == ParameterType_Bool)
	{
		rc = sqlite3_bind_int(Statement, 2, Min.ValBool);
		rc = sqlite3_bind_int(Statement, 3, Max.ValBool);
		rc = sqlite3_bind_int(Statement, 4, Value.ValBool);
	}
	else if(Type == ParameterType_Time)
	{
		rc = sqlite3_bind_int64(Statement, 2, Min.ValTime.SecondsSinceEpoch);
		rc = sqlite3_bind_int64(Statement, 3, Max.ValTime.SecondsSinceEpoch);
		rc = sqlite3_bind_int64(Statement, 4, Value.ValTime.SecondsSinceEpoch);
	}
	
	rc = sqlite3_step(Statement);
	sqlite3_finalize(Statement);
}

static void
WriteParametersForParameterGroupToDatabase(mobius_data_set *DataSet, entity_handle ParameterGroupHandle, sqlite3 *Db, int &RunningID, int& RunningLft, int Dpt, index_t *Indexes, size_t IndexCount)
{
	const mobius_model *Model = DataSet->Model;
	const parameter_group_spec &Spec = Model->ParameterGroupSpecs[ParameterGroupHandle];
	for(entity_handle ParameterHandle : Spec.Parameters)
	{
		const parameter_spec &ParSpec = Model->ParameterSpecs[ParameterHandle];
		
		if(ParSpec.ShouldNotBeExposed) continue;
		
		int IdOfParameter = RunningID++;
		int LftOfParameter = RunningLft++;
		int RgtOfParameter = RunningLft++;
		
		const char *Unit = 0;
		if(IsValid(ParSpec.Unit)) Unit = GetName(Model, ParSpec.Unit);
		const char *Description = ParSpec.Description;
		switch(ParSpec.Type)
		{
			//TODO: Write out the values too.
			case ParameterType_Double:
				WriteStructureEntryToDatabase(Db, IdOfParameter, ParSpec.Name, LftOfParameter, RgtOfParameter, Dpt, "DOUBLE", Unit, Description, false, false);
				break;
			case ParameterType_UInt:
				WriteStructureEntryToDatabase(Db, IdOfParameter, ParSpec.Name, LftOfParameter, RgtOfParameter, Dpt, "UINT", Unit, Description, false, false);
				break;
			case ParameterType_Bool:
				WriteStructureEntryToDatabase(Db, IdOfParameter, ParSpec.Name, LftOfParameter, RgtOfParameter, Dpt, "BOOL", Unit, Description, false, false);
				break;
			case ParameterType_Time:
				WriteStructureEntryToDatabase(Db, IdOfParameter, ParSpec.Name, LftOfParameter, RgtOfParameter, Dpt, "PTIME", Unit, Description, false, false);
				break;
			
		}
		size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, Indexes, IndexCount, DataSet->IndexCounts, ParameterHandle);
		parameter_value Value = DataSet->ParameterData[Offset];
		WriteParameterValueToDatabase(Db, IdOfParameter, ParSpec.Min, ParSpec.Max, Value, ParSpec.Type);
	}
}

static void
ExportParameterGroupRecursivelyToDatabase(mobius_data_set *DataSet, entity_handle ParameterGroupHandle, sqlite3 *Db, int &RunningID, int& RunningLft, int Dpt, index_t *Indexes)
{
	const mobius_model *Model = DataSet->Model;
	const parameter_group_spec &Spec = Model->ParameterGroupSpecs[ParameterGroupHandle];
	
	int IDOfGroup = RunningID++;
	int LftOfGroup = RunningLft++;
	
	index_set_h IndexSet = Spec.IndexSet;
	if(IsValid(IndexSet))
	{
		size_t Level = (size_t) (Dpt + 1) / 2; //NOTE: This is kind of abusive and depends on the structure not changing a lot;
		
		for(index_t Index = {IndexSet, 0}; Index < DataSet->IndexCounts[IndexSet.Handle]; ++Index)
		{
			Indexes[Level-1] = Index;
			
			int IdOfIndex = RunningID++;
			int LftOfIndex = RunningLft++;
			
			WriteParametersForParameterGroupToDatabase(DataSet, ParameterGroupHandle, Db, RunningID, RunningLft, Dpt + 2 , Indexes, Level);
			
			for(parameter_group_h ChildGroup : Spec.ChildrenGroups)
			{
				ExportParameterGroupRecursivelyToDatabase(DataSet, ChildGroup.Handle, Db, RunningID, RunningLft, Dpt + 2, Indexes);
			}
			
			int RgtOfIndex = RunningLft++;
			int DptOfIndex = Dpt + 1;
			const char *IndexName = DataSet->IndexNames[IndexSet.Handle][Index];
			WriteStructureEntryToDatabase(Db, IdOfIndex, IndexName, LftOfIndex, RgtOfIndex, DptOfIndex, 0, 0, 0, false, true);
		}
	}
	else
	{
		WriteParametersForParameterGroupToDatabase(DataSet, ParameterGroupHandle, Db, RunningID, RunningLft, Dpt + 1 , Indexes, 0); //NOTE: May break if a group without an index set has a parent group?
	}
	
	int RgtOfGroup = RunningLft++;
	
	WriteStructureEntryToDatabase(Db, IDOfGroup, Spec.Name, LftOfGroup, RgtOfGroup, Dpt, 0, 0, 0, true, false);
}

static void
WriteParametersToDatabase(mobius_data_set *DataSet, const char *Dbname, const char *Exename = "")
{
	//NOTE: Deletes any (potentially) existing database of the same name.
	//TODO: We should figure out if it is safe to have this here?
	remove(Dbname);
	
	sqlite3 *Db;
	int rc = sqlite3_open_v2(Dbname, &Db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
	if(rc != SQLITE_OK)
	{
		MOBIUS_FATAL_ERROR("ERROR: Unable to open database " << Dbname << " Message: " << sqlite3_errmsg(Db) << std::endl);
	}
	
	//TODO: Check if the tables already exist?
	
	const char *CreateParameterStructureTable = 
		"CREATE TABLE ParameterStructure ("
		"ID INTEGER PRIMARY KEY NOT NULL, "
		"name TEXT, "
		"lft INTEGER NOT NULL, "
		"rgt INTEGER NOT NULL, "
		"dpt INTEGER NOT NULL, "
		"type TEXT, "
		"unit TEXT, "
		"description TEXT, "
		"isIndexer BOOLEAN, "
		"isIndex BOOLEAN )";
	
	sqlite3_stmt *Statement;
	rc = sqlite3_prepare_v2(Db, CreateParameterStructureTable, -1, &Statement, 0);
	
	rc = sqlite3_step(Statement);
	sqlite3_finalize(Statement);
	
	const char *CreateBranchInputTable =
		"CREATE TABLE BranchInputs ("
		"InputOf INTEGER, "
		"Input INTEGER, "
		"IndexSet Text )";
		
	rc = sqlite3_prepare_v2(Db, CreateBranchInputTable, -1, &Statement, 0);
	
	rc = sqlite3_step(Statement);
	sqlite3_finalize(Statement);
	
	const char *CreateParameterDoubleTable = 
		"CREATE TABLE ParameterValues_double ("
		"ID INTEGER PRIMARY KEY NOT NULL, "
		"minimum DOUBLE NOT NULL, "
		"maximum DOUBLE NOT NULL, "
		"value DOUBLE NOT NULL )";
		
	rc = sqlite3_prepare_v2(Db, CreateParameterDoubleTable, -1, &Statement, 0);
	
	rc = sqlite3_step(Statement);
	sqlite3_finalize(Statement);
	
	const char *CreateParameterIntTable = 
		"CREATE TABLE ParameterValues_int ("
		"ID INTEGER PRIMARY KEY NOT NULL, "
		"minimum BIGINT NOT NULL, "
		"maximum BIGINT NOT NULL, "
		"value BIGINT NOT NULL )";
		
	rc = sqlite3_prepare_v2(Db, CreateParameterIntTable, -1, &Statement, 0);
	
	rc = sqlite3_step(Statement);
	sqlite3_finalize(Statement);
	
	const char *CreateParameterBoolTable = 
		"CREATE TABLE ParameterValues_bool ("
		"ID INTEGER PRIMARY KEY NOT NULL, "
		"minimum BOOLEAN NOT NULL, "
		"maximum BOOLEAN NOT NULL, "
		"value BOOLEAN NOT NULL )";
		
	rc = sqlite3_prepare_v2(Db, CreateParameterBoolTable, -1, &Statement, 0);
	
	rc = sqlite3_step(Statement);
	sqlite3_finalize(Statement);
	
	const char *CreateParameterTimeTable = 
		"CREATE TABLE ParameterValues_ptime ("
		"ID INTEGER PRIMARY KEY NOT NULL, "
		"minimum BIGINT NOT NULL, "
		"maximum BIGINT NOT NULL, "
		"value BIGINT NOT NULL )";
		
	rc = sqlite3_prepare_v2(Db, CreateParameterTimeTable, -1, &Statement, 0);
	
	rc = sqlite3_step(Statement);
	sqlite3_finalize(Statement);
	
	const char *CreateInfoTable =
		"CREATE TABLE Info ("
		"ModelName TEXT, "
		"ModelVersion TEXT, "
		"CreationDate TEXT, "
		"Exename TEXT )";
		
	rc = sqlite3_prepare_v2(Db, CreateInfoTable, -1, &Statement, 0);
	
	rc = sqlite3_step(Statement);
	sqlite3_finalize(Statement);
	
	const mobius_model *Model = DataSet->Model;
	
	sqlite3_exec(Db, "BEGIN TRANSACTION;", 0, 0, 0);
	
	const char *InsertModelInfo =
		"INSERT INTO Info (ModelName, ModelVersion, CreationDate, Exename) "
		"VALUES "
		"(?, ?, ?, ?)";
		
	char *Datestr;
	{
		using namespace std::chrono;
		system_clock::time_point P = system_clock::now();
		std::time_t T = system_clock::to_time_t(P);
		Datestr = std::ctime(&T);
	}
		
	rc = sqlite3_prepare_v2(Db, InsertModelInfo, -1, &Statement, 0);
	rc = sqlite3_bind_text(Statement, 1, Model->Name, -1, SQLITE_STATIC);
	rc = sqlite3_bind_text(Statement, 2, Model->Version, -1, SQLITE_STATIC);
	rc = sqlite3_bind_text(Statement, 3, Datestr, -1, SQLITE_STATIC);
	rc = sqlite3_bind_text(Statement, 4, Exename, -1, SQLITE_STATIC);
	
	if(rc != SQLITE_OK)
	{
		MOBIUS_FATAL_ERROR("ERROR: Inserting model info in database " << Dbname << ". Message: " << sqlite3_errmsg(Db) << std::endl);
	}
	
	sqlite3_step(Statement);
	sqlite3_finalize(Statement);
	
	int RunningID = 2;
	int RunningLft = 1;
	index_t Indexes[256]; //NOTE: Probably no parameter will have more index set dependencies than 256???
	for(entity_handle ParameterGroupHandle = 1; ParameterGroupHandle < Model->FirstUnusedParameterGroupHandle; ++ParameterGroupHandle)
	{
		const parameter_group_spec &GroupSpec = Model->ParameterGroupSpecs[ParameterGroupHandle];
		if(!IsValid(GroupSpec.ParentGroup))
		{
			ExportParameterGroupRecursivelyToDatabase(DataSet, ParameterGroupHandle, Db, RunningID, RunningLft, 1, Indexes);
		}
	}

	int ModelRgt = RunningLft;
	
	char ModelIdentifier[1024];
	sprintf(ModelIdentifier, "%s V%s", Model->Name, Model->Version);
	WriteStructureEntryToDatabase(Db, 1, ModelIdentifier, 0, ModelRgt, 0, 0, 0, 0, false, false);
	
	
	
	const char *InsertInputData = 
				"INSERT INTO BranchInputs (InputOf, Input, IndexSet) "
				"VALUES "
				"(?, ?, ?)";
	rc = sqlite3_prepare_v2(Db, InsertInputData, -1, &Statement, 0);
	
	
	for(entity_handle IndexSetHandle = 1; IndexSetHandle < Model->FirstUnusedIndexSetHandle; ++IndexSetHandle)
	{
		const index_set_spec &IndexSetSpec = Model->IndexSetSpecs[IndexSetHandle];
		if(IndexSetSpec.Type == IndexSetType_Branched)
		{
			rc = sqlite3_bind_text(Statement, 3, IndexSetSpec.Name, -1, SQLITE_STATIC);
			
			for(size_t Index = 0; Index < DataSet->IndexCounts[IndexSetHandle]; ++Index)
			{
				branch_inputs &Inputs = DataSet->BranchInputs[IndexSetHandle][Index];
				if(Inputs.Count > 0)
				{
					rc = sqlite3_bind_int(Statement, 1, (int)Index);
					for(size_t InputIdx = 0; InputIdx < Inputs.Count; ++InputIdx)
					{
						rc = sqlite3_bind_int(Statement, 2, (int)Inputs.Inputs[InputIdx]);
						rc = sqlite3_step(Statement);
						rc = sqlite3_reset(Statement);
					}
				}
			}
		}
	}
	
	rc = sqlite3_finalize(Statement);
	
	sqlite3_exec(Db, "COMMIT;", 0, 0, 0);
	
	sqlite3_close(Db);
}



struct database_structure_entry
{
	int ParentID;
	int ID;
	std::string Name;
	bool IsIndexer;
	bool IsIndex;
	int Dpt;
};

static void
ReadParametersFromDatabase(mobius_data_set *DataSet, const char *Dbname)
{
	sqlite3 *Db;
	int rc = sqlite3_open_v2(Dbname, &Db, SQLITE_OPEN_READWRITE, 0);
	if(rc != SQLITE_OK)
	{
		MOBIUS_FATAL_ERROR("ERROR: Unable to open database " << Dbname << " Message: " << sqlite3_errmsg(Db) << std::endl);
	}
	
	const char *GetStructureCommand = 
							  "SELECT parent.ID as parentID, child.ID, child.Name, child.IsIndexer, child.IsIndex, child.dpt "
                              "FROM ParameterStructure as parent, ParameterStructure as child "
                              "WHERE child.lft > parent.lft "
                              "AND child.rgt < parent.rgt "
                              "AND child.dpt = parent.dpt + 1 "
                              "UNION "
                              "SELECT 0 as parentID, child.ID, child.Name, child.IsIndexer, child.IsIndex, child.dpt " 
                              "FROM ParameterStructure as child "
                              "WHERE child.dpt = 0 "
							  "ORDER BY child.ID";
							  
	std::vector<database_structure_entry> ParameterStructure;
	
	sqlite3_stmt *Statement;
	rc = sqlite3_prepare_v2(Db, GetStructureCommand, -1, &Statement, 0);
	
	if(rc == SQLITE_ERROR)
	{
		MOBIUS_FATAL_ERROR("ERROR while requesting parameter data from database: " << sqlite3_errmsg(Db) << std::endl);
	}
	
	while((rc = sqlite3_step(Statement)) != SQLITE_DONE)
	{
		if(rc == SQLITE_ERROR)
		{
			//TODO: Error handling
			break;
		}
		
		database_structure_entry Entry;
		Entry.ParentID = sqlite3_column_int(Statement, 0);
		Entry.ID       = sqlite3_column_int(Statement, 1);
		const char *Name = (const char *)sqlite3_column_text(Statement, 2);
		if(Name)
		{
			Entry.Name = std::string(Name);
		}
		else
		{
			Entry.Name = "";
		}
		Entry.IsIndexer = (bool) sqlite3_column_int(Statement, 3);
		Entry.IsIndex   = (bool) sqlite3_column_int(Statement, 4);
		Entry.Dpt       = sqlite3_column_int(Statement, 5);
		
		ParameterStructure.push_back(Entry);
	}
	
	sqlite3_finalize(Statement);
	
	const char *ValueTypes[4] =
	{
		"double",
		"int",
		"bool",
		"ptime",
	};
	
	parameter_type Types[4] =
	{
		ParameterType_Double,
		ParameterType_UInt,
		ParameterType_Bool,
		ParameterType_Time,
	};
	
	std::map<int, parameter_value> IDToParameterValue;
	
	for(int i = 0; i < 4; ++i)
	{
		char GetValueCommand[512];
		sprintf(GetValueCommand,
			"SELECT "
			 "ParameterStructure.ID, ParameterValues_%s.value "
			 "FROM ParameterStructure INNER JOIN ParameterValues_%s "
			 "ON ParameterStructure.ID = ParameterValues_%s.ID; ",
			 ValueTypes[i], ValueTypes[i], ValueTypes[i]);
		
		rc = sqlite3_prepare_v2(Db, GetValueCommand, -1, &Statement, 0);
		
		while((rc = sqlite3_step(Statement)) != SQLITE_DONE)
		{
			if(rc == SQLITE_ERROR)
			{
				//TODO: Error handling
				break;
			}
			
			parameter_value Value;
			int ID = sqlite3_column_int(Statement, 0);
			if(Types[i] == ParameterType_Double)
			{
				Value.ValDouble = sqlite3_column_double(Statement, 1);
			}
			else if(Types[i] == ParameterType_UInt)
			{
				Value.ValUInt = (u64)sqlite3_column_int64(Statement, 1);
			}
			else if(Types[i] == ParameterType_Bool)
			{
				Value.ValBool = (bool)sqlite3_column_int(Statement, 1);
			}
			else if(Types[i] == ParameterType_Time)
			{
				Value.ValTime.SecondsSinceEpoch = sqlite3_column_int64(Statement, 1);
			}
			
			IDToParameterValue[ID] = Value;
		}
		
		sqlite3_finalize(Statement);
	}
	
	const mobius_model *Model = DataSet->Model;
	std::map<int, index_set_h> IDToIndexSet;
	std::vector<std::vector<token_string>> IndexNames;
	IndexNames.resize(Model->FirstUnusedIndexSetHandle);
	
	//NOTE: This routine assumes that the database entries are in the same order as when created by the CreateParameterDatabase routine.
	
	for(database_structure_entry &Entry : ParameterStructure)
	{
		if(Entry.ParentID == 0) continue; //NOTE: Ignore the root node
		if(Entry.IsIndexer)
		{
			parameter_group_h Group = GetParameterGroupHandle(Model, Entry.Name.data());
			const parameter_group_spec &Spec = Model->ParameterGroupSpecs[Group.Handle];
			IDToIndexSet[Entry.ID] = Spec.IndexSet;
		}
		else if(Entry.IsIndex)
		{
			//NOTE: Indexes appear several times in the database structure, so we have to make sure we add them uniquely.
			index_set_h IndexSet = IDToIndexSet[Entry.ParentID];
			bool Found = false;
			for(token_string IndexName : IndexNames[IndexSet.Handle]) //NOTE: No obvious way to use the std::find since we need to compare using strcmp.
			{
				if(IndexName.Equals(Entry.Name.data()))
				{
					Found = true;
					break;
				}
			}
			if(!Found)
			{
				IndexNames[IndexSet.Handle].push_back(Entry.Name.data());
			}
		}
	}
	
	const char *GetInputsCommand = "SELECT InputOf, Input FROM BranchInputs WHERE IndexSet = ?";
	rc = sqlite3_prepare_v2(Db, GetInputsCommand, -1, &Statement, 0);
	
	for(entity_handle IndexSetHandle = 1; IndexSetHandle < Model->FirstUnusedIndexSetHandle; ++IndexSetHandle)
	{	
		const index_set_spec &Spec = Model->IndexSetSpecs[IndexSetHandle];
		if(Spec.Type == IndexSetType_Basic)
		{
			SetIndexes(DataSet, Spec.Name, IndexNames[IndexSetHandle]);
		}
		else if(Spec.Type == IndexSetType_Branched)
		{
			std::vector<std::pair<token_string, std::vector<token_string>>> Inputs;
			for(token_string Index : IndexNames[IndexSetHandle])
			{
				Inputs.push_back({Index, {}});
			}
			
			rc = sqlite3_bind_text(Statement, 1, Spec.Name, -1, SQLITE_STATIC);
			while((rc = sqlite3_step(Statement)) != SQLITE_DONE)
			{
				if(rc == SQLITE_ERROR)
				{
					MOBIUS_FATAL_ERROR("ERROR while requesting branch input data from database: " << sqlite3_errmsg(Db) << std::endl);
				}
				
				int InputOf = sqlite3_column_int(Statement, 0);
				int Input   = sqlite3_column_int(Statement, 1);
				
				Inputs[InputOf].second.push_back(IndexNames[IndexSetHandle][Input]);
			}
			sqlite3_reset(Statement);
			
			SetBranchIndexes(DataSet, Spec.Name, Inputs);
		}
	}
	
	sqlite3_finalize(Statement);
	
	if(!DataSet->ParameterData)
	{
		AllocateParameterStorage(DataSet);
	}
	
	int CurDpt = 0;
	index_t Indexes[256];
	size_t Level = 0;
	
	
	for(database_structure_entry &Entry : ParameterStructure)
	{
		if(Entry.ParentID == 0) continue; //NOTE: Ignore the root node
		
		if(Entry.IsIndex)
		{
			index_set_h IndexSet = IDToIndexSet[Entry.ParentID];
			index_t Index = {IndexSet, DataSet->IndexNamesToHandle[IndexSet.Handle][Entry.Name.data()]}; //TODO: Should be a "getter" function for this
			
			Level = Entry.Dpt / 2; //NOTE: This is kind of abusive. Depends a lot on the format not changing at all.
			
			Indexes[Level-1] = Index;
		}
		else if(!Entry.IsIndexer)
		{
			entity_handle ParameterHandle = GetParameterHandle(Model, Entry.Name.data());
			
			if(Model->ParameterSpecs[ParameterHandle].ShouldNotBeExposed)
			{
				MOBIUS_FATAL_ERROR("ERROR: In file " << Dbname << ". The parameter " << Entry.Name << " is computed by the model, and should not be provided in a parameter file." << std::endl);
			}
			
			size_t Offset = OffsetForHandle(DataSet->ParameterStorageStructure, Indexes, Level, DataSet->IndexCounts, ParameterHandle);
			DataSet->ParameterData[Offset] = IDToParameterValue[Entry.ID]; //TODO: Check that it exists?
		}
	}	
	sqlite3_close(Db);
}



struct database_structure_tree
{
	index_set_h IndexSet;
	std::vector<entity_handle> Handles;
	std::vector<database_structure_tree> Children;
};

static void
PlaceUnitInTreeStructureRecursively(storage_unit_specifier &Unit, std::vector<database_structure_tree> &TreeStructure, size_t CurrentLevel)
{	
	for(database_structure_tree& Tree : TreeStructure)
	{
		index_set_h IndexSet = {};
		if(!Unit.IndexSets.empty())
		{
			IndexSet = Unit.IndexSets[CurrentLevel];
		
			if(IndexSet == Tree.IndexSet)
			{
				if(CurrentLevel + 1 == Unit.IndexSets.size())
				{
					Tree.Handles.insert(Tree.Handles.end(), Unit.Handles.begin(), Unit.Handles.end());
					return;
				}
				else
				{
					PlaceUnitInTreeStructureRecursively(Unit, Tree.Children, CurrentLevel + 1);
					return;
				}
			}
		}
		else
		{
			if(Tree.IndexSet == index_set_h {})
			{
				Tree.Handles.insert(Tree.Handles.end(), Unit.Handles.begin(), Unit.Handles.end());
				return;	
			}
		}
	}
	
	//NOTE: If we got here we did not find a place in the existing structure, so we need to make one
	database_structure_tree NewTree = {};
	NewTree.IndexSet = {};
	if(!Unit.IndexSets.empty())
	{
		NewTree.IndexSet = Unit.IndexSets[CurrentLevel];
	}
	
	if(Unit.IndexSets.empty() || CurrentLevel + 1 == Unit.IndexSets.size())
	{
		NewTree.Handles.insert(NewTree.Handles.end(), Unit.Handles.begin(), Unit.Handles.end());
	}
	else
	{
		PlaceUnitInTreeStructureRecursively(Unit, NewTree.Children, CurrentLevel + 1);
	}
	TreeStructure.push_back(NewTree);
	return;
}

//NOTE: For debugging
static void
PrintStructureTreeRecursively(mobius_model *Model, std::vector<database_structure_tree> &StructureTree, size_t CurrentLevel, int Mode)
{
	for(database_structure_tree& Tree : StructureTree)
	{
		if(IsValid(Tree.IndexSet))
		{
			for(size_t Lev = 0; Lev < CurrentLevel; ++Lev) std::cout << '\t';
			std::cout << "[" << GetName(Model, Tree.IndexSet) << "]" << std::endl;
		}
		for(entity_handle Handle : Tree.Handles)
		{
			if(IsValid(Tree.IndexSet)) for(size_t Lev = 0; Lev < CurrentLevel; ++Lev) std::cout << '\t';
			if(Mode == 0)
				std::cout << GetName(Model, equation_h {Handle} ) << std::endl;
			else
				std::cout << GetName(Model, input_h {Handle} ) << std::endl;
		}
		PrintStructureTreeRecursively(Model, Tree.Children, CurrentLevel + 1, Mode);
	}
}


static void
WriteValuesToDatabase(mobius_data_set *DataSet, storage_structure &StorageStructure, double *Data, sqlite3 *Db, sqlite3_stmt *Statement, int ID, entity_handle Handle, index_t *Indexes, size_t IndexesCount, datetime StartDate, s64 Step, u64 Timesteps)
{
	size_t Offset = OffsetForHandle(StorageStructure, Indexes, IndexesCount, DataSet->IndexCounts, Handle);
	
	int rc = sqlite3_bind_int(Statement, 3, ID);
	
	datetime AtTime = StartDate;
	
	for(u64 Timestep = 0; Timestep < Timesteps; ++Timestep)
	{	
		double Value = Data[Offset];
		rc = sqlite3_bind_int64(Statement, 1, AtTime.SecondsSinceEpoch);
		
		if(std::isnan(Value))
		{
			rc = sqlite3_bind_null(Statement, 2);
		}
		else
		{
			rc = sqlite3_bind_double(Statement, 2, Value);
		}
		
		rc = sqlite3_step(Statement);
		rc = sqlite3_reset(Statement);
		
		Offset += StorageStructure.TotalCount;
		AtTime.SecondsSinceEpoch += Step;
	}
}

static void
WriteStructureEntryToDatabase(sqlite3 *Db, sqlite3_stmt *Statement, int ID, const char *Name, int Lft, int Rgt, int Dpt, const char *Unit, bool IsIndexer, bool IsIndex)
{	
	int rc = sqlite3_bind_int(Statement, 1, ID);
	rc = sqlite3_bind_text(Statement, 2, Name, -1, SQLITE_STATIC);
	rc = sqlite3_bind_int(Statement, 3, Lft);
	rc = sqlite3_bind_int(Statement, 4, Rgt);
	rc = sqlite3_bind_int(Statement, 5, Dpt);
	if(Unit)
	{
		rc = sqlite3_bind_text(Statement, 6, Unit, -1, SQLITE_STATIC);
	}
	else
	{
		rc = sqlite3_bind_null(Statement, 6);
	}
	rc = sqlite3_bind_int(Statement, 7, IsIndexer);
	rc = sqlite3_bind_int(Statement, 8, IsIndex);
	
	rc = sqlite3_step(Statement);
	
	if(rc == SQLITE_ERROR)
	{
		MOBIUS_FATAL_ERROR("ERROR while writing result data to database: " << sqlite3_errmsg(Db) << std::endl);
	}
	
	sqlite3_reset(Statement);
}

static void
WriteStructureToDatabaseRecursively(mobius_data_set *DataSet, storage_structure &StorageStructure, double *Data, sqlite3 *Db, sqlite3_stmt *InsertValueStatement, sqlite3_stmt *InsertStructureStatement, std::vector<database_structure_tree> &StructureTree, size_t CurrentLevel, int Dpt, int &RunningID, int &RunningLft, index_t *Indexes, datetime StartDate, s64 Step, u64 Timesteps, int Mode)
{
	const mobius_model *Model = DataSet->Model;
	
	for(database_structure_tree &Tree : StructureTree)
	{	
		index_set_h IndexSet = Tree.IndexSet;
		
		if(IsValid(IndexSet))
		{
			int LftOfIndexSet = RunningLft++;
			int IDOfIndexSet = RunningID++;
			
			for(index_t Index = {IndexSet, 0}; Index < DataSet->IndexCounts[IndexSet.Handle]; ++Index)
			{
				int LftOfIndex = RunningLft++;
				int IDOfIndex  = RunningID++;
				
				Indexes[CurrentLevel] = Index;
				
				for(entity_handle Handle : Tree.Handles)
				{
					if(Mode == 1)
					{
						size_t Offset = OffsetForHandle(StorageStructure, Indexes, CurrentLevel+1, DataSet->IndexCounts, Handle);
						if(!DataSet->InputTimeseriesWasProvided[Offset]) continue;
					}
					
					int LftOfHandle = RunningLft++;
					int RgtOfHandle = RunningLft++;
					int IDOfHandle  = RunningID++;
					const char *Name;
					unit_h UnitH = {0};
					if(Mode == 0)
					{
						Name = GetName(Model, equation_h {Handle});
						UnitH = Model->EquationSpecs[Handle].Unit;
					}
					else
					{
						Name = GetName(Model, input_h {Handle});
						UnitH = Model->InputSpecs[Handle].Unit;
					}
					const char *Unit = 0;
					if(IsValid(UnitH)) Unit = GetName(Model, UnitH);
					
					WriteStructureEntryToDatabase(Db, InsertStructureStatement, IDOfHandle, Name, LftOfHandle, RgtOfHandle, Dpt + 2, Unit, false, false);
					WriteValuesToDatabase(DataSet, StorageStructure, Data, Db, InsertValueStatement, IDOfHandle, Handle, Indexes, CurrentLevel + 1, StartDate, Step, Timesteps);
				}
				
				WriteStructureToDatabaseRecursively(DataSet, StorageStructure, Data, Db, InsertValueStatement, InsertStructureStatement, Tree.Children, CurrentLevel + 1, Dpt + 2, RunningID, RunningLft, Indexes, StartDate, Step, Timesteps, Mode);
				
				int RgtOfIndex = RunningLft++;
				const char *IndexName = DataSet->IndexNames[IndexSet.Handle][Index];
				WriteStructureEntryToDatabase(Db, InsertStructureStatement, IDOfIndex, IndexName, LftOfIndex, RgtOfIndex, Dpt + 1, 0, false, true);
			}
			
			int RgtOfIndexSet = RunningLft++;
			const char *IndexSetName = GetName(DataSet->Model, IndexSet);
			WriteStructureEntryToDatabase(Db, InsertStructureStatement, IDOfIndexSet, IndexSetName, LftOfIndexSet, RgtOfIndexSet, Dpt, 0, true, false);
		}
		else
		{
			for(entity_handle Handle : Tree.Handles)
			{
				if(Mode == 1)
				{
					size_t Offset = OffsetForHandle(StorageStructure, Indexes, CurrentLevel+1, DataSet->IndexCounts, Handle);
					if(!DataSet->InputTimeseriesWasProvided[Offset]) continue;
				}
				
				int LftOfHandle = RunningLft++;
				int RgtOfHandle = RunningLft++;
				int IDOfHandle  = RunningID++;
				const char *Name;
				unit_h UnitH = {0};
				if(Mode == 0)
				{
					Name = GetName(Model, equation_h {Handle});
					UnitH = Model->EquationSpecs[Handle].Unit;
				}
				else
				{
					Name = GetName(Model, input_h {Handle});
					UnitH = Model->InputSpecs[Handle].Unit;
				}
				const char *Unit = 0;
				if(IsValid(UnitH)) Unit = GetName(Model, UnitH);
				
				WriteStructureEntryToDatabase(Db, InsertStructureStatement, IDOfHandle, Name, LftOfHandle, RgtOfHandle, Dpt, Unit, false, false);
				WriteValuesToDatabase(DataSet, StorageStructure, Data, Db, InsertValueStatement, IDOfHandle, Handle, Indexes, 0, StartDate, Step, Timesteps);
			}
		}
	}
}

//NOTE: For results and inputs. Parameters have to be handled differently
//NOTE: Mode==0 : results, Mode==1 : inputs. This is unfortunately needed to look up the names correctly because the names are not tied to the storage.
static void
WriteStorageToDatabase(mobius_data_set *DataSet, storage_structure &StorageStructure, double *Data, const char *Dbname, const char *StructureTable, const char *ValueTable, int Mode)
{
	//NOTE: Deletes any (potentially) existing database of the same name.
	//TODO: We should figure out if it is safe to have this here?
	remove(Dbname);
	
	const mobius_model *Model = DataSet->Model;
	
	//NOTE: Deletes any (potentially) existing database of the same name.
	//TODO: We should figure out if it is safe to have this here?
	remove(Dbname);
	
	sqlite3 *Db;
	int rc = sqlite3_open_v2(Dbname, &Db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
	if(rc != SQLITE_OK)
	{
		MOBIUS_FATAL_ERROR("ERROR: Unable to open database " << Dbname << " Message: " << sqlite3_errmsg(Db) << std::endl);
	}
	
	//TODO: Check if the tables already exist?
	char CreateStructureTable[512];
	sprintf(CreateStructureTable,
		"CREATE TABLE %s ("
		"ID INTEGER PRIMARY KEY NOT NULL, "
		"name TEXT, "
		"lft INTEGER NOT NULL, "
		"rgt INTEGER NOT NULL, "
		"dpt INTEGER NOT NULL, "
		"unit TEXT, "
		"isIndexer BOOLEAN, "
		"isIndex BOOLEAN )",
		StructureTable
	);
	
	sqlite3_stmt *Statement;
	rc = sqlite3_prepare_v2(Db, CreateStructureTable, -1, &Statement, 0);
	
	rc = sqlite3_step(Statement);
	sqlite3_finalize(Statement);
	
	//TODO: It is very unnecessary to store the dates for each series. The dates could have been stored as a single series instead.
	
	char CreateValueTable[512];
	sprintf(CreateValueTable,
		"CREATE TABLE %s ("
		"date BIGINT, "
		"value DOUBLE, "
		"ID INTEGER, FOREIGN KEY(ID) REFERENCES ResultsStructure(ID))",
		ValueTable
	);
		
	rc = sqlite3_prepare_v2(Db, CreateValueTable, -1, &Statement, 0);
	
	rc = sqlite3_step(Statement);
	sqlite3_finalize(Statement);
	
	std::vector<database_structure_tree> StructureTree;
	
	for(storage_unit_specifier &Unit : StorageStructure.Units)
	{
		PlaceUnitInTreeStructureRecursively(Unit, StructureTree, 0);
	}
	
	//PrintStructureTreeRecursively(Model, StructureTree, 0, Mode);
	
	char InsertValue[512];
	sprintf(InsertValue,
		"INSERT INTO %s (date, value, ID) "
		"VALUES "
		"(?, ?, ?)",
		ValueTable
	);
	
	rc = sqlite3_prepare_v2(Db, InsertValue, -1, &Statement, 0);
	
	char InsertStructureEntry[512];
	sprintf(InsertStructureEntry,
		"INSERT INTO %s (ID, name, lft, rgt, dpt, unit, isIndexer, isIndex) "
		"VALUES "
		"(?, ?, ?, ?, ?, ?, ?, ?)",
		StructureTable
	);
	
	sqlite3_stmt *StructureStatement;
	rc = sqlite3_prepare_v2(Db, InsertStructureEntry, -1, &StructureStatement, 0);
	
	int RunningID = 1;
	int RunningLft = 0;
	index_t Indexes[256];
	rc = sqlite3_exec(Db, "BEGIN TRANSACTION;", 0, 0, 0);
	
	s64 Step = 86400; //NOTE: The number of seconds in each timestep //TODO: Remember not to hard code this if we later allow for flexible time step.
	
	datetime StartDate = GetStartDate(DataSet); //NOTE: This reads the "Start date" parameter.
	if(Mode == 1 && DataSet->InputDataHasSeparateStartDate)
	{
		StartDate = DataSet->InputDataStartDate;
	}
	
	u64 Timesteps;
	if(Mode == 0)
	{
		Timesteps = DataSet->TimestepsLastRun;
	}
	else if(Mode == 1)
	{
		Timesteps = DataSet->InputDataTimesteps;
	}
	
	WriteStructureToDatabaseRecursively(DataSet, StorageStructure, Data, Db, Statement, StructureStatement, StructureTree, 0, 0, RunningID, RunningLft, Indexes, StartDate, Step, Timesteps, Mode);
	
	rc = sqlite3_exec(Db, "COMMIT;", 0, 0, 0);

	rc = sqlite3_finalize(Statement);
	rc = sqlite3_finalize(StructureStatement);
	
	sqlite3_close(Db);
}

static void
WriteResultsToDatabase(mobius_data_set *DataSet, const char *Dbname)
{
	double *Data = DataSet->ResultData + DataSet->ResultStorageStructure.TotalCount; //NOTE: adding TotalCount advances past the initial values to the first timestep.
	WriteStorageToDatabase(DataSet, DataSet->ResultStorageStructure, Data, Dbname, "ResultsStructure", "Results", 0);
}

static void
WriteInputsToDatabase(mobius_data_set *DataSet, const char *Dbname)
{
	WriteStorageToDatabase(DataSet, DataSet->InputStorageStructure, DataSet->InputData, Dbname, "InputsStructure", "Inputs", 1);
}

#define MOBIUS_SQLITE3_IO_CPP
#endif