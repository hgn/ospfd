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


%token EQUAL AREA ID PREFIX

%token <word>  WORD

%%

commands:
    |        commands command
    ;


command: AREA EQUAL WORD        { rc_add_area($3); }
    |    ID EQUAL WORD          { rc_add_id($3);   }
    |    PREFIX EQUAL WORD      { rc_add_id($3); }
    ;

