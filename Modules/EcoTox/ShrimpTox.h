


void AddShrimpToxModel(mobius_model *Model)
{
	BeginModule(Model, "Shrimp tox", "0.1");
	
	auto Dimensionless        = RegisterUnit(Model);
	auto Individuals          = RegisterUnit(Model, "ind/100m2");
	auto IndividualsPerSeason = RegisterUnit(Model, "ind/100m2/season");
	auto PerInd               = RegisterUnit(Model, "1/(ind/100m2)");
	auto Season               = RegisterUnit(Model, "season");
	
	
	auto AgeClass = RegisterIndexSet(Model, "Age class");
	
	auto GeneralParam = RegisterParameterGroup(Model, "General");
	auto AgeParam     = RegisterParameterGroup(Model, "Age class", AgeClass);
	
	auto StrengthOfDensityDependence = RegisterParameterDouble(Model, GeneralParam, "Strength of density dependence (a)", PerInd, 0.01);
	auto DegreeOfCompensation        = RegisterParameterDouble(Model, GeneralParam, "Degree of compensation (b)", Dimensionless, 1.0);
	auto FirstAdultClass             = RegisterParameterUInt(Model, GeneralParam, "First adult age class", Season, 6, 1, 100);
	
	auto BaseSurvivalRate            = RegisterParameterDouble(Model, AgeParam, "Base survival rate", Dimensionless, 0.9, 0.0, 1.0);
	auto Fertility                   = RegisterParameterDouble(Model, AgeParam, "Fertility", Dimensionless, 0.0, 0.0, 1.0);
	
	auto InitialPopulationSize       = RegisterParameterDouble(Model, AgeParam, "Initial population size", Individuals, 500.0);
	
	
	auto PopulationSize       = RegisterEquation(Model, "Population size", Individuals);
	SetInitialValue(Model, PopulationSize, InitialPopulationSize);
	
	auto TotalAdultPopulation = RegisterEquation(Model, "Total adult population", Individuals);
	auto Birth                = RegisterEquation(Model, "Birth", IndividualsPerSeason);
	auto TotalBirth           = RegisterEquationCumulative(Model, "Total birth", Birth, AgeClass);
	
	EQUATION(Model, PopulationSize,
		index_t Age    = CURRENT_INDEX(AgeClass);
		double a       = PARAMETER(StrengthOfDensityDependence);
		double b       = PARAMETER(DegreeOfCompensation);
		double NA      = LAST_RESULT(TotalAdultPopulation);
		double birth   = LAST_RESULT(TotalBirth);
		u64 firstAdult = PARAMETER(FirstAdultClass)-1;  //0-based indexing
		
		if(Age == FIRST_INDEX(AgeClass))
		{
			return birth;
		}
		
		double Size = PARAMETER(BaseSurvivalRate, Age-1) * LAST_RESULT(PopulationSize, Age-1);
		
		if(Age >= INDEX_NUMBER(AgeClass, firstAdult))
		{
			Size /= (1.0 + pow(a*NA, b));
		}
		
		return Size;
	)
	
	EQUATION(Model, TotalAdultPopulation,
		double Tot = 0.0;
		u64 firstAdult = PARAMETER(FirstAdultClass)-1; //0-based indexing
		for(index_t Age = INDEX_NUMBER(AgeClass, firstAdult); Age < INDEX_COUNT(AgeClass); ++Age)
		{
			Tot += RESULT(PopulationSize, Age);
		}
		
		return Tot;
	)
	
	EQUATION(Model, Birth,
		return RESULT(PopulationSize) * PARAMETER(Fertility);
	)
	
	EndModule(Model);
}