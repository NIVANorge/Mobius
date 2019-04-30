# Documentation

The documentation is being written. 
We welcome feedback on the documentation in order to make it more useful.

### Mostly finished:
**file_format_documentation** - The parameter and input file formats.
**model_builder_documentation** - For people who want to build new models using the framework.

### Planned:
**model_user_documentation** - For people who want to use existing models, typically packaged in INCAView-compatible exes.
**framework_developer_documentation** - Not exhaustive, but to give a good overview of how the framework works internally for people who want to extend on it. How some of the core algorithms and data organization works.

### Maybe:
Documentation of individual models. However this is time consuming. Many of these models are published in articles, and you can combine reading those with reading the model source code (under the Modules folder). The source code is often very readable and self-documenting in the choice of variable names.

## Known Mobius bugs:

-  If you have accidentally made a circular reference between equations, Mobius will report this. This is good. However if the circular reference involves both equations outside and inside a solver batch, Mobius, in its error report, will sometimes get confused about what equation inside the solver was involved.
