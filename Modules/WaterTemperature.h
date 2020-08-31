

static void
AddWaterTemperatureModel(mobius_model *Model)
{
	BeginModule(Model, "INCA water temperature", "1.0");
	
	SetModuleDescription(Model, R""""(
This is a very simple river water temperature model based on a time-lag behind air temperature.
)"""");
	
	auto Dimensionless 				= RegisterUnit(Model);
	auto DegreesCelsius 			= RegisterUnit(Model, "°C");
	
	auto Reach = RegisterIndexSetBranched(Model, "Reaches");
	
	auto WaterTemp = RegisterParameterGroup(Model, "Water temperature", Reach);
	
	
	auto InitialWaterTemperature     = RegisterParameterDouble(Model, WaterTemp, "Initial water temperature",    DegreesCelsius, 20.0, 0.0, 40.0);
	auto MinimumWaterTemperature     = RegisterParameterDouble(Model, WaterTemp, "Minimum water temperature",    DegreesCelsius,  0.0, -5.0, 40.0);
	auto WaterTemperatureLagFactor   = RegisterParameterDouble(Model, WaterTemp, "Water temperature lag factor", Dimensionless,   3.0, 1.0, 10.0);
	
	auto AirTemperature = RegisterInput(Model, "Air temperature", DegreesCelsius);
	
	auto WaterTemperature = RegisterEquation(Model, "Water temperature", DegreesCelsius);
	SetInitialValue(Model, WaterTemperature, InitialWaterTemperature);
	
	EQUATION(Model, WaterTemperature,
	
		return 
		  ((PARAMETER(WaterTemperatureLagFactor) - 1.0) / PARAMETER(WaterTemperatureLagFactor)) * LAST_RESULT(WaterTemperature)
		+ (1.0 / PARAMETER(WaterTemperatureLagFactor)) * Max( PARAMETER(MinimumWaterTemperature), INPUT(AirTemperature) );
	)
	
	EndModule(Model);
}