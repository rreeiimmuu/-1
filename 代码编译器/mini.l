%{
#include "tac.h"
#include "mini.y.h"
%}

%option yylineno

%%
"ref"   {  return REF; }
"%"     {  return REF_OP; }   /* 新增引用操作符 */
"&"     {  return ADDR_OF; }  /* 新增取地址操作符 */
"*"     { return '*'; } /* *号让语法分析器决定用途 */
"char"  {  return CHAR; }
"float" {  return FLOAT; }
"int"  {  return INT;  }

"input"  {  return INPUT;  }

"output"  {  return OUTPUT;  }

"return"  {  return RETURN;  }

"if"  {  return IF;  }

"else"  {  return ELSE;  }

"while"  {  return WHILE;  }
"break" { return BREAK; }

"for"    { return FOR; }

"["     { return '['; }
"]"     { return ']'; }

[A-Za-z]([A-Za-z]|[0-9])*  {  
	yylval.string = strdup(yytext); 
	return IDENTIFIER;
}
[0-9]*    { 
  yylval.string = strdup(yytext); 
  return INTEGER; 
}
\"[^\"]*\"  {
	yylval.string = strdup(yytext); 
	return TEXT;
}

"=="  {  return EQ;  }

"!="  {  return NE;  }

"<="  {  return LE;  }

"<"  {  return LT;  }

">="  {  return GE;  }

">"  {  return GT;  }
"&"     { return AND_OP; }   // 新增位与运算符
"|"     { return OR_OP; }    // 新增位或运算符

[ \t\r\n]|#.*

. 	{	return *yytext; }

%%

int yywrap()
{
	return 1;
}

