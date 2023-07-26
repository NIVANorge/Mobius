# Example of application to Imus River Philippines 
Model inputs, parameters and sensitivity scripts for manuscript "Modelling macroplastic fluxes in the Imus catchment: Impacts of long-term accumulation and extreme events" (Clayer et al.).
These files include input and parameter (default and range of parameter values) for simulating macroplastic export and retention in the Imus River catchment in the Philippines using [MOBIUS](https://doi.org/10.5194/gmd-14-1885-2021).

Files include: 
- IncaMacroplastics input files (inputs_Ismus_####.dat) following two scenarios ("default" and "Remediation2006")
- IncaMacroplastics parameter files (params_Ismus_####.dat) for two calibrations ("LandAcc" and "RiverAcc")
- Two pythons scripts (Sensitivity_####.py) used to perform the Monte Carlo sensitivity analyses for each calibration ("LandAcc" and "RiverAcc") using the "mobius_calib_uncert_lmfit.py" of the [MOBIUS PythonWrapper](https://github.com/NIVANorge/Mobius/tree/master/PythonWrapper)
Note that scenario "Remediation2006" is referred to as "RA 9003 enforcement" in the Clayer et al. manuscript.

To run the model and perform sensitivity analyses with the python scripts above, pre-requirements include to clone the [MOBIUS github repository](https://github.com/NIVANorge/Mobius/tree/master) to your local device at "C:/MOBIUS/". If you clone it elsewhere, you have to update the L. 12, 15 and 18 of the sensitivity scripts. Then, compile IncaMacroplastics following [these instructions](https://github.com/NIVANorge/Mobius#compile-a-model-run-it-and-make-some-changes) to produce the "incamacroplastics_soil.dll" model executable file. An executable file is available here but it might not be compatible with your computer setup. We recommend to compile your own.

You can then use the MOBIUS user interface [MobiView](https://github.com/NIVANorge/Mobius#the-mobiview-graphical-user-interface) to run IncaMacroplastics. Please read the [MOBIUS github repository](https://github.com/NIVANorge/Mobius/tree/master), the [MOBIUS PythonWrapper](https://github.com/NIVANorge/Mobius/tree/master/PythonWrapper) and the [MobiView](https://github.com/NIVANorge/Mobius#the-mobiview-graphical-user-interface) pages for detailed instructions.

Model outputs from the sensitivity scripts are stored [here](http://www.hydroshare.org/resource/8697e34af0224f61baebf0478d6e0ee3)
