# Mobius
*C++ Model Building System. Optimized for speed of execution.*

Mobius is a general framework for running large sets of equations, both discrete-timestep equations and ordinary differential equations, that model biogeochemical systems. Typically each equation can be evaluated over many indexes being they geographical locations, such as river segments and land use classes, or size classes and age classes, and over many timesteps. Mobius is modular, allowing you to combine models of various subsystems into larger models.

Primarily Mobius was developed to model catchments (routing of precipitation through soil and groundwater into rivers and streams and assorted transport and reactions of chemical compounds and sediments), but can also be used to build e.g. biological population models.

The framework lets you focus on specifying parameters, input timeseries (forcings) and equations for your model and it will set up the run structure of the model and handle input/output file formats for you. The run speed of the models is typically very fast compared to something that is written directly in Matlab or python.

Mobius can produce executables that can be run standalone or together with the graphical user interface INCAView. https://github.com/Lecheps/INCAView

There is also the option to compile the models to .dll's that can be called and interacted with from python using our [python wrapper interface](https://github.com/NIVANorge/Mobius/tree/master/PythonWrapper). The python wrapper is great for writing your own calibration systems or for making your own post-processing and plots.

![Alt text](Documentation/img/optimizer_MAP.png?raw=true "Example of a plot made using the framework and the python wrapper.")

Mobius is a reimplementation and extension of the functionality that was provided by https://github.com/biogeochemistry/INCA , but with a higher emphasis on run-time performance. Since then we have also started to add a few new models, ODE solvers and calibration systems.

The implementations of PERSiST, INCA-N and INCA-N-Classic are translated from biogeochemistry/INCA/

Documentation can be found in the Documentation folder. This documentation is however still under development, and will be added gradually. For a start, see the quick start guide below and the tutorials.


Developed by by Magnus Dahler Norling
for [NIVA](https://www.niva.no/) (Norwegian institute for water research)



## Quick start guide
*This is a guide on how to get the tutorials running and to start building your own models.*

First, download the entire Mobius repository. It is recommended that you keep yout copy up to date, so it is probably a good idea to clone it using git and keep it updated that way. There are several tutorials online on how to do that.

Mobius is written in C++, and so to produce a program that the computer can run you need a compiler to produce the executable. We aim to allow you to use any compiler, but for now it has mostly been tested with the g++ compiler.

To install g++ on Windows, you will need one of the MingW g++ distributions. We recommend using one of the MingW-w64 distributions that can compile to 64-bit (this will be needed if you want to use the Python wrapper later).

We would normally have recommended the TDM-gcc distribution of MingW since that allows you to use multithreading and openmp in C++ code, but unfortunately there is a bug with TDM-gcc (as of 2019-02-05) that makes Windows 10 (but not earlier Windows versions) unable to load .dll's that were compiled with it, so you can't use it if you later want to use the python wrapper on Windows 10.

You can use the version of MingW-w64 found here (click the sourceforge link to download):
https://mingw-w64.org/doku.php/download/mingw-builds
**Be sure during installation that you choose x86-64 under the 'Architecture' dropdown list.**

If you use Linux, just use the distribution of g++ that comes with your Linux distribution, and it should work.

Make sure that the /bin/ folder of your installation of g++ is in your PATH variable, and that you don't have any other installations of g++ in the PATH.

To compile Tutorial 1, navigate to the Tutorial1 folder from the command line and run the compile.bat file. Then you can run tutorial1.exe to see the output. Try to make changes in the tutorials by changing or adding new equations and parameters, and see what happens. This is the best way to learn. A detailed documentation of the API will be added later.

To edit the C++ files, you can use whatever text editor you want, such as Notepad++, or you can find an IDE. We will not go through how to learn C++ here, but you will not need to know that many advanced concepts. Any online tutorial of just the basics will hopefully do (and also you can just learn by example from the models that are in this repository already, e.g the tutorials or the files in the Modules folder ).

After understanding basic model building we also recommended you to learn the python wrapper interface to e.g. do your own post-processing and plotting with the model results. See more in the PythonWrapper readme.




The basic functionality of the Mobius system has no other library dependencies than the C++ standard library. However if you want to use more advanced solvers such as the boost odeint solvers, you have to download boost (and copy the header files to your compiler's iclude directory).
https://www.boost.org/
You will not need to compile any of the boost libraries, we only use the header-only libraries.

Other more advanced functionality such as the C++-written calibration systems rely on other libraries. That will eventually be documented somewhere else.

## Building INCAView compatible exes

If you are just a user of existing models, or if you want to use an interface to quickly get visual results out of your own models, you can make model exes that are compatible with INCAView.

We already have .cpp files ready for compilation that are set up to produce INCAView-compatible exes for most of our models. See e.g Applications/INCA-N/incan.cpp
To set up your own .cpp files like these, try to follow the example from one of those files. To compile them you also need the sqlite3 library. Download the source code for sqlite3 from https://www.sqlite.org/download.html . See also e.g. Applications/INCA-N/COMPILE for guidance on how to compile. You should put the sqlite3 files in Mobius/sqlite3 (create a new folder). You also have to download json.hpp from https://github.com/nlohmann/json/tree/develop/single_include/nlohmann and put it in Mobius/json (create a new folder). The json library does not need any separate compilation.

See the INCAView repository for the interface itself.

We may also look into a way of distributing precompiled exes of INCAView and some of the models.

![Alt text](Documentation/img/incaviewpersist.png?raw=true "Example of running the PERSiST model in INCAView.")



