

/*
	This is a Mobius wrapper for the MAGIC core.
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
	auto Log10AlOH3EquilibriumConst= RegisterParameterDouble(Model, GeneralParams, "(log10) Al(OH)3 dissociation equilibrium constant", Dimensionless, 0.0); //TODO
	auto HAlOH3Exponent            = RegisterParameterDouble(Model, GeneralParams, "Al(OH)3 dissociation equation exponent", Dimensionless, 3.0); //TODO
	
	
	auto Compartment = RegisterIndexSet(Model, "Compartment");
	auto CompartmentParams = RegisterParameterGroup(Model, "Compartment parameters");
	
	auto IsSoil                    = RegisterParameterBool(Model, CompartmentParams, "This is a soil compartment", true);
	auto Depth                     = RegisterParameterDouble(Model, CompartmentParams, "Depth", M, 1.0, 0.0, 100.0);
	auto Porosity                  = RegisterParameterDouble(Model, CompartmentParams, "Porosity", Dimensionless, 0.5, 0.0, 1.0);
	auto BulkDensity               = RegisterParameterDouble(Model, CompartmentParams, "Bulk density", KgPerM3, 0.25, 0.0, 2.0);
	auto CationExchangeCapacity    = RegisterParameterDouble(Model, CompartmentParams, "Cation exchange capacity", MeqPerKg, 9.0, 0.0, 50.0);
	auto SO4HalfSat                = RegisterParameterDouble(Model, CompartmentParams, "Soil sulfate adsorption capacity, half saturation", MeqPerKg, 0.0); //TODO
	auto SO4MaxCap                 = RegisterParameterDouble(Model, CompartmentParams, "Soil sulfate adsorption max capacity", MeqPerKg, 0.0); //TODO
	
	
	auto ConcCa        = RegisterEquation(Model, "Ca(2+) ion concentration", MeqPerM3);
	auto ConcMg        = RegisterEquation(Model, "Mg(2+) ion concentration", MeqPerM3);
	auto ConcNa        = RegisterEquation(Model, "Na(+) ion concentration", MeqPerM3);
	auto ConcK         = RegisterEquation(Model, "K(+) ion concentration", MeqPerM3);
	
	auto ConcH         = RegisterEquation(Model, "H(+) ion concentration", MeqPerM3);
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
	auto WaterFlow          = GetEquationHandle(Model, "Water flow");
	
	auto CaExternalFlux     = GetEquationHandle(Model, "Sum of Ca fluxes not related to water flow");
	auto MgExternalFlux     = GetEquationHandle(Model, "Sum of Mg fluxes not related to water flow");
	auto NaExternalFlux     = GetEquationHandle(Model, "Sum of Na fluxes not related to water flow");
	auto KExternalFlux      = GetEquationHandle(Model, "Sum of K fluxes not related to water flow");
	
	
	
	//TODO: Inputs from other compartments for all the total mass equations
	//TODO: Time step size independence? But that can be done via units too.
	EQUATION(Model, TotalCa,
		return LAST_RESULT(TotalCa) + RESULT(CaExternalFlux) - RESULT(WaterFlow)*2.0*RESULT(ConcCa);
	)
	
	EQUATION(Model, TotalMg,
		return LAST_RESULT(TotalMg) + RESULT(MgExternalFlux) - RESULT(WaterFlow)*2.0*RESULT(ConcMg);
	)
	
	EQUATION(Model, TotalNa,
		return LAST_RESULT(TotalNa) + RESULT(NaExternalFlux) - RESULT(WaterFlow)*RESULT(ConcNa);
	)
	
	EQUATION(Model, TotalK,
		return LAST_RESULT(TotalK)  + RESULT(KExternalFlux)  - RESULT(WaterFlow)*RESULT(ConcK);
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
		//Param.pK1DOC                     = ;
		//Param.pK2DOC                     = ;
		//Param.pK3DOC                     = ;
		//Param.pK1AlDOC                   = ;
		//Param.pK2AlDOC                   = ;
		
		Param.Porosity    = PARAMETER(Porosity);
		Param.BulkDensity = PARAMETER(BulkDensity);
		Param.CationExchangeCapacity = PARAMETER(CationExchangeCapacity);
		Param.SO4HalfSat  = PARAMETER(SO4HalfSat);
		Param.SO4MaxCap   = PARAMETER(SO4MaxCap);
		
		//Param.Log10CaAlSelectCoeff         = ;
		//Param.Log10MgAlSelectCoeff         = ;
		//Param.Log10NaAlSelectCoeff         = ;
		//Param.Log10KAlSelectCoeff          = ;
		
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
		SET_RESULT(IonicStrength, Result.IonicStrength);
		
		/*
		double conc_NH4;           // Ammonium ion concentration       (meq/m3)
		double conc_SO4;           // Sulfate ion concentration        (meq/m3)
		double conc_Cl;            // Nitrate ion concentration        (meq/m3)
		double conc_NO3;           // Nitrate ion concentration        (meq/m3)
		double conc_F;             // Fluoride ion concentration       (meq/m3)
		
		double all_SO4;            // Total Sulfate in solution (ionic + Al complexes)  (meq/m3)
		double all_F;              // Total Fluoride in solution (ionic + Al complexes) (meq/m3)
		
		double pH;                 // Solution pH (log10)
		double SumBaseCationConc;  // Sum of base cation concentrations (Ca + Mg + Na + K)  (meq/m3)
		double SumAcidAnionConc;   // Sum of acid anion concentrations  (SO4 + Cl + NO3 + F)
		double ChargeBalanceAlk;   // Charge balance alkalinity         (SumBaseCationConc + NH4 - SumAcidAnionConc)  (= Acid neutralizing capacity)
		double WeakAcidAlk;        // Weak acid alkalinity (HCO3 + 2*CO3 + OH - H - Alxx)  (limnological definition)
		
		double exchangable_Ca;     // Exchangable Calcium on soil as % of cation exchange capacity (meq - ECa/meq - CEC)    (%)
		double exchangable_Mg;     // Exchangable Magnesium on soil as % of cation exchange capacity (meq - EMg/meq - CEC)  (%)
		double exchangable_Na;     // Exchangable Sodium on soil as % of cation exchange capacity (meq - ENa/meq - CEC)     (%)
		double exchangable_K;      // Exchangable Potassium on soil as % of cation exchange capacity (meq - EK/meq - CEC)   (%)
		double exchangable_SO4;    // Exchangable Sulfate on soil as % of cation exchange capacity (meq - ESO4/meq - MaxCap)(%)
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
	
	//TODO: We should maybe have a way to tell the model that we don't need to provide the dummy equations.
	
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
	
	EQUATION(Model, IonicStrength,
		// Dummy, this is set in the ConcH equation
		return RESULT(IonicStrength, CURRENT_INDEX(Compartment));
	)
	
	
	EndModule(Model);
}