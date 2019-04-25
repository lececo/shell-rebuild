%{
  #include <stdio.h>
  #include <string.h>
  #include <stdlib.h>
  #include "utils.h"
  #include "listen.h"
  #include "frontend.h"
  #include "wortspeicher.h"
  #include "kommandos.h"
  #define YYDEBUG 1
  #define FEDEBUG 1

  int yylex(void);
  int yyerror(char*);

  Wortspeicher wsp; /* Speicher für Strings */
  Kommando k;      /* aktuelles Kommando */
  int exitwert=0;

%}

%union {
  Umlenkung*         umlenkungsAttr;
  StringAttr         stringAttr;
  StringlistenAttr   stringlistenAttr;
  Kommando           kommandoAttr;
  Liste              listenAttr;
}

%token ANSONSTEN UNDDANN DATEIANFUEGEN IF THEN ELSE FI
%token <stringAttr>       STRING UNDEF
%type  <stringAttr>       String
%type  <stringlistenAttr> Stringliste
%type  <kommandoAttr>     Kommando Aufruf UndKommando OderKommando Pipeline IfKommando SequenzKommando
%type  <listenAttr>       Umlenkungen
%type  <umlenkungsAttr>   Umlenkung


%left     ';'
%left     '|'
%nonassoc '&'

%%
Zeile:  { exitwert=0; } Kommando '\n' { k=$2; return exitwert; }
       | /* leer */ '\n'  { k=kommandoNeuLeer(); return 0; }
       | /* EOF */        { exit(0); }
;

Trenner: ';' | Newlines;

Newlines: '\n' OptNewlines;

OptNewlines: Newlines | /* Empty */;

Kommando:    Aufruf { $$=$1; }
             | SequenzKommando { $$=$1; }
             | UndKommando { $$=$1; }
             | OderKommando { $$=$1; }
             | Pipeline { $$=$1; }
             | IfKommando { $$=$1; }
             ;

Aufruf:   Stringliste Umlenkungen     { $$ = kommandoNeuEinfach($1.laenge, (wsp->worte)+$1.anfang, $2, 1); }
        | Stringliste Umlenkungen '&' { $$ = kommandoNeuEinfach($1.laenge, (wsp->worte)+$1.anfang, $2, 0); }
;

IfKommando:    IF OptNewlines Aufruf Trenner THEN OptNewlines Aufruf Trenner ELSE OptNewlines Aufruf Trenner FI
                  {
                    $$=kommandoSequenz(K_IF, $3, kommandoSequenz(K_IF, $7, $11));
                  }
             |
             IF OptNewlines Aufruf Trenner THEN OptNewlines Aufruf Trenner FI
                  {
                    /* if k1; then k2; else; k3; fi  =  k1 && k2 || k3 */
                    $$=kommandoSequenz(K_UND, $3, $7);
                  }
;

SequenzKommando:     Aufruf ';' Aufruf            { $$=kommandoSequenz(K_SEQUENZ, $1, $3); }
                   | Aufruf ';' SequenzKommando   { $$=kommandoSequenz(K_SEQUENZ, $1, $3); }
;


OderKommando:  Aufruf ANSONSTEN Aufruf       { $$=kommandoSequenz(K_ODER, $1, $3); }
            |  Aufruf ANSONSTEN OderKommando { $$=kommandoSequenz(K_ODER, $1, $3); }
;

UndKommando:   Aufruf UNDDANN Aufruf       { $$=kommandoSequenz(K_UND, $1, $3); }
            |  Aufruf UNDDANN UndKommando  { $$=kommandoSequenz(K_UND, $1, $3); }
;


Pipeline:      Aufruf '|' Aufruf           { $$=kommandoSequenz(K_PIPE, $1, $3); }
            |  Aufruf '|' Pipeline         { $$=kommandoSequenz(K_PIPE, $1, $3); }
;

Stringliste:         String { $$.laenge=1; $$.anfang=wortspeicherEinfuegen(wsp,$1.str); }
                   | Stringliste String {
                         $$.anfang=$1.anfang;
                         wortspeicherEinfuegen(wsp,$2.str);
                         $$.laenge=$1.laenge+1; }
;

Umlenkungen:   /* leer */ { $$=NULL; }
             | Umlenkung Umlenkungen { $$=listeAnfuegen($2, $1); }
;

Umlenkung:   '>'           String { $$=reserviere(sizeof (Umlenkung));
                                    $$->filedeskriptor = 1;
                                    $$->modus=WRITE;
                                    $$->pfad = $2.str;
                                  }
           | DATEIANFUEGEN String { $$=reserviere(sizeof (Umlenkung));
                                    $$->filedeskriptor = 1;
                                    $$->modus=APPEND;
                                    $$->pfad = $2.str;
	                          }
           | '<'           String { $$=reserviere(sizeof (Umlenkung));
                                    $$->filedeskriptor = 0;
                                    $$->modus=READ;
                                    $$->pfad = $2.str;
                                  }
;

String:    STRING { $$=$1; }
         | UNDEF  { fprintf(stderr, "undefiniertes Zeichen \'%c\' (=0x%0x)\n", $1.str[0], $1.str[0]);
                    exitwert = -1; $$=$1;
                  }
;
%%
