%{
#include <stdlib.h>
#include "utils.h"
#include "listen.h"
#include "kommandos.h"
#include "frontend.h"
#include "parser.h"

int yywrap(void);

#define YY_INPUT(buf,result,max_size) \
         { \
         int c = getchar(); \
         result = (c == EOF) ? YY_NULL : (buf[0] = c, 1); \
         }
%}

%%
[ \t]           {}
"\\\n"          {}

[<>|;&()]       { /* Token der Länge 1 */ 
                  return yytext[0]; 
                }

if              { return IF;  }
then            { return THEN;  }
else            { return ELSE;  }
fi              { return FI;  }
"||"            { return ANSONSTEN;  }
"&&"            { return UNDDANN; }
">>"            { return DATEIANFUEGEN; }

"\n"            { /* Zeilenende = Eingabeende! */
                  return '\n';
                }

\"[^"]+\"       { /* Quoted String */
		  int l=strlen(yytext)-2; /* Anzahl Zeichen zwischen quotes */

		  yylval.stringAttr.str=reserviere(l+1);
		  /* String kopieren, Quotes weglassen */
		  strncpy(yylval.stringAttr.str, yytext+1, l);
		  /* String terminieren */
		  *(yylval.stringAttr.str+l)='\0';
                  return STRING;
		}

[A-Za-z0-9/_.\-+#]+  { /* Unquoted String */
		  int l=strlen(yytext)+1;

		  yylval.stringAttr.str=reserviere(l);
		  /* String kopieren */
		  strcpy(yylval.stringAttr.str, yytext);
                  return STRING;
		}

.               {  
		  yylval.stringAttr.str=reserviere(2);
		  strcpy(yylval.stringAttr.str, yytext);
                  return UNDEF;
                }

%%

int yywrap(){
  return 1;
}
