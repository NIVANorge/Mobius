

library('Rcpp')
setwd('Mobius/rwrapper')
sourceCpp('test.cpp')

none <- vector(mode='character', lenght=0)


mobius_setup_from_parameter_and_input_file('../Applications/SimplyP/Tarland/TarlandParameters_v0-3.dat', '../Applications/SimplyP/Tarland/TarlandInputs.dat')

mobius_set_parameter_time('Start date', none, '2003-01-01')
mobius_set_parameter_uint('Timesteps', none, 1000)
mobius_run_model()
flow <- mobius_get_result_series('Reach flow (daily mean, cumecs)', c('Coull'))
plot(flow, type='l', col='blue')


mobius_set_parameter_double('PET multiplication factor', none, 5.0)
mobius_run_model()
flow2 <- mobius_get_result_series('Reach flow (daily mean, cumecs)', c('Coull'))
lines(flow2, type='l', col='red')