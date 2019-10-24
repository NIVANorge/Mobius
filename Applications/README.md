# Applications

These are C++ projects that combine modules into full models, and can be compiled to produce MobiView- and pythonwrapper- compatible dlls.

These folders also contain some example parameter and input files for the models.

## Completness of models

The following model implementations are considered to be in "release quality", in the sense that they can be (and are!) used in scientific studies. (This does not mean that we will not continue improving on them).
- INCA-N
- SimplyP
- SimplyQ
- PERSiST
- INCA-C

The following model implementations are probably correct and have been tested on some real datasets, but not all aspects of them have been rigorously tested.
- INCA-P
- INCA-MP
- INCA-Sed

The following models are in development and are not finished
- SimplyC
- INCA-Tox
- MAGIC
- EasyLake

INCA-N-classic is not supported by us and has some bugs in it. HBV is a leftover from a previous project that was not finished, and would probably need to be cleaned up a little before proper use.

## Note on example files

Most of the example input and parameter data files are not provided with source or metadata. They are only provided to teach users how to set up the models for use on real data. Please don't use them in studies without obtaining the correct metadata and attributions.
