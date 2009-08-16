%{
/* definition section */
#include <stdio.h>
#include <stdlib.h>
#include "parser.tab.h"
#include "ospfd.h"

#define MAXLEXERR 1024

/* error stuff */
char lexerrbuff[MAXLEXERR];
int lineno = 1;
void yyerror(char *s);

%}

%%

#.*                              ;
area                             { return AREA; }
id                               { return ID; }
prefix                           { return PREFIX; }
[a-zA-Z0-9./][a-zA-Z0-9_./]*     { yylval.word  = strdup(yytext); return WORD; }
=                                { return EQUAL; }
\n.*                             { strncpy(lexerrbuff, yytext + 1, MAXLEXERR - 1);
                                   lineno++;
                                   yyless(1);
                                 }
[ \t]+                           ;                   /* ignone whitespaces */ 
.                                ;                   /* FIXME: add a catch all rule */  

%%


void
yyerror(char *str)
{
	fprintf(stderr, "Failed to parse configuration file (line %d):%s (%s)",
                          lineno, str, lexerrbuff);
	exit(333);
}
