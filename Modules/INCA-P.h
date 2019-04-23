


//NOTE: Just starting to sketch out some parts of the model. It is far from finished.

static void
AddIncaPModel(mobius_model *Model)
{
	auto Dimensionless = RegisterUnit(Model);
	auto GCPerM2       = RegisterUnit(Model, "G C / m^2");
	auto GCPerM2PerDay = RegisterUnit(Model, "G C / m^2 / day");
	auto KgPerKm2      = RegisterUnit(Model, "kg/km^2");
	auto KgPerHaPerDay = RegisterUnit(Model, "kg/Ha/day");
	auto KgPerKm2PerDay = RegisterUnit(Model, "kg/km^2/day");
	auto PerDay         = RegisterUnit(Model, "1/day");
	auto PerKgSoil      = RegisterUnit(Model, "1/(kg soil)");
	auto KgPerHa       = RegisterUnit(Model, "kg/Ha");
	
	auto Land = GetParameterGroupHandle(Model, "Landscape units");
	
	auto LiquidManurePTimeseries     = RegisterInput(Model, "Liquid manure P", KgPerHaPerDay);
	auto LiquidFertilizerPTimeseries = RegisterInput(Model, "Liquid fertilizer P", KgPerHaPerDay);
	auto SolidManurePTimeseries      = RegisterInput(Model, "Solid manure P", KgPerHaPerDay);
	auto SolidFertilizerPTimeseries  = RegisterInput(Model, "Solid fertilizer P", KgPerHaPerDay);
	auto PlantResiduePTimeseries     = RegisterInput(Model, "Plant residue P", KgPerHaPerDay);
	auto PhosphorousDryDepositionTimeseries = RegisterInput(Model, "Phosphorous dry deposition", KgPerHaPerDay);
	
	auto LiquidManureP = RegisterParameterDouble(Model, Land, "Liquid manure P", KgPerHaPerDay, 0.0, 0.0, 100.0, "Inputs of phosphorous to soil through liquid manure. Only used if the timeseries of the same name is not provided.");
	auto LiquidFertilizerP = RegisterParameterDouble(Model, Land, "Liquid fertilizer P", KgPerHaPerDay, 0.0, 0.0, 100.0, "Inputs of phosphorous to soil through liquid fertilizer. Only used if the timeseries of the same name is not provided.");
	auto SolidManureP  = RegisterParameterDouble();
	auto SolidFertilizerP = RegisterParameterDouble();
	auto PlantResidueP = RegisterParameterDouble();
	auto PhosphorousDryDeposition = RegisterParameterDouble();
	
	auto SoilSorptionScalingFactor = RegisterParameterDouble(Model, Land, "Soil sorption scaling factor", PerDay); //TODO
	auto SoilPSorptionCoefficient = RegisterParameterDouble(Model, Land, "Soil P sorption coefficient", PerKgSoil); //TODO
	auto SoilFreundlichIsothermConstant = RegisterParameterDouble(Model, Land, "Soil freundlich isotherm constant", Dimensionless); //TODO
	auto FertilizerAdditionStartDay = RegisterParameterUInt();
	auto FertilizerAdditionPeriod   = RegisterParameterUInt();
	auto GrowingSeasonStart = RegisterParameterUInt();
	auto GrowingSeasonPeriod = RegisterParameterUInt();
	auto PlantGrowthCurveAmplitude = RegisterParameterDouble();
	auto PlantGrowthCurveOffset    = RegisterParameterDouble();
	auto PlantPUptakeFactor = RegisterParameterDouble();
	auto MaxDailyPUptake = RegisterParameterDouble();
	auto MaxAnnualPUptake = RegisterParameterDouble();
	auto MaxSoilLabilePContent = RegisterParameterDouble();
	auto WeatheringFactor = RegisterParameterDouble();
	auto ChemicalImmobilisationFactor = RegisterParameterDouble();
	auto ChangeInRateWithA10DegreeChangeInTemperature = RegisterParameterDouble();
	auto TemperatureAtWhichResponseIs1 = RegisterParameterDouble();
	auto LowerTemperatureThresholdImmobilisation = RegisterParameterDouble();
	
	auto PhosphorousWetDeposition      = RegisterEquation(Model, "Fertilizer wet deposition", KgPerHaPerDay);
	auto LiquidPInputsToSoil           = RegisterEquation(Model, "Liquid P inputs to soil", KgPerHaPerDay);
	auto SoilwaterEPC0                 = RegisterEquation(Model, "SoilwaterEPC0", MgPerL);
	auto SoilPSorptionDesorption       = RegisterEquation(Model, "Soil P sorption/desorption", KgPerKm2PerDay);
	auto TemperatureFactor             = RegisterEquation(Model, "Temperature factor", Dimensionless);
	auto SoilMoistureFactor            = RegisterEquation(Model, "Soil moisture factor", Dimensionless);
	auto SeasonalPlantGrowthIndex      = RegisterEquation(Model, "Seasonal plant growth index", Dimensionless);
	auto AccumulatedAnnualPUptake      = RegisterEquation(Model, "Accumulated annual P uptake", KgPerHa);
	auto PlantPUptakeFromSoilwater     = RegisterEquation(Model, "Plant P uptake from soilwater", KgPerKm2PerDay);
	auto SoilwaterTDPMass              = RegisterEquationODE(Model, "Soil water TDP mass", KgPerKm2);
	auto SoilLabilePMass               = RegisterEquationODE(Model, "Soil labile P mass", KgPerKm2);
	auto SoilInactivePMass             = RegisterEquationODE(Model, "Soil inactive P mass", KgPerKm2);
	
	auto SoilMassInTheOAHorizon = GetEquationHandle(Model, "Soil mass in the O/A horizon"); //From IncaSed.h
	auto SedimentDeliveryToReach = GetEquationHandle(Model, "Sediment delivery to reach"); //From IncaSed.h
	auto SoilTemperature        = GetEquationHandle(Model, "Soil temperature"); //From SoilTemperature.h
	
	
	EQUATION(Model, PhosphorousWetDeposition,
		//TODO: If there is no timeseries, it should be computed in terms of annual averages instead.
		return IF_INPUT_ELSE_PARAMETER(PhosphorousWetDepositionTimeseries, PhosphorousWetDepositionPar);
	)
	
	EQUATION(Model, LiquidPInputsToSoil,
		double Pliqin = RESULT(PhosphorousWetDeposition);
		double Pliqmanure = IF_INPUT_ELSE_PARAMETER(LiquidManurePTimeseries, LiquidManureP);
		double Pliqfert   = IF_INPUT_ELSE_PARAMETER(LiquidFertilizerPTimeseries, LiquidFertilizerP);
		
		u64 Fstart  = PARAMETER(FertilizerAdditionStartDay);
		u64 Fperiod = PARAMETER(FertilizerAdditionPeriod);
		u64 Day = CURRENT_DAY_OF_YEAR();
		
		if(Day >= Fstart && Day <= Fstart + Fperiod)
		{
			Pliqin += Pliqmanure + Pliqfert;
		}
		
		return Pliqin;
	)
	
	EQUATION(Model, SolidPInputsToSoil,
		double Psolidmanure = IF_INPUT_ELSE_PARAMETER(SolidManurePTimeseries, SolidManureP);
		double Psolidfert   = IF_INPUT_ELSE_PARAMETER(SolidFertilizerPTimeseries, SolidFertilizerP);
		double Presidue     = IF_INPUT_ELSE_PARAMETER(PlantResiduePTimeseries, PlantResidueP);
		double Pdep         = IF_INPUT_ELSE_PARAMETER(PhosphorousDryDepositionTimeseries, PhosphorousDryDeposition) / (double)DAYS_THIS_YEAR();
		
		double Psolidin = Presidue + Pdep;
		
		u64 Fstart  = PARAMETER(FertilizerAdditionStartDay);
		u64 Fperiod = PARAMETER(FertilizerAdditionPeriod);
		u64 Day = CURRENT_DAY_OF_YEAR();
		
		if(Day >= Fstart && Day <= Fstart + Fperiod)
		{
			Psolidin += Psolidmanure + Psolidfert;
		}
		
		return Psolidin;
	)
	
	EQUATION(Model, SoilwaterEPC0,
		double Msoil = RESULT(SoilMassInTheOAHorizon);
		double Plabsoil = RESULT(SoilLabilePMass);
		double epc0 = RESULT(SoilwaterTDPConcentration);
		double nsoil = PARAMETER(SoilFreundlichIsothermConstant);
		double Kfsoil = PARAMETER(SoilPSorptionCoefficient);
		if(Msoil > 0.0 && Plabsoil > 0.0 && Kfsoil > 0.0)
		{
			epc0 = pow(1e6 * Plabsoil / (Kfsoil * Msoil), nsoil);
		}
		return epc0;
	)
	
	EQUATION(Model, SoilPSorptionDesorption,
		double nsoilinv = 1.0/PARAMETER(SoilFreundlichIsothermConstant);
		return
			1e-3*PARAMETER(SoilSorptionScalingFactor)*
			(
				  pow(RESULT(SoilwaterTDPConcentration), nsoilinv)
				- pow(RESULT(SoilwaterEPC0), nsoilinv)
			)*RESULT(SoilWaterVolume);
	)
	
	EQUATION(Model, SoilMoistureFactor,
		//TODO
	)
	
	EQUATION(Model, TemperatureFactor,
		return pow(PARAMETER(ChangeInRateWithA10DegreeChangeInTemperature), (RESULT(SoilTemperature) - PARAMETER(TemperatureAtWhichResponseIs1)) / 10.0);
	)
	
	EQUATION(Model, SeasonalPlantGrowthIndex,
		double Day = (double)CURRENT_DAY_OF_YEAR();
		double DayOffset = (double)PARAMETER(GrowingSeasonStart);
		double DayCount  = (double)DAYS_THIS_YEAR();
		return PARAMETER(PlantGrowthCurveOffset) + PARAMETER(PlantGrowthCurveAmplitude)*sin(2.0*Pi*(Day - DayOffset) / DayCount);
	)
	
	EQUATION(Model, PlantPUptakeFromSoilwater,
		double tdpconc = SafeDivide(RESULT(SoilwaterTDPMass), RESULT(SoilwaterVolume));
		double uptake = 1e6 * PARAMETER(PlantPUptakeFactor) * RESULT(TemperatureFactor) * RESULT(SoilMoistureFactor) * tdpconc;
		uptake = Min(uptake, 100.0 * PARAMETER(MaxDailyPUptake));
		if(RESULT(AccumulatedAnnualPUptake) > PARAMETER(MaxAnnualPUptake))
		{
			uptake = 0.0;
		}
		return uptake;
	)
	
	EQUATION(Model, AccumulatedAnnualPUptake,
		double acc = LAST_RESULT(AccumulatedAnnualPUptake) + RESULT(PlantPUptakeFromSoilwater) / 100.0;
		if(CURRENT_DAY_OF_YEAR() == 1) acc = 0.0;
		return acc;
	)
	
	EQUATION(Model, SoilwaterTDPMass,
		return
			  100.0 * RESULT(LiquidPInputsToSoil) 
			- RESULT(SoilPSorptionDesorption) 
			- RESULT(PlantPUptakeFromSoilwater)
			
			- RESULT(SoilWaterTDPMass) * (RESULT(SoilToDirectRunoffFraction) + RESULT(SoilToGroundwaterFraction) + RESULT(SoilToReachFraction))
			+ RESULT(DirectRunoffTDPMass) * RESULT(DirectRunoffToSoilFraction);
	)
	
	EQUATION(Model, ChemicalImmobilisation,
		double Ctemp = RESULT(TemperatureFactor);
		if(INPUT(AirTemperature) <= PARAMETER(LowerTemperatureThresholdImmobilisation)) Ctemp = 0.0;
		return PARAMETER(ChemicalImmobilisationFactor) * Ctemp * RESULT(SoilLabilePMass)
	)
	
	EQUATION(Model, SoilLabilePMass,
		return
			  100.0 * RESULT(SolidPInputsToSoil)
			+ RESULT(SoilPSorptionDesorption)
			+ PARAMETER(WeatheringFactor) * RESULT(TemperatureFactor) * RESULT(SoilInactivePMass)
			- RESULT(SedimentDeliveryToReach) * SafeDivide(RESULT(SoilLabilePMass), RESULT(SoilMassInTheOAHorizon))
			- RESULT(ChemicalImmobilisation);
			
	)
	
	EQUATION(Model, SoilInactivePMass,
		return
			  RESULT(ChemicalImmobilisation)
			- PARAMETER(WeatheringFactor) * RESULT(TemperatureFactor) * RESULT(SoilInactivePMass)
			- RESULT(SedimentDeliveryToReach) * SafeDivide(RESULT(SoilInactivePMass), RESULT(SoilMassInTheOAHorizon));
	)
	
	EQUATION(Model, SoilPControl,
		
		//TODO: Make sure this is executed at the right place!!
	
		double Csatlabile = PARAMETER(MaxSoilLabilePContent);
		double Msoil      = PARAMETER(SoilMassInTheOAHorizon);
		double Plab       = RESULT(SoilLabilePMass);
		double tdp = LAST_RESULT(SoilWaterTDPMass) + Plab - Csatlabile*Msoil;
		
		if(SafeDivide(Plab, Msoil) >= Csatlabile)
		{
			SET_RESULT(SoilWaterTDPMass, tdp);
			SET_RESULT(SoilLabilePMass, Csatlabile*Msoil);
		}
		
		return 0.0;
	)
	
	
	EQUATION(Model, SoilwaterTDPConcentration,
		return 1000.0 * SafeDivide(RESULT(SoilWaterTDPMass), RESULT(SoilwaterVolume));
	)
	
	
	
	
	EQUATION(Model, GroundwaterEPC0,
		double Maquifer = 1e9*PARAMETER(AquiferMass);
		double Psorbedgw = RESULT(PMassInTheAquiferMatrix);
		double epc0 = RESULT(GroundwaterTDPConcentration);
		double ngw = PARAMETER(GroundwaterFreundlichIsothermConstant);
		double Kfgw = PARAMETER(GroundwaterPSorptionCoefficient);
		if(Kfgw > 0.0 && Psorbedgw > 0.0)
		{
			epc0 = pow(1e6 * Psorbedgw / (Kfgw * Maquifer), ngw);
		}
		return epc0;
	)
	
	EQUATION(Model, SoilPSorptionDesorption,
		double nsoilinv = 1.0/PARAMETER(SoilFreundlichIsothermConstant);
		return
			1e-3*PARAMETER(SoilSorptionScalingFactor)*
			(
				  pow(RESULT(SoilwaterTDPConcentration), nsoilinv)
				- pow(RESULT(SoilwaterEPC0), nsoilinv)
			)*RESULT(SoilWaterVolume);
	)
	
	EQUATION(Model, GroundwaterPSorptionDesorption,
		double ngwinv = 1.0/PARAMETER(GroundwaterFreundlichIsothermConstant);
		return
			1e-3*PARAMETER(GroundwaterSorptionScalingFactor)*
			(
				  pow(RESULT(GroundwaterTDPConcentration), ngwinv)
				- pow(RESULT(GroundwaterEPC0), ngwinv)
			)*RESULT(GroundwaterVolume);
	)
	
	EQUATION(Model, GroundwaterTDPMass,
		return
			- RESULT(GroundwaterPSorptionDesorption)
			
			- RESULT(GroundwaterTDPMass) * RESULT(GroundwaterToReachFraction)
			+ RESULT(SoilWaterTDPMass) * RESULT(SoilToGroundwaterFraction);
	)
	
	EQUATION(Model, PMassInTheAquiferMatrix,
		return RESULT(GroundwaterPSorptionDesorption);
	)
	
	EQUATION(Model, GroundwaterPControl,
		//TODO: Make sure this is executed at the right place.
	
		double Maquifer = 1e9*PARAMETER(AquiferMass);
		double Csataquifer = PARAMETER(AquiferMatrixPSaturation);
		double Psorbed = RESULT(PMassInTheAquiferMatrix);
		
		double tdp = LAST_RESULT(GroundwaterTDPMass) + Psorbed - Csataquifer*Maquifer;
		
		if(Psorbed/Maquifer >= Csataquifer)
		{
			SET_RESULT(GroundwaterTDPMass, tdp);
			SET_RESULT(PMassInTheAquiferMatrix, Csataquifer*Maquifer);
		}
	)
	
	EQUATION(Model, GroundwaterTDPConcentration,
		return 1000.0 * SafeDivide(RESULT(GroundwaterTDPMass), RESULT(GroundwaterVolume));
	)
	
	
	
	EQUATION(Model, DirectRunoffTDPMass,
		return 
			  RESULT(SoilwaterTDPMass) * RESULT(SoilToDirectRunoffFraction)
			- RESULT(DirectRunoffTDPMass) * (RESULT(DirectRunoffToSoilFraction) + RESULT(DirectRunoffToReachFraction));
	)
	
	
	
	
	auto Reaches = GetParameterGroupHandle(Model, "Reaches");
	
	auto EpiphyteGrowthRateCoefficient = RegisterParameterDouble(Model, Reaches, "Epiphyte growth rate coefficient", );
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
	
	
	
	
	
	
	EQUATION(Model, ReachPPInputFromLand,
		return
			  PARAMETER(PPEnrichmentFactor)
			* RESULT(TotalSedimentDeliveryToReach)
			* SafeDivide(RESULT(SoilLabilePMass) + RESULT(SoilInactivePMass), RESULT(SoilMassInTheOAHorizon));
	)
	
	EQUATION(Model, AreaScaledReachPPInputFromLand,
		return RESULT(ReachPPInputFromLand) * PARAMETER(SubcatchmentArea) * PARAMETER(Percent) / 100.0;
	)
	
	auto TotalReachPPInputFromLand = RegisterEquationCumulative(Model, "Total reach PP input from land", AreaScaledReachPPInputFromLand, LandscapeUnits);
	
	EQUATION(Model, ReachTDPInputFromLand,
		return
			  RESULT(DirectRunoffTDPMass) * RESULT(DirectRunoffToReachFraction)
			+ RESULT(SoilwaterTDPMass) * RESULT(SoilToReachFraction)
			+ RESULT(GroundwaterTDPMass) * RESULT(GroundwaterToReachFraction);
	)
	
	EQUATION(Model, AreaScaledReachTDPInputFromLand,
		return RESULT(ReachTDPInputFromLand) * PARAMETER(SubcatchmentArea) * PARAMETER(Percent) / 100.0;
	)
	
	auto TotalReachTDPInputFromLand = RegisterEquationCumulative(Model, "Total reach TDP input from land", AreaScaledReachTDPInputFromLand, LandscapeUnits);
	
	EQUATION(Model, WaterColumnTDPInput,
		double upstreamtdp = 0.0;
		
		FOREACH_INPUT(Reach,
			upstreamtdp += RESULT(WaterColumnTDPOutput, *Input);
		)
		
		//TODO: effluent inputs
		
		return upstreamtdp + RESULT(TotalReachTDPInputFromLand);
	)
	
	EQUATION(Model, WaterColumnTDPOutput,
		return 86400.0 * RESULT(ReachFlow) * SafeDivide(RESULT(WaterColumnTDPMass), RESULT(ReachVolume));
	)
	
	EQUATION(Model, ReachTDPUptakeByEpiphytes,
		return 1e-3 * PARAMETER(ProportionOfPInEpiphytes) * RESULT(EpiphyteGrowthRate) * PARAMETER(ReachLength) * PARAMETER(ReachWidth);
	)
	
	EQUATION(Model, WaterColumnTDPMass,
		//TODO: abstraction
		return
			  RESULT(WaterColumnTDPInput)
			- RESULT(WaterColumnTDPOutput)
			- RESULT(ReachTDPUptakeByEpiphytes)
			//TODO: sorption
	)
	
	EQUATION(Model, WaterColumnTDPConcentration,
		return 1e3 * SafeDivide(RESULT(WaterColumnTDPMass), RESULT(ReachVolume));
	)
	
	
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