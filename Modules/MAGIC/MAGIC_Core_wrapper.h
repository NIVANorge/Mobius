

/*
	This is a Mobius wrapper for the MAGIC core.
	
	This has to be used together with another "driver" module that determines things like temperature and external fluxes (nitrification, weathering, deposition etc. etc.)
*/



#include "MAGIC_Core.h"

static void
AddMagicCoreModel(mobius_model *Model)
{
	BeginModule(Model, "MAGIC Core", "_dev");
	
	
	auto Dimensionless = RegisterUnit(Model);
	auto M             = RegisterUnit(Model, "m");
	auto MPerTs        = RegisterUnit(Model, "m/timestep");
	auto Km2           = RegisterUnit(Model, "km2");
	auto KgPerM3       = RegisterUnit(Model, "kg/m3");
	auto MMolPerM2     = RegisterUnit(Model, "mmol/m2");
	auto MMolPerM3     = RegisterUnit(Model, "mmol/m3");
	auto MEqPerKg      = RegisterUnit(Model, "meq/kg");
	auto MEqPerM3      = RegisterUnit(Model, "meq/m3");
	auto Log10MolPerL  = RegisterUnit(Model, "log10(mol/l)")
	auto MMolPerTs     = RegisterUnit(Model, "mmol/timestep");
	auto MMolPerM2PerTs = RegisterUnit(Model, "mmol/m2/timestep");
	auto DegreesCelsius	= RegisterUnit(Model, "°C");
	auto Percent        = RegisterUnit(Model, "%");
	
	auto GeneralParams             = RegisterParameterGroup(Model, "General parameters");
	auto ConvergenceCriterion      = RegisterParameterDouble(Model, GeneralParams, "Convergence criterion", Dimensionless, 1.0, 0.01, 10.0, "Convergence criterion to stop solution routine, difference in total plus and total minus charges in solution NOTE: CONV = 1.0 is usual, but smaller values may be needed (at computational cost) if reliable pH's above 6-7 are needed");
	
	//TODO: Should any of the following be per compartment instead?
	auto Log10AlOH3EquilibriumConst= RegisterParameterDouble(Model, GeneralParams, "(log10) Al(OH)3 dissociation equilibrium constant", Dimensionless, 0.0); //TODO
	auto HAlOH3Exponent            = RegisterParameterDouble(Model, GeneralParams, "Al(OH)3 dissociation equation exponent", Dimensionless, 3.0); //TODO
	
	auto PK1DOC                    = RegisterParameterDouble(Model, GeneralParams, "(-log10) pK 1st equilibrium constant for tripriotic organic acid", Dimensionless, 0.0); //TODO
	auto PK2DOC                    = RegisterParameterDouble(Model, GeneralParams, "(-log10) pK 2nd equilibrium constant for tripriotic organic acid", Dimensionless, 0.0); //TODO
	auto PK3DOC                    = RegisterParameterDouble(Model, GeneralParams, "(-log10) pK 3rd equilibrium constant for tripriotic organic acid", Dimensionless, 0.0); //TODO
	auto PK1AlDOC                  = RegisterParameterDouble(Model, GeneralParams, "(-log10) pK Al(A) equilibrium constant for [(Al3+)(A3-)]", Dimensionless, 0.0); //TODO
	auto PK2AlDOC                  = RegisterParameterDouble(Model, GeneralParams, "(-log10 pK Al(HA)+ equilibrium constant for [(Al3+)(HA2-)+]", Dimensionless, 0.0); //TODO
	
	auto Log10CaAlSelectCoeff      = RegisterParameterDouble(Model, GeneralParams, "(log10) Ca/Al exchange selectivity coefficient", Dimensionless, 0.0); //TODO
	auto Log10MgAlSelectCoeff      = RegisterParameterDouble(Model, GeneralParams, "(log10) Mg/Al exchange selectivity coefficient", Dimensionless, 0.0); //TODO
	auto Log10NaAlSelectCoeff      = RegisterParameterDouble(Model, GeneralParams, "(log10) Na/Al exchange selectivity coefficient", Dimensionless, 0.0); //TODO
	auto Log10KAlSelectCoeff       = RegisterParameterDouble(Model, GeneralParams, "(log10) K/Al exchange selectivity coefficient", Dimensionless, 0.0); //TODO
	
	
	auto Compartment = RegisterIndexSet(Model, "Compartment");
	auto CompartmentParams = RegisterParameterGroup(Model, "Compartment parameters");
	
	auto IsSoil                    = RegisterParameterBool(Model, CompartmentParams, "This is a soil compartment", true);
	auto Area                      = RegisterParameterDouble(Model, CompartmentParams, "Surface area", Km2, 1.0, 0.0, 10000.0);
	auto Depth                     = RegisterParameterDouble(Model, CompartmentParams, "Depth", M, 1.0, 0.0, 100.0);
	auto Porosity                  = RegisterParameterDouble(Model, CompartmentParams, "Porosity", Dimensionless, 0.5, 0.0, 1.0);
	auto BulkDensity               = RegisterParameterDouble(Model, CompartmentParams, "Bulk density", KgPerM3, 0.25, 0.0, 2.0);
	auto CationExchangeCapacity    = RegisterParameterDouble(Model, CompartmentParams, "Cation exchange capacity", MeqPerKg, 9.0, 0.0, 50.0);
	auto SO4HalfSat                = RegisterParameterDouble(Model, CompartmentParams, "Soil sulfate adsorption capacity, half saturation", MeqPerKg, 0.0); //TODO
	auto SO4MaxCap                 = RegisterParameterDouble(Model, CompartmentParams, "Soil sulfate adsorption max capacity", MeqPerKg, 0.0); //TODO
	
	auto FlowFractions = RegisterParameterGroup(Model, "Flow fractions", Compartment, Compartment);
	
	auto FlowFraction = RegisterParameterDouble(Model, FlowFractions, "Flow fraction", Dimensionless, 0.0, 0.0, 1.0, "How large of a fraction of the discharge of this compartment (the row) goes to another compartment (the column)");
	
	
	
	
	auto ConcCa            = RegisterEquation(Model, "Ca(2+) ionic concentration", MEqPerM3);
	auto ConcMg            = RegisterEquation(Model, "Mg(2+) ionic concentration", MEqPerM3);
	auto ConcNa            = RegisterEquation(Model, "Na(+) ionic concentration", MEqPerM3);
	auto ConcK             = RegisterEquation(Model, "K(+) ionic concentration", MEqPerM3);
	auto ConcNH4           = RegisterEquation(Model, "NH4(+) ionic concentration", MEqPerM3);
	auto ConcSO4           = RegisterEquation(Model, "SO4(2-) ionic concentration", MEqPerM3);
	auto ConcCl            = RegisterEquation(Model, "Cl(-) ionic concentration", MEqPerM3);
	auto ConcNO3           = RegisterEquation(Model, "NO3(-) ionic concentration", MEqPerM3);
	auto ConcF             = RegisterEquation(Model, "F(-) ionic concentration", MEqPerM3);
	
	auto ConcAllSO4        = RegisterEquation(Model, "Total Sulfate in solution (ionic + Al complexes)", MEqPerM3);
	auto ConcAllF          = RegisterEquation(Model, "Total Fluoride in solution (ionic + Al complexes)", MEqPerM3);
	
	auto PH                = RegisterEquation(Model, "pH", Log10MolPerL);
	auto SumBaseCationConc = RegisterEquation(Model, "Sum of base cation concentrations (Ca + Mg + Na + K)", MEqPerM3);
	auto SumAcidAnionConc  = RegisterEquation(Model, "Sum of acid anion concentrations (SO4 + Cl + NO3 + F", MEqPerM3);
	auto ChargeBalanceAlk  = RegisterEquation(Model, "Charge balance alkalinity", MEqPerM3);
	auto WeakAcidAlk       = RegisterEquation(Model, "Weak acid alkalinity", MEqPerM3);

	auto ExchangeableCa    = RegisterEquation(Model, "Exchangeable Ca on soil as % of CEC", Percent);
	auto ExchangeableMg    = RegisterEquation(Model, "Exchangeable Mg on soil as % of CEC", Percent);
	auto ExchangeableNa    = RegisterEquation(Model, "Exchangeable Na on soil as % of CEC", Percent);
	auto ExchangeableK     = RegisterEquation(Model, "Exchangeable K on soil as % of CEC", Percent);
	auto ExchangeableSO4   = RegisterEquation(Model, "Exchangeable SO4 on soil as % of CEC", Percent);
	auto BaseSaturationSoil = RegisterEquation(Model, "Base saturation of soil (ECa + EMg + ENa + EK)")

	auto ConcHCO3          = RegisterEquation(Model, "HCO3 (Bicarbonate) ionic concentration", MEqPerM3);
	auto ConcCO3           = RegisterEquation(Model, "CO3 (Carbonate) ionic concentration", MEqPerM3);
	auto ConcAl            = RegisterEquation(Model, "Al(3+) ionic concentration", MEqPerM3);
	auto ConcAllAl         = RegisterEquation(Model, "Total aluminum in solution (ionic + SO4-F-DOC complexes)", MEqPerM3);
	auto ConcOrgAl         = RegisterEquation(Model, "Aluminum in solution as organic complexes (AlA, Al(HA)(+)", MEqPerM3);

	auto ConcH2AM          = RegisterEquation(Model, "[H2A-] Monovalent ion concentration, triprotic organic acid", MEqPerM3);
	auto ConcHA2M          = RegisterEquation(Model, "[HA2-] Divalent ion concentration, triprotic organic acid", MEqPerM3);
	auto ConcA3M           = RegisterEquation(Model, "[A3-] Trivalent ion concentration, triprotic organic acid", MEqPerM3);
	
	auto SumPositive       = RegisterEquation(Model, "Sum of all positive charges in solution (cations)", MEqPerM3);
	auto SumNegative       = RegisterEquation(Model, "Sum of all negative charges in solution (anions)", MEqPerM3);
	
	auto ConcAllDIC        = RegisterEquation(Model, "Total anionic charge from inorganic carbon in solution (HC3O, CO3)", MEqPerM3);
	auto ConcAllDOC        = RegisterEquation(Model, "Total anionic charge from DOC (triprotic acid) in solution (H2AM, HA2M, A3M)", MEqPerM3);
	auto CaAlRatio         = RegisterEquation(Model, "Ca ion to aqueous Al molar ratio", MEqPerM3);
	
	auto ConcH         = RegisterEquation(Model, "H(+) ionic concentration", MEqPerM3);
	SetInitialValue(Model, ConcH, 1.0);
	auto IonicStrength = RegisterEquation(Model, "Ionic strength", Dimensionless);
	SetInitialValue(Model, IonicStrength, 0.0);
	
	
	auto CaInput       = RegisterEquation(Model, "Ca input from other compartments", MMolPerM2PerTs);
	auto MgInput       = RegisterEquation(Model, "Mg input from other compartments", MMolPerM2PerTs);
	auto NaInput       = RegisterEquation(Model, "Na input from other compartments", MMolPerM2PerTs);
	auto KInput        = RegisterEquation(Model, "K input from other compartments", MMolPerM2PerTs);
	auto NH4Input      = RegisterEquation(Model, "NH4 input from other compartments", MMolPerM2PerTs);
	auto SO4Input      = RegisterEquation(Model, "SO4 input from other compartments", MMolPerM2PerTs);
	auto ClInput       = RegisterEquation(Model, "Cl input from other compartments", MMolPerM2PerTs);
	auto NO3Input      = RegisterEquation(Model, "NO3 input from other compartments", MMolPerM2PerTs);
	auto FInput        = RegisterEquation(Model, "F input from other compartments", MMolPerM2PerTs);
	
	
	auto CaOutput      = RegisterEquation(Model, "Ca output via discharge", MMolPerM2PerTs);
	auto MgOutput      = RegisterEquation(Model, "Mg output via discharge", MMolPerM2PerTs);
	auto NaOutput      = RegisterEquation(Model, "Na output via discharge", MMolPerM2PerTs);
	auto KOutput       = RegisterEquation(Model, "K output via discharge", MMolPerM2PerTs);
	auto NH4Output     = RegisterEquation(Model, "NH4 output via discharge", MMolPerM2PerTs);
	auto SO4Output     = RegisterEquation(Model, "SO4 output via discharge", MMolPerM2PerTs);
	auto ClOutput      = RegisterEquation(Model, "Cl output via discharge", MMolPerM2PerTs);
	auto NO3Output     = RegisterEquation(Model, "NO3 output via discharge", MMolPerM2PerTs);
	auto FOutput       = RegisterEquation(Model, "F output via discharge", MMolPerM2PerTs);
	
	
	//TODO: Initial value for these:
	auto TotalCa       = RegisterEquation(Model, "Total Ca mass", MMolPerM2);
	auto TotalMg       = RegisterEquation(Model, "Total Mg mass", MMolPerM2);
	auto TotalNa       = RegisterEquation(Model, "Total Na mass", MMolPerM2);
	auto TotalK        = RegisterEquation(Model, "Total K  mass", MMolPerM2);
	auto TotalNH4      = RegisterEquation(Model, "Total NH4 mass", MMolPerM2);
	auto TotalSO4      = RegisterEquation(Model, "Total SO4 mass", MMolPerM2);
	auto TotalCl       = RegisterEquation(Model, "Total Cl mass", MMolPerM2);
	auto TotalNO3      = RegisterEquation(Model, "Total NO3 mass", MMolPerM2);
	auto TotalF        = RegisterEquation(Model, "Total F mass", MMolPerM2);
	
	
	
	// Equations that have to be defined by an outside "driver", and are not provided in the core, but which the core has to read the values of:
	auto Temperature        = RegisterEquation(Model, "Temperature", DegreesCelsius);
	auto PartialPressureCO2 = RegisterEquation(Model, "CO2 partial pressure", Percent);
	auto DOCConcentration   = RegisterEquation(Model, "DOC concentration", MMolPerM3);
	auto Discharge          = RegisterEquation(Model, "Discharge", MPerTs);
	
	auto CaExternalFlux     = RegisterEquation(Model, "Sum of Ca fluxes not related to discharge", MMolPerM2PerTs);
	auto MgExternalFlux     = RegisterEquation(Model, "Sum of Mg fluxes not related to discharge", MMolPerM2PerTs);
	auto NaExternalFlux     = RegisterEquation(Model, "Sum of Na fluxes not related to discharge", MMolPerM2PerTs);
	auto KExternalFlux      = RegisterEquation(Model, "Sum of K fluxes not related to discharge", MMolPerM2PerTs);
	auto NH4ExternalFlux    = RegisterEquation(Model, "Sum of NH4 fluxes not related to discharge", MMolPerM2PerTs);
	auto SO4ExternalFlux    = RegisterEquation(Model, "Sum of SO4 fluxes not related to discharge", MMolPerM2PerTs);
	auto ClExternalFlux     = RegisterEquation(Model, "Sum of Cl fluxes not related to discharge", MMolPerM2PerTs);
	auto NO3ExternalFlux    = RegisterEquation(Model, "Sum of NO3 fluxes not related to discharge", MMolPerM2PerTs);
	auto FExternalFlux      = RegisterEquation(Model, "Sum of F fluxes not related to discharge", MMolPerM2PerTs);
	
	//TODO: Could the mass balance equations be done using an index set where the indexes are (Ca, Mg, Na ...)?
	
	EQUATION(Model, CaOutput,
		return RESULT(Discharge)*2.0*RESULT(ConcCa);
	)
	
	EQUATION(Model, MgOutput,
		return RESULT(Discharge)*2.0*RESULT(ConcMg);
	)
	
	EQUATION(Model, NaOutput,
		return RESULT(Discharge)*RESULT(ConcNa);
	)
	
	EQUATION(Model, KOutput,
		return RESULT(Discharge)*RESULT(ConcK);
	)
	
	EQUATION(Model, NH4Output,
		return RESULT(Discharge)*RESULT(ConcNH4);
	)
	
	EQUATION(Model, SO4Output,
		return RESULT(Discharge)*2.0*RESULT(ConcAllSO4);
	)
	
	EQUATION(Model, ClOutput,
		return RESULT(Discharge)*RESULT(ConcCl);
	)
	
	EQUATION(Model, NO3Output,
		return RESULT(Discharge)*RESULT(ConcNO3);
	)
	
	EQUATION(Model, FOutput,
		return RESULT(Discharge)*Result(ConcAllF);
	)
	
	EQUATION(Model, CaInput,
		double input = 0.0;
		index_t ThisCompartment = CURRENT_INDEX(Compartment);
		for(index_t OtherCompartment = FIRST_INDEX(Compartment); OtherCompartment < ThisCompartment; ++OtherCompartment)
		{
			input += RESULT(CaOutput, OtherCompartment) * PARAMETER(FlowFraction, OtherCompartment, ThisCompartment) * PARAMETER(Area, OtherCompartment);
		}
		return input / PARAMETER(Area);
	)
	
	EQUATION(Model, MgInput,
		double input = 0.0;
		index_t ThisCompartment = CURRENT_INDEX(Compartment);
		for(index_t OtherCompartment = FIRST_INDEX(Compartment); OtherCompartment < ThisCompartment; ++OtherCompartment)
		{
			input += RESULT(MgOutput, OtherCompartment) * PARAMETER(FlowFraction, OtherCompartment, ThisCompartment) * PARAMETER(Area, OtherCompartment);
		}
		return input / PARAMETER(Area);
	)
	
	EQUATION(Model, NaInput,
		double input = 0.0;
		index_t ThisCompartment = CURRENT_INDEX(Compartment);
		for(index_t OtherCompartment = FIRST_INDEX(Compartment); OtherCompartment < ThisCompartment; ++OtherCompartment)
		{
			input += RESULT(NaOutput, OtherCompartment) * PARAMETER(FlowFraction, OtherCompartment, ThisCompartment) * PARAMETER(Area, OtherCompartment);
		}
		return input / PARAMETER(Area);
	)
	
	EQUATION(Model, KInput,
		double input = 0.0;
		index_t ThisCompartment = CURRENT_INDEX(Compartment);
		for(index_t OtherCompartment = FIRST_INDEX(Compartment); OtherCompartment < ThisCompartment; ++OtherCompartment)
		{
			input += RESULT(KOutput, OtherCompartment) * PARAMETER(FlowFraction, OtherCompartment, ThisCompartment) * PARAMETER(Area, OtherCompartment);
		}
		return input / PARAMETER(Area);
	)
	
	EQUATION(Model, NH4Input,
		double input = 0.0;
		index_t ThisCompartment = CURRENT_INDEX(Compartment);
		for(index_t OtherCompartment = FIRST_INDEX(Compartment); OtherCompartment < ThisCompartment; ++OtherCompartment)
		{
			input += RESULT(NH4Output, OtherCompartment) * PARAMETER(FlowFraction, OtherCompartment, ThisCompartment) * PARAMETER(Area, OtherCompartment);
		}
		return input / PARAMETER(Area);
	)
	
	EQUATION(Model, SO4Input,
		double input = 0.0;
		index_t ThisCompartment = CURRENT_INDEX(Compartment);
		for(index_t OtherCompartment = FIRST_INDEX(Compartment); OtherCompartment < ThisCompartment; ++OtherCompartment)
		{
			input += RESULT(SO4Output, OtherCompartment) * PARAMETER(FlowFraction, OtherCompartment, ThisCompartment) * PARAMETER(Area, OtherCompartment);
		}
		return input / PARAMETER(Area);
	)
	
	EQUATION(Model, ClInput,
		double input = 0.0;
		index_t ThisCompartment = CURRENT_INDEX(Compartment);
		for(index_t OtherCompartment = FIRST_INDEX(Compartment); OtherCompartment < ThisCompartment; ++OtherCompartment)
		{
			input += RESULT(ClOutput, OtherCompartment) * PARAMETER(FlowFraction, OtherCompartment, ThisCompartment) * PARAMETER(Area, OtherCompartment);
		}
		return input / PARAMETER(Area);
	)
	
	EQUATION(Model, NO3Input,
		double input = 0.0;
		index_t ThisCompartment = CURRENT_INDEX(Compartment);
		for(index_t OtherCompartment = FIRST_INDEX(Compartment); OtherCompartment < ThisCompartment; ++OtherCompartment)
		{
			input += RESULT(NO3Output, OtherCompartment) * PARAMETER(FlowFraction, OtherCompartment, ThisCompartment) * PARAMETER(Area, OtherCompartment);
		}
		return input / PARAMETER(Area);
	)
	
	EQUATION(Model, FInput,
		double input = 0.0;
		index_t ThisCompartment = CURRENT_INDEX(Compartment);
		for(index_t OtherCompartment = FIRST_INDEX(Compartment); OtherCompartment < ThisCompartment; ++OtherCompartment)
		{
			input += RESULT(FOutput, OtherCompartment) * PARAMETER(FlowFraction, OtherCompartment, ThisCompartment) * PARAMETER(Area, OtherCompartment);
		}
		return input / PARAMETER(Area);
	)
	
	EQUATION(Model, TotalCa,
		return LAST_RESULT(TotalCa) + RESULT(CaExternalFlux) - RESULT(CaOutput) + RESULT(CaInput);
	)
	
	EQUATION(Model, TotalMg,
		return LAST_RESULT(TotalMg) + RESULT(MgExternalFlux) - RESULT(MgOutput) + RESULT(MgInput);
	)
	
	EQUATION(Model, TotalNa,
		return LAST_RESULT(TotalNa) + RESULT(NaExternalFlux) - RESULT(NaOutput) + RESULT(NaInput);
	)
	
	EQUATION(Model, TotalK,
		return LAST_RESULT(TotalK)  + RESULT(KExternalFlux)  - RESULT(KOutput) + RESULT(KInput);
	)
	
	EQUATION(Model, TotalNH4,
		return LAST_RESULT(TotalNH) + RESULT(NH4ExternalFlux) - RESULT(NH4Output) + RESULT(NH4Input);
	)
	
	EQUATION(Model, TotalSO4,
		return LAST_RESULT(TotalSO4) + RESULT(SO4ExternalFlux) - RESULT(SO4Output) + RESULT(SO4Input);
	)
	
	EQUATION(Model, TotalCl,
		return LAST_RESULT(TotalCl) + RESULT(ClExternalFlux) - RESULT(ClOutput) + RESULT(ClInput);
	)
	
	EQUATION(Model, TotalNO3,
		return LAST_RESULT(TotalNO3) + RESULT(NO3ExternalFlux) - RESULT(NO3Output) + RESULT(NO3Input);
	)
	
	EQUATION(Model, TotalF,
		return LAST_RESULT(TotalF) + RESULT(FExternalFlux) - RESULT(FOutput) + RESULT(FInput);
	)
	
	EQUATION(Model, ConcH,
		magic_input Input;
		magic_param Param;
		magic_output Result;
		
		Input.total_Ca    = LAST_RESULT(TotalCa);
		Input.total_Mg    = LAST_RESULT(TotalMg);
		Input.total_Na    = LAST_RESULT(TotalNa);
		Input.total_K     = LAST_RESULT(TotalK);
		Input.total_NH4   = LAST_RESULT(TotalNH4);
		Input.total_SO4   = LAST_RESULT(TotalSO4);
		Input.total_Cl    = LAST_RESULT(TotalCl);
		Input.total_NO3   = LAST_RESULT(TotalNO3);
		Input.total_F     = LAST_RESULT(TotalF);
		
		Param.Depth       = PARAMETER(Depth);
		Param.Temperature = RESULT(Temperature);
		Param.PartialPressureCO2 = RESULT(PartialPressureCO2);
		Param.conc_DOC    = RESULT(DOCConcentration);
		
		Param.Log10AlOH3EquilibriumConst = PARAMETER(Log10AlOH3EquilibriumConst);
		Param.HAlOH3Exponent             = PARAMETER(HAlOH3Exponent);
		Param.pK1DOC                     = PARAMETER(PK1DOC);
		Param.pK2DOC                     = PARAMETER(PK2DOC);
		Param.pK3DOC                     = PARAMETER(PK3DOC);
		Param.pK1AlDOC                   = PARAMETER(PK1AlDOC);
		Param.pK2AlDOC                   = PARAMETER(PK2AlDOC);
		
		Param.Porosity                   = PARAMETER(Porosity);
		Param.BulkDensity                = PARAMETER(BulkDensity);
		Param.CationExchangeCapacity     = PARAMETER(CationExchangeCapacity);
		Param.SO4HalfSat                 = PARAMETER(SO4HalfSat);
		Param.SO4MaxCap                  = PARAMETER(SO4MaxCap);
		
		Param.Log10CaAlSelectCoeff       = PARAMETER(Log10CaAlSelectCoeff);
		Param.Log10MgAlSelectCoeff       = PARAMETER(Log10MgAlSelectCoeff);
		Param.Log10NaAlSelectCoeff       = PARAMETER(Log10NaAlSelectCoeff);
		Param.Log10KAlSelectCoeff        = PARAMETER(Log10KAlSelectCoeff);
		
		bool   issoil     = PARAMETER(IsSoil);
		double conv       = PARAMETER(ConvergenceCriterion);
		double H_estimate = LAST_RESULT(ConcH);
		double ionic      = LAST_RESULT(IonicStrength);
		
		if(RunState__->Running)   // Safeguard so that we don't crash on initialisation run.
		{
			MagicCore(Input, Param, Result, issoil, conv, H_estimate, ionic);
		}
		
		
		SET_RESULT(ConcCa,            Result.conc_Ca);
		SET_RESULT(ConcMg,            Result.conc_Mg);
		SET_RESULT(ConcNa,            Result.conc_Na);
		SET_RESULT(ConcK,             Result.conc_K);
		SET_RESULT(ConcNH4,           Result.conc_NH4);
		SET_RESULT(ConcSO4,           Result.conc_SO4);
		SET_RESULT(ConcCl,            Result.conc_Cl);
		SET_RESULT(ConcNO3,           Result.conc_NO3);
		SET_RESULT(ConcF,             Result.conc_F);
		
		SET_RESULT(ConcAllSO4,        Result.all_SO4);
		SET_RESULT(ConcAllF,          Result.all_F);
		
		SET_RESULT(IonicStrength,     Result.IonicStrength);
		
		SET_RESULT(PH,                Result.pH);
		SET_RESULT(SumBaseCationConc, Result.SumBaseCationConc);
		SET_RESULT(SumAcidAnionConc,  Result.SumAcidAnionConc);
		SET_RESULT(ChargeBalanceAlk,  Result.ChargeBalanceAlk);
		SET_RESULT(WeakAcidAlk,       Result.WeakAcidAlk);
		
		SET_RESULT(ExchangeableCa,    Result.ExchangeableCa);
		SET_RESULT(ExchangeableMg,    Result.ExchangeableMg);
		SET_RESULT(ExchangeableNa,    Result.ExchangeableNa);
		SET_RESULT(ExchangeableK,     Result.ExchangeableK);
		SET_RESULT(ExchangeableSO4,   Result.ExchangeableSO4);
		SET_RESULT(BaseSaturationSoil,Result.BaseSaturationSoil);
		
		SET_RESULT(ConcHCO3,          Result.conc_HCO3);
		SET_RESULT(ConcCO3,           Result.conc_CO3);
		SET_RESULT(ConcAl,            Result.conc_Al);
		SET_RESULT(ConcAllAl,         Result.all_Al);
		SET_RESULT(ConcOrgAl,         Result.org_Al);
		
		SET_RESULT(ConcH2AM,          Result.conc_H2AM);
		SET_RESULT(ConcHA2M,          Result.conc_HA2M);
		SET_RESULT(ConcA3M,           Result.conc_A3M);
		
		SET_RESULT(SumPositive,       Result.SumPositive);
		SET_RESULT(SumNegative,       Result.SumNegative);

		SET_RESULT(ConcAllDOC,        Result.all_DOC);
		SET_RESULT(ConcAllDIC,        Result.all_DIC);
		SET_RESULT(CaAlRatio,         Result.CaAlRatio);
		
		return Result.conc_H;
	)
	
	//TODO: We should maybe have a way to tell the model that this is not computed "Mobius style" so that we don't need to provide the dummy equations.
	//      Alternatively, make a "multi-equation" that can just take the result of the struct and write it directly into memory in the right place (would be way more efficient...)
	
	EQUATION(Model, ConcCa,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcCa, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ConcMg,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcMg, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ConcNa,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcNa, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ConcK,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcK, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ConcNH4,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcNH4, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ConcSO4,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcSO4, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ConcAllSO4,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcAllSO4, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ConcCl,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcCl, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ConcNO3,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcNO3, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ConcF,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcF, CURRENT_INDEX(Compartment));
	)
	EQUATION(Model, ConcAllF,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcAllF, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, PH,
		// Dummy, this is set in the ConcH equation
		return RESULT(PH, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, SumBaseCationConc,
		// Dummy, this is set in the ConcH equation
		return RESULT(SumBaseCationConc, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, SumAcidAnionConc,
		// Dummy, this is set in the ConcH equation
		return RESULT(SumAcidAnionConc, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ChargeBalanceAlk,
		// Dummy, this is set in the ConcH equation
		return RESULT(ChargeBalanceAlk, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, WeakAcidAlk,
		// Dummy, this is set in the ConcH equation
		return RESULT(WeakAcidAlk, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ExchangeableCa,
		// Dummy, this is set in the ConcH equation
		return RESULT(ExchangeableCa, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ExchangeableMg,
		// Dummy, this is set in the ConcH equation
		return RESULT(ExchangeableMg, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ExchangeableNa,
		// Dummy, this is set in the ConcH equation
		return RESULT(ExchangeableNa, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ExchangeableK,
		// Dummy, this is set in the ConcH equation
		return RESULT(ExchangeableK, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ExchangeableSO4,
		// Dummy, this is set in the ConcH equation
		return RESULT(ExchangeableSO4, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, BaseSaturationSoil,
		// Dummy, this is set in the ConcH equation
		return RESULT(BaseSaturationSoil, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ConcHCO3,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcHCO3, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ConcCO3,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcCO3, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ConcAl,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcAl, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ConcAllAl,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcAllAl, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ConcOrgAl,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcOrgAl, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ConcH2AM,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcH2AM, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ConcHA2M,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcHA2M, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ConcA3M,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcA3M, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, SumPositive,
		// Dummy, this is set in the ConcH equation
		return RESULT(SumPositive, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, SumNegative,
		// Dummy, this is set in the ConcH equation
		return RESULT(SumNegative, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ConcAllDOC,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcAllDOC, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, ConcAllDIC,
		// Dummy, this is set in the ConcH equation
		return RESULT(ConcAllDIC, CURRENT_INDEX(Compartment));
	)
	
	EQUATION(Model, CaAlRatio,
		// Dummy, this is set in the ConcH equation
		return RESULT(CaAlRatio, CURRENT_INDEX(Compartment));
	)
	

	
	EQUATION(Model, IonicStrength,
		// Dummy, this is set in the ConcH equation
		return RESULT(IonicStrength, CURRENT_INDEX(Compartment));
	)
	
	
	EndModule(Model);
}