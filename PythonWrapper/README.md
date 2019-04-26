# Python wrapper

The python wrapper allows you to easily create optimization and calibration routines for a model and do post-processing on the results.

To learn the basics of how to build and compile models, see the [quick start guide](https://github.com/NIVANorge/Mobius#quick-start-guide) at the front page, and the tutorials.

The C++-end code of the C++-python interface is in python_wrapper.h, while the python-end of it is in mobius.py.

To use a model with the python wrapper, create your own .cpp file along the lines of Persist/persistwrapper.cpp, and replace the model building part of it with the modules you want. Then make a .bat file along the lines of compilepersist.bat that compiles your .cpp file. The C++ code compiles to a .dll (dynamically linked library) that can be loaded within Python using mobius.initialize('name_of_dll.dll')

We have already built some examples atop of mobius.py to show you how you can interact with the models through the interface. See e.g. optimization_example.py or the SimplyP example [here](https://nbviewer.jupyter.org/github/NIVANorge/Mobius/blob/master/PythonWrapper/SimplyP/simplyp_calibration.ipynb). There is also docstring documentation in mobius.py, and we have made some calibration utilities available through mobius_calib_uncert_lmfit.py (mobius_calibration.py is older code, but can also serve as a good example of what you can do).

### Installation

#### Dependencies

The Python wrapper has so far only been tested using **64-bit Python 3.6**. The main dependencies of mobius.py and mobius_calib_uncert_lmfit.py are:

 * Numpy
 * Scipy
 * Matplotlib
 * Pandas
 * LMFit
 * EMCEE
 * Corner

Additonally, we recommend using the wrapper via JupyterLab, in which case you will need to install that too.

## Quick start using Anaconda and Jupyter Lab

### Installation

If you don't have an up-to-date Python installation, a good option is the [Anaconda Python distribution](https://www.anaconda.com/distribution/) and the `'conda'` package manager, which allows for easy management of Python package dependencies. After downloading Anaconda, open up the Anaconda prompt (an enhanced command line window), and then:

1. create a new environment (you can replace 'mobius' with the name environment name of your choice)

    conda create -n mobius python=3.6
    
2. Activate the new environment

    activate mobius
    
3. Install the required packages in the new environment

    conda install -c conda-forge numpy scipy matplotlib pandas lmfit emcee corner jupyterlab notebook ipython=7.3

### Run an example Jupyter notebook   

We have used Jupyter notebooks to code up several examples. For an introduction to Jupyter Notebooks, see for example [here](https://realpython.com/jupyter-notebook-introduction/). Jupyter Lab offers some extra features on top of Jupyter notebooks; again, there are plenty of on-line resources documenting these.

To test your installation, or whenever you want to start working with mobius through the wrapper:

1. Open up the Anaconda prompt and `'cd'` into the top level of your local copy of the Mobius repository and run

    jupyter lab
    
2. Using the JupyterLab file browser in the left hand panel, navigate to `'PythonWrapper\SimplyP'` and then double click on `'simplyp_calibration.ipynb'` to open up the notebook. You can then work through it interactively, running cells one at a time and seeing the output they produce.

3. When you have finished, close the notebook and then quick Jupyter Lab (File > Close notebook, File > Quit)

![Alt text](../Documentation/img/triangle_plot.png?raw=true "Triangle plot from running emcee on reach flow in SimplyP")
