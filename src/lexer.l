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
router-id                        { return ROUTERID; }
prefix                           { return PREFIX; }
set                              { return SET; }
interface                        { return INTERFACE; }
costs                            { return COSTS; }
show                             { return SHOW; }
ipv4                             { return IPV4; }
ipv6                             { return IPV6; }
description                      { return DESCRIPTION; }
address                          { return ADDRESS; }
hello-interval                   { return HELLOINTERVAL; }
router-priority                  { return ROUTERPRIORITY; }
router-dead-interval             { return ROUTERDEADINTERVAL; }
up                               { return UP; }
type                             { return TYPE; }
[a-zA-Z0-9./][a-zA-Z0-9_./]*     { yylval.word = strdup(yytext); return WORD; }
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
	fprintf(stderr, "Failed to parse configuration file (line %d):%s (%s)\n",
                          lineno, str, lexerrbuff);
}
