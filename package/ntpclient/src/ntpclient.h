/* when present, debug is a true global */
#ifdef ENABLE_DEBUG
extern int debug;
#else
#define debug 0
#endif

/* global tuning parameter */
extern double min_delay;

/* prototype for function defined in phaselock.c */
int contemplate_data(unsigned int absolute, double skew, double errorbar, int freq);
