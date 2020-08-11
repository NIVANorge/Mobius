# Features it would be nice to have in a new major update

## Module system
- There should be an option for run-time choice of module (i.e. what PET module to use for a specific model run).
- When accessing a timeseries that was declared in another module, one should not be forced to know if it is an input timeseries or an equation timeseries.

## Other
- It would be cleaner just to have a ParameterInt (int64_t) instead of ParameterUInt.
- Built-in system for running sanity checks on parameter values on model startup.
- Should be easier to have variable data availability for some timeseries. So if a timeseries is not provided, it is computed instead. This is possible right now, but is clunky.
- Make it possible to have a batch consist of different equations depending on the index. So e.g. an index can either be a lake or a river segment, and the set of equations for these are different.

## Specific modules
- Factor everything that has to do with solar radiation into one coherent file (from PET.h, EasyLake.h, SolarRadiation.h). Same with air pressure, humidity routines.

## Future
- Actual code generator that generates the model code based on a model description instead of having all equations be lambdas (with std::function call overhead).
