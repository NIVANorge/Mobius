

library('Rcpp')
setwd('Mobius/RWrapper')
sourceCpp('mobius_r.cpp')

none <- vector(mode='character', length=0) #It would be nice to use NULL or NA instead, but that does not interface well with C++ it seems


mobius_setup_from_parameter_and_input_file('../Applications/SimplyP/Tarland/TarlandParameters_v0-3.dat', '../Applications/SimplyP/Tarland/TarlandInputs.dat')

# alternatively, read your input data in from any source:
# airt<-read_it_in_from_whatever_format_you_want()
# precip<-read_this_in_from_whatever_format_too()
# input_data_start_date<-'1999-01-01'
# mobius_setup_from_parameter_file_and_input_series('../Applications/SimplyP/Tarland/TarlandParameters_v0-3.dat', input_data_start_date, airt, precip)


# This does not modify the global dataset and is thread safe:
df<-mobius_run_with(c('PET multiplication factor', 'Baseflow index'), list(none, none), c(2.0, 0.7), c('Reach flow (daily mean, cumecs)', 'Agricultural soil water flow'), list(c('Coull'), c('Coull')))
reachflow<-df[[1]]
soilflow<-df[[2]]



# This does modify the global dataset and is NOT thread safe:
mobius_set_parameter_time('Start date', none, '2003-01-01')
mobius_set_parameter_uint('Timesteps', none, 1000)
mobius_run_model()
flow <- mobius_get_result_series('Reach flow (daily mean, cumecs)', c('Coull'))
plot(flow, type='l', col='blue')

mobius_set_parameter_double('PET multiplication factor', none, 5.0)
mobius_run_model()
flow2 <- mobius_get_result_series('Reach flow (daily mean, cumecs)', c('Coull'))
lines(flow2, type='l', col='red')