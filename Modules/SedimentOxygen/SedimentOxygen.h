

inline double
Diffusion(double Z, double Zlow, double Zupp, double Dlow, double Dupp, double C, double Clow, double Cupp)
{
	//Computes the term of dC(Z,t)/dt that comes from diffusion.
	
	double Zupphalf = 0.5*(Z + Zupp);
	double Zlowhalf = 0.5*(Z + Zlow);
	
	double Flow = Dlow * (C - Clow) / (Z - Zlow);
	double Fupp = Dupp * (Cupp - C) / (Zupp - Z);
	
	return (Fupp - Flow) / (Zupphalf - Zlowhalf);
}

/*
inline double
Rate(double Z, double Zupp, double Zlow, double Rateupp, double Ratelow)
{
	double Zupphalf = 0.5*(Z + Zupp);
	double Zlowhalf = 0.5*(Z + Zlow);
	
	return (Rateupp*(Zupphalf - Z) + Ratelow*(Z - Zlowhalf))/(Zupphalf - Zlowhalf);
}
*/

inline double
DynamicViscosity(double Temperature, double Salinity, double Pressure)
{
	//Temperature: deg C, Salinity: PSS, Pressure: bar
	// return unit Pa*s
	
	//TODO: Needs salinity correction!
	return 2.414e-5*pow(10.0, 247.8 / (Temperature+273.15 - 140.0));
}


void
AddSedimentOxygenModule(mobius_model *Model)
{
	BeginModule(Model, "Sediment oxygen", "0.0");
	
	
	auto Dimensionless  = RegisterUnit(Model);
	auto PSS            = RegisterUnit(Model, "PSS-78");
	auto DegreesCelsius = RegisterUnit(Model, "°C");
	auto UMolsPerM2PerS = RegisterUnit(Model, "µmol(photons)/m2/s");
	auto PercentAtm     = RegisterUnit(Model, "% atm");
	auto Cm2PerS        = RegisterUnit(Model, "cm2/s");
	auto Cm             = RegisterUnit(Model, "cm");
	auto M              = RegisterUnit(Model, "m");
	auto PerM           = RegisterUnit(Model, "1/m");
	auto Degrees        = RegisterUnit(Model, "°");
	auto WPerM2         = RegisterUnit(Model, "W/m2");
	auto UMolsPerCm2PerS  = RegisterUnit(Model, "µmol(oxygen)/cm2/s");
	auto UMolsPerCm3PerS  = RegisterUnit(Model, "µmol(oxygen)/cm3/s");
	auto UMolsPerUMols  = RegisterUnit(Model, "µmol(oxygen)/µmol(photons)");
	auto UMolsPerCm3    = RegisterUnit(Model, "µmol(oxygen)/cm3");
	auto PerS           = RegisterUnit(Model, "1/s");
	auto Days           = RegisterUnit(Model, "days");
	auto Hours          = RegisterUnit(Model, "hours");
	
	auto SedimentOx = RegisterParameterGroup(Model, "Sediment oxygen");
	
	//TODO: Find good default values:
	auto AtmosphericO2Saturation    = RegisterParameterDouble(Model, SedimentOx, "Atmospheric O2 partial pressure", PercentAtm, 21.0, 0.0, 100.0);
	auto Temperature                = RegisterParameterDouble(Model, SedimentOx, "Temperature", DegreesCelsius, 20.0, -5.0, 40.0);
	auto Salinity                   = RegisterParameterDouble(Model, SedimentOx, "Salinity", PSS, 35.0, 0.0, 1000.0);
	auto Latitude                   = RegisterParameterDouble(Model, SedimentOx, "Latitude", Degrees, 60.0, -90.0, 90.0);
	auto YearLength                 = RegisterParameterDouble(Model, SedimentOx, "Year length", Days, 356.0, 200.0, 500.0);
	auto DayLength                  = RegisterParameterDouble(Model, SedimentOx, "Day length", Hours, 24.0, 15.0, 30.0);
	auto CloudCover                 = RegisterParameterDouble(Model, SedimentOx, "Average cloud cover", Dimensionless, 0.0, 0.0, 1.0);
	
	auto SedimentPorosity           = RegisterParameterDouble(Model, SedimentOx, "Sediment porosity", Dimensionless, 0.5, 0.0, 1.0);
	auto AttenuationCoefficient     = RegisterParameterDouble(Model, SedimentOx, "Attenuation coefficient", PerM, 0.01, 0.0, 0.2);
	auto LuminosityConversionFactor = RegisterParameterDouble(Model, SedimentOx, "Luminosity conversion factor", Dimensionless, 4.1, 0.0, 5.0, "µmol(photons)/s per W (depends on desired PAR spectrum)"); //TODO: Unit
	
	auto MaxProduction              = RegisterParameterDouble(Model, SedimentOx, "Maximum O2 production", UMolsPerCm2PerS, 0.0, 0.0, 1000.0);
	auto MaxLightUtilization        = RegisterParameterDouble(Model, SedimentOx, "Maximum light utilization", UMolsPerUMols, 0.0, 0.0, 10.0);
	
	auto RespirationAt20Degrees     = RegisterParameterDouble(Model, SedimentOx, "Respiration rate at 20°C", PerS, 0.0, 0.0, 0.01);
	auto Q10                        = RegisterParameterDouble(Model, SedimentOx, "Respiration rate response to a 10° change in temperature (Q10)", Dimensionless, 2.0, 1.0, 4.0);
	
	
	auto OceanDepth                 = RegisterParameterDouble(Model, SedimentOx, "Ocean depth", M, 10.0, 0.0, 200.0);
	auto DBLThickness               = RegisterParameterDouble(Model, SedimentOx, "Thickness of diffusive boundary layer", Cm, 5.0, 0.0, 30.0);
	auto ProductionZoneThickness    = RegisterParameterDouble(Model, SedimentOx, "Thickness of production zone", Cm, 5.0, 0.0, 30.0);
	auto NonProductionZoneThickness = RegisterParameterDouble(Model, SedimentOx, "Thickness of non-production zone", Cm, 50.0, 0.0, 200.0);
	
	
	
	
	auto SurfaceSolarRadiation = RegisterEquation(Model, "Surface solar radiation", WPerM2);
	auto BottomSolarRadiation  = RegisterEquation(Model, "Bottom solar radiation", WPerM2);
	auto Luminosity            = RegisterEquation(Model, "Bottom luminosity", UMolsPerM2PerS);
	auto OxygenProductionRate  = RegisterEquation(Model, "Oxygen production rate", UMolsPerCm2PerS);
	auto OxygenRespirationRate = RegisterEquation(Model, "Oxygen respiration rate", PerS);

	auto OxygenDiffusivityInWater = RegisterEquation(Model, "Oxygen diffusivity in water", Cm2PerS);
	
	EQUATION(Model, SurfaceSolarRadiation,
		//Based on standard theory for extraterrestrial radiation, but modified for different length of day and length of year.
		s64 Seconds = CURRENT_TIME().DateTime.SecondsSinceEpoch;
		
		double DayLen = PARAMETER(DayLength)*3600.0; //seconds
		
		double Day = (double)Seconds / DayLen;
		double YearLen = PARAMETER(YearLength);  //days
		
		double Dum;
		double DayAngle = 2.0*Pi*modf(Day / YearLen, &Dum);
		
		double SecondOfDay = modf(Day, &Dum)*DayLen;
		
		//TODO: This is probably not correct at all for the given time period....
		double DeclinationAngle = 0.409*sin(DayAngle - 1.39);
		
		double LocalTime = SecondOfDay / 3600.0;
		
		double SolarTime1 = LocalTime;// + ET/60.0;// + (4.0/60.0)*(PARAMETER(StandardMeridianLocalZone) - PARAMETER(Longitude));   //NOTE: Simplify
		
		double SolarTime2 = SolarTime1 + 1.0;
		
		double SolarHourAngle1 = 15.0*(SolarTime1 - 12.0)*Pi/180.0;
		double SolarHourAngle2 = 15.0*(SolarTime2 - 12.0)*Pi/180.0;
		
		double SolarConstant = 1367.0; // W/m2
		
		//TODO: Is probably also incorrect...
		double InverseRelativeDistanceEarthSun = 1.0 + 0.033*cos(DayAngle);
		
		double lat = PARAMETER(Latitude)*Pi/180.0;
		
		double SRad = (12.0 * SolarConstant * InverseRelativeDistanceEarthSun / Pi)*
		(
		  sin(lat)*sin(DeclinationAngle)*(SolarHourAngle2 - SolarHourAngle1)
		+ cos(lat)*cos(DeclinationAngle)*(sin(SolarHourAngle2) - sin(SolarHourAngle1)) 
		);
		
		SRad = Max(SRad, 0.0);
		
		return SRad * 0.75 * (1.0 - 0.65*PARAMETER(CloudCover)); //Note very simple correction for atmosphere and cloud cover
	)
	
	EQUATION(Model, BottomSolarRadiation,
		return RESULT(SurfaceSolarRadiation)*exp(-PARAMETER(AttenuationCoefficient)*PARAMETER(OceanDepth));
	)
	
	EQUATION(Model, Luminosity,
		return RESULT(BottomSolarRadiation)*PARAMETER(LuminosityConversionFactor);
	)
	

	EQUATION(Model, OxygenProductionRate,
		//In production zone only
		
		return PARAMETER(MaxProduction)*(1.0 - exp(-PARAMETER(MaxLightUtilization)*1e4*RESULT(Luminosity)/PARAMETER(MaxProduction)));
	)
	
	EQUATION(Model, OxygenRespirationRate,
		return PARAMETER(RespirationAt20Degrees)*pow(PARAMETER(Q10), (PARAMETER(Temperature) - 20.0)/10.0);
	)
	
	
	EQUATION(Model, OxygenDiffusivityInWater,
		double RefDiff = 1.57e-5;   //Reference diffusivity at 10C, distilled water
		//TODO: Formula of temperature and salinity
		double Pressure = 1.0; //bar  TODO: Should we correct it for ocean depth?
		return RefDiff * (DynamicViscosity(10.0, 0.0, 1.0) / DynamicViscosity(PARAMETER(Temperature), PARAMETER(Salinity), Pressure))*(PARAMETER(Temperature)+273.15)/283.15;
	)
	
	// z = DLB thickness: Constant ocean conc.
	// z = 0:              Conc at ocean-sediment interface
	// z = -prodzone thickness/2 : Conc in center of production zone
	// z = -prodzone thickness   : Conc at interface between prod. and nonprod. zone
	// z = -(prod + nonprod/2)     : Conc at center of nonprod zone.
	// z = -(prod + nonprod)       : Const 0 conc.
	

	auto SedSolver = RegisterSolver(Model, "Sediment solver", 0.1, IncaDascru);
	
	auto Conc0     = RegisterEquation(Model, "C0 - oxygen conc in ocean", UMolsPerCm3);
	//auto Conc1     = RegisterEquationODE(Model, "C1 - oxygen conc at center of production zone", UMolsPerCm3, SedSolver);
	//auto Conc2     = RegisterEquationODE(Model, "C2 - oxygen conc at boundary between prod and non-prod zones", UMolsPerCm3, SedSolver);
	//auto Conc3     = RegisterEquationODE(Model, "C3 - oxygen conc at center of non-production zone", UMolsPerCm3, SedSolver);
	
	EQUATION(Model, Conc0,
		//NOTE: Garcia & Gordon 92
		
		double A0 = 5.80818;
		double A1 = 3.20684;
		double A2 = 4.11890;
		double A3 = 4.93845;
		double A4 = 1.01567;
		double A5 = 1.41575;
		
		double B0 = -7.01211e-03;
		double B1 = -7.25958e-03;
		double B2 = -7.93334e-03;
		double B3 = -5.54491e-03;
		
		double C = -1.32412e-07;
		
		double Ts = log((298.15 - PARAMETER(Temperature)) / (273.15 + PARAMETER(Temperature)));
		
		double AA = ((((A5*Ts + A4)*Ts + A3)*Ts + A2)*Ts + A1)*Ts + A0;
		double BB = ((B3*Ts + B2)*Ts + B1)* Ts + B0;
		
		double S = PARAMETER(Salinity);
		double O2 = exp(AA + S*(BB + S*C));
		
		//Correct for different atmospheric partial pressure
		O2 = O2 * PARAMETER(AtmosphericO2Saturation) / 21.0;
		
		return O2;
	)

	
	
	#define ConcEq(LevIDX) \
	auto Conc##LevIDX = RegisterEquationODE(Model, "C"#LevIDX , UMolsPerCm3, SedSolver);
	
	ConcEq(1)
	ConcEq(2)
	ConcEq(3)
	ConcEq(4)
	ConcEq(5)
	ConcEq(6)
	ConcEq(7)
	ConcEq(8)
	ConcEq(9)
	ConcEq(10)
	ConcEq(11)
	ConcEq(12)
	ConcEq(13)
	ConcEq(14)
	ConcEq(15)
	ConcEq(16)
	ConcEq(17)
	ConcEq(18)
	ConcEq(19)
	ConcEq(20)
	
	auto Conc21 = RegisterEquation(Model, "C21", UMolsPerCm3);
	EQUATION(Model, Conc21,
		return 0.0;
	)
	
	
	#define LevelEq(LevIDXLow, LevIDX, LevIDXUp) \
	EQUATION(Model, Conc##LevIDX,                                                          \
		double Z    = PARAMETER(DBLThickness) - (double)LevIDX*0.1;                        \
		double Zupp = Z + 0.1;                                                             \
		double Zlow = Z - 0.1;                                                             \
		double Cupp = RESULT(Conc##LevIDXUp);                                              \
		double C    = RESULT(Conc##LevIDX);                                                \
		double Clow = RESULT(Conc##LevIDXLow);                                             \
		double D0   = RESULT(OxygenDiffusivityInWater);                                    \
		double D    = D0*PARAMETER(SedimentPorosity);                                      \
		double Dupp = Zupp > 0.0 ? D : D0;                                                 \
		double Dlow = Zlow > 0.0 ? D : D0;                                                 \
		double diff = Diffusion(Z, Zlow, Zupp, Dlow, Dupp, C, Clow, Cupp);                       \
		double prod = RESULT(OxygenProductionRate)/PARAMETER(ProductionZoneThickness);     \
		double Zprod = PARAMETER(ProductionZoneThickness);                                 \
		if(Z > 0.0 || Z < -PARAMETER(ProductionZoneThickness)) prod = 0.0;                 \
		double resp = RESULT(OxygenRespirationRate)*C;                                     \
		return (diff + prod - resp)*3600.0;                                                \
	)
	
	LevelEq(0, 1, 2)
	LevelEq(1, 2, 3)
	LevelEq(2, 3, 4)
	LevelEq(3, 4, 5)
	LevelEq(4, 5, 6)
	LevelEq(5, 6, 7)
	LevelEq(6, 7, 8)
	LevelEq(7, 8, 9)
	LevelEq(8, 9, 10)
	LevelEq(9, 10, 11)
	LevelEq(10, 11, 12)
	LevelEq(11, 12, 13)
	LevelEq(12, 13, 14)
	LevelEq(13, 14, 15)
	LevelEq(14, 15, 16)
	LevelEq(15, 16, 17)
	LevelEq(16, 17, 18)
	LevelEq(17, 18, 19)
	LevelEq(18, 19, 20)
	LevelEq(19, 20, 21)
	
	
	
/*	
	EQUATION(Model, Conc0,
		double Zupp = PARAMETER(DBLThickness);
		double Z    = 0.0;
		double Zlow = -0.5*PARAMETER(ProductionZoneThickness);
		double Cupp = RESULT(ConcOcean);
		double C = RESULT(Conc0);
		double Clow = RESULT(Conc1);
		double Dupp = RESULT(OxygenDiffusivityInWater);
		double Dlow = Dupp*PARAMETER(SedimentPorosity);
		double diff = Diffusion(Z, Zlow, Zupp, Dlow, Dupp, C, Clow, Cupp);
		double ProdRate = RESULT(OxygenProductionRate)/PARAMETER(ProductionZoneThickness);
		double RespRate = RESULT(OxygenRespirationRate)*C;  //TODO
		double prod = ProdRate * 0.5; //To account for that this is on a boundary between prod. and nonprod. TODO: see if this is really ok!
		double resp = RespRate * 0.5; //See above
		return (diff + prod - resp)*3600.0; // Convert 1/s to 1/h
	)
	
	EQUATION(Model, Conc1,
		double Zupp = 0.0;
		double Z    = -0.5*PARAMETER(ProductionZoneThickness);
		double Zlow = -PARAMETER(ProductionZoneThickness);
		double Cupp = RESULT(Conc0);
		double C    = RESULT(Conc1);
		double Clow = RESULT(Conc2);
		double D    = RESULT(OxygenDiffusivityInWater)*PARAMETER(SedimentPorosity);
		double diff = Diffusion(Z, Zlow, Zupp, D, D, C, Clow, Cupp);
		double prod = RESULT(OxygenProductionRate)/PARAMETER(ProductionZoneThickness);
		double resp = RESULT(OxygenRespirationRate)*C;
		return (diff + prod - resp)*3600.0; // Convert 1/s to 1/h
	)
	
	EQUATION(Model, Conc2,
		double Zupp = -0.5*PARAMETER(ProductionZoneThickness);
		double Z    = -PARAMETER(ProductionZoneThickness);
		double Zlow = -PARAMETER(ProductionZoneThickness) - 0.5*PARAMETER(NonProductionZoneThickness);
		double Cupp = RESULT(Conc1);
		double C    = RESULT(Conc2);
		double Clow = RESULT(Conc3);
		double D    = RESULT(OxygenDiffusivityInWater)*PARAMETER(SedimentPorosity);
		double diff = Diffusion(Z, Zlow, Zupp, D, D, C, Clow, Cupp);
		double ProdRate = RESULT(OxygenProductionRate)/PARAMETER(ProductionZoneThickness);
		double prod = ProdRate*0.5; //Again, on a boundary. TODO: Figure out if ok.
		double resp = RESULT(OxygenRespirationRate)*C;
		return (diff + prod - resp)*3600.0; // Convert 1/s to 1/h
	)
	
	EQUATION(Model, Conc3,
		double Zupp = -PARAMETER(ProductionZoneThickness);
		double Z    = -PARAMETER(ProductionZoneThickness) - 0.5*PARAMETER(NonProductionZoneThickness);
		double Zlow = -PARAMETER(ProductionZoneThickness) - PARAMETER(NonProductionZoneThickness);
		double Cupp = RESULT(Conc2);
		double C    = RESULT(Conc3);
		double Clow = 0.0;
		double D    = RESULT(OxygenDiffusivityInWater)*PARAMETER(SedimentPorosity);
		double diff = Diffusion(Z, Zlow, Zupp, D, D, C, Clow, Cupp);
		double prod = 0.0;
		double resp = RESULT(OxygenRespirationRate)*C;
		return (diff + prod - resp)*3600.0; // Convert 1/s to 1/h
	)
*/
	EndModule(Model);
}