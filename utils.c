/*
 * utils.c -- utility functions
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "utils.h"


void abbruch(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  fprintf(stderr, "Error: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

void *reserviere(unsigned size) {
  void *p;

  p = malloc(size);
  if (p == NULL) {
    abbruch("kein Speicher");
  }
  return p;
}

int yyerror(char *s){
   fprintf(stderr, "Fehlerhaftes Kommando");
     return 1;
}

void freigabe(void *p) {
  if (p == NULL) {
    abbruch("NULL pointer detected in release");
  }
  free(p);
}
