%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tac.h"
// 添加缺失的函数声明
EXP *handle_ref(SYM *var);
EXP *handle_deref(EXP *exp);
int yylex();
void yyerror(char* msg);

%}

%union
{
	char character;
	char *string;
	SYM *sym;
	TAC *tac;
	EXP	*exp;
}

%token INT EQ NE LT LE GT GE UMINUS IF ELSE WHILE FUNC INPUT OUTPUT RETURN FOR BREAK REF ADDR_OF CHAR FLOAT 
%token <string> INTEGER IDENTIFIER TEXT FLOAT_LITERAL CHAR_LITERAL
%token AND_OP OR_OP
%left AND_OP OR_OP
%left EQ NE LT LE GT GE
%left '+' '-'
%left '*' '/'
%right UMINUS
%right REF_OP ADDR_OF  // 确保引用/取地址最高优先级
%right CAST
%type <tac> program function_declaration_list function_declaration function parameter_list variable_list statement assignment_statement return_statement if_statement while_statement call_statement block declaration_list declaration statement_list input_statement output_statement for_statement break_statement ref_declaration
%type <exp> argument_list expression_list expression call_expression primary_expression
%type <sym> function_head type_specifier
%type <tac> array_declaration
%type <exp> array_access
%%

program : function_declaration_list
{
	tac_last=$1;
	tac_complete();
}
;

function_declaration_list : function_declaration
| function_declaration_list function_declaration
{
	$$=join_tac($1, $2);
}
;

function_declaration : function
| declaration
;

declaration : INT variable_list ';'
{
	$$=$2;
}| INT '*' IDENTIFIER ';'  // 单个指针变量声明
{
	$$ = declare_var($3);
	$$->a->is_ptr = 1;
}| INT array_declaration ';' {
    $$ = $2;  
}|ref_declaration {
    $$ = $1;
}| type_specifier variable_list ';'
{
	$$ = do_declare($1, $2);
}| ref_declaration ';' // 确保分号正确处理
;

// 在 ref_declaration 规则中
ref_declaration : 
    REF INT IDENTIFIER '=' REF_OP IDENTIFIER ';'
    {
        SYM *target = get_var($6);
        if (!target) {
            // 错误处理...
        }
        // 创建引用变量并建立关联
        SYM *ref_sym = declare_ref($3, target);
        // 生成引用声明指令
        TAC *ref_tac = mk_tac(TAC_REF_DECL, ref_sym, target, NULL);
        $$
 = ref_tac;
    }
;
variable_list : IDENTIFIER
{
	$$=declare_var($1);
}               
| variable_list ',' IDENTIFIER
{
	$$=join_tac($1, declare_var($3));
}               
;
array_declaration : IDENTIFIER '[' INTEGER ']' 
{
    $$ = declare_array($1, atoi($3));
}
| array_declaration ',' IDENTIFIER '[' INTEGER ']' 
{
    $$ = join_tac($1, declare_array($3, atoi($5)));
};
function : function_head '(' parameter_list ')' block
{
	$$=do_func($1, $3, $5);
	scope=0; /* Leave local scope. */
	sym_tab_local=NULL; /* Clear local symbol table. */
}
| error
{
	error("Bad function syntax");
	$$=NULL;
}
;
array_access : IDENTIFIER '[' expression ']' 
{
    $$ = do_array_access($1, $3);
};
function_head : IDENTIFIER
{
	$$=declare_func($1);
	scope=1; /* Enter local scope. */
	sym_tab_local=NULL; /* Init local symbol table. */
}
;

parameter_list : IDENTIFIER
{
	$$=declare_para($1);
}               
| parameter_list ',' IDENTIFIER
{
	$$=join_tac($1, declare_para($3));
}               
|
{
	$$=NULL;
}
;

statement : assignment_statement ';'
| input_statement ';'
| output_statement ';'
| call_statement ';'
| return_statement ';'
| if_statement
| while_statement
| for_statement
| break_statement
| block
| error
{
	error("Bad statement syntax");
	$$=NULL;
}
;

block : '{' declaration_list statement_list '}'
{
	$$=join_tac($2, $3);
}               
;

declaration_list        :
{
	$$=NULL;
}
| declaration_list declaration
{
	$$=join_tac($1, $2);
}
;

statement_list : statement
| statement_list statement
{
	$$=join_tac($1, $2);
}               
;

assignment_statement : IDENTIFIER '=' expression
{
	 SYM *var = get_var($1);
    if (!var) {
        yyerror("Variable not found");
        YYERROR;
    }
    
    // 对引用变量的特殊处理
    if (var->is_ref) {
        // 生成解引用赋值指令: *target = value
        $$ = mk_tac(TAC_DEREF_ASSIGN, var->ref_target, $3->ret, NULL);
    } else {
        // 普通赋值
        $$= mk_tac(TAC_COPY, var, $3->ret, NULL);
    }
    $$->prev = $3->tac;
}
| '*' IDENTIFIER '=' expression  // 指针解引用赋值: *p = value
{
	SYM *var = get_var($2);
	if (!var) {
		error("Variable not declared");
		YYERROR;
	}
	if (!var->is_ptr) {
		error("Cannot dereference non-pointer type");
		YYERROR;
	}
	$$ = mk_tac(TAC_PTR_ASSIGN, var, $4->ret, NULL);
	$$->prev = $4->tac;
}
|
array_access '=' expression 
{
    $$ = do_array_assign($1, $3);
}
;

expression : array_access
|primary_expression
|expression '+' expression
{
	$$=do_bin(TAC_ADD, $1, $3);
}
| '*' IDENTIFIER  // 指针解引用表达式: *p
{
	SYM *var = get_var($2);
	if (!var) {
		error("Variable not declared");
		YYERROR;
	}
	if (!var->is_ptr) {
		error("Cannot dereference non-pointer type");
		YYERROR;
	}
	$$ = handle_ptr_deref(var);
}
| ADDR_OF IDENTIFIER  // 取地址操作: &a
{
	SYM *var = get_var($2);
	if (!var) {
		error("Variable not declared");
		YYERROR;
	}
	$$ = handle_addr_of(var);
}
| REF_OP expression  // 取引用表达式：%a (保持原有引用功能)
    {
        if ($2->ret->type != SYM_VAR) {
            yyerror("Reference operator requires a variable");
            YYERROR;
        }
        $$
 = handle_ref($2->ret);
    }
| expression '-' expression
{
	$$=do_bin(TAC_SUB, $1, $3);
}
| expression '*' expression
{
	$$=do_bin(TAC_MUL, $1, $3);
}
| expression '/' expression
{
	$$=do_bin(TAC_DIV, $1, $3);
}
| '-' expression  %prec UMINUS
{
	$$=do_un(TAC_NEG, $2);
}
| expression EQ expression
{
	$$=do_cmp(TAC_EQ, $1, $3);
}
| expression NE expression
{
	$$=do_cmp(TAC_NE, $1, $3);
}
| expression LT expression
{
	$$=do_cmp(TAC_LT, $1, $3);
}
| expression LE expression
{
	$$=do_cmp(TAC_LE, $1, $3);
}
| expression GT expression
{
	$$=do_cmp(TAC_GT, $1, $3);
}
| expression GE expression
{
	$$=do_cmp(TAC_GE, $1, $3);
}
|expression AND_OP expression 
           { 
             $$ = do_bin(TAC_AND, $1, $3); 
           }
           | expression OR_OP expression  
           { 
             $$ = do_bin(TAC_OR, $1, $3); 
           }| '(' type_specifier ')' expression  %prec CAST
{
	$$ = do_cast($2, $4);
};       
primary_expression : INTEGER
{
	$$ = mk_exp(NULL, mk_const_int(atoi($1)), NULL);
}
| FLOAT_LITERAL
{
	$$ = mk_exp(NULL, mk_const_float(atof($1)), NULL);
}
| CHAR_LITERAL
{
	$$ = mk_exp(NULL, mk_const_char($1[1]), NULL);
}
| IDENTIFIER
{
	$$ = mk_exp(NULL, get_var($1), NULL);
}
| '(' expression ')'
{
	$$ = $2;
}
| call_expression
{
	$$ = $1;
}              
| error
{
	error("Bad expression syntax");
	$$=mk_exp(NULL, NULL, NULL);
}
;

argument_list           :
{
	$$=NULL;
}
| expression_list
;

expression_list : expression
|  expression_list ',' expression
{
	$3->next=$1;
	$$=$3;
}
;

input_statement : INPUT IDENTIFIER
{
	$$=do_input(get_var($2));
}
;

output_statement : OUTPUT IDENTIFIER
{
	$$=do_output(get_var($2));
}
| OUTPUT TEXT
{
	$$=do_output(mk_text($2));
}
| OUTPUT expression  // 支持输出表达式，如 output *p;
{
	TAC *output_tac = mk_tac(TAC_OUTPUT, $2->ret, NULL, NULL);
	output_tac->prev = $2->tac;
	$$ = output_tac;
}
;

return_statement : RETURN expression
{
	TAC *t=mk_tac(TAC_RETURN, $2->ret, NULL, NULL);
	t->prev=$2->tac;
	$$=t;
}               
;

if_statement : IF '(' expression ')' block
{
	$$=do_if($3, $5);
}
| IF '(' expression ')' block ELSE block
{
	$$=do_test($3, $5, $7);
}
;

while_statement : WHILE '(' expression ')' 
{
                   loop_depth++;  // 在解析循环体前增加深度
}block
{
	$$=do_while($3, $6);
	loop_depth--; 
}               
;

call_statement : IDENTIFIER '(' argument_list ')'
{
	$$=do_call($1, $3);
}
;

call_expression : IDENTIFIER '(' argument_list ')'
{
	$$=do_call_ret($1, $3);
}
;
// 新增 for_statement 规则
for_statement : FOR '(' assignment_statement ';' expression ';' assignment_statement ')' 
               {
                   loop_depth++;  // 在解析循环体前增加深度
               }
               block
               {
                   $$ = do_for($3, $5, $7, $10);
                   loop_depth--;  // 在do_for中减少可能太早，改在这里
               }
               ;
// 新增 break_statement 规则
break_statement : BREAK ';'
                {
                
                if (loop_depth <= 0) {
                       yyerror("break statement not within loop");
                        YYERROR;
                    }
                    $$=do_break();
                }
;
type_specifier : INT   { $$ = mk_type(TYPE_INT); }
               | CHAR  { $$ = mk_type(TYPE_CHAR); }
               | FLOAT { $$ = mk_type(TYPE_FLOAT); }
               ;
%%

void yyerror(char* msg) 
{
	fprintf(stderr, "%s: line %d\n", msg, yylineno);
	exit(0);
}
