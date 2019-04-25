#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "listen.h"
#include "kommandos.h"
#include "frontend.h"
#include "parser.h"


Prozessinfo prozessinfoNeu(int pid, int pgid, int status, char *prog){
  Prozessinfo neu = reserviere(sizeof (struct prozessinfo));
  char *prog2 = reserviere(strlen(prog)+1);
  strcpy(prog2, prog);
  neu->pid = pid;
  neu->pgid = pgid;
  neu->status = status;
  neu->prog = prog2;
  return neu;
}

Kommando kommandoNeuLeer(){
  Kommando neu = reserviere(sizeof (struct kommando));
  neu -> typ=K_LEER;
  return neu;
}

Kommando kommandoNeuEinfach(int laenge, char**wortliste, Liste umlenkungen, int vordergrund) {
  int i;

  Kommando neu      = reserviere(sizeof (struct kommando));
  neu->typ          = K_EINFACH;
  neu->endeabwarten = vordergrund;
  neu->u.einfach.umlenkungen = umlenkungen;
  neu->u.einfach.wortanzahl = laenge;

  /* NULL-terminierten Stringvektor mit den Kommandobestandteilen erzeugen */
  neu->u.einfach.worte = reserviere ( (laenge+1) * sizeof (char*));


  for(i=0; i<laenge; i++)
    (neu->u.einfach.worte)[i] = wortliste[i];
  (neu->u.einfach.worte)[laenge] = NULL;

  return neu;
}

Kommando kommandoSequenz(Kommandotyp t, Kommando k1, Kommando rest) {

  /* Listenkonstruktion für Sequenzen (z.B. Pipelines)
     falls rest schon eine Sequenz des benötigten Typs ist, wird
     die Sequenz als flache Liste "verlängert"
  */
  Kommando neu;

  if(rest->typ == t) {
    /* flaches Anfügen: vorhandenen Kommandobaum verwenden
                        an die zugehörige Liste neues Element anhängen
    */
    rest->u.sequenz.liste = listeAnfuegen (rest->u.sequenz.liste, k1);
    rest->u.sequenz.laenge++;
    return rest;
  }

  /* neue Sequenz mit 2 Elementen */
  neu = reserviere(sizeof (struct kommando));
  neu -> typ=t;
  neu -> endeabwarten=1;
  neu -> u.sequenz.liste = listeAnfuegen(listeNeu(rest), k1);
  neu -> u.sequenz.laenge= 2;
  return neu;
}

void kommandoLoeschen (Kommando k ) {

  switch(k->typ){
  case K_LEER:
    free(k);
    return;
  case K_EINFACH:
    /* ACHTUNG: Die Strings des Kommandos werden in wortlisteLeeren geloescht */
    free(k->u.einfach.worte);
    free(k);
    return;
  case K_PIPE:
  case K_SEQUENZ:
  case K_UND:
  case K_ODER:
  case K_IF:
    {
      Liste l = k->u.sequenz.liste;

      /* zuerst die Listenelemente loeschen, dies sind Kommandos */
      while(l){
        kommandoLoeschen( listeKopf(l) );
        l=listeRest(l);
      }

      /* jetzt die Liste löschen */
      listeLoeschen (k->u.sequenz.liste);

      /* zu guter Letzt */
      free(k);
    }
    return;
  default:
    abbruch("loescheKommando für Typ=%d nicht implementiert\n", k->typ);
  }
}

void einruecken(int ebene){
  int i;
  for(i=0; i<ebene; i++)
    fprintf(stderr, "  ");
}

void kommandoZeigen1 ( Kommando k, int ebene ) {
  int i;

  switch(k->typ){
  case K_LEER:
    einruecken(ebene);
    fprintf(stderr, "<leeres Kommando>\n");
    return;
  case K_EINFACH: {
    Liste ul = k->u.einfach.umlenkungen;
    Umlenkung *u;

    einruecken(ebene);
    fprintf(stderr, "<einfaches Kommando> %c\n", k->endeabwarten ? ' ' : '&');
    einruecken(ebene);
    for(i=0; i < k->u.einfach.wortanzahl; i++)
      fprintf(stderr, "\"%s\" ", (k->u.einfach.worte)[i]);
    /* ggf. Umleitungen anzeigen */
    while(ul){
      u = listeKopf(ul);
      fprintf(stderr, "%d%s %s", u->filedeskriptor, u->modus==READ ? "< " : u->modus==WRITE ? "> " : ">> ", u->pfad);
      ul = listeRest(ul);
    }
    fprintf(stderr, "\n");
    return;
  }
  case K_PIPE:
  case K_SEQUENZ:
  case K_UND:
  case K_ODER:
  case K_IF:
    {
      einruecken(ebene);
      /* Typ anzeigen */
      switch(k->typ){
      case K_PIPE:
	fprintf(stderr, "<Pipe-Kommando>%c \n", k->endeabwarten ? ' ' : '&');
	break;
      case K_SEQUENZ:
	fprintf(stderr, "<Kommandosequenz>%c \n", k->endeabwarten ? ' ' : '&');
	break;
      case K_UND:
	fprintf(stderr, "<UND-Verkettung>%c \n", k->endeabwarten ? ' ' : '&');
	break;
      case K_ODER:
	fprintf(stderr, "<ODER-Verkettung>%c \n", k->endeabwarten ? ' ' : '&');
	break;
      case K_IF:
	fprintf(stderr, "<IF-THEN-ELSE>%c \n", k->endeabwarten ? ' ' : '&');
	break;
      }
      /* Bestandteile anzeigen */
      {
	Liste l = k->u.sequenz.liste;
	while(l){
	  kommandoZeigen1( listeKopf(l), ebene+1 );
	  l=listeRest(l);
	}
      }
      return;
    }
  default:
    abbruch("zeigeKommando für Typ=%d nicht implementiert\n", k->typ);
  }
}

void kommandoZeigen ( Kommando k ) {
  kommandoZeigen1(k, 0);
}
