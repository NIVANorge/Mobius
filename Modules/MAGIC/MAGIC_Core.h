

/*
This is a translation from FORTRAN of a program written by Bernard J. Cosby .

The model has been published as
B.J Cosby, R. C. Ferrier, A. Jenkins and R. F. Wright, 2001, Modelling the effects of acid deposition: refinements, adjustments and inclusion of nitrogen dynamics in the MAGIC model. Hydrol. Earth Syst. Sci, 5(3), 499-517

This adaptation is a work in progress
*/



struct magic_input
{
	double total_Ca;   // Total mass of Calcium   (mmol/m2)
	double total_Mg;   // Total mass of Magnesium (mmol/m2)
	double total_Na;   // Total mass of Sodium    (mmol/m2)
	double total_K;    // Total mass of Potassium (mmol/m2)
	double total_NH4;  // Total mass of Ammonium  (mmol/m2)
	double total_SO4;  // Total mass of Sulfate   (mmol/m2)
	double total_Cl;   // Total mass of Chloride  (mmol/m2)
	double total_NO3;  // Total mass of Nitrate   (mmol/m2)
	double total_F;    // Total mass of Fluoride  (mmol/m2)
};

struct magic_output
{
	double conc_Ca;            // Calcium ion concentration        (mmol/m3)
	double conc_Mg;            // Magnesium ion concentration      (mmol/m3)
	double conc_Na;            // Sodium ion concentration         (mmol/m3)
	double conc_K;             // Potassium ion concentration      (mmol/m3)
	double conc_NH4;           // Ammonium ion concentration       (mmol/m3)
	double conc_SO4;           // Sulfate ion concentration        (mmol/m3)
	double conc_Cl;            // Nitrate ion concentration        (mmol/m3)
	double conc_NO3;           // Nitrate ion concentration        (mmol/m3)
	double conc_F;             // Fluoride ion concentration       (mmol/m3)
	
	double all_SO4;            // Total Sulfate in solution (ionic + Al complexes)  (mmol/m3)
	double all_F;              // Total Fluoride in solution (ionic + Al complexes) (mmol/m3)
	
	double pH;                 // Solution pH (log10)
	double SumBaseCationConc;  // Sum of base cation concentrations (Ca + Mg + Na + K)  (meq/m3)
	double SumAcidAnionConc;   // Sum of acid anion concentrations  (SO4 + Cl + NO3 + F) (meq/m3)
	double ChargeBalanceAlk;   // Charge balance alkalinity         (SumBaseCationConc + NH4 - SumAcidAnionConc)  (= Acid neutralizing capacity)
	double WeakAcidAlk;        // Weak acid alkalinity (HCO3 + 2*CO3 + OH - H - Alxx)  (limnological definition)
	
	double exchangeable_Ca;    // Exchangeable Calcium on soil as % of cation exchange capacity (meq - ECa/meq - CEC)    (%)
	double exchangeable_Mg;    // Exchangeable Magnesium on soil as % of cation exchange capacity (meq - EMg/meq - CEC)  (%)
	double exchangeable_Na;    // Exchangeable Sodium on soil as % of cation exchange capacity (meq - ENa/meq - CEC)     (%)
	double exchangeable_K;     // Exchangeable Potassium on soil as % of cation exchange capacity (meq - EK/meq - CEC)   (%)
	double exchangeable_SO4;   // Exchangeable Sulfate on soil as % of cation exchange capacity (meq - ESO4/meq - MaxCap)(%)
	double BaseSaturationSoil; // Base saturation of soil (ECa + EMg + ENa + EK)
	
	double conc_H;             // Hydrogen ion concentration       (mmol/m3)
	double conc_HCO3;          // Bicarbonate ion concentration    ((mmol/m3)
	double conc_CO3;           // Carbonate ion concentration      (mmol/m3)
	
	double conc_Al;            // [Al3+] Trivalent aluminum ion concentration                    (mmol/m3)
	double all_Al;             // Total aluminum in solution (ionic + SO4-F-DOC-complexes)       (mmol/m3)
	double org_Al;             // Aluminum in solution as organic complexes (AlA, Al(HA)+)       (mmol/m3)
	
	double conc_H2AM;          // [H2A-] Monovalent ion concentration, triprotic organic acid   (mmol/m3)
	double conc_HA2M;          // [HA2-] Divalent ion concentration, triprotic organic acid     (mmol/m3)
	double conc_A3M;           // [A3-]  Trivalent ion concentration, triprotic organic acid    (mmol/m3)
	
	double SumPositive;        // Sum of all positive charges in solution (cations)             (meq/m3)
	double SumNegative;        // Sum of all negative charges in solution (anions)              (meq/m3)
	
	double all_DIC;            // Total anionic charge from inorganic carbon in solution (HC3O, CO3)             (meq/m3)
	double all_DOC;            // Total anionic charge from DOC (triprotic acid) in solution (H2AM, HA2M, A3M)   (meq/m3)
	double CaAlRatio;          // Molar ratio of Calcium ion to aqueous Aluminum                                 (mol/mol)
	
	double IonicStrength;      // ionic strength of the aqueous solution (0 - 1)
	
};

struct magic_param
{
	double Depth;                     // Soil depth (m)
	
	double Temperature;               // Temperature (°C)
	double PartialPressureCO2;        // Partial pressure CO2 (%atm)
	double conc_DOC;                  // Dissolved organic carbon concentration (mmol/m3)
	
	double Log10AlOH3EquilibriumConst;// Equilibrium constant Al(OH)3 dissociation (log10)
	double HAlOH3Exponent;            // Exponent for H ion in Al(OH)3 dissociation equation (usually = 3)
	
	double pK1DOC;                    // pK1 DOC - equilibrium constant for triprotic organic acid, first     (-log10, pK)
	double pK2DOC;                    // pK2 DOC - equilibrium constant for triprotic organic acid, second    (-log10, pK)
	double pK3DOC;                    // pK3 DOC - equilibrium constant for triprotic organic acid, third     (-log10, pK)
	
	double pK1AlDOC;                  // pK Al(A) - equilibrium constant for DOC-Al complexes, first complex  [(Al3+)(A3-)]   (-log10, pK)
	double pK2AlDOC;                  // pK Al(HA)+ - equilibrium constant for DOC-Al complexes, second complex [(Al3+)(HA2-)+] (-log10, pK)
	
	double Porosity;                  // Porosity (frac) - (m3/m3)
	double BulkDensity;               // Soil bulk density (kg/m3)
	double CationExchangeCapacity;    // Cation exchange capacity (meq/kg)
	double SO4HalfSat;                // Soil sulfate adsorption capacity, half saturation constant (meq/kg)
	double SO4MaxCap;                 // Soil sulfate adsorption maximum capacity (meq/kg)
	
	double Log10CaAlSelectCoeff;      // Selectivity coefficient for Ca/Al exchange (log10)
	double Log10MgAlSelectCoeff;      // Selectivity coefficient for Mg/Al exchange (log10)
	double Log10NaAlSelectCoeff;      // Selectivity coefficient for Na/Al exchange (log10)
	double Log10KAlSelectCoeff;       // Selectivity coefficient for K/Al exchange (log10)
};

struct magic_coeff
{
	//TODO: Document these!
	
	double K_C[2];            // K_C[0]=[HCO3-][H+]/[CO2], K_CO2[1]=[CO3 2-][H+]/[HCO3-]
	double K_SO4[2];
	double K_Al[4];
	double K_F[6];
	double K_DOC[3];
	double K_AlDOC[2];
	double K_W;               // Water dissociation constant K_W = [H+][OH-]
	double HenrysLawConstant; // Henry's law constant for CO2
	double GibbsAlSolubility; // Gibbs constant for aluminum solubility ( [Al] = GibbsAlSolubility * [H]^HAlOH3Exponent )
	double K_AlCa;
	double K_AlMg;
	double K_AlNa;
	double K_AlK;
};




void
SetEquilibriumConstants(const magic_param &Param, magic_coeff &CoeffOut, bool IsSoil, double IonicStrength, double SoilCationExchange)
{
	// calculate equilibrium coefficients, correcting for ionic strength and temperature. If coefficients are being set for soil (SCEC > 0), calculate cation exchange coefficients also
	
	
	double R  = 4.5752; // Ideal gas constant (?)
	double T0 = 298.15; // Reference temperature
	
	//Additional data for computing equilibrium constants
	double StAlSO4[2] = {3.21, 5.11};
	double StAlOH[4]  = {-4.99, -10.13, -16.76, -23.00};
	double StAlF[6]   = {6.98, 12.60, 16.65, 19.03, 20.91, 20.86};
	double dAlSO4[2]  = {2150.0, 2840.0};
	double dHAlOH[4]  = {11900.0, 22000.0, 33000.0, 44060.0};
	double dAlF[6]    = {1060.0, 1980.0, 2160.0, 2200.0, 1840.0, 100.0};
	
	//Calculate terms in the equilibrium equations depending on temperature and ionic strength, converting umol/l to mol/l
	double G[4];
	G[0] = -6.0;
	G[1] = -0.5*IonicStrength - 6.0;
	G[2] = -2.0*IonicStrength - 6.0;
	G[3] = -4.5*IonicStrength - 6.0;
	
	double TempKelvin = 273.15 + Param.Temperature;
	
	// Calculate values of equilibrium constants
	
	// Calculate corrected equilibrium coefficients for dissociation of water (pH, pOH)
	CoeffOut.K_W = pow(10.0, -4470.99/TempKelvin + 6.0875 - 0.01706*TempKelvin - 2.0*G[1]);
	
	// Henry's law constant for CO2 solubility in water
	CoeffOut.HenrysLawConstant = pow(10.0, 2385.7/TempKelvin - 14.0184 + 0.015264*TempKelvin - G[0]);
	
	// Calculate corrected equilibrium coefficients for trivalent Al solubility (Gibbsite) 
	CoeffOut.GibbsAlSolubility = pow(10.0, Param.Log10AlOH3EquilibriumConst + Param.HAlOH3Exponent*G[1] - G[3]);
	
	// Calculate corrected equilibrium coefficients for dissociation of carbonic acid (carbonate, bicarbonate)
	CoeffOut.K_C[0] = pow(10.0, 14.8435 - 3404.7/TempKelvin - 0.032786*TempKelvin + G[0] - 2.0*G[1]);
	CoeffOut.K_C[1] = pow(10.0, 6.4980 - 2902.39/TempKelvin - 0.02379*TempKelvin + G[1] - G[1] - G[2]);
	
	// Calculate corrected equilibrium coefficients for dissociation of trivalent organic acids
	CoeffOut.K_DOC[0] = pow(10.0, -Param.pK1DOC + G[0] - 2.0*G[1]);
	CoeffOut.K_DOC[1] = pow(10.0, -Param.pK2DOC + G[1] - G[1] - G[2]);
	CoeffOut.K_DOC[2] = pow(10.0, -Param.pK3DOC + G[2] - G[1] - G[3]);
	
	// Calculate corrected equilibrium coefficients for complexation of AL with organic acid anions
	CoeffOut.K_AlDOC[0] = pow(10.0, -Param.pK1AlDOC   + G[3] + G[3] - G[0]);
	CoeffOut.K_AlDOC[1] = pow(10.0, -Param.pK2AlDOC   + G[3] + G[3] + G[1] - G[1]);
	
	// Set term for departure of temperature from standard Temp/Press (STP) in thermodynamic expression PV=nRT 
	double Tmtr = (1.0/T0 - 1.0/TempKelvin) / R;
	
	// Correct equilibrium coeeficients for Al-SO4 complexation using standard coefficient & enthalpy of reactions
	for(int Idx = 0; Idx < 2; ++Idx)
	{
		CoeffOut.K_SO4[Idx] = pow(10.0, StAlSO4[Idx] + dAlSO4[Idx]*Tmtr + G[3] + (double)(Idx+1)*G[2] - G[1]);
	}
	
	// Correct equilibrium coeeficients for Al-hydroxide dissociation using standard coefficient & enthalpy of reactions
	for(int Idx = 0; Idx < 4; ++Idx)
	{
		int J = abs(Idx - 2);
		CoeffOut.K_Al[Idx] = pow(10.0, StAlOH[Idx] + dHAlOH[Idx]*Tmtr + G[3] - G[J] - (double)(Idx+1)*G[1]);
	}
	
	// Correct equilibrium coeeficients for Al-F complexation using standard coefficient & enthalpy of reactions
	for(int Idx = 0; Idx < 6; ++Idx)
	{
		int J = abs(Idx - 2);
		CoeffOut.K_F[Idx] = pow(10.0, StAlF[Idx] + dAlF[Idx]*Tmtr + G[3] - G[J] + (double)(Idx+1)*G[1]);
	}
	
	// Calculate GainesThomas cation exchange coeeficients - from selectivity coefficients (correct mmol to meq)
	if(IsSoil)
	{
		double Log10SCEC = log10(SoilCationExchange);
		CoeffOut.K_AlCa = Log10SCEC - (Param.Log10CaAlSelectCoeff + 6.0)/3.0;
		CoeffOut.K_AlMg = Log10SCEC - (Param.Log10MgAlSelectCoeff + 6.0)/3.0;
		CoeffOut.K_AlNa = Log10SCEC - (Param.Log10NaAlSelectCoeff + 12.0)/3.0;
		CoeffOut.K_AlK  = Log10SCEC - (Param.Log10KAlSelectCoeff  + 12.0)/3.0;
	}
	else
	{
		CoeffOut.K_AlCa = 0.0;
		CoeffOut.K_AlMg = 0.0;
		CoeffOut.K_AlNa = 0.0;
		CoeffOut.K_AlK  = 0.0;
	}
}

inline double
SolveQuadratic(double A, double B, double C)
{
	//Solve quadratic equation A*X^2 + B*X + C = 0, only caring about the (largest) positive result.
	
	return (-B + sqrt(B*B - 4.0*A*C))/(2.0*A);
}

inline double
Sign(double A, double B)
{
	// Return the value of A with the sign of B
	return B > 0.0 ? abs(A) : -abs(A);
	
	/*
	//TODO: More efficient implementation of this?
	//The following should work. Not that the speed of this is going to matter anyway, though, so....
	
	double C;

	u64* AA = (u64*)&A;
	u64* BB = (u64*)&B;
	u64* CC = (u64*)&C;

	u64 Msk = ((u64)1) << 63;
	*CC = (*BB & Msk) | (*AA & (Msk-1));
	
	return C;
	*/
}

double
SolveFreeFluoride(const magic_coeff &Coeff, double all_F, double conc_Al)
{
	// Solve for free Fluoride ion concen (mmol/m3) from aqueous Fluoride concen (mmol/m3) and trivalent Al ion concen (mmol/m3)
	
	// If total F concen is low, solutions are unstable,  F is set to zero (mmol/m3).
	if(all_F < .0000001) return 0.0;
	
	// Iteratively solve the nested set of 6 simultaneous equilibrium equations for AL-F complexes
	double Term2 = all_F / (1.0 + conc_Al*Coeff.K_F[0]);
	while(true)
	{
		double Term1 = 0;
		for(int Idx = 5; Idx >= 0; --Idx)
		{
			Term1 = (Term1 + (double)(Idx+1)*Coeff.K_F[Idx])*Term2;
		}
		double Term3 = conc_Al*Term1 + Term2;
		double Term4 = all_F - Term3;
		if((abs(Term4) / all_F)*100.0 < 0.1) break;   // Stop when resolution among species < 0.1%
		double Df = Term4*Term2/Term3;
		Term2 += Df;
	}
	return Term2;
}

double
SolveAqueousSO4(const magic_coeff &Coeff, double total_SO4, double conc_Al, double WaterVolume, double SO4AdsorptionCap, double SO4HalfSat)
{
	// Solve for aqueous Sulfate concen (free + Al-complexed) (mmol/m3) - from total Sulfate amount (meg/m2), trivalent AL ion concen (mmol/m3) and sulfate adsorption parameters
	
	// If input variable is not positive, no solution. All SO4 values are set to zero (mmol/m3)
	if(total_SO4 <= 0.0) return 0.0;
	
	if(SO4AdsorptionCap <= 0.0)
	{
		// No adsorbed phase, all SO4 is aqueous
		// Calculate aqueous SO4 concen (free + Al-complexed) (mmol/m3) - from total SO4 amount (meq/m2) and volume (m) (convert meq to mmol)
		return total_SO4 / (WaterVolume * 2.0);
	}
	
	if(conc_Al < 0.0333)
	{
		// For low AL concen, the full solutions are unstable. Low AL means negligible AL complexes, so a solution ignoring Al is acceptable.
		double AA = 2.0 * WaterVolume;
		double BB = WaterVolume*SO4HalfSat + SO4AdsorptionCap - total_SO4;
		double CC = -SO4HalfSat*total_SO4/2.0;
		double xSO4 = SolveQuadratic(AA, BB, CC);
		return conc_Al * (Coeff.K_SO4[0] + 2.0*Coeff.K_SO4[1]*xSO4)*xSO4 + xSO4;
	}
	
	// Calculate the variables for input to algorithm used for solving a cubic equation
	// TODO: Should factor out solution algorithm as a general cubic solver here!!!
	
	double PP = (1.0 + Coeff.K_SO4[0]*conc_Al + SO4HalfSat*Coeff.K_SO4[1]*conc_Al)/(2.0*Coeff.K_SO4[1]*conc_Al);
	double QQ = (SO4HalfSat + SO4AdsorptionCap/WaterVolume - total_SO4/WaterVolume + SO4HalfSat*Coeff.K_SO4[0]*conc_Al)/(4.0*Coeff.K_SO4[1]*conc_Al);
	double RR = -SO4HalfSat*total_SO4/(8.0*WaterVolume*Coeff.K_SO4[1]*conc_Al);
	double AA2 = (3.0*QQ -PP*PP)/3e10;
	double BB2 = (2.0*PP*PP*PP - 9.0*PP*QQ + 27.0*RR)/27e15;
	double CC  = BB2*BB2/4.0 + AA2*AA2*AA2/27.0;
	
	if(CC >= 0.0)
	{
		// Real, posiive value for free SO4 ion concentration (mmol/m3) - calculate and return
		CC = sqrt(CC);
		double AA = pow(10.0, log10(abs(-BB2/2.0 + CC)*1e15)/3.0);
		AA2 = -BB2/2.0 + CC;
		AA = Sign(AA, AA2);
		double BB = pow(10.0, log10(abs(-BB2/2.0 + CC)*1e15)/3.0);
		BB2 = -BB2/2.0 - CC;
		BB = Sign(BB, BB2);
		double xSO4 = AA + BB - PP/3.0;
		return conc_Al * (Coeff.K_SO4[0] + 2.0*Coeff.K_SO4[1]*xSO4)*xSO4 + xSO4;
	}
	else
	{
		// Imaginary value, try alternate solutions for free SO4 ion concentration (mmol/m3) - calculate and return
		
		double AA = acos(-BB2/(2.0*sqrt(-AA2*AA2*AA2/27.0)));
		double BB = 2.0*sqrt(-AA2/3.0)*1e5;
		double xSO4 = BB*cos(AA/3.0) - PP/3.0;
		if(xSO4 > 0.0)
		{
			return conc_Al * (Coeff.K_SO4[0] + 2.0*Coeff.K_SO4[1]*xSO4)*xSO4 + xSO4;
		}
		xSO4 = BB*cos(AA/3.0 + 2.0*Pi/3.0) - PP/3.0;
		if(xSO4 > 0.0)
		{
			return conc_Al * (Coeff.K_SO4[0] + 2.0*Coeff.K_SO4[1]*xSO4)*xSO4 + xSO4;
		}
		xSO4 = BB*cos(AA/3.0 + 4.0*Pi/3.0) - PP/3.0;
		if(xSO4 > 0.0)
		{
			return conc_Al * (Coeff.K_SO4[0] + 2.0*Coeff.K_SO4[1]*xSO4)*xSO4 + xSO4;
		}
	}
	
	// No solution found, set results to zero (mmol/m3)
	return 0.0;
}

double
SolveFreeSO4(const magic_coeff &Coeff, double all_SO4, double conc_Al)
{
	// Solve for free Sulfate ion concen (mmol/m3) from aqueous Sulfate concentration (meg/m3), trivalent AL ion concen (mmol/m3)
	
	// If total aqueous SO4 not positive, no solution. All SO4 values are set to zero (mmol/m3).
	if(all_SO4 <= 0.0) return 0.0;
	
	// For low AL concen, the full solutions are unstable. But low AL means negligible Al complexes, so a solution ignoring Al is acceptable.
	if(conc_Al < 0.0333) return all_SO4;
	
	// Solve quadratic equation for unique solution of free SO4 concentration (mmol/m3)
	double AA = 2.0*conc_Al*Coeff.K_SO4[1];
	double BB = 1.0 + conc_Al*Coeff.K_SO4[0];
	double CC = -all_SO4;
	return SolveQuadratic(AA, BB, CC);
}

void
CalculateDOC(const magic_coeff &Coeff, double conc_DOC, double conc_H, double conc_Al,
		// the following are outputs of the function
		double &conc_H3A, double &conc_H2AM, double &conc_HA2M, double &conc_A3M, double &conc_AlA, double &conc_AlHA)
{
	// Calculate DOC species concen (H3A,H2AM,HA2M,A3M, AlA,ALHA) (mmol/m3) from DOC, H and Al3+ ion concens (mmol/m3)
	
	// Set constants to solve the 5 simultaneous equilibrium equations
	double Term1 = conc_H*conc_H*conc_H + Coeff.K_DOC[0]*conc_H*conc_H + Coeff.K_DOC[0]*Coeff.K_DOC[1]*conc_H
		+ Coeff.K_DOC[0]*Coeff.K_DOC[1]*Coeff.K_DOC[2]*(1.0 + Coeff.K_AlDOC[0]*conc_Al + Coeff.K_AlDOC[1]*conc_Al*conc_H);
		
	// Calculate the concentrations (mmol/m3) of the triprotic organic acid components
	conc_H3A  = conc_DOC*conc_H*conc_H*conc_H/Term1;
	conc_H2AM = conc_H3A*Coeff.K_DOC[0]/conc_H;
	conc_HA2M = conc_H2AM*Coeff.K_DOC[1]/conc_H;
	conc_A3M  = conc_HA2M*Coeff.K_DOC[2]/conc_H;
	
	// Calculate the concentrations (mmol/m3) of the organic complexes with Al
	conc_AlA  = conc_A3M*Coeff.K_AlDOC[0]*conc_Al;
	conc_AlHA = conc_A3M*Coeff.K_AlDOC[1]*conc_Al*conc_H;
}

double
CalculateNetAlCharge(const magic_coeff &Coeff, double conc_H, double conc_Al, double conc_SO4, double conc_F, double conc_A3M)
{
	// Calculate net charge on all Al species (positive-negative) (meq/m3) from H, AL3+, SO4, F and organic acid trivalent anion concens (mmol/m3)
	
	// Al-hydroxide dissociation
	double Term1 = 0.0;
	for(int Idx = 3; Idx >= 0; --Idx)
	{
		Term1 = (Term1 + (double)(2-Idx)*Coeff.K_Al[Idx])/conc_H;
	}
	
	// Al-SO4 complexes
	double Term2 = (Coeff.K_SO4[0] + Coeff.K_SO4[1]*conc_SO4)*conc_SO4;
	
	// Al-F complexes
	double Term3 = 0.0;
	for(int Idx = 5; Idx >= 0; --Idx)
	{
		Term3 = (Term3 + (double)(2-Idx)*Coeff.K_F[Idx])*conc_F;
	}
	
	// Al-DOC complexes
	double Term4 = Coeff.K_AlDOC[1]*conc_H*conc_A3M;
	
	// Sum the net charge from all Al species (meq/m3)
	return conc_Al*(3.0 + Term1 + Term2 + Term3 + Term4);
}

double
ComputeAlHEquivalent(const magic_coeff &Coeff, double conc_H, double conc_Al, double conc_SO4, double conc_F, double conc_A3M)
{
	// Calculate H ion equivalent of all Al species (meq/m3) - H neutralized by Al species in acidimetric titration from H, AL3+, SO4, F and organic acid trivalent	anion concens (mmol/m3)
	
	// Al-hydroxide dissociation
	double Term1 = 0.0;
	for(int Idx = 3; Idx >= 0; --Idx)
	{
		Term1 = (Term1 + (double)(2-Idx)*Coeff.K_Al[Idx])/conc_H;
	}
	
	// Al-SO4 complexes
	double Term2 = (3.0*Coeff.K_SO4[0] + 3.0*Coeff.K_SO4[1]*conc_SO4)*conc_SO4;
	
	// Al-F complexes
	double Term3 = 0.0;
	for(int Idx = 5; Idx >= 0; --Idx)
	{
		Term3 = (Term3 + 3.0*Coeff.K_F[Idx])*conc_F;
	}
	
	// Al-DOC complexes
	double Term4 = (3.0*Coeff.K_AlDOC[0] + 4.0*Coeff.K_AlDOC[1]*conc_H)*conc_A3M;
	
	// Sum calculated concentrations of H ion equivalents from all Al species (meq/m3)
	return conc_Al*(3.0 + Term1 + Term2 + Term3 + Term4);
}

double
CalculateTotalAqueousAl(const magic_coeff &Coeff, double conc_H, double conc_Al, double conc_SO4, double conc_F, double conc_A3M)
{
	// Calculate total aqueous Al concen (free + F-SO4-DOC-complexed) (mmol/m3) from H, AL3+, SO4, F and organic acid trivalent anion concens (mmol/m3)
	
	// Al-hydroxide dissociation
	double Term1 = 0.0;
	for(int Idx = 3; Idx >= 0; --Idx)
	{
		Term1 = (Term1 + Coeff.K_Al[Idx])/conc_H;
	}
	
	// Al-SO4 complexes
	double Term2 = (Coeff.K_SO4[0] + Coeff.K_SO4[1]*conc_SO4)*conc_SO4;
	
	// Al-F complexes
	double Term3 = 0.0;
	for(int Idx = 5; Idx >= 0; --Idx)
	{
		Term3 = (Term3 + Coeff.K_F[Idx])*conc_F;
	}
	
	// Al-DOC complexes
	double Term4 = (Coeff.K_AlDOC[0] + Coeff.K_AlDOC[1]*conc_H)*conc_A3M;
	
	// Sum total Al concentrations across all species (mmol/m3)
	return conc_Al*(1.0 + Term1 + Term2 + Term3 + Term4);
}

double
ComputeIonicStrength(const magic_output &Result, const magic_coeff &Coeff, double conc_AlHA)
{
	// Calculate the ionic strength of the aqueous solution, used to correct equilibrium constants with Debye-Huckel equation
	
	double H2 = Result.conc_H*Result.conc_H;
	double F2 = Result.conc_F*Result.conc_F;
	double F3 = F2*Result.conc_F;
	
	// Ionic strength is approximated using Debye-Huckel expression (changing umol/L to mol/L)
	double Xxis = (4.0*(Result.conc_Ca + Result.conc_Mg + Result.conc_SO4 + Result.conc_CO3 + Result.conc_HA2M) + Result.conc_Na + Result.conc_K + Result.conc_NH4 + Result.conc_Cl + Result.conc_NO3 + Result.conc_F + Result.conc_H + Coeff.K_W/Result.conc_H + Result.conc_HCO3 + Result.conc_H2AM + 9.0*Result.conc_A3M + conc_AlHA + Result.conc_Al*(9.0 + 4.0*Result.conc_F*Coeff.K_F[0] + F2*Coeff.K_F[1] + F2*F2*Coeff.K_F[3] + 4.0*F2*F3*Coeff.K_F[4] + 9.0*F3*F3*Coeff.K_F[5] + Result.conc_SO4*Coeff.K_SO4[0] + Result.conc_SO4*Result.conc_SO4*Coeff.K_SO4[1] + 4.0*Coeff.K_Al[0]/Result.conc_H + Coeff.K_Al[1]/H2 + Coeff.K_Al[3]/(H2*H2)))*0.0000005;
	return sqrt(Xxis)/(1.0 + sqrt(Xxis));
}



void
MagicCore(const magic_input &Input, const magic_param &Param, magic_output &Result, bool IsSoil, double Conv, double H_estimate, double IonicStrength)
{
	/*
		Controls:
			IsSoil - control switch, true = current compartment is soil (ionic exchange reactions with solid phase occur)
			                         false = current compartment is water (no soil present for solid phase exchanges)
			Conv   - (meq/m3) - Convergence criterion to stop solution routine, difference in total plus and total minus charges in solution
			             NOTE: CONV = 1.0 is usual, but smaller values may be needed (at computational cost) if reliable pH's above 6-7 are needed
			H_estimate -  (meq/m3) - initial estimate of H ion concentration for iterative solution routine
			IonicStrength - (0 to 1) - ionic strength for equilibrium constant corrections (Debye-Huckel approximation) 
			            NOTE: Values for these 2 conditions are needed as inputs from the external program. The external program must save 
			                  these updated values and provide them as inputs on the next call to this compartment at the next time step.
			                  (for first call to solution routine for each compartment, initialize HEST = 1.0, STRI = 0.0)
	
	*/
	
	double SoilCationExchange;
	double WaterVolume;
	double SO4AdsorptionCap;
	if(IsSoil)
	{
		// Calculate soil water volume (m) (m3/m2) - from soil depth (m) and porosity (frac) - this is pore water volume for soils
		WaterVolume = Param.Depth * Param.Porosity;
		
		// Calculate soil total cation exchange capacity (meq/m2) - from depth (m), bulk density (kg/m3), cation exchange capacity (meq/kg) 
		SoilCationExchange = Param.Depth * Param.BulkDensity * Param.CationExchangeCapacity;
		
		// Calculate soil total SO4 adsorption capacity (meq/m2) - from depth (m), bulk density (kg/m3), maximum SO4 adsorption capacity (meq/kg)
		SO4AdsorptionCap = Param.Depth * Param.BulkDensity *  Param.SO4MaxCap;
	}
	else
	{
		WaterVolume = Param.Depth;
		SoilCationExchange = 0.0;
		SO4AdsorptionCap = 0.0;
	}
	
	magic_coeff Coeff;
	// Set equilibrium constants for chemical reactions (temp & ionic strength dependent)
	SetEquilibriumConstants(Param, Coeff, IsSoil, IonicStrength, SoilCationExchange);
	
	// Retreive initial estimate of hydrogen ion conc (mmol/m3) - from last time step
	Result.conc_H = H_estimate;
	
	// Calculate pH - from H concen (convert mmol/m3 to mol/L)
	Result.pH     = -log10(Result.conc_H*1e-6);
	
	// Calculate trivalent Aluminum ion concen (Al3+) (mmol/m3) - from H ion concen (mmol/m3), aluminum solubility coeff and H ion power
	Result.conc_Al = Coeff.GibbsAlSolubility * pow(Result.conc_H, Param.HAlOH3Exponent);
	
	// Calculate free Ammonium ion concen (mmol/m3) - from total Ammonium amount (meq/m2) and pore volume (m)  (convert meq to mmol)
	Result.conc_NH4 = Input.total_NH4 / WaterVolume;
	
	// Calculate free Nitrate ion concen (mmol/m3) - from total Nitrate amount (meq/m2) and pore volume (m)  (convert meq to mmol)
	Result.conc_NO3 = Input.total_NO3 / WaterVolume;
	
	// Calculate free Chloride ion concen (mmol/m3) - from total Chloride amount (meq/m2) and pore volume (m)  (convert meq to mmol)
	Result.conc_Cl  = Input.total_Cl  / WaterVolume;
	
	// Calculate aqueous Flouride concen (free + Al-complexed) (mmol/m3) - from total Fluoride amount (meq/m2) and pore volume (m)  (convert meq to mmol)
	Result.all_F    = Input.total_F   / WaterVolume;
	
	// Solve for free Fluoride ion concen (mmol/m3) - from aqueous Fluoride concen (mmol/m3) and trivalent Al ion concen (mmol/m3)  	
	Result.conc_F = SolveFreeFluoride(Coeff, Result.all_F, Result.conc_Al);
	
	// Solve for aqueous Sulfate concen (free + Al-complexed) (mmol/m3) - from total Sulfate amount (meg/m2), trivalent AL ion concen (mmol/m3) and sulfate adsorption parameters  
	Result.all_SO4 = SolveAqueousSO4(Coeff, Input.total_SO4, Result.conc_Al, WaterVolume, SO4AdsorptionCap, Param.SO4HalfSat);
	
	// Solve for free Sulfate ion concen (mmol/m3) - from aqueous Sulfate concentration (meg/m3), trivalent AL ion concen (mmol/m3)
	Result.conc_SO4 = SolveFreeSO4(Coeff, Result.all_SO4, Result.conc_Al);
	
	// Calculate Sum Acid Anions (meq/m3) (convert mmol to meq) 
	Result.SumAcidAnionConc = 2.0*Result.conc_SO4 + Result.conc_Cl + Result.conc_NO3 + Result.conc_F;
	
	
	// Calculate CO2 concentration (mmol/m3) - from Henry's law constant (XKH) and CO2 partial pressure (PCO2; atm) (convert %atm to atm)
	double conc_CO2 = Coeff.HenrysLawConstant*Param.PartialPressureCO2/100.0;
	
	// Calculate bicarbonate conc (mmol/m3) - from H concen (mmol/m3) and equil constants
	Result.conc_HCO3 = Coeff.K_C[0]*conc_CO2/Result.conc_H;
	
	// Calculate carbonate conc (mmol/m3) - from H concen (mmol/m3) and equil constants
	Result.conc_CO3 = Coeff.K_C[1]*Result.conc_HCO3/Result.conc_H;
	
	// Calculate anion charge (meq/m3) - from dissolved inorganic carbon species (convert mmol to meq)
	Result.all_DIC = Result.conc_HCO3 + 2.0*Result.conc_CO3;
	
	
	// Calculate DOC species concen (H3A,H2AM,HA2M,A3M, AlA,ALHA) (mmol/m3) - from DOC, H and Al3+ ion concens (mmol/m3)
	double conc_H3A, conc_AlA, conc_AlHA;
	CalculateDOC(Coeff, Param.conc_DOC, Result.conc_H, Result.conc_Al,
		// the following are outputs of the function
		conc_H3A, Result.conc_H2AM, Result.conc_HA2M, Result.conc_A3M, conc_AlA, conc_AlHA);
	
	// Calculate anion charge (meq/m3) - from dissolved organic carbon
	Result.all_DOC = Result.conc_H2AM + 2.0*Result.conc_HA2M + 3.0*Result.conc_A3M;
	
	
	// Calculate net charge on all Al species (positive-negative) (meq/m3) - from H, AL3+, SO4, F and organic acid trivalent anion concens (mmol/m3)
	double NetAlCharge = CalculateNetAlCharge(Coeff, Result.conc_H, Result.conc_Al, Result.conc_SO4, Result.conc_F, Result.conc_A3M);
	
	// Calculate Total Base Cation amount (meq/m2) - from individual inputs (meq/m2)
	double TotalBaseCations = Input.total_Ca + Input.total_Mg + Input.total_Na + Input.total_K;
	
	double CaExchange, MgExchange, NaExchange, KExchange;   // Exchange fractions
	if(IsSoil)
	{
		// If soil compartment - calculate exchangeable fraction of base cations
		
		// Estimate Sum Base Cations concen (meq/m3) - from positive charge remaining after all other ions
		double SumBaseCations1 = Result.SumAcidAnionConc + Result.all_DIC + Result.all_DOC + Coeff.K_W/Result.conc_H - NetAlCharge - Result.conc_H - Result.conc_NH4;
	
		// Calculate exchangeable Al fraction on soil (EAL=1-BS) (fraction) - from TBC (meq/m2), SBC1 (meq/m3), soil pore volume (m) and soil cation exchange capacity (meq/m2) 
		double exchangeable_Al = 1.0 - TotalBaseCations/SoilCationExchange + WaterVolume*SumBaseCations1/SoilCationExchange;
		
		// Solve cation exchange equations using selectivity coefficients - calculations based on GainesThomas cation exchange using equivalents of charge
		double XK0 = log10(exchangeable_Al/Coeff.GibbsAlSolubility)/3.0 - log10(Result.conc_H)*(Param.HAlOH3Exponent/3.0);
		
		CaExchange = pow(10.0, 2.0*XK0 + Coeff.K_AlCa);
		MgExchange = pow(10.0, 2.0*XK0 + Coeff.K_AlMg);
		NaExchange = pow(10.0, XK0 + Coeff.K_AlNa);
		KExchange  = pow(10.0, XK0 + Coeff.K_AlK);
	}
	else
	{
		CaExchange = 0.0;
		MgExchange = 0.0;
		NaExchange = 0.0;
		KExchange  = 0.0;
	}
	
	// Calculate free Calcium ion concen (mmol/m3) - from total Calcium (meg/m2), soil pore volume (m) and soil exchange fraction (convert meq to mmol)
	Result.conc_Ca = Input.total_Ca/(CaExchange + 2.0*WaterVolume);
	
	// Calculate free Magnesium ion concen (mmol/m3) - from total Magnesiumium (meq/m2), soil pore volume (m) and soil exchange fraction (convert meq to mmol)
	Result.conc_Mg = Input.total_Mg/(MgExchange + 2.0*WaterVolume);
	
	// Calculate free Sodium ion concen (mmol/m3) - from total Sodium (meq/m2), soil pore volume (m) and soil exchange fraction (convert meq to mmol)
	Result.conc_Na = Input.total_Na/(NaExchange + WaterVolume);
	
	// Calculate free Potassium ion concen (mmol/m3) - from total Potassium (meq/m2), soil pore volume (m) and soil exchange fraction (convert meq to mmol)
	Result.conc_K = Input.total_K/(KExchange + WaterVolume);
	
	// Calculate Sum Base Cations concen (meq/m3) - (convert mmol to meq)
	Result.SumBaseCationConc = 2.0*Result.conc_Ca + 2.0*Result.conc_Mg + Result.conc_Na + Result.conc_K;
	
	// Calculate charge balance for all aqueous ions
	double ChargeBalance = Result.SumBaseCationConc + NetAlCharge + Result.conc_H + Result.conc_NH4 - Result.SumAcidAnionConc - Result.all_DIC - Result.all_DOC - Coeff.K_W/Result.conc_H;
	
	int Iter = 0;
	if(abs(ChargeBalance) >= Conv)  // If charge balance is within convergence criterion, the solution has converged. Otherwise, do an iterative solution loop
	{
		// Set increment for changing pH and begin the iterative solution loop
		double dpH = (ChargeBalance < 0.0) ? -0.02 : 0.02;
		while(true)
		{
			//NOTE: It could be possible to factor out some of the below since it repeats what is done above, but it is a little tricky since some small details are done differently.
			
			Result.pH += dpH;
			// Convert to new H conc (mmol/m3) and repeat solution for charge balance
			Result.conc_H = 1.0 / pow(10.0, Result.pH - 6.0);
			
			// Calculate trivalent Aluminum ion concen (Al3+) (mmol/m3) - from H ion concen (mmol/m3), aluminum solubility coeff (XKG) and H ion power (XPON)
			Result.conc_Al = Coeff.GibbsAlSolubility * pow(Result.conc_H, Param.HAlOH3Exponent);
			
			// Solve for free Fluoride ion concen (mmol/m3) - from aqueous Fluoride concen (mmol/m3) and trivalent Al ion concen (mmol/m3)  	
			Result.conc_F = SolveFreeFluoride(Coeff, Result.all_F, Result.conc_Al);
			
			// Solve for aqueous Sulfate concen (free + Al-complexed) (mmol/m3) - from total Sulfate amount (meg/m2), trivalent AL ion concen (mmol/m3) and sulfate adsorption parameters  
			Result.all_SO4 = SolveAqueousSO4(Coeff, Input.total_SO4, Result.conc_Al, WaterVolume, SO4AdsorptionCap, Param.SO4HalfSat);
	
			// Solve for free Sulfate ion concen (mmol/m3) - from aqueous Sulfate concentration (meg/m3), trivalent AL ion concen (mmol/m3)
			Result.conc_SO4 = SolveFreeSO4(Coeff, Result.all_SO4, Result.conc_Al);
			
			// Calculate Sum Acid Anions (meq/m3) (convert mmol to meq) 
			Result.SumAcidAnionConc = 2.0*Result.conc_SO4 + Result.conc_Cl + Result.conc_NO3 + Result.conc_F;
			
			// Calculate bicarbonate conc (mmol/m3) - from H concen (mmol/m3) and equil constants
			Result.conc_HCO3 = Coeff.K_C[0]*conc_CO2/Result.conc_H;
			
			// Calculate carbonate conc (mmol/m3) - from H concen (mmol/m3) and equil constants
			Result.conc_CO3 = Coeff.K_C[1]*Result.conc_HCO3/Result.conc_H;
			
			// Calculate anion charge (meq/m3) - from dissolved inorganic carbon species (convert mmol to meq)
			Result.all_DIC = Result.conc_HCO3 + 2.0*Result.conc_CO3;
			
			CalculateDOC(Coeff, Param.conc_DOC, Result.conc_H, Result.conc_Al,
			// the following are outputs of the function
			conc_H3A, Result.conc_H2AM, Result.conc_HA2M, Result.conc_A3M, conc_AlA, conc_AlHA);
		
			// Calculate anion charge (meq/m3) - from dissolved organic carbon
			Result.all_DOC = Result.conc_H2AM + 2.0*Result.conc_HA2M + 3.0*Result.conc_A3M;
			
			// Calculate net charge on all Al species (positive-negative) (meq/m3) - from H, AL3+, SO4, F and organic acid trivalent anion concens (mmol/m3)
			NetAlCharge = CalculateNetAlCharge(Coeff, Result.conc_H, Result.conc_Al, Result.conc_SO4, Result.conc_F, Result.conc_A3M);
			
			if(IsSoil)
			{
				// If soil compartment - calculate exchangeable fraction of base cations
				
				//TODO: Is the order of the next two expressions correct?
				// Estimate Sum Base Cations concen (meq/m3) - from positive charge remaining after all other ions
				double SumBaseCations1 = Result.SumAcidAnionConc + Result.all_DIC + Result.all_DOC + Coeff.K_W/Result.conc_H - NetAlCharge - Result.conc_H - Result.conc_NH4;
				
				// Calculate exchangeable Al fraction on soil (EAL=1-BS) (fraction) - from TBC (meq/m2), SBC1 (meq/m3), soil pore volume (m) and soil cation exchange capacity (meq/m2) 
				double exchangeable_Al = 1.0 - TotalBaseCations/SoilCationExchange + WaterVolume*SumBaseCations1/SoilCationExchange;
	
			
				// Solve cation exchange equations using selectivity coefficients - calculations based on GainesThomas cation exchange using equivalents of charge
				double XK0 = log10(exchangeable_Al/Coeff.GibbsAlSolubility)/3.0 - log10(Result.conc_H)*(Param.HAlOH3Exponent/3.0);
				
				CaExchange = pow(10.0, 2.0*XK0 + Coeff.K_AlCa);
				MgExchange = pow(10.0, 2.0*XK0 + Coeff.K_AlMg);
				NaExchange = pow(10.0, XK0 + Coeff.K_AlNa);
				KExchange  = pow(10.0, XK0 + Coeff.K_AlK);
			}
			else
			{
				CaExchange = 0.0;
				MgExchange = 0.0;
				NaExchange = 0.0;
				KExchange  = 0.0;
			}
			
			// Calculate free Calcium ion concen (mmol/m3) - from total Calcium (meg/m2), soil pore volume (m) and soil exchange fraction (convert meq to mmol)
			Result.conc_Ca = Input.total_Ca/(CaExchange + 2.0*WaterVolume);
			
			// Calculate free Magnesium ion concen (mmol/m3) - from total Magnesiumium (meq/m2), soil pore volume (m) and soil exchange fraction (convert meq to mmol)
			Result.conc_Mg = Input.total_Mg/(MgExchange + 2.0*WaterVolume);
			
			// Calculate free Sodium ion concen (mmol/m3) - from total Sodium (meq/m2), soil pore volume (m) and soil exchange fraction (convert meq to mmol)
			Result.conc_Na = Input.total_Na/(NaExchange + WaterVolume);
			
			// Calculate free Potassium ion concen (mmol/m3) - from total Potassium (meq/m2), soil pore volume (m) and soil exchange fraction (convert meq to mmol)
			Result.conc_K = Input.total_K/(KExchange + WaterVolume);
			
			// Calculate Sum Base Cations concen (meq/m3) - (convert mmol to meq)
			Result.SumBaseCationConc = 2.0*Result.conc_Ca + 2.0*Result.conc_Mg + Result.conc_Na + Result.conc_K;
			
			// Calculate charge balance for all aqueous ions
			double ChargeBalance0 = Result.SumBaseCationConc + NetAlCharge + Result.conc_H + Result.conc_NH4 - Result.SumAcidAnionConc - Result.all_DIC - Result.all_DOC - Coeff.K_W/Result.conc_H;
			
			// If charge balance is within convergence criterion the solution is complete - leave iteration loop
			if(abs(ChargeBalance0) < Conv) break;
			
			// If solution has overshot, reduce step size and change increment direction
			double ChargeSgn = ChargeBalance0 / ChargeBalance;
			if(ChargeSgn < 0.0) dpH = -dpH/2.0;
			
			//printf("pH: %g, dpH: %g  charge balance: %g\n", Result.pH, dpH, ChargeBalance0);
			
			ChargeBalance = ChargeBalance0;
			
			//if(Iter++ > 100) exit(0);
		}
	}
	
	// Calculate charge balance alkalinity of solution (meq/m3) (ANC,acid neutralizing capacity)
	Result.ChargeBalanceAlk = Result.SumBaseCationConc + Result.conc_NH4 - Result.SumAcidAnionConc;
	
	// Calculate H neutralized by Al species in acidimetric titration (meq/m3)- from H, AL3+, SO4, F and organic acid trivalent anion concens (mmol/m3)
	double AlTitration = ComputeAlHEquivalent(Coeff, Result.conc_H, Result.conc_Al, Result.conc_SO4, Result.conc_F, Result.conc_A3M);
	
	// Calulate weak acid alkalinity of solution (meq/m3) )limnological definition)
	Result.WeakAcidAlk = Result.conc_HCO3 + 2.0*Result.conc_CO3 + Result.conc_H2AM + 3.0*Result.conc_A3M + Coeff.K_W/Result.conc_H - AlTitration - Result.conc_H;
	
	// Calculate total aqueous Al concen (free + F-SO4-DOC-complexed) (mmol/m3) - from H, AL3+, SO4, F and organic acid trivalent anion concens (mmol/m3)
	Result.all_Al = CalculateTotalAqueousAl(Coeff, Result.conc_H, Result.conc_Al, Result.conc_SO4, Result.conc_F, Result.conc_A3M);
	
	// Calculate DOC-complexed aqueous Al concen (mmol/m3)
	Result.org_Al = conc_AlA + conc_AlHA;
	
	// Calculate molar ratio of Calcium to aqueous Aluminum (mol/mol)
	Result.CaAlRatio = 0.0;
	if(Result.all_Al > 0.0) Result.CaAlRatio = Result.conc_Ca / Result.all_Al;
	
	if(IsSoil)
	{
		// If soil compartment - calculate solid phase output variables
		
		// If SO4 adsorption, calculate fraction of adsorption capacity filled (convert mmol to meq for adsorption isotherm)
		Result.exchangeable_SO4 = 0.0;
		if(Param.SO4MaxCap > 0.0 && Param.SO4HalfSat > 0.0)
		{
			Result.exchangeable_SO4 = (Param.SO4MaxCap*2.0*Result.conc_SO4/(Param.SO4HalfSat + 2.0*Result.conc_SO4)) / Param.SO4MaxCap;
		}
		
		// Calculate excangeable Ca/Mg/Na/K on soil (fraction) (convert mmol to meq)
		Result.exchangeable_Ca = Input.total_Ca/SoilCationExchange - Result.conc_Ca*2.0*WaterVolume/SoilCationExchange;
		Result.exchangeable_Mg = Input.total_Mg/SoilCationExchange - Result.conc_Mg*2.0*WaterVolume/SoilCationExchange;
		Result.exchangeable_Na = Input.total_Na/SoilCationExchange - Result.conc_Na*WaterVolume/SoilCationExchange;
		Result.exchangeable_K  = Input.total_K /SoilCationExchange - Result.conc_K *WaterVolume/SoilCationExchange;
		Result.BaseSaturationSoil = Result.exchangeable_Ca + Result.exchangeable_Mg + Result.exchangeable_Na + Result.exchangeable_K;
	}
	else
	{
		// If not soil compartment - set solid phase output variables to zero
		Result.exchangeable_SO4 = 0.0;
		Result.exchangeable_Ca = 0.0;
		Result.exchangeable_Mg = 0.0;
		Result.exchangeable_Na = 0.0;
		Result.exchangeable_K  = 0.0;
		Result.BaseSaturationSoil = 0.0;
	}
	
	// Calculate sum of plus and minus ionic concentrations (meq/m3) for output for charge balance checks (convert mmol to meq)
	double H2 = Result.conc_H*Result.conc_H;
	double F2 = Result.conc_F*Result.conc_F;
	double F3 = F2*Result.conc_F;
	Result.SumPositive = Result.SumBaseCationConc + Result.conc_H + Result.conc_NH4 + Result.conc_Al*(3.0 + 2.0*Coeff.K_Al[0]/Result.conc_H + Coeff.K_Al[1]/H2 + 2.0*Coeff.K_F[0]*Result.conc_F + Coeff.K_F[1]*F2 + Coeff.K_SO4[0]*Result.conc_SO4) + conc_AlHA;
	
	Result.SumNegative = Result.SumAcidAnionConc + Coeff.K_W/Result.conc_H + Result.conc_HCO3 + 2.0*Result.conc_CO3 + Result.conc_H2AM + 2.0*Result.conc_HA2M + 3.0*Result.conc_A3M + Result.conc_Al*(Coeff.K_Al[3]/(H2*H2) + Coeff.K_F[3]*F2*F2 + 2.0*Coeff.K_F[4]*F2*F3 + 3.0*Coeff.K_F[5]*F3*F3 + Coeff.K_SO4[1]*Result.conc_SO4*Result.conc_SO4);
	
	Result.IonicStrength = ComputeIonicStrength(Result, Coeff, conc_AlHA);
}
