/*
  Shell-Beispielimplementierung

  Die Implementierung verzichtet aus Gründen der Einfachheit
  des Parsers auf eine vernünftige Heap-Speicherverwaltung:

  Falls der Parser die Analyse eines Kommandos wegen eines
  Syntaxfehlers abbricht, werden die ggf. vorhandenen
  Syntaxbäume für erfolgreich analysierte Unterstrukturen des
  fehlerhaften Kommandos nicht gelöscht.

  Beispiel: if test ! -d /tmp; then mkdir /tmp; else echo "/tmp vorhanden" fi

  Die Analyse wird mit Fehlermeldung abgebrochen, weil vor dem "fi" das
  obligatorische Semikolon fehlt. Im Heap stehen zu diesem Zeitpunkt die
  Bäume für das test- und das mkdir-Kommando. Diese verbleiben als Müll
  im Heap, da die Verweise ausschließlich auf dem Parser-Stack stehen,
  der im Fehlerfall nicht mehr ohne weiteres zugänglich ist.

  Um dies zu beheben, müsste man
  a) sich beim Parsen die Zeiger auf die Wurzeln aller
     konstruierten Substruktur-Bäume solange in einer globalen Liste merken,
     bis die Analyse der kompletten Struktur ERFOLGREICH beendet ist
  oder
  b) die Grammatik mit Fehlerregeln anreichern, in denen die Freigabe
     im Fehlerfall explizit vorgenommen wird.

  Da beides die Grammatik aber stark aufbläht, wird darauf verzichtet.
*/

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "utils.h"
#include "listen.h"
#include "wortspeicher.h"
#include "kommandos.h"
#include "frontend.h"
#include "parser.h"
#include "variablen.h"

extern int yydebug;
extern int yyparse(void);
extern int interpretiere(Kommando k, int forkexec);

int shellpid;

/* Zum Debuggen: Ausgaben in Sigchldhandler, wer welche PID zu welcher ändert*/

void endesubprozess(int sig){
  int wstatus;
  int pid = waitpid(-1, &wstatus, WNOHANG);
  if(pid > 0){
    tcsetpgrp(0, getpgid(getpid()));
    if(prozessinfoIstVorhanden(pid)){
     // printf("signalhandler: Update it man! pid:%d  status: %d", pid, wstatus); 
      updateStatus(pid, wstatus);
    }
    else{
      updatePid(0, pid);
     // printf("signalhandler: Update it man! pid:%d  status: %d", pid, wstatus);
         
      updateStatus(pid, wstatus);
    }
  }
}

void strgcSignal(int sig){
  printf("\nSTRG-C gedrückt\n");
}

void init_signalbehandlung(){
  struct sigaction sigchldSignalbehandlung;
  struct sigaction sigttouSignalbehandlung;
  struct sigaction sigintSignalbehandlung;

  sigintSignalbehandlung.sa_handler = strgcSignal;
  sigemptyset(&sigintSignalbehandlung.sa_mask);
  sigintSignalbehandlung.sa_flags = 0;
  sigaction(SIGINT, &sigintSignalbehandlung, NULL);

  sigttouSignalbehandlung.sa_handler = SIG_IGN;
  sigemptyset(&sigttouSignalbehandlung.sa_mask);
  sigttouSignalbehandlung.sa_flags = 0;
  sigaction(SIGTTOU, &sigttouSignalbehandlung, NULL);

  sigchldSignalbehandlung.sa_handler = endesubprozess;
  sigemptyset(&sigchldSignalbehandlung.sa_mask);
  sigchldSignalbehandlung.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  sigaction(SIGCHLD, &sigchldSignalbehandlung, NULL);
}

int main(int argc, char *argv[]){
  int  zeigen=0, ausfuehren=1;
  int status, i;
  prozessliste = listeLeer();
  init_signalbehandlung();
  yydebug=0;

  for(i=1; i<argc; i++){
    if (!strcmp(argv[i],"--zeige"))
      zeigen=1;
    else if  (!strcmp(argv[i],"--noexec"))
      ausfuehren=0;
    else if  (!strcmp(argv[i],"--yydebug"))
      yydebug=1;
    else {
      fprintf(stderr, "Aufruf: %s [--zeige] [--noexec] [--yydebug]\n", argv[0]);
      exit(1);
    }
  }

  wsp=wortspeicherNeu();

  while(1){
    int res;
    fputs(">> ", stdout);
    fflush(stdout);
    res=yyparse();
    if(res==0){
      if(zeigen)
        kommandoZeigen(k);
      if(ausfuehren)
        status=interpretiere(k, 1);
      if(zeigen)
        fprintf(stderr, "Status: %d\n", status);
      kommandoLoeschen(k);
    }
    else
      fputs("Fehlerhaftes Kommando\n", stderr);
    wortspeicherLeeren(wsp);
  }
}
