# Recent large updates
**(2019-09-18):** The module system was formalized. MobiView and model dlls compiled before and after this change are not compatible with each other respectively.

**(2019-08-26):** Timeseries in input files no longer have to be ended with 'end_timeseries'

**(2019-08-05):** We recently changed the format of dates in parameter and input files. A date should now be formatted like
2003-01-01
instead of what it was previously
"2003-01-01"
I.e. remove the quotations.



# Mobius
*C++ Model Building System. Optimized for speed of execution.*

Mobius is a general framework for running large sets of equations, both discrete-timestep equations and ordinary differential equations, that model biogeochemical systems. Typically each equation can be evaluated over many indexes, representing e.g. geographical locations, such as river segments and land use classes, or size classes and age classes, and over many timesteps. Mobius is modular, allowing you to combine models of various subsystems into larger models.

Primarily Mobius was developed to model catchments (routing of precipitation through soil and groundwater into rivers and streams and assorted transport and reactions of chemical compounds and sediments), but can also be used to build e.g. biological population models.

The framework lets you focus on specifying parameters, input timeseries (forcings) and equations for your model and it will set up the structure of the model and handle input/output file formats for you. The run speed of the models is typically very fast compared to something that is written directly in Matlab or python.

Mobius can produce standalone executables. There is also the option to compile the models to .dll's that can be loaded in the graphical user interface [MobiView](https://github.com/NIVANorge/Mobius#the-mobiview-graphical-user-interface) or be called and interacted with from Python using our [python wrapper interface](https://github.com/NIVANorge/Mobius/tree/master/PythonWrapper). The python wrapper is great for writing your own calibration systems or for making your own post-processing and plots.

![Alt text](Documentation/img/optimizer_MAP.png?raw=true "Example of a plot made using the framework and the python wrapper.")

## Contents
* [Available models](https://github.com/NIVANorge/Mobius#available-models)
* [Documentation](https://github.com/NIVANorge/Mobius#documentation)
* [Quick start guide](https://github.com/NIVANorge/Mobius#quick-start-guide)
  - [Running existing pre-compiled models (e.g. INCA and Simply models)](https://github.com/NIVANorge/Mobius#use-existing-pre-compiled-mobius-models-eg-inca-and-simply-models)
  - [Download the repository](https://github.com/NIVANorge/Mobius#download-the-repository)
  - [Compile a model, run it, and make some changes](https://github.com/NIVANorge/Mobius#compile-a-model-run-it-and-make-some-changes)
  - [Python wrapper](https://github.com/NIVANorge/Mobius#python-wrapper)
  - [Navigating around the repository](https://github.com/NIVANorge/Mobius#navigating-around-the-repository)
  - [The MobiView graphical user interface](https://github.com/NIVANorge/Mobius#the-mobiview-graphical-user-interface)
* [Dependencies](https://github.com/NIVANorge/Mobius#dependencies)
* [Authors and acknowledgements](https://github.com/NIVANorge/Mobius#authors-and-acknowledgements)

## Available models

The following models are already implemented using the Mobius framework (see the model-specific folders and associated readmes [here](https://github.com/NIVANorge/Mobius/tree/master/Applications) for details):

 * "Simply" models ([SimplyP](https://github.com/NIVANorge/Mobius/blob/master/Applications/SimplyP/README.md), SimplyQ, SimplyC)
 * "INCA" models ([INCA-N](https://github.com/NIVANorge/Mobius/tree/master/Applications/IncaN), INCA-C, INCA-P, INCA-Sed, INCA-Microplastics, PERSiST) 
 * HBV
 
Simply models have been ported from [here](https://github.com/LeahJB/SimplyP); the implementations of PERSiST, INCA-N and INCA-N-Classic were translated from https://github.com/biogeochemistry/INCA . INCA models are re-implementations (and sometimes slight modifications) of models developed by many other people and for most of them there exist other non-Mobius implementations by Dan Butterfield. Model/module version numbers refer to their Mobius implementations only and are not comparable with version numbers from other implementations.

## Documentation

Documentation can be found in the [Documentation](https://github.com/NIVANorge/Mobius/tree/master/Documentation) folder. This is still under development, and will be added to gradually. See also the quick start guide below and the tutorials.


# Quick start guide
*This is a guide on how to get the tutorials running and to start building your own models. We also provide a quick introduction to the python wrapper, navigating around the repository, and to getting started with the MobiView Graphical User Interface.*

## Use existing pre-compiled Mobius models (e.g. INCA and Simply models)

Stable releases of available models are available for download, pre-compiled, from ftp://mobiserver.niva.no/ (downloadable binaries are Windows only at the moment). See the model-specific folders and associated readmes [here](https://github.com/NIVANorge/Mobius/tree/master/Applications) for for further instructions on setting up and running individual models. We also recommend downloading [MobiView]((https://github.com/NIVANorge/Mobius#the-mobiview-graphical-user-interface)), a graphical user interface for interacting with Mobius models. A pre-compiled version of MobiView is also available from ftp://mobiserver.niva.no/ (downloadable binary is Windows only).

## Download the repository

First, download the entire Mobius repository. It is recommended that you keep your copy up to date, so it is probably a good idea to clone it and keep it updated using git, to save you having to re-download the repository whenever files are changed. There are several tutorials online on how to do that. One option, using GitHub desktop for a friendly user interface, is:

-	Download GitHub Desktop (https://desktop.github.com/)
-	Navigate to the online repository (https://github.com/NIVANorge/Mobius)
-	Click the green ‘Clone or download’ button at the right, and then the bottom left option ‘Open in Desktop’
-	Set the 'Local path' to wherever you want the repository to be stored on your system then click ‘Clone’. GitHub desktop will then be launched, and the files will be saved to your computer.
- Open GitHub Desktop whenever you want to check for updates. Any changes you make to the files should be saved somewhere else locally on your machine (unless you are an active mobius contributor with permission to commit changes). If files have changed online, you can update the repository on your computer by hitting the blue 'pull origin' box in the right panel.

## Compile a model, run it, and make some changes

If the idea of compiling is offputting, and you want to just have a go with one of the pre-existing models, you can download them from ftp://mobiserver.niva.no/ (downloadable binaries are Windows only at the moment).

### First download a compiler

Mobius is written in C++, and so to produce a program that the computer can run you need a compiler to produce the executable.

| Compiler      | Windows        | Linux        | MacOS       | Note    |
| :------------ | :------------- | :----------- | :---------- | :------ |
| g++ / MingW64 | works          | works        | works       | need gcc version 5.0 or later |
| clang++ (llvm)| works          |              |             | many compiler warnings |
| Visual C++    | works          |              |             | tested with VC2019. Some compiler warnings |
| Intel C++     | works          |              |             |          |

**Table:** Compilers that have been tested with Mobius. Empty cell means that it has not been tested on that platform.

We aim to allow you to use any compiler, but for now it has mostly been tested with the g++ compiler. g++ also seems to produce the fastest running code of the compilers we have tested. It is also the only one we have set up compilation scripts for for all models.

To install g++ on Windows, you will need one of the MingW g++ distributions. We recommend using one of the MingW-w64 distributions that can compile to 64-bit, for compatibility with the Python wrapper. To install the compiler:

- You can use the version of MingW-w64 found here (click the sourceforge link to download):
https://mingw-w64.org/doku.php/download/mingw-builds **Be sure during installation that you choose x86-64 under the 'Architecture' dropdown list.** Take note of where the compiler is installed, you will need it shortly.
- Make sure that the /bin/ folder of your installation of g++ is in your PATH variable, and that you don't have any other installations of g++ in the PATH: in Windows 10, hit the start button, then type 'system environment variables', and select the option that best matches this. Then click on 'Environment variables'. Within the 'User variables' or 'System variables', there should be a variable called 'Path' or 'PATH'. Within this there should be something like mingw64\bin. If it isn't there, add it (click 'New' and then 'Browse', and find and select the location of the \bin\ folder of your compiler, making sure to click 'ok' every time you close a dialogue box).

On Linux you should be able to get g++ using the command `sudo apt install g++` or something similar (maybe depending on the Linux distribution).

(Note: we would normally have recommended the TDM-gcc distribution of MingW, as that allows you to use multithreading and openmp in C++ code, but unfortunately there is a bug in this (last checked 2019-02-05) so that Windows 10 is unable to load .dll's compiled with it)

### Compiling

#### Windows
The basics of compiling are simple once you have a compiler installed:
- Make a .bat file, which contains the command line instruction(s) for the compiler to compile the .cpp file into an executable or dll. All of the models already built using Mobius (in the Applications folder) have such .bat files.
- Open the command line prompt (e.g. press the windows key and type 'cmd' and then hit enter)
- From the command line, navigate to wherever your .bat file is located. To change drives (e.g. from C: to D:), just type the name of drive you want to be in and hit enter. When you're in the right drive, change to your desired folder by typing cd then the filepath, e.g. `cd C:\GitHub\Mobius\Applications\SimplyP`
- Run the .bat file for the model of interest: from the command line, type in the name of the .bat file and hit enter. A new executable should appear, with whatever name and extension (e.g. .exe or .dll) is specified in the .bat file

You could also set up a build system or IDE of your choice, however this is not really needed here since most models are set up to be easily compilable in a single command line instruction.

#### Linux
We have only set up compilation scripts for SimplyP and PERSiST for Linux, but you can easily copy these to be able to compile other models.

### Run a model and explore the principles of building models using Mobius

We have put together a number of [tutorials](https://github.com/NIVANorge/Mobius/tree/master/Tutorials) to get you started running and developing models, starting from very basic to more complicated.

For example, to compile **Tutorial 1**, navigate to the Tutorial1 folder from the command line and run the compile.bat file. Then you can run tutorial1.exe (just type `tutorial1` and hit enter) to see the output printed to the command line.

Try to make changes in the tutorials by changing or adding new equations and parameters, and see what happens (remembering that you have to recompile after any change, then run, to see an effect in the output). This is the best way to learn. Along with following Tutorials 1-3, you can read the [Documentation](https://github.com/NIVANorge/Mobius/tree/master/Documentation) for a more detailed description of the API.

To edit the C++ files, you can use whatever text editor you want, such as Notepad++, or you can find an IDE. We will not go through how to learn C++ here, but you will not need to know that many advanced concepts. Any online tutorial of just the basics will hopefully do (and also you can just learn by example from the models that are in this repository already, e.g the tutorials or the files in the Modules folder).

Further instructions for running models using the MobiView graphical user interface are given [below](https://github.com/NIVANorge/Mobius#the-mobiview-graphical-user-interface).

## Python wrapper

After understanding basic model building we recommended you explore the python wrapper interface to e.g. do your own post-processing and plotting with the model results. See more in the [PythonWrapper readme](https://github.com/NIVANorge/Mobius/tree/master/PythonWrapper), including examples of optimisation and auto-calibration.

*Python functions to automate generation of the input .dat files from excel files will be added soon.*

## Navigating around the repository

- The main model definition files are within the 'Modules' folder. This is where the meat is in terms of parameter definitions, equations, etc. All these files have a .h file extension.
- The 'Applications' folder is where the .cpp files for each model live, as well as the .bat files for compling them. Each model typically includes and uses one or more of the modules that are in the 'Modules' folder. The application folders typically also have a few example parameter and input files.
- For now all the source code of the main Mobius functionality is in the base folder. The models rely on this base functionality to organize data in memory and execute the equations in the right order etc. Mobius is designed to be compiled as a unity build. This means that instead of compiling various .cpp files into separate object files and linking them as is common in C++ projects, all the source files for Mobius are included into mobius.h, and so you can just include mobius.h in your model.cpp and compile everything as a single unit. This was done to speed up development (not having to forward-declare functions), and make compilation easier for non-experts. Beware however that you should make sure not to include mobius.h into several different compilation units in your project if you need to have more than one compilation unit.
- The 'Calibration' folder contains some experiments with setting up Mobius with C++ calibration software. Right now this is experimental and not completely supported since we have found Python calibration packages easier to use. They may be revisited in the future.

## The MobiView graphical user interface

[MobiView](https://github.com/NIVANorge/MobiView) is a GUI designed to provide a quick way of running models, manually calibrating them, and exploring model output. If you are just a user of existing models, or if you want to use an interface to quickly get visual results out of your own models, you can make model dlls that are compatible with MobiView.

![Alt text](Documentation/img/mobiviewpersist.png?raw=true "Example of running the PERSiST model in MobiView.")

### Getting MobiView

You can download a compiled version of MobiView from ftp://mobiserver.niva.no/ (downloadable binary is Windows only). If you want to [compile it yourself](https://github.com/NIVANorge/MobiView#building-mobiview-yourself) feel free, you need to install Ultimate++.

### Creating MobiView compatible .dlls

MobiView can load the same .dlls as the python wrapper. Most models now have applications set up to compile such dlls. The script that compiles them is usually called compile.bat or compile_wrapper.bat. If you want to set up your own model for use with MobiView or the python wrapper, try to follow one of the existing examples.

### Load a model and run it

- Double click MobiView.exe
- Click the 'open' icon in the top left, then select a model dll. Next, select an input file and then a parameter file (both in .dat formats). Almost all the pre-built models have example input files that you can use to get you started. Input file formats are documented [here](https://github.com/NIVANorge/Mobius/blob/master/Documentation/file_format_documentation.pdf), and for some pre-built models, model-specific information is available [here](https://github.com/NIVANorge/Mobius/tree/master/Documentation/ModelInputRequirements).
- **To create a new parameter file with your own index sets**, take an existing parameter file (e.g. any parameter.dat file from within the 'Applications' folder) and delete everything except the index set setup. Set up the index sets however you want (following the [file format guide](https://github.com/NIVANorge/Mobius/blob/master/Documentation/file_format_documentation.pdf)). Once you load this file from MobiView and save it again (disk icon), the parameter file will contain default values for all the parameters, which you can then edit.
- Click the little runner or (F7) to run the model.
- Click the name of a result or input series to plot it. There are a lot of different (combinations) of plot modes to select between.
- When viewing input series with sparse values, check the 'Scatter inputs' box to plot them as scatter plots instead of line plots.
- **Ctrl-click** to select multiple timeseries or to **deselect** a timeseries.
- Some timeseries exist for multiple indexes. You can choose the indexes to show in the views below the plot. Multiselection is possible here too.
- If you want to calibrate the model and view **goodness of fit** statistics: Make sure your observation timeseries is in the input file as an 'additional timeseries' (see file format docs). Select the result series you want to calibrate for and your observation timeseries, and don't have any other timeseries selected. Also make sure you have only one index combination selected at a time. Select the 'Residuals' plot mode. The goodness of fit stats are displayed in the Results info text box.
- Click the name of a parameter group to see the parameters in that group. You can click a parameter value to edit it. Then you can re-run the model and see the effect. Some parameter groups index over multiple indexes. These indexes can be chosen above the parameter view. If 'lock' is checked, any edits will apply to all values across that index set.
- Right click the plot for options to change text, formatting and to export it to an image or pdf.
- More in-depth documentation will follow later.

There are a number of **known issues** with MobiView, e.g. relating to window sizing problems on some Windows 10 computers. See [here](https://github.com/NIVANorge/MobiView#needed-improvements-todo-list) for further details and possible workarounds. 


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

Additional contributions:
James Sample (statistics and auto-calibration, python utilities)
Leah Jackson-Blake (Simply models, general testing)
Jose-Luis Guerrero (Json format, general testing)
Many others: General testing, feedback, ideas.

For general questions and feedback, please contact (magnus.norling@niva.no)
