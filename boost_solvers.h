
//#define MOBIUS_SOLVER_FUNCTION(Name) void Name(double h, size_t n, double* x0, double* wk, model_run_state *RunState, equation_batch *Batch, double AbsErr, double RelErr)


/*

	There are a lot of different solvers int the boost::numeric::odeint package. We have not wrapped all of them here. If you want one you could ask for it to be included here, or just do it yourself, which should be pretty easy.

*/


#if !defined(BOOST_SOLVERS_H)

#include <boost/numeric/odeint.hpp>

typedef boost::numeric::ublas::vector< double > vec_boost;
typedef boost::numeric::ublas::matrix< double > mat_boost;

struct boost_ode_system
{
	model_run_state *RunState;
	const equation_batch *Batch;
	
	boost_ode_system(model_run_state *RunState, const equation_batch *Batch) : RunState(RunState), Batch(Batch)
	{
	}
	
	void operator()( const vec_boost &X , vec_boost &DXDT , double /* t */ )
    {
        ODEEquationFunction((double *)X.data().begin(), (double *)DXDT.data().begin(), RunState, Batch);
    }
};

struct boost_ode_system_jacobi
{
	model_run_state *RunState;
	const equation_batch *Batch;
	
	boost_ode_system_jacobi(model_run_state *RunState, const equation_batch *Batch) : RunState(RunState), Batch(Batch)
	{
	}
	
	void operator()(const vec_boost &X, mat_boost &J, const double & /* t */ , vec_boost /* &DFDT */ )
    {
		//NOTE: We are banking on not having to clear DFDT each time. We assume it is inputed as 0 from the solver.. However I don't know if this is documented functionality
		
		J.clear(); //NOTE: Unfortunately it seems like J contains garbage values at the start of each run unless we clear it. And we have to clear it since we only set the nonzero values in the JacobiEstimation.
		
		//TODO: it is inefficient to do it this way, it is probably better to have the EstimateJacobian function write the matrix in its own format, then copy the matrix over to the format used by the specific solver? The problem with that is if we use a sparse matrix format, though.
		mobius_matrix_insertion_function MatrixInserter = [&](size_t Row, size_t Col, double Value){ J(Row, Col) = Value; };
		
		EstimateJacobian((double *)X.data().begin(), MatrixInserter, RunState, Batch);
	}
};

MOBIUS_SOLVER_FUNCTION(BoostRosenbrock4Impl_)
{
	using namespace boost::numeric::odeint;
	
	//It is a little stupid that we have to copy the state back and forth, but it seems like we can't create a ublas vector that has an existing pointer as its data (or correct me if I'm wrong!)
	vec_boost X(n);
	for(size_t Idx = 0; Idx < n; ++Idx)
	{
		X[Idx] = x0[Idx];
	}

	try
	{
	size_t NSteps = integrate_adaptive( 
			make_controlled< rosenbrock4< double > >(AbsErr, RelErr),
			std::make_pair(boost_ode_system(RunState, Batch), boost_ode_system_jacobi(RunState, Batch)),
			X, 0.0 , 1.0 , h 
			/*TODO: add an observer to handle errors? */);
	}
	catch(...)
	{
		//TODO: Better handling to get a proper description of the error?
		FatalError("ERROR: An error occurred in the Rosenbrock4 solver.\n");
	}
	//std::cout << "N steps : " << NSteps << std::endl;
	
	for(size_t Idx = 0; Idx < n; ++Idx)
	{
		x0[Idx] = X[Idx];
	}
}

MOBIUS_SOLVER_SETUP_FUNCTION(BoostRosenbrock4)
{
	SolverSpec->SolverFunction = BoostRosenbrock4Impl_;
	SolverSpec->UsesJacobian = true;
	SolverSpec->UsesErrorControl = true;
}



MOBIUS_SOLVER_FUNCTION(BoostRK4Impl_)
{
	using namespace boost::numeric::odeint;
	vec_boost X(n);
	for(size_t Idx = 0; Idx < n; ++Idx)
	{
		X[Idx] = x0[Idx];
	}

	size_t NSteps = integrate_adaptive( 
			runge_kutta4<vec_boost>(),
			boost_ode_system(RunState, Batch),
			X, 0.0 , 1.0 , h 
			/*TODO: add an observer to handle errors? */);
			
	//std::cout << "N steps : " << NSteps << std::endl;
	
	for(size_t Idx = 0; Idx < n; ++Idx)
	{
		x0[Idx] = X[Idx];
	}
}

MOBIUS_SOLVER_SETUP_FUNCTION(BoostRK4)
{
	SolverSpec->SolverFunction = BoostRK4Impl_;
	SolverSpec->UsesJacobian = false;
	SolverSpec->UsesErrorControl = false;
}


MOBIUS_SOLVER_FUNCTION(BoostCashCarp54Impl_)
{
	using namespace boost::numeric::odeint;
	vec_boost X(n);
	for(size_t Idx = 0; Idx < n; ++Idx)
	{
		X[Idx] = x0[Idx];
	}

	size_t NSteps = integrate_adaptive( 
			controlled_runge_kutta<runge_kutta_cash_karp54<vec_boost>>(AbsErr, RelErr),
			boost_ode_system(RunState, Batch),
			X, 0.0 , 1.0 , h 
			/*TODO: add an observer to handle errors? */);
			
	//std::cout << "N steps : " << NSteps << std::endl;
	
	for(size_t Idx = 0; Idx < n; ++Idx)
	{
		x0[Idx] = X[Idx];
	}
}

MOBIUS_SOLVER_SETUP_FUNCTION(BoostCashCarp54)
{
	SolverSpec->SolverFunction = BoostCashCarp54Impl_;
	SolverSpec->UsesJacobian = false;
	SolverSpec->UsesErrorControl = true;
}

#define BOOST_SOLVERS_H
#endif