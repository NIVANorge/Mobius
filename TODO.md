# Features it would be nice to have in a new major update

## Module system
- There should be an option for run-time choice of module (i.e. what PET module to use for a specific model run).
- When accessing a timeseries that was declared in another module, one should not be forced to know if it is an input timeseries or an equation timeseries.

## Other
- It would be cleaner just to have a ParameterInt (int64_t) instead of ParameterUInt.
- Maybe remove units as separate model entities, and just have other entities store the string name of their entity. They don't do anything right now.
- Built-in system for running sanity checks on parameter values on model startup.
- Should be easier to have variable data availability for some timeseries. So if a timeseries is not provided, it is computed instead. This is possible right now, but is clunky.
- Better encapsulation of the model_run_state subsystem. Unify lookup systems for parameters, inputs, results, last_results
- Figure out if the initial value equation system we have currently is good. Maybe provide error checking if the order of evaluation of input equations becomes whacky.
- (Even more) convenience accessors for the mobius_data_set so that io and application code does not have to understand the inner structure of the DataSet that much. Or rather combine the accessors with the .dll API?

## Specific modules
- Factor everything that has to do with solar radiation into one coherent file (from PET.h, EasyLake.h, SolarRadiation.h). Same with air pressure, humidity routines.

## Future
- Actual code generator that generates the model code based on a model description instead of having all equations be lambdas (with std::function call overhead).
