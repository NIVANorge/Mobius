

library('Rcpp')
setwd('Mobius/RWrapper')
sourceCpp('test.cpp')


mobius_setup_from_parameter_and_input_file('../Applications/SimplyP/Tarland/TarlandParameters_v0-3.dat', '../Applications/SimplyP/Tarland/TarlandInputs.dat')
mobius_run_model()
flow <- mobius_get_result_series('Reach flow (daily mean, cumecs)', c('Coull'))
plot(flow, type='l', col='blue')

none <- vector(mode='character', lenght=0)
mobius_set_parameter_double('PET multiplication factor', none, 5.0)
mobius_run_model()
flow2 <- mobius_get_result_series('Reach flow (daily mean, cumecs)', c('Coull'))
lines(flow2, type='l', col='red')