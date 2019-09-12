# Features it would be nice to have in a new major update

## Module system
- Modules should be more well-defined. They should have their own version numbers (instead of the version number being tied to the application), and each model entity (parameter, input, equation, etc.) should know what module they are tied to.
- Expose such data for use in organization e.g. in MobiView.
- There should be an option for run-time choice of module (i.e. what PET module to use for a specific model run).
- When accessing a timeseries from another module, one should not have to specify if it is an input timeseries or an equation timeseries.
- Parameter group system to be tied to the module system.
- Index set dependencies to be decoupled from parameter group system? Or at least, get rid of sub-group system and allow parameter groups to have multiple index set dependencies directly.


## Syntax
It could be nice to have registration syntax be something like
```
Model.RegisterParameter(Land, "Baseflow index", Dimensionless, 0.0, 0.0, 1.0); //Type auto-infered from type of default value
```
or even
```
auto IncaNModule = Model.RegisterModule("INCA-N", "1.0");
auto Land = IncaNModule->RegisterParameterGroup("Land", Reaches, LandscapeUnits);
auto BFI  = Land.RegisterParameter("Baseflow index", Dimensionless, 0.0, 0.0, 1.0);
```
(would be a lot of work rewriting all the existing modules and documentation though)
