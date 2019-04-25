#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "listen.h"

Liste listeLeer(void){
  return NULL;
}

Liste listeNeu(void *element){
  Liste neu=reserviere(sizeof (struct liste));
  neu->kopf = element;
  neu->rest = NULL;
  return neu;
}

Liste listeAnfuegen(Liste l, void* element){
  Liste neu=reserviere(sizeof (struct liste));
  neu->kopf = element;
  neu->rest = l;
  return neu;
}

void  listeLoeschen(Liste l){
  if (l==NULL) return;
  listeLoeschen(l->rest);
  free(l);
}

void* listeKopf(Liste l) { 
  if(l==NULL)
    abbruch("listeKopf(listeLeer) undefiniert");
  return l->kopf; 
}

Liste listeRest(Liste l){
  if(l==NULL)
    abbruch("listeRest(listeLeer) undefiniert");
  return l->rest; 
}

int  listeIstleer(Liste l){
  return l==NULL;
}

int   listeLaenge(Liste l){
  return l==NULL ? 0 : 1+listeLaenge(l->rest);
}


