

inline double
ComputeDecompRate(double Precipitation, double Temperature, double BaseRate, double Temp1, double Temp2, double Precip1)
{
	return BaseRate * std::exp(Temp1*Temperature + Temp2*Temperature*Temperature) * (1.0 - std::exp(Precip1*Precipitation*0.001));
}



static void
AddYassoModel(mobius_model *Model)
{
	BeginModule(Model, "YASSO", "_dev");
	
	SetModuleDescription(Model, R""""(
This is an un-official implementation of the Yasso model

[^https://doi.org/10.1016/j.foreco.2005.12.023^ De Wit et. al. 2006, A carbon budget of forest biomass and soils in southeast Norway calculated using a widely applicable method, Forest Ecology and Management 226(1-3) pp15-26.]
)"""");


	
	auto Dimensionless     = RegisterUnit(Model);
	auto PerYear           = RegisterUnit(Model, "1/year");
	auto MmPerYear         = RegisterUnit(Model, "mm/year");
	auto GPerM2            = RegisterUnit(Model, "g/m2");
	auto GPerM2PerYear     = RegisterUnit(Model, "g/m2/year");
	auto DegreesCelsius    = RegisterUnit(Model, "°C");
	auto PerDegC           = RegisterUnit(Model, "1/°C");
	auto PerDegC2          = RegisterUnit(Model, "1/°C2");
	auto PerMm             = RegisterUnit(Model, "1/mm");
	
	auto Compartment       = RegisterIndexSet(Model, "Compartment");
	
	auto ACompartment      = RequireIndex(Model, Compartment, "(A) Acid-hydrolyzable");
	auto WCompartment      = RequireIndex(Model, Compartment, "(W) Water-soluble");
	auto ECompartment      = RequireIndex(Model, Compartment, "(E) Ethanol-soluble");
	auto NCompartment      = RequireIndex(Model, Compartment, "(N) Non-soluble");
	auto HCompartment      = RequireIndex(Model, Compartment, "(H) Humus");
	
	
	
	auto BoxPar            = RegisterParameterGroup(Model, "Compartment parameters", Compartment);
	
	auto DecompositionRate = RegisterParameterDouble(Model, BoxPar, "Decomposition rate", PerYear, 0.0, 0.0, 20.0);
	auto DecompTempLinear  = RegisterParameterDouble(Model, BoxPar, "Decomposition rate linear temperature dependence", PerDegC, 0.0, 0.0, 0.2);
	auto DecompTempQuad    = RegisterParameterDouble(Model, BoxPar, "Decomposition rate quadratic temperature dependence", PerDegC2, 0.0, -0.05, 0.0);
	auto DecompPrecipLinear= RegisterParameterDouble(Model, BoxPar, "Decomposition rate linear precipitation dependence", PerMm, 0.0, -20.0, 0.0);
	
	
	auto FoilageComp       = RegisterParameterDouble(Model, BoxPar, "Foilage chemical composition", Dimensionless, 0.0, 0.0, 1.0);
	auto FineWoodComp      = RegisterParameterDouble(Model, BoxPar, "Fine wood chemical composition", Dimensionless, 0.0, 0.0, 1.0);
	auto CoarseWoodComp    = RegisterParameterDouble(Model, BoxPar, "Coarse wood chemical composition", Dimensionless, 0.0, 0.0, 1.0);
	
	
	auto TransferPar       = RegisterParameterGroup(Model, "Transfer matrix", Compartment, Compartment);
	
	auto RelativeMassFlow  = RegisterParameterDouble(Model, TransferPar, "Relative mass flow", Dimensionless, 0.0, 0.0, 1.0);
	
	
	auto GlobalPar         = RegisterParameterGroup(Model, "Other parameters");
	
	auto FineWoodDecompR   = RegisterParameterDouble(Model, GlobalPar, "Fine wood decomposition rate", PerYear, 0.0, 0.0, 1.0);
	auto CoarseWoodDecompR = RegisterParameterDouble(Model, GlobalPar, "Coarse wood decomposition rate", PerYear, 0.0, 0.0, 1.0);
	
	
	
	auto Precipitation     = RegisterInput(Model, "Precipitation", MmPerYear);
	auto AirTemperature    = RegisterInput(Model, "Air temperature", DegreesCelsius);
	auto FoilageLitter     = RegisterInput(Model, "Foilage and fine root litter", GPerM2PerYear);
	auto FineWoodLitter    = RegisterInput(Model, "Fine wood litter", GPerM2PerYear);
	auto CoarseWoodLitter  = RegisterInput(Model, "Coarse wood litter", GPerM2PerYear);
	
	
	auto SoilSolver = RegisterSolver(Model, "YASSO soil solver", 0.02, IncaDascru);
	
	auto FineWoodDecomp = RegisterEquation(Model, "Fine wood decomposition", GPerM2PerYear, SoilSolver);
	auto CoarseWoodDecomp=RegisterEquation(Model, "Coarse wood decomposition", GPerM2PerYear, SoilSolver);
	
	auto ADecayRate     = RegisterEquation(Model, "(A) decay rate", PerYear, SoilSolver);
	auto WDecayRate     = RegisterEquation(Model, "(W) decay rate", PerYear, SoilSolver);
	auto EDecayRate     = RegisterEquation(Model, "(E) decay rate", PerYear, SoilSolver);
	auto NDecayRate     = RegisterEquation(Model, "(N) decay rate", PerYear, SoilSolver);
	auto HDecayRate     = RegisterEquation(Model, "(H) decay rate", PerYear, SoilSolver);
	
	auto AInputFromLitter = RegisterEquation(Model, "(A) input from litter", GPerM2PerYear, SoilSolver);
	auto WInputFromLitter = RegisterEquation(Model, "(W) input from litter", GPerM2PerYear, SoilSolver);
	auto EInputFromLitter = RegisterEquation(Model, "(E) input from litter", GPerM2PerYear, SoilSolver);
	auto NInputFromLitter = RegisterEquation(Model, "(N) input from litter", GPerM2PerYear, SoilSolver);
	
	auto ADecay         = RegisterEquation(Model, "(A) decay", GPerM2PerYear, SoilSolver);
	auto WDecay         = RegisterEquation(Model, "(W) decay", GPerM2PerYear, SoilSolver);
	auto EDecay         = RegisterEquation(Model, "(E) decay", GPerM2PerYear, SoilSolver);
	auto NDecay         = RegisterEquation(Model, "(N) decay", GPerM2PerYear, SoilSolver);
	auto HDecay         = RegisterEquation(Model, "(H) decay", GPerM2PerYear, SoilSolver);
	
	auto FineWoodMass   = RegisterEquationODE(Model, "Fine wood mass", GPerM2, SoilSolver);
	auto CoarseWoodMass = RegisterEquationODE(Model, "Coarse wood mass", GPerM2, SoilSolver);
	
	auto AMass          = RegisterEquationODE(Model, "(A) mass", GPerM2, SoilSolver);
	auto WMass          = RegisterEquationODE(Model, "(W) mass", GPerM2, SoilSolver);
	auto EMass          = RegisterEquationODE(Model, "(E) mass", GPerM2, SoilSolver);
	auto NMass          = RegisterEquationODE(Model, "(N) mass", GPerM2, SoilSolver);
	auto HMass          = RegisterEquationODE(Model, "(H) mass", GPerM2, SoilSolver);
	
	auto InitialFineWood   = RegisterEquationInitialValue(Model, "Initial fine wood mass", GPerM2);
	auto InitialCoarseWood = RegisterEquationInitialValue(Model, "Initial coarse wood mass", GPerM2);
	
	auto InitialAMass      = RegisterEquationInitialValue(Model, "Initial (A) mass", GPerM2);
	auto InitialWMass      = RegisterEquationInitialValue(Model, "Initial (W) mass", GPerM2);
	auto InitialEMass      = RegisterEquationInitialValue(Model, "Initial (E) mass", GPerM2);
	auto InitialNMass      = RegisterEquationInitialValue(Model, "Initial (N) mass", GPerM2);
	auto InitialHMass      = RegisterEquationInitialValue(Model, "Initial (H) mass", GPerM2);
	
	SetInitialValue(Model, FineWoodMass, InitialFineWood);
	SetInitialValue(Model, CoarseWoodMass, InitialCoarseWood);
	SetInitialValue(Model, AMass, InitialAMass);
	SetInitialValue(Model, WMass, InitialWMass);
	SetInitialValue(Model, EMass, InitialEMass);
	SetInitialValue(Model, NMass, InitialNMass);
	SetInitialValue(Model, HMass, InitialHMass);
	
	
	
	EQUATION(Model, InitialFineWood,
		return INPUT(FineWoodLitter) / PARAMETER(FineWoodDecompR);
	)
	
	EQUATION(Model, InitialCoarseWood,
		return INPUT(CoarseWoodLitter) / PARAMETER(CoarseWoodDecompR);
	)
	
	
	//NOTE: These can only use decay from "above" in the steady-state, otherwise we would have to solve a matrix equation here..
	
	EQUATION(Model, InitialAMass,
		double fromlitter = RESULT(AInputFromLitter);

		return SafeDivide(fromlitter, RESULT(ADecayRate));
	)
	
	EQUATION(Model, InitialWMass,
		double fromlitter = RESULT(WInputFromLitter);
	
		double fromothers = 
			  PARAMETER(RelativeMassFlow, WCompartment, ACompartment) * RESULT(ADecay);
		
		return SafeDivide(fromlitter + fromothers, RESULT(WDecayRate));
	)
	
	EQUATION(Model, InitialEMass,
		double fromlitter = RESULT(EInputFromLitter);
	
		double fromothers = 
			  PARAMETER(RelativeMassFlow, ECompartment, ACompartment) * RESULT(ADecay)
			+ PARAMETER(RelativeMassFlow, ECompartment, WCompartment) * RESULT(WDecay);
		
		return SafeDivide(fromlitter + fromothers, RESULT(EDecayRate));
	)
	
	EQUATION(Model, InitialNMass,
		double fromlitter = RESULT(NInputFromLitter);
	
		double fromothers = 
			  PARAMETER(RelativeMassFlow, NCompartment, ACompartment) * RESULT(ADecay)
			+ PARAMETER(RelativeMassFlow, NCompartment, WCompartment) * RESULT(WDecay)
			+ PARAMETER(RelativeMassFlow, NCompartment, ECompartment) * RESULT(EDecay);
		
		return SafeDivide(fromlitter + fromothers, RESULT(NDecayRate));
	)
	
	EQUATION(Model, InitialHMass,

		double fromothers = 
			  PARAMETER(RelativeMassFlow, HCompartment, ACompartment) * RESULT(ADecay)
			+ PARAMETER(RelativeMassFlow, HCompartment, WCompartment) * RESULT(WDecay)
			+ PARAMETER(RelativeMassFlow, HCompartment, ECompartment) * RESULT(EDecay)
			+ PARAMETER(RelativeMassFlow, HCompartment, NCompartment) * RESULT(NDecay);
		
		return SafeDivide(fromothers, RESULT(HDecayRate));
	)
	
	

	EQUATION(Model, FineWoodDecomp,
		return PARAMETER(FineWoodDecompR)*RESULT(FineWoodMass);
	)
	
	EQUATION(Model, CoarseWoodDecomp,
		return PARAMETER(CoarseWoodDecompR)*RESULT(CoarseWoodMass);
	)
	
	EQUATION(Model, FineWoodMass,
		return INPUT(FineWoodLitter) - RESULT(FineWoodDecomp);
	)
	
	EQUATION(Model, CoarseWoodMass,
		return INPUT(CoarseWoodLitter) - RESULT(CoarseWoodDecomp);
	)

	
	EQUATION(Model, ADecayRate,
		double rate = ComputeDecompRate(INPUT(Precipitation), INPUT(AirTemperature), PARAMETER(DecompositionRate, ACompartment), PARAMETER(DecompTempLinear, ACompartment), PARAMETER(DecompTempQuad, ACompartment), PARAMETER(DecompPrecipLinear, ACompartment));
		return rate;
	)
	
	EQUATION(Model, WDecayRate,
		double rate = ComputeDecompRate(INPUT(Precipitation), INPUT(AirTemperature), PARAMETER(DecompositionRate, WCompartment), PARAMETER(DecompTempLinear, WCompartment), PARAMETER(DecompTempQuad, WCompartment), PARAMETER(DecompPrecipLinear, WCompartment));
		return rate;
	)
	
	EQUATION(Model, EDecayRate,
		double rate = ComputeDecompRate(INPUT(Precipitation), INPUT(AirTemperature), PARAMETER(DecompositionRate, ECompartment), PARAMETER(DecompTempLinear, ECompartment), PARAMETER(DecompTempQuad, ECompartment), PARAMETER(DecompPrecipLinear, ECompartment));
		return rate;
	)
	
	EQUATION(Model, NDecayRate,
		double rate = ComputeDecompRate(INPUT(Precipitation), INPUT(AirTemperature), PARAMETER(DecompositionRate, NCompartment), PARAMETER(DecompTempLinear, ACompartment), PARAMETER(DecompTempQuad, NCompartment), PARAMETER(DecompPrecipLinear, NCompartment));
		return rate;
	)
	
	EQUATION(Model, HDecayRate,
		double rate = ComputeDecompRate(INPUT(Precipitation), INPUT(AirTemperature), PARAMETER(DecompositionRate, HCompartment), PARAMETER(DecompTempLinear, ACompartment), PARAMETER(DecompTempQuad, HCompartment), PARAMETER(DecompPrecipLinear, HCompartment));
		return rate;
	)

	
	EQUATION(Model, ADecay,
		return RESULT(AMass) * RESULT(ADecayRate);
	)
	
	EQUATION(Model, WDecay,
		return RESULT(WMass) * RESULT(WDecayRate);
	)
	
	EQUATION(Model, EDecay,
		return RESULT(EMass) * RESULT(EDecayRate);
	)
	
	EQUATION(Model, NDecay,
		return RESULT(NMass) * RESULT(NDecayRate);
	)
	
	EQUATION(Model, HDecay,
		return RESULT(HMass) * RESULT(HDecayRate);
	)
	
	
	EQUATION(Model, AInputFromLitter,
		return
			  INPUT(FoilageLitter) * PARAMETER(FoilageComp, ACompartment)
			+ RESULT(FineWoodDecomp) * PARAMETER(FineWoodComp, ACompartment)
			+ RESULT(CoarseWoodDecomp) * PARAMETER(CoarseWoodComp, ACompartment);
	)
	
	EQUATION(Model, WInputFromLitter,
		return
			  INPUT(FoilageLitter) * PARAMETER(FoilageComp, WCompartment)
			+ RESULT(FineWoodDecomp) * PARAMETER(FineWoodComp, WCompartment)
			+ RESULT(CoarseWoodDecomp) * PARAMETER(CoarseWoodComp, WCompartment);
	)
	
	EQUATION(Model, EInputFromLitter,
		return
			  INPUT(FoilageLitter) * PARAMETER(FoilageComp, ECompartment)
			+ RESULT(FineWoodDecomp) * PARAMETER(FineWoodComp, ECompartment)
			+ RESULT(CoarseWoodDecomp) * PARAMETER(CoarseWoodComp, ECompartment);
	)
	
	EQUATION(Model, NInputFromLitter,
		return
			  INPUT(FoilageLitter) * PARAMETER(FoilageComp, NCompartment)
			+ RESULT(FineWoodDecomp) * PARAMETER(FineWoodComp, NCompartment)
			+ RESULT(CoarseWoodDecomp) * PARAMETER(CoarseWoodComp, NCompartment);
	)
	
	
	EQUATION(Model, AMass,
		double fromlitter = RESULT(AInputFromLitter);
	
		double fromothers = 
			  PARAMETER(RelativeMassFlow, ACompartment, WCompartment) * RESULT(WDecay)
			+ PARAMETER(RelativeMassFlow, ACompartment, ECompartment) * RESULT(EDecay)
			+ PARAMETER(RelativeMassFlow, ACompartment, NCompartment) * RESULT(NDecay);
		
		return fromlitter + fromothers - RESULT(ADecay);
	)
	
	EQUATION(Model, WMass,
		double fromlitter = RESULT(WInputFromLitter);
	
		double fromothers = 
			  PARAMETER(RelativeMassFlow, WCompartment, ACompartment) * RESULT(ADecay)
			+ PARAMETER(RelativeMassFlow, WCompartment, ECompartment) * RESULT(EDecay)
			+ PARAMETER(RelativeMassFlow, WCompartment, NCompartment) * RESULT(NDecay);
		
		return fromlitter + fromothers - RESULT(WDecay);
	)
	
	EQUATION(Model, EMass,
		double fromlitter = RESULT(EInputFromLitter);
	
		double fromothers = 
			  PARAMETER(RelativeMassFlow, ECompartment, ACompartment) * RESULT(ADecay)
			+ PARAMETER(RelativeMassFlow, ECompartment, WCompartment) * RESULT(WDecay)
			+ PARAMETER(RelativeMassFlow, ECompartment, NCompartment) * RESULT(NDecay);
		
		return fromlitter + fromothers - RESULT(EDecay);
	)
	
	EQUATION(Model, NMass,
		double fromlitter = RESULT(NInputFromLitter);
	
		double fromothers = 
			  PARAMETER(RelativeMassFlow, NCompartment, ACompartment) * RESULT(ADecay)
			+ PARAMETER(RelativeMassFlow, NCompartment, WCompartment) * RESULT(WDecay)
			+ PARAMETER(RelativeMassFlow, NCompartment, ECompartment) * RESULT(EDecay);
		
		return fromlitter + fromothers - RESULT(NDecay);
	)
	
	EQUATION(Model, HMass,

		double fromothers = 
			  PARAMETER(RelativeMassFlow, HCompartment, ACompartment) * RESULT(ADecay)
			+ PARAMETER(RelativeMassFlow, HCompartment, WCompartment) * RESULT(WDecay)
			+ PARAMETER(RelativeMassFlow, HCompartment, ECompartment) * RESULT(EDecay)
			+ PARAMETER(RelativeMassFlow, HCompartment, NCompartment) * RESULT(NDecay);
		
		return fromothers - RESULT(HDecay);
	)
	
	
	EndModule(Model);
}