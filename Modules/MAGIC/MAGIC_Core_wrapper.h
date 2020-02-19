

/*
	This is a Mobius wrapper for the MAGIC core.
	
	This has to be used together with another "driver" module that determines things like temperature and external fluxes (nitrification, weathering, deposition etc. etc.)
*/



#include "MAGIC_Core.h"

static void
AddMagicCoreModel(mobius_model *Model)
{
	BeginModule(Model, "MAGIC Core", "0.1");
	
	
	auto Dimensionless = RegisterUnit(Model);
	auto M             = RegisterUnit(Model, "m");
	auto KgPerM3       = RegisterUnit(Model, "kg/m3");
	auto MMolPerM2     = RegisterUnit(Model, "mmol/m2");
	auto MeqPerKg      = RegisterUnit(Model, "meq/kg");
	auto MeqPerM3      = RegisterUnit(Model, "meq/m3");
	auto Log10MolPerL  = RegisterUnit(Model, "log10(mol/l)")
	
	
	auto GeneralParams     = RegisterParameterGroup(Model, "General parameters");
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
	auto Depth                     = RegisterParameterDouble(Model, CompartmentParams, "Depth", M, 1.0, 0.0, 100.0);
	auto Porosity                  = RegisterParameterDouble(Model, CompartmentParams, "Porosity", Dimensionless, 0.5, 0.0, 1.0);
	auto BulkDensity               = RegisterParameterDouble(Model, CompartmentParams, "Bulk density", KgPerM3, 0.25, 0.0, 2.0);
	auto CationExchangeCapacity    = RegisterParameterDouble(Model, CompartmentParams, "Cation exchange capacity", MeqPerKg, 9.0, 0.0, 50.0);
	auto SO4HalfSat                = RegisterParameterDouble(Model, CompartmentParams, "Soil sulfate adsorption capacity, half saturation", MeqPerKg, 0.0); //TODO
	auto SO4MaxCap                 = RegisterParameterDouble(Model, CompartmentParams, "Soil sulfate adsorption max capacity", MeqPerKg, 0.0); //TODO
	
	
	auto ConcCa        = RegisterEquation(Model, "Ca(2+) ionic concentration", MeqPerM3);
	auto ConcMg        = RegisterEquation(Model, "Mg(2+) ionic concentration", MeqPerM3);
	auto ConcNa        = RegisterEquation(Model, "Na(+) ionic concentration", MeqPerM3);
	auto ConcK         = RegisterEquation(Model, "K(+) ionic concentration", MeqPerM3);
	auto ConcNH4       = RegisterEquation(Model, "NH4(+) ionic concentration", MeqPerM3);
	auto ConcSO4       = RegisterEquation(Model, "SO4(2-) ionic concentration", MeqPerM3);
	auto ConcCl        = RegisterEquation(Model, "Cl(-) ionic concentration", MeqPerM3);
	auto ConcNO3       = RegisterEquation(Model, "NO3(-) ionic concentration", MeqPerM3);
	auto ConcF         = RegisterEquation(Model, "F(-) ionic concentration", MeqPerM3);
	
	auto ConcAllSO4    = RegisterEquation(Model, "Total Sulfate in solution (ionic + Al complexes)", MeqPerM3);
	auto ConcAllF      = RegisterEquation(Model, "Total Fluoride in solution (ionic + Al complexes)", MeqPerM3);
	
	auto ConcH         = RegisterEquation(Model, "H(+) ionic concentration", MeqPerM3);
	SetInitialValue(Model, ConcH, 1.0);
	auto IonicStrength = RegisterEquation(Model, "Ionic strength", Dimensionless);
	SetInitialValue(Model, IonicStrength, 0.0);
	
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
	
	// Equations that have to be defined by an outside "driver":
	auto Temperature        = GetEquationHandle(Model, "Temperature");
	auto PartialPressureCO2 = GetEquationHandle(Model, "CO2 partial pressure");
	auto DOCConcentration   = GetEquationHandle(Model, "DOC concentration");
	auto Discharge          = GetEquationHandle(Model, "Discharge");
	
	auto CaExternalFlux     = GetEquationHandle(Model, "Sum of Ca fluxes not related to discharge");
	auto MgExternalFlux     = GetEquationHandle(Model, "Sum of Mg fluxes not related to discharge");
	auto NaExternalFlux     = GetEquationHandle(Model, "Sum of Na fluxes not related to discharge");
	auto KExternalFlux      = GetEquationHandle(Model, "Sum of K fluxes not related to discharge");
	auto NH4ExternalFlux    = GetEquationHandle(Model, "Sum of NH4 fluxes not related to discharge");
	auto SO4ExternalFlux    = GetEquationHandle(Model, "Sum of SO4 fluxes not related to discharge");
	auto ClExternalFlux     = GetEquationHandle(Model, "Sum of Cl fluxes not related to discharge");
	auto NO3ExternalFlux    = GetEquationHandle(Model, "Sum of NO3 fluxes not related to discharge");
	auto FExternalFlux      = GetEquationHandle(Model, "Sum of F fluxes not related to discharge");
	
	//TODO: Inputs from other compartments for all the total mass equations
	//TODO: Time step size independence? But that can be done via units too.
	EQUATION(Model, TotalCa,
		return LAST_RESULT(TotalCa) + RESULT(CaExternalFlux) - RESULT(Discharge)*2.0*RESULT(ConcCa);
	)
	
	EQUATION(Model, TotalMg,
		return LAST_RESULT(TotalMg) + RESULT(MgExternalFlux) - RESULT(Discharge)*2.0*RESULT(ConcMg);
	)
	
	EQUATION(Model, TotalNa,
		return LAST_RESULT(TotalNa) + RESULT(NaExternalFlux) - RESULT(Discharge)*RESULT(ConcNa);
	)
	
	EQUATION(Model, TotalK,
		return LAST_RESULT(TotalK)  + RESULT(KExternalFlux)  - RESULT(Discharge)*RESULT(ConcK);
	)
	
	EQUATION(Model, TotalNH4,
		return LAST_RESULT(TotalNH) + RESULT(NH4ExternalFlux) - RESULT(Discharge)*RESULT(ConcNH4);
	)
	
	EQUATION(Model, TotalSO4,
		return LAST_RESULT(TotalSO4) + RESULT(SO4ExternalFlux) - RESULT(Discharge)*2.0*RESULT(ConcAllSO4);
	)
	
	EQUATION(Model, TotalCl,
		return LAST_RESULT(TotalCl) + RESULT(ClExternalFlux) - RESULT(Discharge)*RESULT(ConcCl);
	)
	
	EQUATION(Model, TotalNO3,
		return LAST_RESULT(TotalNO3) + RESULT(NO3ExternalFlux) - RESULT(Discharge)*RESULT(ConcNO3);
	)
	
	EQUATION(Model, TotalF,
		return LAST_RESULT(TotalF) + RESULT(FExternalFlux) - RESULT(Discharge)*Result(ConcAllF);
	)
	
	EQUATION(Model, ConcH,
		magic_input Input;
		magic_param Param = {};
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
		
		
		SET_RESULT(ConcCa,        Result.conc_Ca);
		SET_RESULT(ConcMg,        Result.conc_Mg);
		SET_RESULT(ConcNa,        Result.conc_Na);
		SET_RESULT(ConcK,         Result.conc_K);
		SET_RESULT(ConcNH4,       Result.conc_NH4);
		SET_RESULT(ConcSO4,       Result.conc_SO4);
		SET_RESULT(ConcCl,        Result.conc_Cl);
		SET_RESULT(ConcNO3,       Result.conc_NO3);
		SET_RESULT(ConcF,         Result.conc_F);
		
		SET_RESULT(ConcAllSO4,    Result.all_SO4);
		SET_RESULT(ConcAllF,      Result.all_F);
		
		SET_RESULT(IonicStrength, Result.IonicStrength);
		
		/*      //TODO: Export the following values too
		double pH;                 // Solution pH (log10)
		double SumBaseCationConc;  // Sum of base cation concentrations (Ca + Mg + Na + K)  (meq/m3)
		double SumAcidAnionConc;   // Sum of acid anion concentrations  (SO4 + Cl + NO3 + F)
		double ChargeBalanceAlk;   // Charge balance alkalinity         (SumBaseCationConc + NH4 - SumAcidAnionConc)  (= Acid neutralizing capacity)
		double WeakAcidAlk;        // Weak acid alkalinity (HCO3 + 2*CO3 + OH - H - Alxx)  (limnological definition)
		
		double exchangeable_Ca;    // Exchangeable Calcium on soil as % of cation exchange capacity (meq - ECa/meq - CEC)    (%)
		double exchangeable_Mg;    // Exchangeable Magnesium on soil as % of cation exchange capacity (meq - EMg/meq - CEC)  (%)
		double exchangeable_Na;    // Exchangeable Sodium on soil as % of cation exchange capacity (meq - ENa/meq - CEC)     (%)
		double exchangeable_K;     // Exchangeable Potassium on soil as % of cation exchange capacity (meq - EK/meq - CEC)   (%)
		double exchangeable_SO4;   // Exchangeable Sulfate on soil as % of cation exchange capacity (meq - ESO4/meq - MaxCap)(%)
		double BaseSaturationSoil; // Base saturation of soil (ECa + EMg + ENa + EK)
		
		double conc_HCO3;          // Bicarbonate ion concentration    (meq/m3)
		double conc_CO3;           // Carbonate ion concentration      (meq/m3)
		
		double conc_Al;            // [Al3+] Trivalent aluminum ion concentration                    (meq/m3)
		double all_Al;             // Total aluminum in solution (ionic + SO4-F-DOC-complexes)       (meq/m3)
		double org_Al;             // Aluminum in solution as organic complexes (AlA, Al(HA)+)       (meq/m3)
		
		double conc_H2AM;          // [H2A-] Monovalent ion concentration, triprotic organic acid   (meq/m3)
		double conc_HA2M;          // [HA2-] Divalent ion concentration, triprotic organic acid     (meq/m3)
		double conc_A3M;           // [A3-]  Trivalent ion concentration, triprotic organic acid    (meq/m3)
		
		double SumPositive;        // Sum of all positive charges in solution (cations)             (meq/m3)
		double SumNegative;        // Sum of all negative charges in solution (anions)              (meq/m3)
		
		double all_DIC;            // Total anionic charge from inorganic carbon in solution (HC3O, CO3)             (meq/m3)
		double all_DOC;            // Total anionic charge from DOC (triprotic acid) in solution (H2AM, HA2M, A3M)   (meq/m3)
		double CaAlRatio;          // Molar ratio of Calcium ion to aqueous Aluminum                                 (mol/mol)
		*/
		
		return Result.conc_H;
	)
	
	//TODO: We should maybe have a way to tell the model that this is not computed Mobius style so that we don't need to provide the dummy equations.
	
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
	
	
	EQUATION(Model, IonicStrength,
		// Dummy, this is set in the ConcH equation
		return RESULT(IonicStrength, CURRENT_INDEX(Compartment));
	)
	
	
	EndModule(Model);
}