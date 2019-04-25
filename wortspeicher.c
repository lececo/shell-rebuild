#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "wortspeicher.h"

Wortspeicher wortspeicherNeu(){
  Wortspeicher ws = reserviere(sizeof (struct ws));
  ws->frei = ws->groesse = 0;
  ws->worte = NULL;
  return ws;
}

int wortspeicherEinfuegen(Wortspeicher ws, char *wort) {
  /* noch Speicher da? */
  if(ws->frei >= ws->groesse){
    /* mehr reservieren */
    ws->groesse += 100;
    ws->worte=realloc(ws->worte, sizeof (char*) * ws->groesse);
    if(ws->worte==NULL)
      abbruch("Wortspeicherüberlauf");
  }

  ws->worte[ws->frei++] = wort;
  return ws->frei - 1;
}

void wortspeicherLeeren(Wortspeicher ws){
  int i;

  for(i=0; i < ws->frei; i++)
     free((ws->worte)[i]);
  free(ws->worte);
  ws->frei = ws->groesse = 0;
  ws->worte = NULL;
}

void wortspeicherZeigen(Wortspeicher ws) {
  int i;

  if(ws->frei) {
    fprintf(stderr, "Wortspeicher:\n");    
    for(i=0; i<ws->frei; i++)
      fprintf(stderr, "%3d %s\n", i, (ws->worte)[i]);
  }
  else
    fprintf(stderr, "Wortspeicher leer\n");
}

