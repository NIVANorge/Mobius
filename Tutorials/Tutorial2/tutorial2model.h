
#if !defined(TUTORIAL_TWO_MODEL_H)

//NOTE: We put all of the model definition into a separate file.
// This allows us to separate the model definition from the specific application, and so we can make several applications from the same model.

void AddTutorial2Model(mobius_model *Model)
{
	auto Dimensionless = RegisterUnit(Model);
	auto Kg            = RegisterUnit(Model, "kg");
	
	auto LandscapeUnits = RegisterIndexSet(Model, "Landscape units");
	
	auto Land = RegisterParameterGroup(Model, "Land", LandscapeUnits);
	
	auto A = RegisterParameterDouble(Model, Land, "A", Dimensionless, 1.0);
	auto B = RegisterParameterDouble(Model, Land, "B", Dimensionless, 0.0);
	
	
	auto SoilBoxes = RegisterIndexSet(Model, "Soil boxes");

	auto Soils = RegisterParameterGroup(Model, "Soils", SoilBoxes);
	
	auto C = RegisterParameterDouble(Model, Soils, "C", Dimensionless, 1.3);
	auto ShouldExponentiate = RegisterParameterBool(Model, Soils, "Should exponentiate", false);
	
	
	auto X = RegisterInput(Model, "X");
	
	auto AnotherEquation = RegisterEquation(Model, "Another equation", Dimensionless);
	auto ATimesXPlusB = RegisterEquation(Model, "A times X plus B", Dimensionless);
	
	//NOTE: Even though AnotherEquation is declared before ATimesXPlusB, the framework will figure out that it has to evaluate ATimesXPlusB before it evaluates AnotherEquation since AnotherEquation depends on the result of ATimesXPlusB.
	EQUATION(Model, AnotherEquation,
		
		//NOTE: There is something a little tricky going on here. If you have branches inside your equations you have to make sure that EVERY parameter, input or result you potentially extract the value of in this equation gets extracted regardless of what branch the equation takes. So in this case we can not extract the value of C or ATimesXPlusB inside the if clause, because that could be skipped. This is an unfortunate consequence of how the dependency system for equations gets resolved. We hope to fix this in the future. -MDN.
		
		double prev = RESULT(ATimesXPlusB);
		double cval = PARAMETER(C);
		if(PARAMETER(ShouldExponentiate))
		{
			return pow(prev, cval);
		}
		return prev;
	)
	
	EQUATION(Model, ATimesXPlusB,
		return PARAMETER(A) * INPUT(X) + PARAMETER(B);
	)
}


#define TUTORIAL_TWO_MODEL_H
#endif