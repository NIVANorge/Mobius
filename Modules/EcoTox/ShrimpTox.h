


// This model is based on
// S. Jannicke Moe & al. 2019, Effects of an aquaculture pesticide (diflubenzuron) on non-target shrimp populations: Extrapolation from laboratory experiments to the risk of population decline.  Ecological Modelling 413, 108833


void AddShrimpToxModel(mobius_model *Model)
{
	BeginModule(Model, "Shrimp tox", "0.1");
	
	auto Dimensionless        = RegisterUnit(Model);
	auto Individuals          = RegisterUnit(Model, "ind/100m2");
	auto IndividualsPerSeason = RegisterUnit(Model, "ind/100m2/season");
	auto PerInd               = RegisterUnit(Model, "1/(ind/100m2)");
	auto Season               = RegisterUnit(Model, "season");
	auto Year                 = RegisterUnit(Model, "year");
	
	auto AgeClass = RegisterIndexSet(Model, "Age class");
	
	auto GeneralParam = RegisterParameterGroup(Model, "General");
	auto AgeParam     = RegisterParameterGroup(Model, "Age class", AgeClass);
	
	auto StrengthOfDensityDependence = RegisterParameterDouble(Model, GeneralParam, "Strength of density dependence (a)", PerInd, 0.01);
	auto DegreeOfCompensation        = RegisterParameterDouble(Model, GeneralParam, "Degree of compensation (b)", Dimensionless, 1.0);
	auto FirstAdultClass             = RegisterParameterUInt(Model, GeneralParam, "First adult age class", Season, 6, 1, 100);
	
	auto BadYearType                 = RegisterParameterUInt(Model, GeneralParam, "Bad year type", Dimensionless, 0, 0, 2, "0=None, 1=Once, 2=0.25% chance each year");
	auto BadYearNumber               = RegisterParameterUInt(Model, GeneralParam, "Bad year", Year, 2040, 1000, 3000, "What year is the bad year if there is only one");
	auto BadYearSurvivalReduction  = RegisterParameterDouble(Model, GeneralParam, "Reduction of larva survival in bad year", Dimensionless, 0.5, 0.0, 1.0, "Multiplier to larva survival rate");
	auto VariationInSurvival         = RegisterParameterDouble(Model, GeneralParam, "Variation in survival", Dimensionless, 1.0, 0.8, 1.2, "Exponent in survival rate. Used for changing rate when running the model stochastically"); 
	
	auto BaseSurvivalRate            = RegisterParameterDouble(Model, AgeParam, "Base survival rate", Dimensionless, 0.9, 0.0, 1.0);
	auto Fertility                   = RegisterParameterDouble(Model, AgeParam, "Fertility", Dimensionless, 0.0, 0.0, 1.0);
	
	auto InitialPopulationSize       = RegisterParameterDouble(Model, AgeParam, "Initial population size", Individuals, 500.0);
	
	
	auto TotalAdultPopulation = RegisterEquation(Model, "Total adult population", Individuals);
	auto Birth                = RegisterEquation(Model, "Birth", IndividualsPerSeason);
	auto TotalBirth           = RegisterEquationCumulative(Model, "Total birth", Birth, AgeClass);
	auto SurvivalRate         = RegisterEquation(Model, "Survival rate", Dimensionless);
	
	auto PopulationSize       = RegisterEquation(Model, "Population size", Individuals);
	SetInitialValue(Model, PopulationSize, InitialPopulationSize);
	
	EQUATION(Model, SurvivalRate,
		index_t Age    = CURRENT_INDEX(AgeClass);
		double survivalRate = PARAMETER(BaseSurvivalRate);
		double lastRate = LAST_RESULT(SurvivalRate);
		
		survivalRate = std::pow(survivalRate, PARAMETER(VariationInSurvival));
		
		double badYearReduction = PARAMETER(BadYearSurvivalReduction);
		u64    badYearNumber    = PARAMETER(BadYearNumber);
		
		u64 badtype = PARAMETER(BadYearType);
		if(badtype > 0 && Age == FIRST_INDEX(AgeClass))
		{
			if(badtype == 1 && CURRENT_TIME().Year == badYearNumber)
				survivalRate *= badYearReduction;
			else if(badtype == 2)
			{
				if(CURRENT_TIME().Month == 1)
				{
					if(UNIFORM_RANDOM_UINT(1, 4)==1) survivalRate *= badYearReduction;
				}
				else
					survivalRate = lastRate;
			}
		}
		
		return survivalRate;
	)
	
	EQUATION(Model, PopulationSize,
		index_t Age    = CURRENT_INDEX(AgeClass);
		double a       = PARAMETER(StrengthOfDensityDependence);
		double b       = PARAMETER(DegreeOfCompensation);
		double NA      = LAST_RESULT(TotalAdultPopulation);
		double birth   = LAST_RESULT(TotalBirth);
		u64 firstAdult = PARAMETER(FirstAdultClass)-1;  //0-based indexing
		
		if(Age == FIRST_INDEX(AgeClass))
			return birth;
		
		double survival = RESULT(SurvivalRate, Age-1);
		double Size = survival * LAST_RESULT(PopulationSize, Age-1);
		
		if(Age >= INDEX_NUMBER(AgeClass, firstAdult))
			Size /= (1.0 + pow(a*NA, b));
		
		return Size;
	)
	
	EQUATION(Model, TotalAdultPopulation,
		double Tot = 0.0;
		u64 firstAdult = PARAMETER(FirstAdultClass)-1; //0-based indexing
		for(index_t Age = INDEX_NUMBER(AgeClass, firstAdult); Age < INDEX_COUNT(AgeClass); ++Age)
			Tot += RESULT(PopulationSize, Age);
		
		return Tot;
	)
	
	EQUATION(Model, Birth,
		return RESULT(PopulationSize) * PARAMETER(Fertility);
	)
	
	EndModule(Model);
}