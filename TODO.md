# Features it would be nice to have in a new major update

## Module system
- There should be an option for run-time choice of module (i.e. what PET module to use for a specific model run).
- When accessing a timeseries that was declared in another module, one should not be forced to know if it is an input timeseries or an equation timeseries.

## Other
- It would be cleaner just to have a ParameterInt (int64_t) instead of ParameterUInt.
- Built-in system for running sanity checks on parameter values on model startup.
- See if we could get rid of some of the overhead when calling the equation lambdas as std::function (i.e. maybe make our own function class with fixed static capture storage).

- Make the parameter/input file lexer able to read files with special characters in their names (i.e. utf8 preferrably, otherwise utf16).

## Future
- Actual code generator that generates model code based on model file instead of having all equations be lambdas (with std::function callin overhead).
