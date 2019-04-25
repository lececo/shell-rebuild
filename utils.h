/*
 * utils.h -- utility functions
 */


#ifndef _UTILS_H_
#define _UTILS_H_

#ifdef __cplusplus
#define EXTERNCPP extern "C"
#else
#define EXTERNCPP
#endif


EXTERNCPP void abbruch(char *fmt, ...);
EXTERNCPP void *reserviere(unsigned size);
EXTERNCPP void freigabe(void *p);


#endif /* _UTILS_H_ */
