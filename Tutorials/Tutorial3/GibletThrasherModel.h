

//NOTE: This is the saga of the Giblets and the Thrashers. The Giblets are furry little creatures that live a peaceful life skipping around on the rocks on the nice and sunny plateau. However incursions of large Giblet-eating Thrashers onto the plateau may force some of the Giblets to seek refuge in the nasty forests and bogs.

static void
AddGibletThrasherModel(mobius_model *Model)
{
	//It can be nice to organize parts of the model in modules for reusability with other models. Though in this case we only use one module for the entire model. Module organization shows up in e.g. MobiView.
	BeginModule(Model, "Giblet-Thrasher", "1.0");
	
	auto Dimensionless = RegisterUnit(Model, "dimensionless");
	auto Days = RegisterUnit(Model, "days");
	auto Individuals = RegisterUnit(Model, "individuals");
	auto IndividualsPerDay = RegisterUnit(Model, "ind./day");
	
	auto GeographicalLocation = RegisterIndexSet(Model, "Geographical location");
	
	auto Population = RegisterParameterGroup(Model, "Population", GeographicalLocation);
	
	auto InitialThrasherPopulation = RegisterParameterDouble(Model, Population, "Initial thrasher population", Individuals, 20.0);
	
	auto GibletBirthRate = RegisterParameterDouble(Model, Population, "Giblet birth rate", Dimensionless, 0.2);
	auto PredationRate = RegisterParameterDouble(Model, Population, "Predation rate", Dimensionless, 0.0002);
	auto ThrasherDeathRate = RegisterParameterDouble(Model, Population, "Thrasher death rate", Dimensionless, 0.01);
	
	auto GibletCarryingCapacity = RegisterParameterDouble(Model, Population, "Giblet carrying capacity", Individuals, 100.0);
	auto FearFactor = RegisterParameterDouble(Model, Population, "Thrasher fear factor", Dimensionless, 0.2, 0.0, 1.0, "How much a thrasher fears a location"); //We can put min, max values and comments on a parameter. These will for instance be displayed in the graphical user interface MobiView.
	
	
	auto All = RegisterParameterGroup(Model, "All");
	
	auto ThrasherDeathFromMeteorStrikes = RegisterParameterDouble(Model, All, "Thrasher death rate from meteor strikes", Dimensionless, 0.1);
	auto ThrasherGrowthFromPredation = RegisterParameterDouble(Model, All, "Thrasher growth from predation", Dimensionless, 0.5);
	auto GibletBirthdayMean = RegisterParameterUInt(Model, All, "Giblet birthday mean", Days, 60);
	auto GibletBirthdayStandardDeviation = RegisterParameterUInt(Model, All, "Giblet birthday standard deviation", Days, 20);
	
	//Meteors strike some locations. Giblets are very sensitive and can feel the meteors arriving, and so can escape them, but the Thrashers are not smart enough to dodge the meteors.
	auto MeteorStrikes = RegisterInput(Model, "Meteor strikes");
	
	auto GibletWillingnessToMigrate = RegisterParameterDouble(Model, All, "Giblet willingness to migrate", Dimensionless, 0.5);
	auto ThrasherWillingnessToMigrate = RegisterParameterDouble(Model, All, "Thrasher willingness to migrate", Dimensionless, 0.4);
	
	//NOTE: The MigrationMatrix has a double dependency on the "Geographical location" index set. So the "Migration matrix" parameter has one value fore each pair of geographical locations, which one can visualize as a matrix where both the rows and columns are indexed by "Geographical location"
	auto MigrationMatrixGrp = RegisterParameterGroup(Model, "Migration matrix", GeographicalLocation, GeographicalLocation);
	
	// The migration matrix says something about how easy it is to move from one location to another
	auto MigrationMatrix = RegisterParameterDouble(Model, MigrationMatrixGrp, "Migration matrix", Dimensionless, 1.0, 0.0, 1.0);
	
	
	auto GibletMigration = RegisterEquation(Model, "Giblet migration rate", IndividualsPerDay);
	auto ThrasherMigration = RegisterEquation(Model, "Thrasher migration rate", IndividualsPerDay);
	auto GibletTemporalBirthRate = RegisterEquation(Model, "Giblet temporal birth rate", Dimensionless);
	
	//NOTE: Create a solver 
	auto Solver = RegisterSolver(Model, "Population solver", 0.1, IncaDascru);
	
	//NOTE: An EquationInitialValue is only evaluated once at the start of the model and sets the initial value for another equation, most often an ODE equation.
	auto InitialGibletPopulation = RegisterEquationInitialValue(Model, "Initial giblet population", Individuals);
	
	//NOTE: An EquationODE returns the d/dt of its associated result value rather than returning that result value directly. The solver then integrates it to obtain the result value.
	auto GibletPopulation = RegisterEquationODE(Model, "Giblet population", Individuals);
	SetSolver(Model, GibletPopulation, Solver);
	SetInitialValue(Model, GibletPopulation, InitialGibletPopulation);
	
	auto ThrasherPopulation = RegisterEquationODE(Model, "Thrasher population", Individuals);
	SetSolver(Model, ThrasherPopulation, Solver);
	SetInitialValue(Model, ThrasherPopulation, InitialThrasherPopulation); //The Thrasher population has its initial value set from a parameter.
	
	//NOTE: We add giblet birth to the solver even though it is not an ODE equation. This is because we want its value computed at every sub-timestep of the solver for use in the ODE equations.
	auto GibletBirth = RegisterEquation(Model, "Giblet birth", IndividualsPerDay);
	SetSolver(Model, GibletBirth, Solver);
	
	//NOTE: The EquationCumulative sums up all the values for this equation over an index set. In this case we just want to test that the migration sums to 0 so that no giblets are created or destroyed by migration.
	auto NetGibletMigration = RegisterEquationCumulative(Model, "Net giblet migration", GibletMigration, GeographicalLocation);
	
	//NOTE: It would not currently work correctly to add this migration calculation to the solver since it accesses result values from other indexes than the current index. Currently the solvers work so that they just solve one index at a time for every timestep. We hope to add the possibility for the solver to solve across multiple indexes at a time in a later version. -MDN
	EQUATION(Model, GibletMigration,
		double migrationrate = 0.0;
		double innatemigration = PARAMETER(GibletWillingnessToMigrate);
		for(index_t Index = FIRST_INDEX(GeographicalLocation); Index < INDEX_COUNT(GeographicalLocation); ++Index)
		{
			// The Giblet migration rate between two locations is proportional to the predation rate at the current location minus the predation rate at the other location. (modified by giblet population. High giblet population at the current location creates migration pressure away).
			migrationrate += (
								LAST_RESULT(ThrasherPopulation)        * LAST_RESULT(GibletPopulation, Index) * PARAMETER(PredationRate)
							  - LAST_RESULT(ThrasherPopulation, Index) * LAST_RESULT(GibletPopulation)        * PARAMETER(PredationRate, Index)
							) 
							* innatemigration * PARAMETER(MigrationMatrix, Index); //NOTE: We only provide the index for the column of the matrix, and so the row is automatically set to the current index of GeographicalLocation.
		}
		return migrationrate;
	)
	
	EQUATION(Model, ThrasherMigration,
		double migrationrate = 0.0;
		double innatemigration = PARAMETER(ThrasherWillingnessToMigrate);
		for(index_t Index = FIRST_INDEX(GeographicalLocation); Index < INDEX_COUNT(GeographicalLocation); ++Index)
		{
			//Thrashers desire to move to locations with few Thrashers and many Giblets, but they fear some locations more than others.
			
			migrationrate += 
			(
			LAST_RESULT(ThrasherPopulation, Index) * PARAMETER(PredationRate) * LAST_RESULT(GibletPopulation) * (1.0 - PARAMETER(FearFactor))
			- LAST_RESULT(ThrasherPopulation) * LAST_RESULT(GibletPopulation, Index) * PARAMETER(PredationRate, Index) * (1.0 - PARAMETER(FearFactor, Index))
			) 
			
			* innatemigration * PARAMETER(MigrationMatrix, Index); 
		}
		return migrationrate;
	)
	
	EQUATION(Model, GibletTemporalBirthRate,
		//TODO: This does not work that well if GibletBirthdayMean is too close to one of the edges of the year, and so we should make it wrap around in some way.
		//TODO: This is not normalized!
		double exponent = (double)(CURRENT_TIME().DayOfYear - (s32)PARAMETER(GibletBirthdayMean)) / (double)PARAMETER(GibletBirthdayStandardDeviation);
		return PARAMETER(GibletBirthRate) * std::exp(-Square(exponent) / 2.0 );
	)
	
	EQUATION(Model, GibletBirth,
		return RESULT(GibletPopulation) * RESULT(GibletTemporalBirthRate) * (1 - RESULT(GibletPopulation) / PARAMETER(GibletCarryingCapacity));
	)
	
	EQUATION(Model, ThrasherPopulation,
		return
		  RESULT(GibletPopulation) * RESULT(ThrasherPopulation) * PARAMETER(PredationRate) * PARAMETER(ThrasherGrowthFromPredation) //Apparently Thrashers immediately spew out offspring when they get to eat. Also, lack of food doesn't immediately kill them (this model is maybe not that good with a daily timestep like we use here).
		- RESULT(ThrasherPopulation) * (PARAMETER(ThrasherDeathRate) + INPUT(MeteorStrikes) * PARAMETER(ThrasherDeathFromMeteorStrikes))
		+ RESULT(ThrasherMigration);
	)
	
	EQUATION(Model, InitialGibletPopulation,
		return PARAMETER(GibletCarryingCapacity) * 0.5;
	)
	
	EQUATION(Model, GibletPopulation,
		return RESULT(GibletBirth) - RESULT(GibletPopulation) * RESULT(ThrasherPopulation) * PARAMETER(PredationRate) + RESULT(GibletMigration);
	)
	
	
	//Remember to end the module if you started one.
	EndModule(Model);
}