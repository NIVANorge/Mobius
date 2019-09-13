

//NÒTE: This module is based on the formulas given in "Crop evapotranspiration - Guidelines for computing crop water requirements - FAO Irrigation and drainage paper 56" http://www.fao.org/3/X0490E/x0490e07.htm


inline double
DailyExtraTerrestrialRadiation(double Latitude, s32 DayOfYear)
{
	const double SolarConstant = 0.0820;
	
	double InverseRelativeDistanceEarthSun = 1.0 + 0.033*cos(2.0*Pi*(double)DayOfYear / 365.0);
	
	double SolarDecimation = 0.409*sin(2.0*Pi*(double)DayOfYear / 365.0 - 1.39);
	
	double LatitudeRad = Latitude * Pi / 180.0;
	
	double SunsetHourAngle = acos(-tan(LatitudeRad)*tan(SolarDecimation));
	
	return (24.0 * 60.0 / Pi) * SolarConstant * InverseRelativeDistanceEarthSun * (SunsetHourAngle * sin(LatitudeRad) * sin(SolarDecimation) + cos(LatitudeRad) * cos(SolarDecimation) * sin(SunsetHourAngle));
}

static void
AddMaxSolarRadiationModule(mobius_model *Model)
{
	auto Degrees        = RegisterUnit(Model, "°");
	auto MJPerM2PerDay  = RegisterUnit(Model, "MJ/M2/day");
	
	auto System = GetParameterGroupHandle(Model, "System");
	
	auto Latitude  = RegisterParameterDouble(Model, System, "Latitude", Degrees, 60.0, -90.0, 90.0);
	
	auto SolarRadiationMax = RegisterEquation(Model, "Solar radiation on a clear sky day", MJPerM2PerDay);
	
	EQUATION(Model, SolarRadiationMax,
		double latitude = PARAMETER(Latitude);
	
		s32 DOY = (s32)CURRENT_DAY_OF_YEAR();
		return DailyExtraTerrestrialRadiation(latitude, DOY);	
	)
}

static void
AddSolarRadiationModule(mobius_model *Model)
{
	AddMaxSolarRadiationModule(Model);
	
	//NOTE: This module is made primarily for use in INCA-C and INCA-P
	
	auto WPerM2         = RegisterUnit(Model, "W/m^2");
	auto Metres         = RegisterUnit(Model, "m");
	
	auto SolarRadiationTimeseries = RegisterInput(Model, "Solar radiation", WPerM2);
	
	auto System    = GetParameterGroupHandle(Model, "System");
	auto Elevation = RegisterParameterDouble(Model, System, "Elevation", Metres, 0.0, 0.0, 8848.0);
	
	auto SolarRadiationMax = GetEquationHandle(Model, "Solar radiation on a clear sky day");
	auto SolarRadiation    = RegisterEquation(Model,  "Solar radiation", WPerM2);
	
	EQUATION(Model, SolarRadiation,
	
		double elevation = PARAMETER(Elevation);
		
		double DETR = RESULT(SolarRadiationMax);
		
		//Short-wave radiation  //TODO: Estimate cloud cover based on some kind of proxy?
		double SWR  = (0.75 + 2e-5*elevation)*DETR;
		
		double sradin = INPUT(SolarRadiationTimeseries);
		
		if(INPUT_WAS_PROVIDED(SolarRadiationTimeseries)) return sradin;
			
		return SWR * 11.5740741;  //NOTE: Converting MJ/m2/day to W/m2
	)
}
