




//NOTE: THIS MODULE IS IN DEVELOPMENT!


static void
AddIncaToxModule(mobius_model *Model)
{
	auto Dimensionless = RegisterUnit(Model);
	auto KgPerKm2      = RegisterUnit(Model, "kg/km2");
	auto NgPerM2PerDay = RegisterUnit(Model, "ng/m2/day");
	auto M3PerKg       = RegisterUnit(Model, "m3/kg");
	
	auto AtmosphericDryDeposition = RegisterInput(Model, "Atmospheric dry contaminant deposition", NgPerM2PerDay);
	auto AtmosphericWetDeposition = RegisterInput(Model, "Atmospheric wet contaminant deposition", NgPerM2PerDay);
	auto LandManagementDeposition = RegisterInput(Model, "Land management conatminant deposition", NgPerM2PerDay);
	auto LitterFallDeposition     = RegisterInput(Model, "Litter fall contaminant deposition",     NgPerM2PerDay);
	
	
	auto Chemistry = RegisterParameterGroup(Model, "Chemistry");

	
	auto AirWaterPhaseTranspherEnthalpy = RegisterParameterDouble(Model, Chemistry, "Enthalpy of phase transpher between air and water",);
	auto OctanolWaterPhaseTranspherEntalphy = RegisterParameterDouble(Model, Chemistry, "Enthalpy of phase transper between octanol and water", );
	auto HenrysConstant25       = RegisterParameterDouble(Model, Chemistry, "Henry's constant at 25°C", );
	auto OctanolWaterPartitioningCoefficient25 = RegisterParameterDouble(Model, Chemistry, "Octanol-water partitioning coefficient at 25°C", );
	
	auto SoilTemperature = GetEquationHandle(Model, "Soil temperature"); //SoilTemperature.h
	
	
	
	auto HenrysConstant = RegisterEquation(Model, "Henry's constant", );
	
	
	//TODO: These should have different units!
	auto AirWaterPartitioningCoefficient = RegisterEquation(Model, "Air-water partitioning coefficient", Dimensionless);
	auto OctanolWaterPartitioningCoefficient = RegisterEquation(Model, "Octanol-water partitioning coefficient", Dimensionless);
	auto WaterSOCPartitioningCoefficient = RegisterEquation(Model, "Water-SOC partitioning coefficient", Dimensionless);
	auto WaterDOCPartitioningCoefficient = RegisterEquation(Model, "Water-DOC partitioning coefficient", Dimensionless);
	
	EQUATION(Model, HenrysConstant,
		double LogH25 = std::log(PARAMETER(HenrysConstant25));
		double R = 8.314; //Ideal gas constant (J K^-1 mol^-1)
		double tdiff = (1.0 / RESULT(SoilTemperature) - 1.0/(273.15 + 25.0));
		double LogHT = LogH25 - ((1e3 * PARAMETER(AirWaterPhaseTranspherEnthalpy) + R*(273.15 + 25.0)) / (std::log(10.0)*R) )  * tdiff;
		return std::exp(LogHT);
	)
	
	EQUATION(Model, AirWaterPartitioningCoefficient,
		double R = 8.314; //Ideal gas constant (J K^-1 mol^-1)
		return RESULT(HenrysConstant) / (R * RESULT(SoilTemperature));
	)
	
	EQUATION(Model, OctanolWaterPartitioningCoefficient,
		double LogKOW25 = std::log(PARAMETER(OctanolWaterPartitioningCoefficient25));
		double R = 8.314; //Ideal gas constant (J K^-1 mol^-1)
		double tdiff = (1.0 / RESULT(SoilTemperature) - 1.0/(273.15 + 25.0));
		double LogKOWT = LogKOW25 - (1e3*PARAMETER(OctanolWaterPhaseTranspherEntalphy) / (std::log(10.0)*R))   * tdiff;
		return std::exp(LogKOWT);
	)
	
	EQUATION(Model, WaterSOCPartitioningCoefficient,
		double densityofSOC = 1900.0; // kg/m^3
		double rOC = 0.41;   // Empirical constant.
		return RESULT(OctanolWaterPartitioningCoefficient) * rOC / densityofSOC;
	)
	
	EQUATION(Model, WaterDOCPartitioningCoefficient,
		double densityofDOC = 1100.0; // kg/m^3
		return std::pow(10.0, 0.93*RESULT(OctanolWaterPartitioningCoefficient) - 0.45) / densityofDOC;
	)
	
	EQUATION(Model, SoilWaterContaminantConcentration,
		return 
			RESULT(ContaminantMassInSoil) /
			(
			  RESULT(WaterSOCPartitioningCoefficient) * PARAMETER(SOCMassInSoil)
			+ RESULT(WaterDOCPartitioningCoefficient) * RESULT(DOCMassInSoil)
			+ RESULT(AirWaterPartitioningCoefficient) * RESULT(SoilAirVolume)
			+ RESULT(SoilWaterVolume)
			);
	)
	
	EQUATION(Model, SoilSOCContaminantConcentration,
		return RESULT(SoilWaterContaminantConcentration) * RESULT(WaterSOCPartitioningCoefficient);
	)
	
	EQUATION(Model, SoilDOCContaminantConcentration,
		return RESULT(SoilWaterContaminantConcentration) * RESULT(SoilDOCContaminantConcentration);
	)
	
	EQUATION(Model, SoilAirContaminantConcentration,
		return RESULT(SoilWaterContaminantConcentration) * RESULT(AirWaterPartitioningCoefficient);
	)
	
	EQUATION(Model, ContaminantMassInSoil,
		//inputs minus fluxes...
	
	)
}