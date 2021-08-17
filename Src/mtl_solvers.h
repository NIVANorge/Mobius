

#if !defined(MTL_SOLVERS_H)

#include <boost/numeric/odeint.hpp>
#include <boost/phoenix/phoenix.hpp>
#include <boost/numeric/mtl/mtl.hpp>
#include <boost/numeric/odeint/external/mtl4/implicit_euler_mtl4.hpp>


typedef mtl::dense_vector< double > vec_mtl4;
typedef mtl::compressed2D< double > mat_mtl4;

struct mtl4_ode_system
{	
	const mobius_solver_equation_function &EquationFunction;
	
	mtl4_ode_system(const mobius_solver_equation_function &EquationFunction) : EquationFunction(EquationFunction)
	{
	}
	
	void operator()( const vec_mtl4 &X , vec_mtl4 &DXDT , double /* t */ )
    {
        EquationFunction((double *)X.data, (double *)DXDT.data);
    }
};

struct mtl4_ode_system_jacobi
{
	const mobius_solver_jacobi_function &JacobiFunction;
	
	mtl4_ode_system_jacobi(const mobius_solver_jacobi_function &JacobiFunction) : JacobiFunction(JacobiFunction)
	{
	}
	
    void operator()( const vec_mtl4 &X , mat_mtl4 &J , const double & /* t */ )
    {
        mtl::mat::inserter<mat_mtl4> Ins(J);

		mobius_matrix_insertion_function MatrixInserter = [&](size_t Row, size_t Col, double Value) { Ins[Row][Col] = Value; };
		
        JacobiFunction((double *)X.data, MatrixInserter);
    }
};



MOBIUS_SOLVER_FUNCTION(Mtl4ImplicitEulerImpl_)
{
	using namespace boost::numeric::odeint;
	
	//It is a little stupid that we have to copy the state back and forth, but it seems like we can't create a ublas vector that has an existing pointer as its data (or correct me if I'm wrong!)
	vec_mtl4 X(n);
	for(size_t Idx = 0; Idx < n; ++Idx)
	{
		X[Idx] = x0[Idx];
	}
	
	size_t NSteps = integrate_const(
        implicit_euler_mtl4<double>() ,
        std::make_pair( mtl4_ode_system(EquationFunction) , mtl4_ode_system_jacobi(JacobiFunction) ) ,
        X, 0.0, 1.0, h);
			
	//std::cout << "N steps : " << NSteps << std::endl;
	
	for(size_t Idx = 0; Idx < n; ++Idx)
	{
		x0[Idx] = X[Idx];
	}
}

MOBIUS_SOLVER_SETUP_FUNCTION(Mtl4ImplicitEuler)
{
	SolverSpec->SolverFunction = Mtl4ImplicitEulerImpl_;
	SolverSpec->UsesJacobian = true;
	SolverSpec->UsesErrorControl = false;
}

#define MTL_SOLVERS_H
#endif