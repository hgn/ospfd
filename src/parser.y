%{
#include <stdio.h>
#include <string.h>

#include "rc.h"

int yywrap(void)
{
        return 1;
}

%}

%{
int yylex(void);
%}


%union {
    char *word;
}


%token EQUAL AREA ID PREFIX INTERFACE SET METRIC

%token <word>  WORD

%%

commands:
    |        commands command
    ;


command: INTERFACE WORD SET AREA WORD    { rc_set_area($2, $5); }
		|    INTERFACE WORD SET METRIC WORD  { rc_set_metric($2, $5); }
    |    ID EQUAL WORD                   { rc_set_id($3);   }
    |    PREFIX EQUAL WORD               { rc_set_id($3); }
    ;

