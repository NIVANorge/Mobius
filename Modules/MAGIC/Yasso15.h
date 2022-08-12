

#include <armadillo>


static void
AddYasso15Model(mobius_model *Model)
{
	//TODO; add right reference.
	BeginModule(Model, "YASSO 15", "0.1.0");
	
	SetModuleDescription(Model, R""""(
This is an un-official implementation of the Yasso and Yasso15 models

Yasso :
[De Wit et. al. 2006, A carbon budget of forest biomass and soils in southeast Norway calculated using a widely applicable method, Forest Ecology and Management 226(1-3) pp15-26.](https://doi.org/10.1016/j.foreco.2005.12.023)

Yasso15 :
[Järvenpää et. al. 2018, Soil carbon model Yasso15 - Bayesian calibration using worldwide litter decomposition and carbon stock data](https://en.ilmatieteenlaitos.fi/documents/31422/0/Yasso15+manuscript/a3cd1a95-11a6-431e-ac81-6c1470fa7e1d)
)"""");


	auto Dimensionless     = RegisterUnit(Model);
	auto PerYear           = RegisterUnit(Model, "1/year");
	auto MmPerYear         = RegisterUnit(Model, "mm/year");
	auto KgPerM2           = RegisterUnit(Model, "kg/m2");
	auto KgPerM2PerYear    = RegisterUnit(Model, "kg/m2/year");
	auto DegreesCelsius    = RegisterUnit(Model, "°C");
	auto PerDegC           = RegisterUnit(Model, "1/°C");
	auto PerDegC2          = RegisterUnit(Model, "1/°C2");
	auto PerM             = RegisterUnit(Model, "1/m");
	
	auto Compartment       = RegisterIndexSet(Model, "Compartment");
	
	auto GlobalPar         = RegisterParameterGroup(Model, "Climate general");
	
	auto ResponseModel     = RegisterParameterEnum(Model, GlobalPar, "Climate response model", {"Yasso", "Yasso15" }, "Yasso15");
	auto YassoResponse = EnumValue(Model, ResponseModel, "Yasso");
	auto Yasso15Response = EnumValue(Model, ResponseModel, "Yasso15");
	auto ReferenceTemp     = RegisterParameterDouble(Model, GlobalPar, "Temperature at which base rates are measured", DegreesCelsius, 3.3, -20.0, 20.0);
	
	
	auto BoxPar            = RegisterParameterGroup(Model, "Compartment parameters", Compartment);
	
	auto DecompositionRate = RegisterParameterDouble(Model, BoxPar, "Decomposition or fractionation rate", PerYear, 0.0, 0.0, 20.0);
	auto DecompTempLinear0 = RegisterParameterDouble(Model, BoxPar, "Yasso Decomposition rate linear temperature dependence", PerDegC, 0.0, 0.0, 0.2);
	auto DecompTempLinear  = RegisterParameterDouble(Model, BoxPar, "Yasso15 Decomposition rate linear temperature dependence", PerDegC, 0.0, 0.0, 0.2);
	auto DecompTempSquare  = RegisterParameterDouble(Model, BoxPar, "Yasso15 Decomposition rate square temperature dependence", PerDegC2, 0.0, -0.05, 0);
	auto DecompPrecipLinear= RegisterParameterDouble(Model, BoxPar, "Yasso15 Decomposition rate linear precipitation dependence", PerM, 0.0, -20.0, 0.0);
	auto FoliageComp       = RegisterParameterDouble(Model, BoxPar, "Foliage chemical composition", Dimensionless, 0.0, 0.0, 1.0);
	
	auto TransferPar       = RegisterParameterGroup(Model, "Transfer matrix", Compartment, Compartment);
	
	auto RelativeMassFlow  = RegisterParameterDouble(Model, TransferPar, "Relative mass flow", Dimensionless, 0.0, 0.0, 1.0);

	
	
	
	auto Precipitation     = RegisterInput(Model, "Precipitation", MmPerYear);
	auto AirTemperature    = RegisterInput(Model, "Air temperature", DegreesCelsius);
	auto FoliageLitter     = RegisterInput(Model, "Foliage and fine root litter inputs", KgPerM2PerYear);
	auto FineWoodLitter    = RegisterInput(Model, "Fine wood litter inputs", KgPerM2PerYear);
	auto CoarseWoodLitter  = RegisterInput(Model, "Coarse wood litter inputs", KgPerM2PerYear);
	
	
	auto FwlCompartment    = RequireIndex(Model, Compartment, "Fine wood");
	auto CwlCompartment    = RequireIndex(Model, Compartment, "Coarse wood");
	auto ACompartment      = RequireIndex(Model, Compartment, "(A) Acid-hydrolyzable");
	auto WCompartment      = RequireIndex(Model, Compartment, "(W) Water-soluble");
	auto ECompartment      = RequireIndex(Model, Compartment, "(E) Ethanol-soluble");
	auto NCompartment      = RequireIndex(Model, Compartment, "(N) Non-soluble");
	auto HCompartment      = RequireIndex(Model, Compartment, "(H) Humus");
	
	
	auto DecompOrFracRate  = RegisterEquation(Model, "Decomposition or fractionation rate (adjusted)", PerYear);
	SetInitialValue(Model, DecompOrFracRate, DecompOrFracRate); //We need to force it to compute an initial value.
	
	auto Mass              = RegisterEquation(Model, "Carbon mass per compartment", KgPerM2);
	
	auto ComputeYasso      = RegisterEquation(Model, "Compute Yasso", Dimensionless);
	auto ComputeSteady     = RegisterEquationInitialValue(Model, "Steady state", Dimensionless);
	SetInitialValue(Model, ComputeYasso, ComputeSteady);
	
	auto TotalCarbon       = RegisterEquation(Model, "Total soil carbon", KgPerM2);
	auto TotalCarbonLitter = RegisterEquationCumulative(Model, "Total soil carbon + litter", Mass, Compartment);
	
	EquationIsComputedBy(Model, Mass, ComputeYasso, Compartment);
	
	EQUATION(Model, TotalCarbon,
		double result = 0.0;
		for(index_t Comp = ACompartment; Comp < INDEX_COUNT(Compartment); ++Comp)
			result += RESULT(Mass, Comp);
		return result;
	)
	
	EQUATION(Model, DecompOrFracRate,
		auto Comp = CURRENT_INDEX(Compartment);
		double base_rate = PARAMETER(DecompositionRate);
		double beta0 = PARAMETER(DecompTempLinear0);
		double beta = PARAMETER(DecompTempLinear);
		double beta2 = PARAMETER(DecompTempSquare);
		double gamma = PARAMETER(DecompPrecipLinear);
		double temp = INPUT(AirTemperature);
		double reftemp = PARAMETER(ReferenceTemp);
		double precip = INPUT(Precipitation) * 1e-3;
		
		double adjusted_yasso   = base_rate * (1.0 + beta0*(temp - reftemp));
		double adjusted_yasso15 = base_rate * std::exp(beta*temp + beta2*temp*temp)*(1.0 - std::exp(gamma*precip));
		
		auto resp = PARAMETER(ResponseModel);
		
		if(Comp == FwlCompartment || Comp == CwlCompartment)
			return base_rate;
		if(resp == YassoResponse)
			return adjusted_yasso;
		return adjusted_yasso15;
	)
	
	
	auto MakeMatrix = [=](arma::mat &A, arma::vec &b, model_run_state *RunState__) -> void {
	//MakeMatrix(arma::mat &A, arma::vec &b, model_run_state *RunState__,
	//index_set_h Compartment, index_t FwlCompartment, index_t CwlCompartment, input_h FineWoodLitter, input_h CoarseWoodLitter, parameter_h FoliageComp, equation_h DecompOrFracRate, parameter_h RelativeMassFlow) {
		
		for(index_t Comp = FIRST_INDEX(Compartment); Comp < INDEX_COUNT(Compartment); ++Comp) {
			int i = Comp.Index;
			
			if(Comp == FwlCompartment) {
				b(i) = INPUT(FineWoodLitter);
			} else if(Comp == CwlCompartment) {
				b(i) = INPUT(CoarseWoodLitter);
			} else {
				b(i) = PARAMETER(FoliageComp, Comp) * INPUT(FoliageLitter);
			}
			
			for(index_t ToComp = FIRST_INDEX(Compartment); ToComp < INDEX_COUNT(Compartment); ++ToComp) {
				double decomp_rate = RESULT(DecompOrFracRate, Comp);
				if(Comp == ToComp)
					A(Comp, Comp) = -decomp_rate;
				else
					A(ToComp, Comp) = decomp_rate * PARAMETER(RelativeMassFlow, ToComp, Comp);
			}
		}
	};
	
	
	EQUATION(Model, ComputeYasso,
		/*
		NOTE: the linear system
			dx/dt = A*x + b
		has solution
			x(t) = exp(A*t)x(0) + A^-1(exp(A*t) - I)b
		*/
		int N = INDEX_COUNT(Compartment);
		arma::vec x0(N, arma::fill::zeros);       // Masses
		arma::mat A(N, N, arma::fill::zeros);     // Transfer matrix
		arma::vec b(N, arma::fill::zeros);        // Sources
		
		for(index_t Comp = FIRST_INDEX(Compartment); Comp < INDEX_COUNT(Compartment); ++Comp) {
			int i = Comp.Index;
			x0(i) = LAST_RESULT(Mass, Comp);
		}
		
		MakeMatrix(A, b, RunState__);//, Compartment, FwlCompartment, CwlCompartment, FineWoodLitter, CoarseWoodLitter, FoliageComp, DecompOrFracRate, RelativeMassFlow);
		
		double dt = 1.0;

		arma::mat expAdt(N, N, arma::fill::none);
		bool Success = arma::expmat(expAdt, dt*A);
		if(!Success) FatalError("ERROR(Yasso): Failed to exponentiate matrix.");
		arma::vec x(N, arma::fill::none);
		Success  = arma::solve(x, A, expAdt*b - b);
		if(!Success) FatalError("ERROR(Yasso): Failed to solve linear system.");
		x += expAdt*x0;
		
		for(index_t Comp = FIRST_INDEX(Compartment); Comp < INDEX_COUNT(Compartment); ++Comp) {
			int i = Comp.Index;
			SET_RESULT(Mass, x(i), Comp);
		}
		return 0.0;
	)
	
	EQUATION(Model, ComputeSteady,
		
		int N = INDEX_COUNT(Compartment);
		
		arma::vec x(N, arma::fill::zeros);       // Masses
		arma::mat A(N, N, arma::fill::zeros);    // Transfer matrix
		arma::vec b(N, arma::fill::zeros);       // Sources
		
		MakeMatrix(A, b, RunState__);
		
		bool Success = arma::solve(x, -A, b);
		if(!Success) FatalError("ERROR(Yasso): Failed to solve steady state.");
		
		for(index_t Comp = FIRST_INDEX(Compartment); Comp < INDEX_COUNT(Compartment); ++Comp) {
			int i = Comp.Index;
			SET_RESULT(Mass, x(i), Comp);
		}
		
		//WarningPrint("x = ", x, "\nb = ", b, "\nA = ", A, "\n");
		
		return 0.0;
	)
	
	EndModule(Model);
}