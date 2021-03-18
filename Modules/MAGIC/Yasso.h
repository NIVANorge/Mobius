

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

[^https://en.ilmatieteenlaitos.fi/yasso-description^ Model description]
)"""");


	
	auto Dimensionless     = RegisterUnit(Model);
	auto PerMonth          = RegisterUnit(Model, "1/month");
	auto MmPerMonth        = RegisterUnit(Model, "mm/month");
	auto MMolPerM2         = RegisterUnit(Model, "mmol/m2");
	auto MMolPerM2PerMonth = RegisterUnit(Model, "mmol/m2/month");
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
	
	auto DecompositionRate = RegisterParameterDouble(Model, BoxPar, "Decomposition rate", PerMonth, 0.0, 0.0, 20.0);
	auto DecompTempLinear  = RegisterParameterDouble(Model, BoxPar, "Decomposition rate linear temperature dependence", PerDegC, 0.0, 0.0, 0.2);
	auto DecompTempQuad    = RegisterParameterDouble(Model, BoxPar, "Decomposition rate quadratic temperature dependence", PerDegC2, 0.0, -0.05, 0.0);
	auto DecompPrecipLinear= RegisterParameterDouble(Model, BoxPar, "Decomposition rate linear precipitation dependence", PerMm, 0.0, -20.0, 0.0);
	auto LitterFraction    = RegisterParameterDouble(Model, BoxPar, "Litter fraction", Dimensionless, 0.0, 0.0, 1.0);
	
	
	auto TransferPar       = RegisterParameterGroup(Model, "Transfer matrix", Compartment, Compartment);
	
	auto RelativeMassFlow  = RegisterParameterDouble(Model, TransferPar, "Relative mass flow", PerMonth, 0.0, 0.0, 1.0);
	
	
	
	
	auto Precipitation = RegisterInput(Model, "Precipitation", MmPerMonth);
	auto AirTemperature= RegisterInput(Model, "Air temperature", DegreesCelsius);
	auto Litter        = RegisterInput(Model, "Litter", MMolPerM2PerMonth);
	
	
	auto SoilSolver = RegisterSolver(Model, "YASSO soil solver", 0.1, IncaDascru);
	
	auto AMass      = RegisterEquationODE(Model, "(A) mass", MMolPerM2, SoilSolver);
	auto WMass      = RegisterEquationODE(Model, "(W) mass", MMolPerM2, SoilSolver);
	auto EMass      = RegisterEquationODE(Model, "(E) mass", MMolPerM2, SoilSolver);
	auto NMass      = RegisterEquationODE(Model, "(N) mass", MMolPerM2, SoilSolver);
	auto HMass      = RegisterEquationODE(Model, "(H) mass", MMolPerM2, SoilSolver);
	
	auto ADecay     = RegisterEquation(Model, "(A) decay", MMolPerM2PerMonth, SoilSolver);
	auto WDecay     = RegisterEquation(Model, "(W) decay", MMolPerM2PerMonth, SoilSolver);
	auto EDecay     = RegisterEquation(Model, "(E) decay", MMolPerM2PerMonth, SoilSolver);
	auto NDecay     = RegisterEquation(Model, "(N) decay", MMolPerM2PerMonth, SoilSolver);
	auto HDecay     = RegisterEquation(Model, "(H) decay", MMolPerM2PerMonth, SoilSolver);
	
	EQUATION(Model, ADecay,
		double rate = ComputeDecompRate(INPUT(Precipitation), INPUT(AirTemperature), PARAMETER(DecompositionRate, ACompartment), PARAMETER(DecompTempLinear, ACompartment), PARAMETER(DecompTempQuad, ACompartment), PARAMETER(DecompPrecipLinear, ACompartment));
		return RESULT(AMass) * rate;
	)
	
	EQUATION(Model, WDecay,
		double rate = ComputeDecompRate(INPUT(Precipitation), INPUT(AirTemperature), PARAMETER(DecompositionRate, WCompartment), PARAMETER(DecompTempLinear, WCompartment), PARAMETER(DecompTempQuad, WCompartment), PARAMETER(DecompPrecipLinear, WCompartment));
		return RESULT(WMass) * rate;
	)
	
	EQUATION(Model, EDecay,
		double rate = ComputeDecompRate(INPUT(Precipitation), INPUT(AirTemperature), PARAMETER(DecompositionRate, ECompartment), PARAMETER(DecompTempLinear, ECompartment), PARAMETER(DecompTempQuad, ECompartment), PARAMETER(DecompPrecipLinear, ECompartment));
		return RESULT(EMass) * rate;
	)
	
	EQUATION(Model, NDecay,
		double rate = ComputeDecompRate(INPUT(Precipitation), INPUT(AirTemperature), PARAMETER(DecompositionRate, NCompartment), PARAMETER(DecompTempLinear, ACompartment), PARAMETER(DecompTempQuad, NCompartment), PARAMETER(DecompPrecipLinear, NCompartment));
		return RESULT(NMass) * rate;
	)
	
	EQUATION(Model, HDecay,
		double rate = ComputeDecompRate(INPUT(Precipitation), INPUT(AirTemperature), PARAMETER(DecompositionRate, HCompartment), PARAMETER(DecompTempLinear, ACompartment), PARAMETER(DecompTempQuad, HCompartment), PARAMETER(DecompPrecipLinear, HCompartment));
		return RESULT(HMass) * rate;
	)
	
	EQUATION(Model, AMass,
		double fromothers = 
			  PARAMETER(RelativeMassFlow, ACompartment, WCompartment) * RESULT(WDecay)
			+ PARAMETER(RelativeMassFlow, ACompartment, ECompartment) * RESULT(EDecay)
			+ PARAMETER(RelativeMassFlow, ACompartment, NCompartment) * RESULT(NDecay);
		
		return fromothers - RESULT(ADecay) + INPUT(Litter) * PARAMETER(LitterFraction, ACompartment); //TODO: the size correction term!
	)
	
	EQUATION(Model, WMass,
		double fromothers = 
			  PARAMETER(RelativeMassFlow, WCompartment, ACompartment) * RESULT(ADecay)
			+ PARAMETER(RelativeMassFlow, WCompartment, ECompartment) * RESULT(EDecay)
			+ PARAMETER(RelativeMassFlow, WCompartment, NCompartment) * RESULT(NDecay);
		
		return fromothers - RESULT(WDecay) + INPUT(Litter) * PARAMETER(LitterFraction, WCompartment); //TODO: the size correction term!

	)
	
	EQUATION(Model, EMass,
		double fromothers = 
			  PARAMETER(RelativeMassFlow, ECompartment, ACompartment) * RESULT(ADecay)
			+ PARAMETER(RelativeMassFlow, ECompartment, WCompartment) * RESULT(WDecay)
			+ PARAMETER(RelativeMassFlow, ECompartment, NCompartment) * RESULT(NDecay);
		
		return fromothers - RESULT(EDecay) + INPUT(Litter) * PARAMETER(LitterFraction, ECompartment); //TODO: the size correction term!

	)
	
	EQUATION(Model, NMass,
		double fromothers = 
			  PARAMETER(RelativeMassFlow, NCompartment, ACompartment) * RESULT(ADecay)
			+ PARAMETER(RelativeMassFlow, NCompartment, WCompartment) * RESULT(WDecay)
			+ PARAMETER(RelativeMassFlow, NCompartment, ECompartment) * RESULT(EDecay);
		
		return fromothers - RESULT(NDecay) + INPUT(Litter) * PARAMETER(LitterFraction, NCompartment); //TODO: the size correction term!

	)
	
	EQUATION(Model, HMass,
		double fromothers = 
			  PARAMETER(RelativeMassFlow, HCompartment, ACompartment) * RESULT(ADecay)
			+ PARAMETER(RelativeMassFlow, HCompartment, WCompartment) * RESULT(WDecay)
			+ PARAMETER(RelativeMassFlow, HCompartment, ECompartment) * RESULT(EDecay)
			+ PARAMETER(RelativeMassFlow, HCompartment, NCompartment) * RESULT(NDecay);
		
		return fromothers - RESULT(HDecay) + INPUT(Litter) * PARAMETER(LitterFraction, HCompartment); //TODO: the size correction term!

	)
	
	
	EndModule(Model);
}