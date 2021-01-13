# PERSiST

This is a Mobius adaptation of original software coded by Dan Butterfield.

PERSiST (the Precipitation, Evapotranspiration and Runoff Simulator for Solute Transport) is described in the [PERSiST paper](https://pdfs.semanticscholar.org/2e46/db20c4f6dfa1bcdb45f071ce784cc5a6a873.pdf) (Futter et. al. 2014)

The model is a flexible, semi-distributed landscape-scale rainfall-runoff modelling toolkit suitable for simulating a broad range of user-specified perceptual models of runoff generation and stream flow occurring in different climatic regions and landscape types. (Futter et. al. 2014)

![alt text](../../Documentation/img/persist.png "Figure taken from (Futter et. al. 2014)")

Conceptual representation of the landscape in PERSiST adapted from Wate et. al. (2002). A watershed (level 1) is represented as one or more reach/subcatchments (level 2). Within each subcatchment, there are one ore more hydrologic response ynits (level 3). Each hydrologic response unit is made up of one or more buckets through which water is routed (level 4). (Futter et. al. 2014)

![alt text](../../Documentation/img/persist2.png "Figure taken from (Futter et. al. 2014)")

Simple hydrologic response unit comprised of three buckets, representing direct runoff, soil water and groundwater (Futter et. al. 2014)

## Difference from earlier versions

This version does not simulate inundation or infiltration from the stream to land.

Change in version 1.3: Saturation excess input is added to the quick box after percolation to other boxes is considered. This is to avoid movement of water back and forth between boxes.

The version numbers found in this repository refer to the version number of the Mobius implementation of the model only, and is not compatible with earlier version numbers.


## Parameter and input formats

See the [general description of parameter and input formats in Mobius](https://github.com/NIVANorge/Mobius/blob/master/Documentation/file_format_documentation.pdf).

Required inputs have to have a value for every day of the period you intend to run the model. If you don't have values for all days, you have do use interpolation methods to fill in the missing values yourself. How to do this in a smart way is a big topic on its own, and will not be covered here.

* **"Air temperature"**. The air temperature is the mean daily air temperature in degrees C. Recommended index set dependencies: none or {"Reaches"}.
* **"Actual precipitation"**. The actual precipitation is the amount of measured precipitation in mm/day. Recommended index set dependencies: none or {"Reaches"}.

### Optional inputs

If an optional input is provided at all it has to have to have a value for every day of the period you intend to run the model. If it is not included, a value is typically computed for it instead based on parameter values. If you give an optional input an index set dependency, it is fine if you only provide a timeseries for some of the indexes. The value for the other indexes will then be computed as normal.


* **"Abstraction flow"**. How much water is removed from the reach by human activity, in m^3/s. Replaces the parameter of the same name. Recommended index set dependencies: none or {"Reaches"}.
* **"Effluent flow"**. How much water is added to the reach e.g. by sewer treatment works, in m^3/s. Replaces the parameter of the same name. Recommended index set dependencies: none or {"Reaches"}.



## References

M. N. Futter, M. A. Erlandsson, D. Butterfield, P. G. Whitehead, S. K. Oni, and A. J. Wade, 2014, PERSiST: a flexible rainfall-runoff modelling toolkit for use with the INCA family of models, Hydrol. Earth Syst. Sci., 18, 855-873
