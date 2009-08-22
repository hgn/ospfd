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

%token EQUAL AREA ROUTERID PREFIX INTERFACE SET METRIC SHOW
%token IPV4 IPV6 ADDRESS DESCRIPTION

%token <word>  WORD

%%

commands:
    |        commands command
    ;


command: INTERFACE WORD SET AREA WORD    { rc_set_area($2, $5); }
		|    INTERFACE WORD SET METRIC WORD  { rc_set_metric($2, $5); }
		|    INTERFACE WORD SET IPV4 ADDRESS WORD WORD  { rc_set_ipv4_address($2, $6, $7); }
		|    INTERFACE WORD SHOW             { rc_show_interface($2); }
		|    INTERFACE WORD DESCRIPTION WORD { rc_set_description($2, $4); }
        |    ROUTERID EQUAL WORD                   { rc_set_id($3);   }
    |    PREFIX EQUAL WORD               { rc_set_id($3); }
    ;

