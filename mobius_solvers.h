
//#define MOBIUS_SOLVER_FUNCTION(Name) void Name(double h, size_t n, double* x0, double* wk, model_run_state *RunState, equation_batch *Batch, double AbsErr, double RelErr)

#if !defined(MOBIUS_SOLVERS_H)

MOBIUS_SOLVER_FUNCTION(MobiusEulerImpl_)
{
	//NOTE: This is not meant to be used as a proper solver, it is just an illustration of how a solver function works.
	
	double haccum = 0.0;
	while(true)
	{
		double hleft = 1.0 - haccum;
		double use_h = h;
		bool Done = false;
		if(h >= hleft)
		{
			use_h = hleft;
			Done = true;
		}
		
		ODEEquationFunction(x0, wk, RunState, Batch);
		
		for(u32 Idx = 0; Idx < n; ++Idx)
			x0[Idx] += use_h*wk[Idx];
		
		if(Done) break;
		
		haccum += use_h;
	}
}

MOBIUS_SOLVER_SETUP_FUNCTION(MobiusEuler)
{
	SolverSpec->SolverFunction = MobiusEulerImpl_;
	SolverSpec->UsesJacobian = false;
	SolverSpec->UsesErrorControl = false;
}




MOBIUS_SOLVER_FUNCTION(IncaDascruImpl_)
{
	//NOTE: This is the original solver from INCA based on the DASCRU Runge-Kutta 4 solver. See also
	// Rational Runge-Kutta Methods for Solving Systems of Ordinary Differential Equations, Computing 20, 333-342.

	double hmin = 0.01 * h;   //NOTE: The solver is only allowed to adjust the step length h to be 1/100 of the desired value, not smaller.

	double t = 0.0;           // 0 <= t <= 1 is the time progress of the solver.
	
	// Divide up "workspaces" for equation values.
	double *wk0 = wk + n;
	double *wk1 = wk0 + n;
	double *wk2 = wk1 + n;

	bool Continue = true;
	
    while(Continue)
    {
        double t_backup = t;
		bool StepWasReduced = false;
		bool StepCanBeReduced = true;

		for(size_t EqIdx = 0; EqIdx < n; ++EqIdx)
			wk0[EqIdx] = x0[EqIdx];

FT:
		
        bool StepCanBeIncreased = true;

		if (h + t > 1.0)
		{
			h = 1.0 - t;
			Continue = false;
		}
		
		for(int SubStep = 0; SubStep < 5; SubStep++)  // TODO: I really want to unroll this loop!
		{
			//NOTE: The ODEEquationFunction computes dx/dt at x0 and puts the results in wk.
			ODEEquationFunction(x0, wk, RunState, Batch);
			
			double h3 = h / 3.0;

            for(size_t EqIdx = 0; EqIdx < n; ++EqIdx)
            {
                double dx0 = h3 * wk[EqIdx];
				double dx;

                switch(SubStep)
                {
                    case 0:
						dx = dx0;
						wk1[EqIdx] = dx0;
					break;

                    case 1:
						dx = 0.5 * (dx0 + wk1[EqIdx]);
                    break;

                    case 2:
						dx = 3.0 * dx0;
						wk2[EqIdx] = dx;
						dx = 0.375 * (dx + wk1[EqIdx]);
					break;

                    case 3:
						dx = wk1[EqIdx] + 4.0 * dx0;
						wk1[EqIdx] = dx;
						dx = 1.5*(dx - wk2[EqIdx]);
					break;

                    case 4:
						dx = 0.5 * (dx0 + wk1[EqIdx]);
					break;
                }

                x0[EqIdx] = wk0[EqIdx] + dx;

                if(SubStep == 4)
                {
                    double Tol = 0.0005;
					double abs_x0 = fabs(x0[EqIdx]);
                    if (abs_x0 >= 0.001) Tol = abs_x0 * 0.0005;
					
					double Est = fabs(dx + dx - 1.5 * (dx0 + wk2[EqIdx]));
					
                    if (Est < Tol || !StepCanBeReduced)
					{
						if (Est >= (0.03125 * Tol))
							StepCanBeIncreased = false;
					}
					else
					{
						Continue = true; // If we thought we reached the end of the integration, that may no longer be true since we are reducing the step size.
						StepWasReduced = true;
						
						h = 0.5 * h; // Reduce the step size.

						if(h < hmin)
						{
							h = hmin;
							StepCanBeReduced = false;
						}

						for (size_t Idx = 0; Idx < n; ++Idx)
							x0[Idx] = wk0[Idx];

						t = t_backup;
						
						goto FT;  //TODO: I really don't like this goto, but there is no easy syntax for breaking the outer loop.
					}
                }
            }

            if(SubStep == 0)      t += h3;
            else if(SubStep == 2) t += 0.5 * h3;
            else if(SubStep == 3) t += 0.5 * h;
        }

        if(StepCanBeIncreased && !StepWasReduced && Continue)
        {
            h = h + h;
            StepCanBeReduced = true;
        }
    }
}

MOBIUS_SOLVER_SETUP_FUNCTION(IncaDascru)
{
	SolverSpec->SolverFunction = IncaDascruImpl_;
	SolverSpec->UsesJacobian = false;
	SolverSpec->UsesErrorControl = false; //NOTE: It actually DOES use error control, but the error control is not governed by any externally provided parameters.
}


#define MOBIUS_SOLVERS_H
#endif
