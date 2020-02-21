
#include "../../mobius.h"
#include "../../Modules/MAGIC/MAGIC_Core.h"



int main()
{
	magic_input Input = {};
	magic_param Param = {};
	magic_output Result = {};
	
	Input.total_Ca  = 5505.8;
	Input.total_Mg  = 3870.2;
	Input.total_Na  =  770.7;
	Input.total_K   =  951.8;
	Input.total_NH4 =    0.0;
	Input.total_SO4 =   14.1;
	Input.total_Cl  =   28.0;
	Input.total_NO3 =    0.0;
	Input.total_F   =    0.0;
	
	Param.Depth                      = 0.4;
	Param.Temperature                = 5.0;
	Param.PartialPressureCO2         = 0.3;
	Param.conc_DOC                   = 30.0;
	
	Param.Log10AlOH3EquilibriumConst = 7.8;
	Param.HAlOH3Exponent             = 3.0;
	
	Param.pK1DOC                     = 3.04;
	Param.pK2DOC                     = 4.51;
	Param.pK3DOC                     = 6.46;
	Param.pK1AlDOC                   = 0.0;
	Param.pK2AlDOC                   = 0.0;
	
	Param.Porosity                   = 0.5;
	Param.BulkDensity                = 656.0;
	Param.CationExchangeCapacity     = 113.3;
	Param.SO4HalfSat                 = 100.0;
	Param.SO4MaxCap                  = 0.1;
	
	Param.Log10CaAlSelectCoeff       = -0.93;
	Param.Log10MgAlSelectCoeff       = -0.59;
	Param.Log10NaAlSelectCoeff       = -1.09;
	Param.Log10KAlSelectCoeff        = -6.81;
	
	bool IsSoil = true;
	double Conv = 1.0;
	double H_estimate = 1.0;
	double IonicStrength = /*0.0;*/0.0166965;
	
	MagicCore(Input, Param, Result, IsSoil, Conv, H_estimate, IonicStrength);
	
	printf("conc_Ca is %g, should be 29.8\n",  2.0*Result.conc_Ca);
	printf("conc_Mg is %g, should be 26.4\n",  2.0*Result.conc_Mg);
	printf("conc_Na is %g, should be 137.1\n", Result.conc_Na);
	printf("conc_K is %g, should be 2.2\n",    Result.conc_K);
	printf("conc_NH4 is %g, should be 0.0\n",  Result.conc_NH4);
	printf("conc_SO4 is %g, should be 35.9\n", 2.0*Result.conc_SO4);
	printf("conc_Cl is %g, should be 139.0\n", Result.conc_Cl);
	printf("conc_NO3 is %g, should be 0.0\n",  Result.conc_NO3);
	printf("conc_F is %g, should be 0.0\n",    Result.conc_F);
	printf("Total SO4 is %g, should be 35.9\n",  2.0*Result.all_SO4);
	printf("Total F is %g, should be 0.0\n",  Result.all_F);
	printf("pH is %g, should be 4.58\n", Result.pH);
	printf("Tot Al is %g, should be 4.2\n", 3.0*Result.all_Al);
	printf("Org Al is %g, should be 0.0\n", 3.0*Result.org_Al);
	printf("conc_Al is %g, should be 3.8\n", 3.0*Result.conc_Al);
	printf("conc_H is %g, should be 26.1\n", Result.conc_H);
	printf("conc_H2A is %g, should be 12.9\n", Result.conc_H2AM);
	printf("conc_HA2 is %g, should be 33.0\n", 2.0*Result.conc_HA2M);
	printf("Sum positive is %g, should be 224.9\n", Result.SumPositive);
	printf("Sum negative is %g, should be 224.9\n", Result.SumNegative);
	printf("ionic strength is %g\n", Result.IonicStrength);
}