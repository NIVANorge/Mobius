# Python wrapper

The python wrapper allows you to easily create optimization and calibration routines for a model and do post-processing on the results.

This has currently only been tested with the 64bit version of python 3.

To learn the basics of how to build and compile models, see the [quick start guide](https://github.com/NIVANorge/Mobius#quick-start-guide) at the front page, and the tutorials.

The C++-end code of the C++-python interface is in python_wrapper.h, while the python-end of it is in mobius.py.

To use a model with the python wrapper, create your own .cpp file along the lines of Persist/persistwrapper.cpp, and replace the model building part of it with the modules you want. Then make a .bat file along the lines of compilepersist.bat that compiles your .cpp file. The C++ code compiles to a .dll (dynamically linked library) that can be loaded using mobius.initialize('name_of_dll.dll')

We have already built some examples atop of mobius.py to show you how you can interact with the models through the interface. See e.g. optimization_example.py or the SimplyP example [here](https://nbviewer.jupyter.org/github/NIVANorge/Mobius/blob/master/PythonWrapper/SimplyP/simplyp_calibration.ipynb). There is also docstring documentation in mobius.py, and we have made some calibration utilities available through mobius_calib_uncert_lmfit.py (mobius_calibration.py is older code, but can also serve as a good example of what you can do).

### Dependencies

The Python wrapper has so far only been tested using **64-bit Python 3.6**. The main dependencies of mobius.py and mobius_calib_uncert_lmfit.py are:

 * Numpy
 * Scipy
 * Matplotlib
 * Pandas
 * LMFit
 * EMCEE
 * Corner

Additonally, we recommend using the wrapper via JupyterLab, in which case you will need to install that too.

Python dependencies are most easily managed using the [Anaconda distribution](https://www.anaconda.com/distribution/) and the `'conda'` package manager. From the Anaconda prompt, first create a new environment using

    conda create -n mobius python=3.6
    
and activate it

    activate mobius
    
Then install the required packages

    conda install -c conda-forge numpy scipy matplotlib pandas lmfit emcee corner jupyterlab notebook ipython=7.3
    
To test your installation, `'cd'` into the top level of your local copy of the Mobius repository and run

    jupyter lab
    
Using the JupyterLab file browser, navigate to `'PythonWrapper\SimplyP'` and work through `'simplyp_calibration.ipynb'`.  

![Alt text](../Documentation/img/triangle_plot.png?raw=true "Triangle plot from running emcee on reach flow in SimplyP")
