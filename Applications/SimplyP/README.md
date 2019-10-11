# SimplyP (Mobius version)

SimplyP is a parsimonious hydrology, sediment and phosphorus model. The model is dynamic, with a daily time step, and is spatially semi-distributed, i.e. there is the ability to include differences in hydrology, sediment and phosphorus processes between land use types and sub-catchments (with associated stream reaches). Key philosophies behind SimplyP development are:

1. Process representation should be as simple as possible, only including those processes that appear to dominate the *catchment-scale* response, whilst maintaining sufficient complexity for the model to be useful in hypothesis and scenario testing.
2. Process representation should be simple enough to allow parameter values to be constrained using available data. This involves keeping the number of parameters requiring calibration to a minimum, and aiming for as many parameters as possible to be in principle measurable, so their values can be based on observed data (either gathered from the study site or literature-based). We aim for it to be possible to include *all* uncertain parameters in uncertainty analysis.

Examples of potential model uses include:

1. Interpolating sparse monitoring data, to provide more ecologically-relevant estimates of in-stream phosphorus concentrations, or more accurate estimates of loads delivered downstream to lakes or estuaries;
2. Hypothesis testing and highlighting knowledge and data gaps. This in turn could be used to help design monitoring strategies and experimental needs, and prioritise areas for future model development;
3. Exploring the potential response of the system to future environmental change (e.g. climate, land use and management), including potential storm and low-flow dynamics;
4. Providing evidence to support decision-making, e.g. to help set water quality and load reduction goals, and advise on means of achieving those goals.

The model was [originally developed in Python](https://github.com/LeahJB/SimplyP) and is described here:

> Jackson-Blake LA, Sample JE, Wade AJ, Helliwell RC, Skeffington RA. 2017. *Are our dynamic water quality models too complex? A comparison of a new parsimonious phosphorus model, SimplyP, and INCA-P*. Water Resources Research, **53**, 5382–5399. [doi:10.1002/2016WR020132](http://onlinelibrary.wiley.com/doi/10.1002/2016WR020132/abstract;jsessionid=7E1F1066482B9FFDBC29BA6B5A80042C.f04t01)

Full details of version 0.1 of the model are provided in the [Supplementary Information](https://agupubs.onlinelibrary.wiley.com/action/downloadSupplement?doi=10.1002%2F2016WR020132&file=wrcr22702-sup-0001-2016WR020132-s01.pdf), and the main updates and changes to the model are documented in the [change log](https://github.com/NIVANorge/Mobius/blob/master/Applications/SimplyP/SimplyP_development_log.txt). The Python version of the model (up to v0.2) is [still available](https://github.com/LeahJB/SimplyP).

The Mobius version offers dramatic performance improvements compared to the Python version (run times are at least two orders of magnitude faster). It is now also very easy to make changes to the model and create alternative versions, and we hope that this flexibility will encourage collaborative testing and further model development. The Mobius [Python wrapper](https://github.com/NIVANorge/Mobius/tree/master/PythonWrapper) means the benefits of interacting with the model through Python are still available, and we have already used the wrapper to develop very nice optimization and MCMC procedures for SimplyP (see tutorial 2 below). As with all models coded using Mobius, SimplyP can be run using the Mobius Graphical User Interface (a relatively user-friendly program where you can click to change parameters, run the model and explore the output).

**Further development of SimplyP will likely take place using the Mobius framework**. We have yet to shift the known issues and other documentation across from the original Python repository, so please visit the [original repository](https://github.com/LeahJB/SimplyP) for more information. Please report any bugs either by [submitting a pull request](https://github.com/NIVANorge/Mobius/pulls) via GitHub (in which case, please link to the Simply project), or by emailing Leah Jackson-Blake (ljb@niva.no). Suggested improvements are also very welcome.

## Folder contents

* The main model definition file is not in this folder, but [here](https://github.com/NIVANorge/Mobius/blob/master/Modules/SimplyP.h).

* Morsa and Tarland folders contain example input files for two catchments, the Tarland catchment in northeast Scotland, and the Morsa (also known as the Vansjø-Hobøl) catchment in southeast Norway. Input files include (1) parameter .dat files, which in this case are rough manually calibrated values for the two catchments, and (2) time series .dat files. The time series files include meteorological data to drive the models, as well as observed data for comparing simulated output to. See [the documentation](https://github.com/NIVANorge/Mobius/tree/master/Documentation) for a further description of input file formats, to help you modify or make your own files.

* Two .cpp files and accompanying compile.bat files which are used to compile different kinds of model executables (don't worry, compiling is easy, see [here](https://github.com/NIVANorge/Mobius#compile-a-model-run-it-and-make-some-changes) for a quick description of why/when you compile, and a walk through of how it's done):

  - simplyp_dll.cpp (compiled using compile_dll.bat): this creates a .dll which is then used by the python wrapper or the MobiView GUI. You shouldn't need to alter this .cpp for different model applications.
  - simplyp_testing.cpp (compiled using compile_testing.bat): this creates a .exe which is useful for testing during model devleopment. **This .cpp contains things you should change for different model applications.** It also contains flags for doing useful things like creating an .exe which, when run, **creates a new parameter file** using the structure you define in the .cpp file and the default parameter values defined in the model header file SimplyP.h. This can also be done in MobiView.
  
* three .dat files for setting up optimization and uncertainty analysis in C++, i.e. without use of the python wrapper. More info on this will come shortly.


## Tutorials

**Tutorial 1: Quick start - Run the Tarland or Morsa examples using the MobiView graphical user interface**
 
* Download/clone the GitHub Mobius repository onto your computer (see instructions [here](https://github.com/NIVANorge/Mobius#download-the-repository)), and familiarize yourself with the basics of the Mobius model building system and the contents of the repository (introductory documentation is [here](https://github.com/NIVANorge/Mobius#mobius)).
 
* Follow the instructions [here](https://github.com/NIVANorge/Mobius#the-mobiview-graphical-user-interface) to:
  - Download a MobiView executable from **ftp://mobiserver.niva.no/**
  - Download or compile SimplyP: Pre-compiled stable releases of SimplyP are available for download from **ftp://mobiserver.niva.no/**. Alternatively, you can compile yourself, which you will need to be able to do if you want to make changes to the model equations. To compile: after [installing a compiler](https://github.com/NIVANorge/Mobius#first-download-a-compiler), from the command line `cd` into the Applications/SimplyP folder and run `compile_dll.bat`. simplyp.dll should be created.
  - Open MobiView.exe and read in (more detailed instructions for using MobiView are [here](https://github.com/NIVANorge/Mobius#the-mobiview-graphical-user-interface):
  
       * The compiled SimplyP dll
	* An input file, which contains precipitation and temperature data to drive the model, and optionally also potential evapotranspiration data and observed data to compare model output to. Example input files are in the Morsa and Tarland folders, e.g. TarlandInputs.dat
	* A parameter file. Example files are in the Morsa and Tarland folders, e.g. TarlandParameters_v0-3.dat. Note that the parameter file version must match the model version.
More detailed descriptions of the structure of Mobius model input and parameter files are given in the [documentation](https://github.com/NIVANorge/Mobius/blob/master/Documentation/file_format_documentation.pdf).
  - Run the model, then explore the results. Try changing parameter values and re-running the model to see what happens. If you want to know more about what the results variables are or how they are calculated, you can look directly into the SimplyP.h file ([here](https://github.com/NIVANorge/Mobius/blob/master/Modules/SimplyP.h)), as well as in the model description paper. See also the comments which accompany the parameters within the parameter.dat file or in MobiView.
 
**Notes:**
 * Some people have issues installing software (e.g. the MingW compiler) on their system due to admin rights and/or over-zealous antivirus software. We recommend you beg your IT department to give you full admin rights at least temporarily, and disable your antivirus software during install if you're having troubles.

 **Tutorial 2: [Auto-calibration and uncertainty estimation with the Python wrapper](https://nbviewer.jupyter.org/github/NIVANorge/Mobius/blob/master/PythonWrapper/SimplyP/simplyp_calibration.ipynb)**. An example illustrating how to auto-calibrate SimplyP and explore parametric uncertainty using MCMC.
 
The Jupyter notebook used in Tutorial 2 is located within the PythonWrapper/SimplyP folder in the repository, and so will be available for you to work through interactively when you download the Mobius repository (instructions on how to do that are [here](https://github.com/NIVANorge/Mobius#download-the-repository)). Otherwise, you can just view it online. For instructions on getting started with Python, Jupyter notebooks and the python wrapper, see https://github.com/NIVANorge/Mobius/tree/master/PythonWrapper#python-wrapper.

**More coming soon!**
