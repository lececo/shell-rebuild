#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>


#include "utils.h"
#include "listen.h"
#include "wortspeicher.h"
#include "kommandos.h"
#include "frontend.h"
#include "parser.h"
#include "variablen.h"

Liste prozessliste;
int interpretiere(Kommando k, int forkexec);
void sigttouAktivieren(void);
void zeigeProzessliste();
void loescheTerminierteProzesse(void);

int pgid_global = -100;

void do_execvp(int argc, char **args){
  execvp(*args, args);
  perror("exec-Fehler");
  fprintf(stderr, "bei Aufruf von \"%s\"\n", *args);
  exit(1);
}

int interpretiere_pipeline(Kommando k){
  Liste l = k->u.sequenz.liste;
  int anzKomms = k->u.sequenz.laenge;
  int anzPipes = anzKomms - 1;
  int pipeDeskriptoren[anzPipes*2];
  int j=0, c=0, status, pgid, pid;
  for(int i=0; i<anzPipes; i++){
    if(pipe(pipeDeskriptoren + i*2) < 0){
      perror("pipe-Fehler");
      return -1;
    }
  }
  while(!listeIstleer(l)){
    Prozessinfo pi = prozessinfoNeu(0, 0, -1, *(((Kommando)listeKopf(l))->u.einfach.worte));
    prozessliste = listeAnfuegen(prozessliste, pi);
    pid = fork();
    switch(pid){
      case -1: perror("fork-Fehler");
               return -1;
      case 0:  if(c == 0){
                  pgid = getpid();
                  if(setpgid(0, 0) < 0){
                    perror("Fehler bei setpgid1");
                    return -1;
                  }
                  tcsetpgrp(0, pgid);
               }
               else{
                  if(setpgid(0, pgid) < 0){
                    perror("Fehler bei setpgid2");
                    return -1;
                  }
               }
               if(c>0){
                  if(dup2(pipeDeskriptoren[j-2], 0) < 0){
                    perror("dup2-Fehler bei Lesedeskriptor");
                    return -1;
                  }
               }
               if(c<(anzKomms-1)){
                 if(dup2(pipeDeskriptoren[j+1], 1) < 0){
                   perror("dup2-Fehler bei Schreibdeskriptor");
                   return -1;
                 }
               }
               for(int i=0; i<anzPipes*2; i++){
                 close(pipeDeskriptoren[i]);
               }
               status = interpretiere((Kommando)listeKopf(l), 0);
      default: if(c == 0){
                  pgid = pid;
                  if(setpgid(pid, pgid) < 0){
                    perror("Fehler bei setpgid1");
                    return -1;
                  }
                  tcsetpgrp(0, pgid);
               }
               else{
                 if(setpgid(pid, pgid) < 0){
                   perror("Fehler bei setpgid2");
                   return -1;
                 }
               }
               updatePid(0, pid);
               updatePgid(pid, getpgid(pid));
    }
    l = listeRest(l);
    c++;
    j+=2;
  }
  for(int i=0; i<anzPipes*2; i++){
    close(pipeDeskriptoren[i]);
  }
  for(int i=0; i<anzKomms; i++){
    int wstatus = -1;
    if(k->endeabwarten){
      int pid2 = waitpid(-1, &wstatus, WUNTRACED | WCONTINUED);
      if(pid2 != -1){
        updateStatus(pid2, wstatus);
      }
    }
    else{
      fprintf(stderr, "PID: %d, PGID: %d des im Hintergrund laufenden Prozesses\n", pid, pgid);
    }
  }
  tcsetpgrp(0, getpgid(getpid()));
  return status;
}

int umlenkungen(Kommando k){
  Liste uml = k->u.einfach.umlenkungen;
  while(!listeIstleer(uml)){
    Umlenkung u = *(Umlenkung*)listeKopf(uml);
    switch(u.modus){
      case READ:
       {
        int fdr = open(u.pfad, O_RDONLY);
        if(fdr < 0){
          perror("");
          fprintf(stderr, "open-Fehler bei Eingabeumlenkung der Datei: %s\n", u.pfad);
          return -1;
        }
        if(dup2(fdr, 0) < 0){
          perror("");
          fprintf(stderr, "dup2-Fehler bei Eingabeumlenkung der Datei: %s\n", u.pfad);
          return -1;
        }
        close(fdr);
        break;
       }
      case WRITE:
       {
        int fdw = open(u.pfad, O_CREAT | O_TRUNC | O_WRONLY, 00700);
        if(fdw < 0){
          perror("");
          fprintf(stderr, "open-Fehler bei Ausgabeumlenkung der Datei: %s\n", u.pfad);
          return -1;
        }
        if(dup2(fdw, 1) < 0){
          perror("");
          fprintf(stderr, "dup2-Fehler bei Ausgabeumlenkung der Datei: %s\n", u.pfad);
          return -1;
        }
        close(fdw);
        break;
      }
      case APPEND:
       {
        int fda = open(u.pfad, O_CREAT | O_APPEND | O_WRONLY, 00700);
        if(fda < 0){
          perror("");
          fprintf(stderr, "open-Fehler bei Ausgabeumlenkung mit Anfügen der Datei: %s\n", u.pfad);
          return -1;
        }
        if(dup2(fda, 1) < 0){
          perror("");
          fprintf(stderr, "dup2-Fehler bei Ausgabeumlenkung mit Anfügen der Datei: %s\n", u.pfad);
          return -1;
        }
        close(fda);
        break;
       }
    }
    uml = listeRest(uml);
  }
  return 0;
}

int aufruf(Kommando k, int forkexec){
  if(forkexec){
    Prozessinfo pi = prozessinfoNeu(0, 0, -1, *(k->u.einfach.worte));
    prozessliste = listeAnfuegen(prozessliste, pi);
    int pid=fork();
    int wstatus = -1;
    int pgid;
    switch (pid){
    case -1:
      perror("Fehler bei fork");
      return(-1);
    case 0:
      sigttouAktivieren();
      if(setpgid(0, 0) == -1){
        perror("Fehler bei setpgid");
        return -1;
      }
      if(k->endeabwarten){
        if(tcsetpgrp(0, getpgid(getpid())) == -1){
          perror("Fehler bei tcsetpgrp im Kind");
          return -1;
        }
      }
      if(umlenkungen(k) == -1) exit(1);
      do_execvp(k->u.einfach.wortanzahl, k->u.einfach.worte);
      abbruch("interner Fehler 001"); /* sollte nie ausgeführt werden */
    default:
      if(setpgid(pid, pid) == -1){
        perror("Fehler bei setpgid");
        return -1;
      }
      if(k->endeabwarten){
        if(tcsetpgrp(0, getpgid(pid)) == -1){
          perror("Fehler bei tcsetpgrp im Elternprozess 1");
          return -1;
        }
      }
      updatePid(0, pid);
      updatePgid(pid, pid);
      pgid = getpgid(pid);
      if(k->endeabwarten){
        int pid2 = waitpid(pid, &wstatus, WUNTRACED | WCONTINUED);
        if(pid2 != -1){
          if(tcsetpgrp(0, getpgid(getpid())) == -1){
            perror("Fehler bei tcsetpgrp im Elternprozess 2");
            return -1;
          }
          updatePgid(pid, pgid);
          updateStatus(pid, wstatus);
        }
      }
      else{
        fprintf(stderr, "PID: %d, PGID: %d des im Hintergrund laufenden Prozesses\n", pid, pid);
      }

      if(WIFEXITED(wstatus)){
        return WEXITSTATUS(wstatus);
      }
      else if(WIFSIGNALED(wstatus)){
        return 1;
      }
      return 0;
    }
  }

  /* nur exec, kein fork */

  if(umlenkungen(k) == -1) exit(1);
  do_execvp(k->u.einfach.wortanzahl, k->u.einfach.worte);
  abbruch("interner Fehler 001"); /* sollte nie ausgeführt werden */
  exit(1);
}


int interpretiere_einfach(Kommando k, int forkexec){
  char **worte = k->u.einfach.worte;
  int anzahl=k->u.einfach.wortanzahl;

  if (strcmp(worte[0], "exit")==0) {
    switch(anzahl){
    case 1:
      exit(0);
    case 2:
      exit(atoi(worte[1]));
    default:
      fputs( "Aufruf: exit [ ZAHL ]\n", stderr);
      return -1;
    }
  }

  if (strcmp(worte[0], "cd")==0) {
    switch(anzahl){
      case 1:
        if(chdir(getenv("HOME")) < 0){
          perror("Fehler bei chdir zum Home-Verzeichnis");
          return -1;
        }
        break;
      case 2:
        if(chdir(worte[1]) < 0){
          perror("Fehler bei chdir");
          return -1;
        }
        break;
      default:
        fputs("Aufruf: cd [ PFAD ], zu viele Argumente!\n", stderr);
        return -1;
    }
    return 0;
  }

  if(strcmp(worte[0], "fg") == 0){
    int pgid = atoi(worte[1]);
    tcsetpgrp(0, pgid);
    if(kill(-(pgid), SIGCONT) == -1){
      perror("Fehler bei kill in fg");
      return -1;
    }
    fgGezieltesWarten(pgid);
    tcsetpgrp(0, getpgid(getpid()));
    return 0;
  }

// Wir updaten die Statusse der Prozesse mit der angegebenen PGID, um bei Prozessen, die durch SIGTTIN blockiert werden, den richtigen Status in der Prozessliste zu haben
  if(strcmp(worte[0], "bg") == 0){
    int pgid = atoi(worte[1]);
    if(kill(-(pgid), SIGCONT) == -1){
      perror("Fehler bei kill in bg");
      return -1;
    }
    pgid_global = pgid;
    statusseUpdaten(pgid);
    return 0;
  }

  if(strcmp(worte[0], "status") == 0){
    statusseUpdaten(pgid_global);
    zeigeProzessliste();
    if(!listeIstleer(prozessliste)) loescheTerminierteProzesse();
    return 0;
  }

  return aufruf(k, forkexec);
}

int interpretiere(Kommando k, int forkexec){
  int status;
  switch(k->typ){
  case K_LEER:
    return 0;
  case K_EINFACH:
    return interpretiere_einfach(k, forkexec);
  case K_SEQUENZ:
    {
      Liste l = k->u.sequenz.liste;
      while(!listeIstleer(l)){
	       status=interpretiere ((Kommando)listeKopf(l), forkexec);
	       l=listeRest(l);
         return status;
      }
    }
  case K_IF:
   {
     Liste l = k->u.sequenz.liste;
    if(interpretiere((Kommando)listeKopf(l), forkexec) == 0){
      return interpretiere((Kommando)listeKopf(listeRest(l)), forkexec);
    }
    else{
      return interpretiere((Kommando)listeKopf(listeRest(listeRest(l))), forkexec);
    }
   }
  case K_UND:
   {
     Liste l = k->u.sequenz.liste;
     while(!listeIstleer(l)){
       if((status = interpretiere((Kommando)listeKopf(l), forkexec)) == 0){
         l = listeRest(l);
       }
       else{
         return -1;
       }
     }
     return status;
   }
  case K_ODER:
   {
    Liste l = k->u.sequenz.liste;
    while(!listeIstleer(l)){
      if((status = interpretiere((Kommando)listeKopf(l), forkexec)) == 0){
        return -1;
      }
      else{
        l = listeRest(l);
      }
    }
    return status;
   }
  case K_PIPE:
   return interpretiere_pipeline(k);
  default:
    fputs("unbekannter Kommandotyp, Bearbeitung nicht implementiert\n", stderr);
    break;
  }
  return 0;
}


void statusseUpdaten(int pgid){
  int status = 0;
  Liste aktuellePL = prozessliste;
  while(!listeIstleer(aktuellePL)){
    Prozessinfo aktuellePI = (Prozessinfo)listeKopf(aktuellePL);
    status = 0;
    if(aktuellePI-> pgid == pgid || aktuellePI->pid == pgid){
      if(waitpid(-(aktuellePI->pgid), &status, WNOHANG | WUNTRACED | WCONTINUED) > 0){
          aktuellePI->status = status;
      }
    }
    aktuellePL = listeRest(aktuellePL);
  }
}



void zeigeProzessliste(){
  Liste aktuellePL = prozessliste;
  printf("PID \t PGID \t STATUS \t PROG\n");
  while(!listeIstleer(aktuellePL)){
    Prozessinfo aktuellePI = (Prozessinfo)listeKopf(aktuellePL);
    int pid = aktuellePI->pid;
    int pgid = aktuellePI->pgid;
    int status = aktuellePI->status;
    char *prog = aktuellePI->prog;

    // Prozesse, die in der Prozessliste noch im Zustand running sind, aber ihr tatsächlicher Status sich geändert hat, werden hier in der Prozessliste geupdatet (z.B. vi &)
    if(status == -1){
        int wstatus;
        int pid3 = waitpid(pid, &wstatus, WNOHANG | WUNTRACED | WCONTINUED);
        if(pid3 > 0){
          updateStatus(pid, wstatus);
          status = wstatus;
        }
    }
    if(status == -1){
      printf("%d \t %d \t running \t %s\n", pid, pgid, prog);
    }
    else if(WIFCONTINUED(status)){
      printf("%d \t %d \t continued \t %s\n", pid, pgid, prog);
    }
    else if(WIFEXITED(status)){
      printf("%d \t %d \t exit(%d) \t %s\n", pid, pgid, WEXITSTATUS(status), prog);
    }
    else if(WIFSTOPPED(status)){
      printf("%d \t %d \t stopped \t %s\n", pid, pgid, prog);
    }
    else if(WIFSIGNALED(status)){
      printf("%d \t %d \t signal(%d) \t %s\n", pid, pgid, WTERMSIG(status), prog);
    }
    aktuellePL = listeRest(aktuellePL);
  }
}

void loescheTerminierteProzesse(void){
  Liste neueProzessliste = NULL;
  while(!listeIstleer(prozessliste)){
    if((((Prozessinfo)listeKopf(prozessliste))->status) == -1 || WIFSTOPPED(((Prozessinfo)listeKopf(prozessliste))->status) || WIFCONTINUED(((Prozessinfo)listeKopf(prozessliste))->status)){
      neueProzessliste = listeAnfuegen(neueProzessliste, listeKopf(prozessliste));
    }
    prozessliste = listeRest(prozessliste);
  }
  prozessliste = neueProzessliste;
}

void sigttouAktivieren(void){
  struct sigaction sigttouSignalbehandlung;
  sigttouSignalbehandlung.sa_handler = SIG_DFL;
  sigemptyset(&sigttouSignalbehandlung.sa_mask);
  sigttouSignalbehandlung.sa_flags = 0;
  sigaction(SIGTTOU, &sigttouSignalbehandlung, NULL);
}


int prozessinfoIstVorhanden(int pid){
  Liste aktuellePL = prozessliste;
  while(!listeIstleer(aktuellePL)){
    if(((Prozessinfo)listeKopf(aktuellePL))->pid == pid){
      return 1;
    }
    aktuellePL = listeRest(aktuellePL);
  }
  return 0;
}

int updatePid(int oldPID, int pid){
  Liste aktuellePL = prozessliste;
  while(!listeIstleer(aktuellePL)){
    Prozessinfo aktuellePI = (Prozessinfo)listeKopf(aktuellePL);
    if(aktuellePI->pid == oldPID){
      aktuellePI->pid = pid;
      return 1;
    }
    aktuellePL = listeRest(aktuellePL);
  }
  return 0;
}

int updatePgid(int pid, int pgid){
  Liste aktuellePL = prozessliste;
  while(!listeIstleer(aktuellePL)){
    Prozessinfo aktuellePI = (Prozessinfo)listeKopf(aktuellePL);
    if(aktuellePI->pid == pid){
      aktuellePI->pgid = pgid;
      return 1;
    }
    aktuellePL = listeRest(aktuellePL);
  }
  return 0;
}

int updateStatus(int pid, int status){
  Liste aktuellePL = prozessliste;
  while(!listeIstleer(aktuellePL)){
    Prozessinfo aktuellePI = (Prozessinfo)listeKopf(aktuellePL);
    if(aktuellePI->pid == pid){
      aktuellePI->status = status;
      updateGroupStatus(aktuellePI->pgid, status);
      return 1;
    }
    aktuellePL = listeRest(aktuellePL);
  }
  return 0;
}

void updateGroupStatus(int pgid, int status){
  Liste aktuellePL = prozessliste;
  while(!listeIstleer(aktuellePL)){
    Prozessinfo aktuellePI = (Prozessinfo)listeKopf(aktuellePL);
    if(aktuellePI-> pgid == pgid){
          aktuellePI->status = status;
    }

    aktuellePL = listeRest(aktuellePL);
  }
}

void fgGezieltesWarten(int pgid){
  Liste aktuellePL = prozessliste;
  while(!listeIstleer(aktuellePL)){
    int status;
    Prozessinfo aktuellePI = (Prozessinfo)listeKopf(aktuellePL);
    int aktuellePID = aktuellePI->pid;
    if(aktuellePI->pgid == pgid){
      if(waitpid(aktuellePID, &status, WUNTRACED) == aktuellePID){
        updateStatus(aktuellePID, status);
      }
    }
    aktuellePL = listeRest(aktuellePL);
  }
}
