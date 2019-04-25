/* Speicherfeld fuer alle Strings im Kommando */

typedef struct ws {
    int groesse, frei;
    char** worte;
  } *Wortspeicher;



EXTERNCPP Wortspeicher wortspeicherNeu();
EXTERNCPP int wortspeicherEinfuegen(Wortspeicher ws, char *wort);
EXTERNCPP void wortspeicherLeeren(Wortspeicher ws);
EXTERNCPP void wortspeicherZeigen(Wortspeicher ws);
