# SimplyP (Mobius version)

This folder contains the applications of the Mobius implemetation of [SimplyP](https://github.com/LeahJB/SimplyP), a parsimonious hydrology, sediment and phosphorus model. The main model definition file is [here](https://github.com/NIVANorge/Mobius/blob/master/Modules/SimplyP.h). The original version of SimplyP was coded in Python and is described here:

> Jackson-Blake LA, Sample JE, Wade AJ, Helliwell RC, Skeffington RA. 2017. *Are our dynamic water quality models too complex? A comparison of a new parsimonious phosphorus model, SimplyP, and INCA-P*. Water Resources Research, **53**, 5382–5399. [doi:10.1002/2016WR020132](http://onlinelibrary.wiley.com/doi/10.1002/2016WR020132/abstract;jsessionid=7E1F1066482B9FFDBC29BA6B5A80042C.f04t01)

Full details of version 0.1 of the model are provided in the [Supplementary Information](https://agupubs.onlinelibrary.wiley.com/action/downloadSupplement?doi=10.1002%2F2016WR020132&file=wrcr22702-sup-0001-2016WR020132-s01.pdf). The Python version of the model (up to v0.2) is still available [here](https://github.com/LeahJB/SimplyP).

The Mobius version offers dramatic performance improvements and increased flexibility, while still making it possible to interact with the model via the [Python wrapper](https://github.com/NIVANorge/Mobius/tree/master/PythonWrapper), as well as the GUI available with all Mobius models. **Further development of SimplyP will likely take place using the Mobius framework**. We have yet to shift the known issues, change log and other documentation across from the original Python repository, so please visit the [original repository](https://github.com/LeahJB/SimplyP) for more information.

## Tutorials

For instructions on getting started with Python, Jupyter notebooks and the python wrapper, see https://github.com/NIVANorge/Mobius/tree/master/PythonWrapper#python-wrapper.

The notebooks used in these tutorials are located within the PythonWrapper/SimplyP folder in the repository, and so will be available for you to work through interactively when you download the Mobius repository. 

 1. **[Auto-calibration and uncertainty estimation](https://nbviewer.jupyter.org/github/NIVANorge/Mobius/blob/master/PythonWrapper/SimplyP/simplyp_calibration.ipynb)**. An example illustrating how to auto-calibrate SimplyP and explore parametric uncertainty using MCMC

**More coming soon!**
