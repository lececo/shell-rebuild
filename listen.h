typedef struct liste {
  void *kopf;
  struct liste *rest;
} *Liste;
  
Liste listeLeer(void);
Liste listeNeu(void *element);
Liste listeAnfuegen(Liste l, void* elem);
void  listeLoeschen(Liste l);
void* listeKopf(Liste l);
Liste listeRest(Liste l);
int   listeIstleer(Liste l);
int   listeLaenge(Liste l);
