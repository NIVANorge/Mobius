


//NOTE: Just starting to sketch out some parts of the model. It is far from finished.

static void
AddIncaPModel(mobius_model *Model)
{
	auto GCPerM2       = RegisterUnit(Model, "G C / m^2");
	auto GCPerM2PerDay = RegisterUnit(Model, "G C / m^2 / day");
	
	auto EpiphyteGrowthRateCoefficient = RegisterParameterDouble;
	auto EpiphyteTemperatureDependency = RegisterParameterDouble;
	auto HalfSaturationOfPForEpiphyteGrowth = RegisterParameterDouble;
	auto EpiphyteDeathRateCoefficent;
	auto MacrophyteGrowthRateCoefficient;
	auto MacrophyteTemperatureDependency;
	auto SelfShadingForMacrophytes;
	auto HalfSaturationOfPForMacrophyteGrowth;
	auto MacrophyteDeathRateCoefficient;
	
	
	auto SolarRadiation = GetEquationHandle(Model, "Solar radiation"); //NOTE: From SolarRadiation.h This one is not computed exactly as specified in published INCA-P, but similar.
	auto WaterTemperature = GetEquationHandle(Model, "Water temperature"); //NOTE: From WaterTemperature.h
	auto ReachFlow        = GetEquationHandle(Model, "Reach flow"); //NOTE: From Persist.h
	
	auto MacrophyteGrowthRate = RegisterEquation(Model, "Macrophyte growth rate", GCPerM2PerDay);
	auto MacrophyteDeathRate  = RegisterEquation(Model, "Macrophyte death rate", GCPerM2PerDay);
	auto MacrophyteMass       = RegisterEquationODE(Model, "Macrophyte mass", GCPerM2);
	
	auto EpiphyteGrowthRate = RegisterEquation(Model, "Epiphyte growth rate", GCPerM2PerDay);
	auto EpiphyteDeathRate  = RegisterEquation(Model, "Epiphyte death rate", GCPerM2PerDay);
	auto EpiphyteBiomass    = RegisterEquationODE(Model, "Epiphyte biomass", GCPerM2);
	
	EQUATION(Model, MacrophyteGrowthRate,
		double TDPpw = RESULT(PorewaterTDPConcentration);
		return
			  PARAMETER(MacrophyteGrowthRateCoefficient)
			* pow(PARAMETER(MacrophyteTemperatureDependency), RESULT(WaterTemperature) - 20.0);
			* RESULT(MacrophyteMass)
			* RESULT(SolarRadiation)
			* PARAMETER(SelfShadingForMacrophytes)
			* TDPpw / 
				(
				  (PARAMETER(HalfSaturationOfPForMacrophyteGrowth) + TDPpw)
				* (PARAMETER(SelfShadingForMacrophytes) + RESULT(MacrophyteMass)
				);
	)
	
	EQUATION(Model, MacrophyteDeathRate,
		return PARAMETER(MacrophyteDeathRateCoefficient) * RESULT(MacrophyteMass) * RESULT(EpiphyteMass) * RESULT(ReachFlow);
	)
	
	EQUATION(Model, MacrophyteMass,
		return RESULT(MacrophyteGrowthRate) - RESULT(MacrophyteDeathRate);
	)
	
	EQUATION(Model, EpiphyteGrowthRate,
		double TDPwc = RESULT(WaterColumnTDPConcentration);
		return
			  PARAMETER(EpiphyteGrowthRateCoefficient)
			* pow(PARAMETER(EpiphyteTemperatureDependency), RESULT(WaterTemperature) - 20.0) 
			* RESULT(EpiphyteBiomass)
			* RESULT(MacrophyteBiomass)
			* RESULT(SolarRadiation)
			* TDPwc / (PARAMETER(HalfSaturationOfPForEpiphyteGrowth) + TDPwc);
	)
	
	EQUATION(Model, EpiphyteDeathRate,
		return PARAMETER(EpiphyteDeathRateCoefficent) * RESULT(EpiphyteBiomass) * RESULT(ReachFlow);
	)
	
	EQUATION(Model, EpiphyteBiomass,
		return RESULT(EpiphyteGrowthRate) - RESULT(EpiphyteDeathRate);
	)
	
	
}