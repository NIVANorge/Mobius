# SimplyC

This folder contains an application of a first prototype version of SimplyC, a simple model to simulate daily in-stream DOC concentrations and fluxes. The main model definition file, SimplyC.h is within the Mobius 'Modules' folder. SimplyC uses SimplyQ to simulate hydrology.

**The model is currently under development** and so far has only been applied in the [Langtjern catchment](https://www.niva.no/en/services/environmental-monitoring/langtjern), a small (<5km^2) catchment in south-central Norway. The model does not currently include any long-term process-representation (e.g. the link between acid deposition and DOC leaching), and climatic drivers currently only affect simulated DOC leaching through changes in simulated hydrology (i.e. possible changes in soil carbon pools due to climate effects are not included).

The scientific motivation behind the development of SimplyC is similar to that for SimplyP, which is briefly outlined in the [SimplyP readme](https://github.com/NIVANorge/Mobius/tree/master/Applications/SimplyP#simplyp-mobius-version). The SimplyC folder contents are  similar to the contents of the Applications/SimplyP folder, described in the SimplyP readme.

Contributions to SimplyC development and testing are welcome.
