

#if !defined(MOBIUS_MATH_H)

#define Min(A, B) ((A) < (B) ? (A) : (B))
#define Max(A, B) ((A) > (B) ? (A) : (B))
#define Square(A) ((A)*(A))

#define Pi 3.141592653589793238462643383279502884

inline double
LinearInterpolate(double X, double MinX, double MaxX, double MinY, double MaxY)
{
	double XX = (X - MinX) / (MaxX - MinX);
	return MinY + (MaxY - MinY)*XX;
}

inline double
LinearResponse(double X, double MinX, double MaxX, double MinY, double MaxY)
{
	if(X <= MinX) return MinY;
	if(X >= MaxX) return MaxY;
	double XX = (X - MinX) / (MaxX - MinX);
	return MinY + (MaxY - MinY)*XX;
}

inline double
SCurveResponse(double X, double MinX, double MaxX, double MinY, double MaxY)
{
	if(X <= MinX) return MinY;
	if(X >= MaxX) return MaxY;
	double XX = (X - MinX) / (MaxX - MinX);
	double T = (3.0 - 2.0*XX)*XX*XX;
	return MinY + (MaxY - MinY)*T;
}

inline double
InverseGammaResponse(double X, double MinX, double MaxX, double MinY, double MaxY)
{
	const double GammaMinX = 1.461632144968362341262;
	const double GammaMin  = 0.885603194410888;
	
	if(X <= MinX) return MinY;
	
	double XX = (X - MinX) / (MaxX - MinX);
	
	double T = GammaMin / tgamma(XX * GammaMinX);
	
	return MinY + (MaxY - MinY) * T;
}

inline double
SafeDivide(double A, double B)
{
	//NOTE: This function is mostly meant for computing concentration = SafeDivide(mass, watervolume). If the watervolume is 0, it is safe to treat the concentration as 0 as there would be no flow taking the concentration to another compartment anyway. Moreover, all chemical processes involving concentrations are usually turned off if the watervolume is 0.
	
	double Result = A / B;
	if(std::isfinite(Result)) return Result;
	return 0.0;
}

inline double
Clamp01(double A)
{
	if(A < 0.0) return 0.0;
	if(A > 1.0) return 1.0;
	return A;
}


#define MOBIUS_MATH_H
#endif
