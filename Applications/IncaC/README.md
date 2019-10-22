# INCA-C

INCA-C is a Mobius continuation of the INtegrated CAtchment model for Carbon dynamics, previously coded by Dan Butterfield.

INCA-C simulates sorption, desorption and mineralisation of dissolved organic carbon (DOC) in the soil, along with transport between different soil layers and into a (possibly branched) river network.

INCA-C was originally developed by Martyn Futter and others in 2007 (Futter et. al. 2007). It later received contributions by Heleen De Wit and others, adding impacts of acidification on soil carbon processes. Moreover, solid organic carbon was divided into an easily accessible and a non-accessible fraction.

INCA-C follows the same conceptual framework as INCA-N, and was the first landscape-scale model of catchment carbon processing suitable for predicting long-term changes in surface water DOC concentrations in forested, temperate and boreal environments.

![alt text](../../Documentation/img/incac.png "Illustration of paths taken by carbon in INCA-C (Futter et. al. 2007)")


## Difference from earlier versions

The Mobius version of INCA-C is different from earlier versions in that it is integrated with the PERSiST hydrology model (Futter et. al. 2014). This saves the user from running a separate hydrology model first and then port the results of that over to a format readable by INCA. A description of the PERSiST hydrology model can be found here: [PERSiST paper](https://pdfs.semanticscholar.org/2e46/db20c4f6dfa1bcdb45f071ce784cc5a6a873.pdf)


Apart from that, Mobius INCA-C has mostly the same features as the final version of the original INCA-C.

Some known small differences are:
- We don't model PDC (non-dissolved carbon objects floating in the direct runoff pathways and the river)
- We lump all carbon inputs to the soil into the category 'litterfall'.

The version numbers found in this repository refer to the version number of the Mobius implementation of the model only, and is not compatible with earlier version numbers.

## Completely new users

Unfortunately there is no comprehensive full description of this version of the model at the moment, but you can piece together the essentials from the listed papers, and also see the [module source code](https://github.com/NIVANorge/Mobius/blob/master/Modules/INCA-C.h), which is written to be as readable as possible.


## Parameter and input formats

See the [general description of parameter and input formats in Mobius](https://github.com/NIVANorge/Mobius/blob/master/Documentation/file_format_documentation.pdf).

See also a short description of [input requirements to PERSiST](https://github.com/NIVANorge/Mobius/tree/master/Documentation/ModelInputRequirements).

## Existing application examples

None of the provided example datasets are correctly calibrated at the moment.
- The *Storelva* dataset may be very difficult to correctly model using just a catchment model due to the presence of a large lake that dominates the DOC concentrations in the river.
- The *Langtjern* and *Storgama* datasets may be more approachable, but due to the small size of the catchments, irregularities make it difficult to get a very good model fit.

## References

M. N. Futter, D. Butterfield, B. J. Cosby, P. J. Dillon, A. J. Wade, and P. G. Whitehead, 2007, *Modelling the mechanism that control in-stream dissolved organic carbon dynamics in upland and forested catchments*, Water resources research, 43

M. N. Futter, M. A. Erlandsson, D. Butterfield, P. G. Whitehead, S. K. Oni, and A. J. Wade, 2014, *PERSiST: a flexible rainfall-runoff modelling toolkit for use with the INCA family of models*, Hydrol. Earth Syst. Sci., 18, 855-873
