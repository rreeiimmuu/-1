%{
#include "asm.y.h"
%}

%option yylineno

%%

"ARR_LOAD"  { return ARR_LOAD; }
"ARR_STORE" { return ARR_STORE; }
"AND"  {  return AND;  }
"OR"   {  return OR;   }
"ADD"  {  return ADD;  }

"SUB"  {  return SUB;  }

"MUL"  {  return MUL;  }

"DIV"  {  return DIV;  }

"TST"  {  return TST;  }

"STO"  {  return STO;  }

"LOD"  {  return LOD;  }

"JMP"  {  return JMP;  }

"JEZ"  {  return JEZ;  }

"JLZ"  {  return JLZ;  }

"JGZ"  {  return JGZ;  }

"DBN"  {  return DBN;  }

"DBS"  {  return DBS;  }

"IN"   {  return IN;   }

"OUTN" {  return OUTN; }

"OUTC" {  return OUTC; }

"OUTS" {  return OUTS; }

"NOP"  {  return NOP;  }

"END"  {  return END;  }

[0-9]*	{
	yylval.number = atoi(yytext);
	return INTEGER;
}

R[0-9]*  {
	yylval.number = atoi(yytext+1);
	return REG;
}

[A-Za-z]([A-Za-z]|[0-9])*  {  
	yylval.string = strdup(yytext);
	return LABEL;
}

[ \t\r\n]|#.*

.  {  return *yytext;  }

%%

int yywrap()
{
	return 1;
}

