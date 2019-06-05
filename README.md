# Mobius
*C++ Model Building System. Optimized for speed of execution.*

Mobius is a general framework for running large sets of equations, both discrete-timestep equations and ordinary differential equations, that model biogeochemical systems. Typically each equation can be evaluated over many indexes, representing e.g. geographical locations, such as river segments and land use classes, or size classes and age classes, and over many timesteps. Mobius is modular, allowing you to combine models of various subsystems into larger models.

Primarily Mobius was developed to model catchments (routing of precipitation through soil and groundwater into rivers and streams and assorted transport and reactions of chemical compounds and sediments), but can also be used to build e.g. biological population models.

The framework lets you focus on specifying parameters, input timeseries (forcings) and equations for your model and it will set up the structure of the model and handle input/output file formats for you. The run speed of the models is typically very fast compared to something that is written directly in Matlab or python.

Mobius can produce executables that can be run standalone or together with the graphical user interface [INCAView](https://github.com/Lecheps/INCAView). There is also the option to compile the models to .dll's that can be called and interacted with from python using our [python wrapper interface](https://github.com/NIVANorge/Mobius/tree/master/PythonWrapper). The python wrapper is great for writing your own calibration systems or for making your own post-processing and plots.

![Alt text](Documentation/img/optimizer_MAP.png?raw=true "Example of a plot made using the framework and the python wrapper.")

## Contents
* [Available models](https://github.com/NIVANorge/Mobius#available-models)
* [Documentation](https://github.com/NIVANorge/Mobius#documentation)
* [Quick start guide](https://github.com/NIVANorge/Mobius#quick-start-guide)
  - [Download the repository](https://github.com/NIVANorge/Mobius#download-the-repository)
  - [Compile a model, run it, and make some changes](https://github.com/NIVANorge/Mobius#compile-a-model-run-it-and-make-some-changes)
  - [Python wrapper](https://github.com/NIVANorge/Mobius#python-wrapper)
  - [Navigating around the repository](https://github.com/NIVANorge/Mobius#navigating-around-the-repository)
  - [The INCAView graphical user interface](https://github.com/NIVANorge/Mobius#the-incaview-graphical-user-interface)
* [Dependencies](https://github.com/NIVANorge/Mobius#dependencies)
* [Authors and acknowledgements](https://github.com/NIVANorge/Mobius#authors-and-acknowledgements)

## Available models

The following models are already implemented using the Mobius framework (see the model-specific folders and associated readmes [here](https://github.com/NIVANorge/Mobius/tree/master/Applications) for details):

 * "Simply" models ([SimplyP](https://github.com/NIVANorge/Mobius/blob/master/Applications/SimplyP/README.md), SimplyQ, SimplyC)
 * "INCA" models (INCA-N, INCA-C, INCA-P, INCA-Sed, INCA-Microplastics, PERSiST) 
 * HBV
 
Simply models have been ported from [here](https://github.com/LeahJB/SimplyP); the implementations of PERSiST, INCA-N and INCA-N-Classic were translated from https://github.com/biogeochemistry/INCA

## Documentation

Documentation can be found in the [Documentation](https://github.com/NIVANorge/Mobius/tree/master/Documentation) folder. This is still under development, and will be added to gradually. See also the quick start guide below and the tutorials.


# Quick start guide
*This is a guide on how to get the tutorials running and to start building your own models. We also provide a quick introduction to the python wrapper, navigating around the repository, and to getting started with the INCAView Graphical User Interface.*

## Download the repository

First, download the entire Mobius repository. It is recommended that you keep your copy up to date, so it is probably a good idea to clone it and keep it updated using git, to save you having to re-download the repository whenever files are changed. There are several tutorials online on how to do that. One option, using GitHub desktop for a friendly user interface, is:

-	Download GitHub Desktop (https://desktop.github.com/)
-	Navigate to the online repository (https://github.com/NIVANorge/Mobius)
-	Click the green ‘Clone or download’ button at the right, and then the bottom left option ‘Open in Desktop’
-	Set the 'Local path' to wherever you want the repository to be stored on your system then click ‘Clone’. GitHub desktop will then be launched, and the files will be saved to your computer.
- Open GitHub Desktop whenever you want to check for updates. Any changes you make to the files should be saved somewhere else locally on your machine (unless you are an active mobius contributor with permission to commit changes). If files have changed online, you can update the repository on your computer by hitting the blue 'pull origin' box in the right panel.

## Compile a model, run it, and make some changes

**If the idea of compiling is offputting, and you want to just have a go with one of the pre-existing models, please get in touch and we can supply a pre-compiled executable.** Email: [magnus.norling@niva.no*]

**Note**: We know getting set up is cumbersome at the moment. We plan on creating a build and install system very soon which will make things much easier.

### First download a compiler

Mobius is written in C++, and so to produce a program that the computer can run you need a compiler to produce the executable. We aim to allow you to use any compiler, but for now it has mostly been tested with the g++ compiler. To install g++ on Windows, you will need one of the MingW g++ distributions. We recommend using one of the MingW-w64 distributions that can compile to 64-bit, for compatibility with the Python wrapper. To install the compiler:

- You can use the version of MingW-w64 found here (click the sourceforge link to download):
https://mingw-w64.org/doku.php/download/mingw-builds **Be sure during installation that you choose x86-64 under the 'Architecture' dropdown list.** Take note of where the compiler is installed, you will need it shortly.
- Linux: just use the distribution of g++ that comes with your Linux distribution, and it should work.
- Make sure that the /bin/ folder of your installation of g++ is in your PATH variable, and that you don't have any other installations of g++ in the PATH: in Windows 10, hit the start button, then type 'system environment variables', and select the option that best matches this. Then click on 'Environment variables'. Within the 'User variables' or 'System variables', there should be a variable called 'Path' or 'PATH'. Within this there should be something like mingw64\bin. If it isn't there, add it (click 'New' and then 'Browse', and find and select the location of the \bin\ folder of your compiler, making sure to click 'ok' every time you close a dialogue box).

(Note: we would normally have recommended the TDM-gcc distribution of MingW, as that allows you to use multithreading and openmp in C++ code, but unfortunately there is a bug in this (last checked 2019-02-05) so that Windows 10 is unable to load .dll's compiled with it)

### Compiling

The basics of compiling are simple once you have a compiler installed:
- Make a .bat file, which contains the command line instruction(s) for the compiler to compile the .cpp file into an executable. All of the models already built using Mobius already have such .bat files.
- Open the command line prompt (e.g. press the windows key and type 'cmd' and then hit enter)
- From the command line, navigate to wherever your .bat file is located. To change drives (e.g. from C: to D:), just type the name of drive you want to be in and hit enter. When you're in the right drive, change to your desired folder by typing cd then the filepath, e.g. `cd C:\GitHub\Mobius\Applications\SimplyP`
- Run the .bat file for the model of interest: from the command line, type in the name of the .bat file and hit enter. A new executable should appear, with whatever name and extension (e.g. .exe or .dll) is specified in the .bat file

You could also set up a build system of your choice, however this is not really needed here since most models are set up to be easily compilable in a single command line instruction.

### Run a model

To compile **Tutorial 1**, navigate to the Tutorial1 folder from the command line and run the compile.bat file. Then you can run tutorial1.exe (just type `tutorial1` and hit enter) to see the output printed to the command line.

Try to make changes in the tutorials by changing or adding new equations and parameters, and see what happens (remembering that you have to recompile after any change, then run, to see an effect in the output). This is the best way to learn. Along with following Tutorials 1-3, you can read the [Documentation](https://github.com/NIVANorge/Mobius/tree/master/Documentation) for a more detailed description of the API.

To edit the C++ files, you can use whatever text editor you want, such as Notepad++, or you can find an IDE. We will not go through how to learn C++ here, but you will not need to know that many advanced concepts. Any online tutorial of just the basics will hopefully do (and also you can just learn by example from the models that are in this repository already, e.g the tutorials or the files in the Modules folder).

## Python wrapper

After understanding basic model building we recommended you explore the python wrapper interface to e.g. do your own post-processing and plotting with the model results. See more in the [PythonWrapper readme](https://github.com/NIVANorge/Mobius/tree/master/PythonWrapper), including examples of optimisation and auto-calibration.

*Python functions to automate generation of the input .dat files from excel files will be added soon.*

## Navigating around the repository

- The main model definition files are within the 'Modules' folder. This is where the meat is in terms of parameter definitions, equations, etc. All these files have a .h file extension.
- The 'Applications' folder is where the .cpp files for each model live, as well as the .bat files for compling them. Each model typically includes and uses one or more of the modules that are in the 'Modules' folder. The application folders typically also have a few example parameter and input files.
- For now all the source code of the main Mobius functionality is in the base folder. The models rely on this base functionality to organize data in memory and executie the equations in the right order etc. Mobius is designed to be compiled as a unity build. This means that instead of compiling various .cpp files into separate object files and linking them as is common in C++ projects, all the source files for Mobius are included into mobius.h, and so you can just include mobius.h in your model.cpp and compile everything as a single unit. This was done to speed up development (not having to forward-declare functions), and make compilation easier. Beware however that you should make sure not to include mobius.h into several different compilation units in your project if you need to have more than one compilation unit.

## The MobiView graphical user interface

[MobiView](https://github.com/NIVANorge/MobiView) is a GUI designed to provide a quick way of running models, manually calibrating them, and exploring model output. If you are just a user of existing models, or if you want to use an interface to quickly get visual results out of your own models, you can make model exes that are compatible with INCAView.

![Alt text](Documentation/img/mobiviewpersist.png?raw=true "Example of running the PERSiST model in MobiView.")

### Getting MobiView

For now, we recommend you email us ([magnus.norling@niva.no]) to get a pre-compiled version of MobiView. If you want to compile it yourself feel free, you need to install Ultimate++. We will soon find a more reliable way of distributing this.

### Creating MobiView compatible .dlls

MobiView can load the same .dlls as the python wrapper. Most models now have applications set up to compile such dlls. The bat script that compiles them is usually called compile.bat or compile_wrapper.bat. If you want to set up your own model for use with MobiView or the python wrapper, try to follow one of the existing examples.

### Load a model and run it

- Double click MobiView.exe
- Click the 'open' icon in the top left, then select a model dll. Next, select an input file and then a parameter file (both in the .dat formats).
- Click the little runner or (F7) to run the model.
- Click result or input series to plot them. There are a lot of different (combinations) of plot modes to select between.
- More in-depth documentation will follow later.


## Dependencies

### C++

The basic functionality of the Mobius system has no other library dependencies than the C++ standard library, which will be installed when you install g++ as described above. However, if you want to use more advanced solvers such as the boost::numeric::odeint solvers, you have to download boost (and copy the header files to your compiler's include directory).
https://www.boost.org/
You will not need to compile any of the boost libraries separately; we use the header-only libraries.

Other more advanced functionality such as the C++-written calibration systems rely on other libraries. That will eventually be documented somewhere else.

### Python wrapper

See the [PythonWrapper readme](https://github.com/NIVANorge/Mobius/tree/master/PythonWrapper).

## Authors and acknowledgements

Mobius is a reimplementation and extension of the functionality that was provided by the INCA Core Framework ( https://github.com/biogeochemistry/INCA ), developed by Dan Butterfield, but with a higher emphasis on run-time performance. Since then we have also started to add new models, ODE solvers, calibration systems and a new GUI.

This project was partially funded by Nordforsk “Nordic eScience Globalisation Initiative (NeGI)” via the project [An open access, generic ePlatform for environmental model-building at the river-basin scale (Machu-Picchu)](https://www.nordforsk.org/en/programmes-and-projects/projects/an-open-access-generic-eplatform-for-environmental-model-building-at-the-river-basin-scale-machu-picchu).

Developed by by Magnus Dahler Norling for [NIVA](https://www.niva.no/) (Norwegian institute for water research)
