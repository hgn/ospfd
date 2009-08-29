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

%token EQUAL AREA ROUTERID PREFIX INTERFACE SET COSTS SHOW
%token IPV4 IPV6 ADDRESS DESCRIPTION HELLOINTERVAL UP ROUTERPRIORITY
%token ROUTERDEADINTERVAL

%token <word>  WORD

%%

commands:
    |        commands command
    ;


command: INTERFACE WORD SET AREA WORD    { rc_set_area($2, $5); }
		|    INTERFACE WORD SET COSTS WORD  { rc_set_costs($2, $5); }
		|    INTERFACE WORD SET IPV4 ADDRESS WORD WORD  { rc_set_ipv4_address($2, $6, $7); }
		|    INTERFACE WORD SHOW             { rc_show_interface($2); }
		|    INTERFACE WORD DESCRIPTION WORD { rc_set_description($2, $4); }
    |    ROUTERID EQUAL WORD             { rc_set_id($3);   }
    |    INTERFACE WORD SET HELLOINTERVAL WORD  { rc_set_hello_interval($2, $5);   }
    |    INTERFACE WORD UP               { rc_set_interface_up($2);   }
		|    INTERFACE WORD SET ROUTERPRIORITY WORD  { rc_set_router_priority($2, $5); }
		|    INTERFACE WORD SET ROUTERDEADINTERVAL WORD  { rc_set_router_dead_interval($2, $5); }
    |    SET ROUTERID WORD               { rc_set_id($3); }
    ;

