

library('Rcpp')
setwd('Mobius/rwrapper')
Sys.setenv('PKG_CXXFLAGS"="-std=c++11')
sourceCpp('test.cpp')
flow<-Run()
plot(flow, type='l')