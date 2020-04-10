
#include "../../mobius.h"
#include "../../Modules/MAGIC/MAGIC_Core.h"


void SetParam(magic_param &Param)
{
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
}

void TestCore()
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
	
	SetParam(Param);
	
	bool IsSoil = true;
	double Conv = 0.01;
	double H_estimate = 14.0;
	//double IonicStrength = 0.0167825;
	double IonicStrength = 0.0;
	
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
	printf("conc_HCO3 is %g, should be 2.5\n", Result.conc_HCO3);
	printf("conc_CO3 is %g, should be 0.0\n", Result.conc_CO3);
	printf("conc_H2A is %g, should be 12.9\n", Result.conc_H2AM);
	printf("conc_HA2 is %g, should be 33.0\n", 2.0*Result.conc_HA2M);
	printf("conc_A3- is %g, should be 0.7\n", 3.0*Result.conc_A3M);
	printf("Total DOC is %g, should be 46.6\n", Result.all_DOC);
	printf("Total DIC is %g, should be 2.5\n", Result.all_DIC);
	printf("CB Alk is %g, should be 19.0\n", Result.ChargeBalanceAlk);
	printf("SBC is %g, should be 194.7\n", Result.SumBaseCationConc);
	printf("SAA is %g, should be 175.7\n", Result.SumAcidAnionConc);
	printf("Sum positive is %g, should be 224.9\n", Result.SumPositive);
	printf("Sum negative is %g, should be 224.9\n", Result.SumNegative);
	printf("ES is %g, should be 26.4\n", 100.0*Result.exchangeable_SO4);
	printf("ECa is %g, should be 18.5\n", 100.0*Result.exchangeable_Ca);
	printf("EMg is %g, should be 13.0\n", 100.0*Result.exchangeable_Mg);
	printf("ENa is %g, should be 2.5\n", 100.0*Result.exchangeable_Na);
	printf("EK is %g, should be 3.2\n", 100.0*Result.exchangeable_K);
	printf("Base saturation soil is %g, should be 37.2\n", 100.0*Result.BaseSaturationSoil);
	printf("Ca/Al ratio is %g, should be 10.4\n", Result.CaAlRatio);
	printf("ionic strength is %g\n", Result.IonicStrength);
}

void TestInitCore()
{
	magic_param Param = {};
	magic_init_input Input = {};
	magic_init_result Result = {};

	Input.conc_Ca  = 29.8/2.0;
	Input.conc_Mg  = 26.4/2.0;
	Input.conc_Na  = 137.1;
	Input.conc_K   = 2.2;
	Input.conc_NH4 = 0.0;
	Input.all_SO4  = 35.9/2.0;
	Input.conc_Cl  = 139.0;
	Input.conc_NO3 = 0.0;
	Input.all_F    = 0.0;
	
	Input.exchangeable_Ca = 0.185;
	Input.exchangeable_Mg = 0.13;
	Input.exchangeable_Na = 0.025;
	Input.exchangeable_K  = 0.032;
	
	SetParam(Param);
	
	MagicCoreInitial(Input, Param, Result, true, 0.01);
	
	printf("conc_SO4 is %g, should be 35.9\n", Result.conc_SO4*2.0);
	printf("Ca-Al is %g, should be -0.93\n", Result.Log10CaAlSelectCoeff);
	printf("Mg-Al is %g, should be -0.59\n", Result.Log10MgAlSelectCoeff);
	printf("Na-Al is %g, should be -1.09\n", Result.Log10NaAlSelectCoeff);
	printf("K-Al is %g, should be -6.81\n", Result.Log10KAlSelectCoeff);
}

int main()
{
	TestCore();
	printf("\n");
	TestInitCore();
}