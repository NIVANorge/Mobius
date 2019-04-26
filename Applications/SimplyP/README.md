# SimplyP (Mobius version)

This folder contains the applications of the Mobius implemetation of [SimplyP](https://github.com/LeahJB/SimplyP), a parsimonious hydrology, sediment and phosphorus model. The main model definition file is [here](https://github.com/NIVANorge/Mobius/blob/master/Modules/SimplyP.h). The original version of SimplyP was coded in Python and is described here:

> Jackson-Blake LA, Sample JE, Wade AJ, Helliwell RC, Skeffington RA. 2017. *Are our dynamic water quality models too complex? A comparison of a new parsimonious phosphorus model, SimplyP, and INCA-P*. Water Resources Research, **53**, 5382–5399. [doi:10.1002/2016WR020132](http://onlinelibrary.wiley.com/doi/10.1002/2016WR020132/abstract;jsessionid=7E1F1066482B9FFDBC29BA6B5A80042C.f04t01)

Full details of version 0.1 of the model are provided in the [Supplementary Information](https://agupubs.onlinelibrary.wiley.com/action/downloadSupplement?doi=10.1002%2F2016WR020132&file=wrcr22702-sup-0001-2016WR020132-s01.pdf). The Python version of the model (up to v0.2) is still available [here](https://github.com/LeahJB/SimplyP).

The Mobius version offers dramatic performance improvements and increased flexibility, while still making it possible to interact with the model via the [Python wrapper](https://github.com/NIVANorge/Mobius/tree/master/PythonWrapper), as well as the GUI available with all Mobius models. **Further development of SimplyP will likely take place using the Mobius framework**. We have yet to shift the known issues, change log and other documentation across from the original Python repository, so please visit the [original repository](https://github.com/LeahJB/SimplyP) for more information.

## Folder contents

* Morsa and Tarland folders contain example input files for two catchments, the Tarland catchment in northeast Scotland, and the Morsa (also known as the Vansjø-Hobøl catchment) in southeast Norway. Input files include (1) parameter .dat files, which in this case are rough manually calibrated values for the two catchments, and (2) time series .dat files. The time series files include meteorological data to drive the models, as well as observed data for comparing simulated output to. See [the documentation](https://github.com/NIVANorge/Mobius/tree/master/Documentation) for a further description of input file formats, to help you modify or make your own files.

* Three .cpp files and three accompanying compile.bat files which are used to compile different kinds of model executables (don't worry, compiling is easy, see [here](https://github.com/NIVANorge/Mobius#compile-a-model-run-it-and-make-some-changes) for a quick description of why/when you compile, and a walk through of how it's done):

  - simplyp_testing.cpp (compiled using compile_testing.bat): this creates a .exe which is useful for testing during model devleopment. **This .cpp contains things you should change for different model applications.** It also contains flags for doing useful things like, for example, creating an .exe which, when run, **creates a new parameter file using the structure you define in the .cpp file** and the default parameter values defined in the model header file. This can save a lot of typing if you have lots of parameters.
  - simplyp_incaview.cpp (compiled using compile_incaview.bat): this creates a .exe which can be used with [INCAView](https://github.com/NIVANorge/Mobius#the-incaview-graphical-user-interface), and to create the parameter database required by INCAView (further instructions are provided [here](https://github.com/NIVANorge/Mobius#the-incaview-graphical-user-interface)). You shouldn't need to alter this .cpp for different model applications.
  - simplyp_wrapper.cpp (compiled using compile_pythonwrapper.bat): this creates a .dll which is then used by the python wrapper. You shouldn't need to alter this .cpp for different model applications.
  
* three .dat files for setting up optimization and uncertainty analysis in C++, i.e. without use of the python wrapper. More info on this will come shortly.


## Tutorials

For instructions on getting started with Python, Jupyter notebooks and the python wrapper, see https://github.com/NIVANorge/Mobius/tree/master/PythonWrapper#python-wrapper.

Jupyter notebooks used in these tutorials are located within the PythonWrapper/SimplyP folder in the repository, and so will be available for you to work through interactively when you download the Mobius repository (instructions on how to do that are [here](https://github.com/NIVANorge/Mobius#download-the-repository)). Otherwise, you can just view them online.

 1. **Run the Tarland or Morsa example using INCAView**
 Follow the instructions [here](https://github.com/NIVANorge/Mobius#the-incaview-graphical-user-interface) to:
 * get an INCAView executable
 * compile an INCAView-compatible simplyP executable (by running `simplyp_incaview.bat` from the command line). SimplyP_INCAView.exe should be created
 * create a parameter database for either the Morsa or Tarland catchment by using the .exe you just created to convert parameters in the .dat file to a .db file. E.g. by typing `SimplyP_INCAView convert_parameters Tarland/TarlandParameters.dat TarlandParameters.db` in the command line, then hitting enter.
 * Open INCAView.exe, read in the parameter database and input data and run the model, then explore the results. Try changing parameter values and re-running the model to see what happens. If you want to know more about what the results variables are or how they are calculated, you can look directly into the simplyp.h file ([here](https://github.com/NIVANorge/Mobius/blob/master/Modules/SimplyP.h)).
 
 2. **[Auto-calibration and uncertainty estimation](https://nbviewer.jupyter.org/github/NIVANorge/Mobius/blob/master/PythonWrapper/SimplyP/simplyp_calibration.ipynb)**. An example illustrating how to auto-calibrate SimplyP and explore parametric uncertainty using MCMC

**More coming soon!**
