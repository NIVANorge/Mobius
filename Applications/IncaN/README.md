# INCA-N

INCA-N is a Mobius continuation of the Integrated Nitrogen CAtchment model, previously coded by Dan Butterfield.

INCA-N was originally developed (under the name INCA) by Paul G. Whitehead in 1995 (Whitehead et. al. 1998). The model later received contributions by Andrew Wade, Kateri Rankinen and others.

INCA-N was an early attempt to assess the impact of point and diffuse nitrogen sources on in-stream chemistry in an integrated manner, simulating soil and stream water flow and nitrate and ammonium concentrations, as well as calculating nitrogen process loads (Whitehead et. al. 1998)

One description of the model can be found in Wade et. al. 2002, though there have been some additions to the model since then.

## Difference from earlier versions

The Mobius version of INCA-N is different from earlier versions in that it is integrated with the PERSiST hydrology model (Futter et. al. 2014). This saves the user from running a separate hydrology model first and then port the results of that over to a format readable by INCA. A description of the PERSiST hydrology model can be found here: [PERSiST paper](https://pdfs.semanticscholar.org/2e46/db20c4f6dfa1bcdb45f071ce784cc5a6a873.pdf)

Other small differences are:
- Wet depositions are currently not spread out with precipitation, but is instead delivered as a constant daily dose (or following an input timeseries if that is provided)
- Changing land cover is not implemented until we find a way to do it that preserves stored quantities (but there may be workarounds. Take contact if you need this).

If you are a previous user of INCA-N, see this [guidance on switching to the Mobius version](https://github.com/NIVANorge/Mobius/blob/master/Applications/IncaN/Guidance_on_switching_to_framework_version.txt)


The version numbers found in this repository refer to the number of the Mobius version of the model only, and is not compatible with earlier version numbers.


## Parameter and input formats

See the [general description of parameter and input formats in Mobius](https://github.com/NIVANorge/Mobius/blob/master/Documentation/file_format_documentation.pdf).

See also a short description of [input requirements to PERSiST and INCA-N](https://github.com/NIVANorge/Mobius/tree/master/Documentation/ModelInputRequirements)

## Existing examples

The *Tarland* example is roughly calibrated on Nitrate and Ammonium concentrations in the reach. The *Storelva* example is calibrated on Nitrate concentration only, but with a fairly good fit. The *Tovdal* example is not calibrated, but can serve as a starting point if you want to have an exercise at calibrating on a multi-reach setup.



## References

P. G. Whitehead, E. J. Wilson, and D. Butterfield, 1998a, *A semi-distributed Integrated Nitrogen Catchment model for multiple source assessment in Catchments (INCA): Part I - model structure and process equations*, The science of the Total Environment, 210/211, 547-558

P. G. Whitehead, E. J. Wilson, and D. Butterfield, 1998b, *A semi-distributed Integrated Nitrogen Catchment model for multiple source assessment in Catchments (INCA): Part II - application to large river basins in south Wales and eastern England*, The science of the Total Environment, 210/211, 559-583

A.J Wade, P. Durand, V. Beaujouan, W. W. Wessel, K. J. Raat, P. G. Whitehead, D. Butterfield, K. Rankinen and A. Lepisto 2002, *A nitrogen model for European catchments: INCA, new model structure and equations*, Hydrology and Earth System Sciences, 6(3), 559-582 (2002)

M. N. Futter, M. A. Erlandsson, D. Butterfield, P. G. Whitehead, S. K. Oni, and A. J. Wade, 2014, *PERSiST: a flexible rainfall-runoff modelling toolkit for use with the INCA family of models*, Hydrol. Earth Syst. Sci., 18, 855-873
