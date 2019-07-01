
// #define MOBIUS_SOLVER_FUNCTION(Name) void Name(double h, size_t n, double* x0, double* wk, const mobius_solver_equation_function &EquationFunction, const mobius_solver_equation_function &JacobiFunction, double AbsErr, double RelErr)

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
		
		EquationFunction(x0, wk);
		
		for(u32 Idx = 0; Idx < n; ++Idx)
		{
			x0[Idx] += use_h*wk[Idx];
		}
		
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
	
	double x, hmin, xs, hs, q, h3, r, e;
    int ib1, ib2, sw, i, j, ijk0, ijk1, ijk2, be, bh=1, br=1, bx=1;

	//NOTE: Substituting a = 0.0, b = 1.0
/*
    if ( a == b )
    {
        for (i=0; i<n; i++)
        {
            x0[i] = 0.0;
        }

        return;
    }
*/

    ib1 = n + n;
    ib2 = ib1 + n;
    //hmin = 0.01 * fabs( h );
	hmin = 0.01 * h;

/*
    h = SIGN( fabs( h ), ( b - a ) );
    x = double( a );
*/
	x = 0.0;

    while ( br )
    {
        xs = x;

        for (j=0; j<(int)n; j++)
        {
            ijk0 = n + j;
            wk[ijk0] = x0[j];
        }

FT:     hs = h;
        //q = x + h - b;
		q = x + h - 1.0;
        be = 1;

        if (!((h > 0.0 && q >= 0.0) || (h < 0.0 && q <= 0.0))) goto TT;

        //h = b - x;
		h = 1.0 - x;
        br = 0;
TT:     h3 = h / 3.0;

        for (sw=0; sw<5; sw++)
        {

            EquationFunction( x0, wk );

            for (i=0; i<(int)n; i++)
            {
                q = h3 * wk[i];
                ijk0 = n + i;
                ijk1 = ib1 + i;
                ijk2 = ib2 + i;

                switch( sw )
                {
                    case 0  :   r = q;
                                wk[ijk1] = q;
                                break;

                    case 1  :   r = 0.5 * ( q + wk[ijk1] );
                                break;

                    case 2  :   r = 3.0 * q;
                                wk[ijk2] = r;
                                r= 0.375 * ( r + wk[ijk1] );
                                break;

                    case 3  :   r = wk[ijk1] + 4.0 * q;
                                wk[ijk1] = r;
                                r = 1.5 * ( r - wk[ijk2] );
                                break;

                    case 4  :   r = 0.5 * ( q + wk[ijk1] );
                                q = fabs( r + r - 1.5 * ( q + wk[ijk2] ) );
                                break;
                }

                x0[i] = wk[ijk0] + r;

                if ( sw == 4 )
                {
                    e = fabs( x0[i] );
                    r = 0.0005;

                    if ( e >= 0.001 ) r = e * 0.0005;
                    if ( q < r || !bx ) goto SXYFV;

                    br = 1;
                    bh = 0;
                    h = 0.5 * h;

                    if ( fabs(h) < hmin )
                    {
						#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))
                        h = SIGN( hmin, h ); //NOTE: This is probably unnecessary since we are never solving backwards?? -MDN
						#undef SIGN
                        bx = 0;
                    }

                    for (j=0; j<(int)n; j++)
                    {
                        ijk0 = n + j;
                        x0[j] = wk[ijk0];
                    }

                    x = xs;
                    goto FT;

SXYFV:	            if ( q >= ( 0.03125 * r ) ) be=0;
                }
            }

            if ( sw == 0 ) x = x + h3;
            if ( sw == 2 ) x = x + 0.5 * h3;
            if ( sw == 3 ) x = x + 0.5 * h;
        }

        if ( be && bh && br )
        {
            h = h + h;
            bx = 1;
        }

        bh = 1;
    }

    h = hs;

    if ( bx || be )
    {
        return;
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
