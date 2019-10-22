# INCA-P

INCA-P is a Mobius continuation of the INtegrated CAtchment model for phosphorous dynamics, previously coded by Dan Butterfield.

INCA-P simulates transport and reaction of phosphorous through soil and groundwater and into (a possibly branched) river system.

INCA-P was originally developed by Andrew Wade and Paul Whitehead in 2002 (Wade et. al. 2002), using the semi-distributed conceptual framework of INCA-N. Many others later contributed to the model structure, such as the incorporation of the INCA-Sed sediment module to transport particulate P to the stream. The final version of the original implementation of INCA-P was fully described in Jackson-Blake et. al. 2016.

![alt text](../../Documentation/img/incap.png "Illustration of INCA-P processes taken from (Jackson-Blake et. al. 2016)")


## Difference from earlier versions

The Mobius version of INCA-P is different from earlier versions in that it is integrated with the PERSiST hydrology model (Futter et. al. 2014). This saves the user from running a separate hydrology model first and then port the results of that over to a format readable by INCA. A description of the PERSiST hydrology model can be found here: [PERSiST paper](https://pdfs.semanticscholar.org/2e46/db20c4f6dfa1bcdb45f071ce784cc5a6a873.pdf)

This means however that the hydrology behaves slightly differently, which mostly impacts erosion and suspended sediment generation, but also soil concentrations. Earlier parameter setups of the model can not be reused and have to be recalibrated.

Apart from that, Mobius INCA-P has mostly the same features as the original INCA-P. If you find something is missing, please contact us.

Some known small differences are:
- Wet and dry depositions are currently not spread out with precipitation, but are instead delivered as constant daily doses (or following an input timeseries if that is provided).
- Land cover that changes over time is not implemented until we find a way to do it that preserves stored quantities (but there may be workarounds. Take contact if you need this).

The version numbers found in this repository refer to the version number of the Mobius implementation of the model only, and is not compatible with earlier version numbers.

## Completely new users

Unfortunately there is no comprehensive full description of this version of the model at the moment, but you can piece together the essentials from the listed papers, and also see the [module source code](https://github.com/NIVANorge/Mobius/blob/master/Modules/INCA-P.h), which is written to be as readable as possible.


## Parameter and input formats

See the [general description of parameter and input formats in Mobius](https://github.com/NIVANorge/Mobius/blob/master/Documentation/file_format_documentation.pdf).

See also a short description of [input requirements to PERSiST](https://github.com/NIVANorge/Mobius/tree/master/Documentation/ModelInputRequirements).

## Existing application examples

- The *Tarland* example is very roughly calibrated on reach flow, suspended sediment concentration, reach PP concentration and reach TDP concentration.


## References

A. J. Wade, P. G. Whitehead and D. Butterfield, 2002, *The integrated catchment model of phosporous dynamics (INCA-P), a new approach for multiple source assessment in heterogeneous river systems: model structure and equations*, Hydrol Earth. Syst. Sci. 6, 583-606.

L. A. Jackson-Blake, A. J. Wade, M. N. Futter, D. Butterfield, R.-M. Couture, B. A. Cox, J. Crossman, P. Ekholm, S. J. Halliday, L. Jin, D. S. L. Lawrence, A. Lepistö, Y. Lin, K. Rankinen and P. G. Whitehead, 2016, *The INtegrated CAtchment model of phosphorous dynamics (INCA-P): Description and demonstration of new model structure and equations*, Environmental Modelling & Software, 83, 356-386.

M. N. Futter, M. A. Erlandsson, D. Butterfield, P. G. Whitehead, S. K. Oni, and A. J. Wade, 2014, *PERSiST: a flexible rainfall-runoff modelling toolkit for use with the INCA family of models*, Hydrol. Earth Syst. Sci., 18, 855-873
