#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "tac.h"

/* global var */
int scope, next_tmp, next_label;
SYM *sym_tab_global, *sym_tab_local;
TAC *tac_first, *tac_last;
int loop_depth;
int tof, tos;
void tac_init()
{
	scope=0;
	sym_tab_global=NULL;
	sym_tab_local=NULL;	
	next_tmp=0;
loop_depth = 0;
	next_label=1;
	tof = 0;   // 初始化 tof
    tos = 0; 
}

void tac_complete()
{
	TAC *cur=NULL; 		/* Current TAC */
	TAC *prev=tac_last; 	/* Previous TAC */

	while(prev !=NULL)
	{
		prev->next=cur;
		cur=prev;
		prev=prev->prev;
	}

	tac_first = cur;
}

SYM *lookup_sym(SYM *symtab, char *name)
{
	SYM *t=symtab;

	while(t !=NULL)
	{
		if(strcmp(t->name, name)==0) break; 
		else t=t->next;
	}
	
	return t; /* NULL if not found */
}

void insert_sym(SYM **symtab, SYM *sym)
{
	sym->next=*symtab; /* Insert at head */
	*symtab=sym;
}

SYM *mk_sym(void)
{
	SYM *t;
	t=(SYM *)malloc(sizeof(SYM));
	t->is_ref = 0; // 初始化 is_ref 标志
	return t;
}

SYM *mk_var(char *name)
{
	SYM *sym=NULL;

	if(scope)  
		sym=lookup_sym(sym_tab_local,name);
	else
		sym=lookup_sym(sym_tab_global,name);

	/* var already declared */
	if(sym!=NULL)
	{
		error("variable already declared");
		return NULL;
	}

	/* var unseen before, set up a new symbol table node, insert_sym it into the symbol table. */
	sym=mk_sym();
	sym->type=SYM_VAR;
	sym->name=name;
	sym->offset=-1; /* Unset address */
	sym->data_type=TYPE_INT; // 默认为int类型，实际类型会在declare时设置
	sym->is_ref = 0; // 普通变量
	if(scope)  
		insert_sym(&sym_tab_local,sym);
	else
		insert_sym(&sym_tab_global,sym);

	return sym;
}

TAC *join_tac(TAC *c1, TAC *c2)
{
	TAC *t;

	if(c1==NULL) return c2;
	if(c2==NULL) return c1;

	/* Run down c2, until we get to the beginning and then add c1 */
	t=c2;
	while(t->prev !=NULL) 
		t=t->prev;

	t->prev=c1;
	return c2;
}

TAC *declare_var(char *name)
{
	return mk_tac(TAC_VAR,mk_var(name),NULL,NULL);
}

TAC *mk_tac(int op, SYM *a, SYM *b, SYM *c)
{
	TAC *t=(TAC *)malloc(sizeof(TAC));

	t->next=NULL; /* Set these for safety */
	t->prev=NULL;
	t->op=op;
	t->a=a;
	t->b=b;
	t->c=c;

	return t;
}  

SYM *mk_label(char *name)
{
	SYM *t=mk_sym();

	t->type=SYM_LABEL;
	t->name=strdup(name);

	return t;
}  

TAC *do_func(SYM *func, TAC *args, TAC *code)
{
	TAC *tlist; /* The backpatch list */

	TAC *tlab; /* Label at start of function */
	TAC *tbegin; /* BEGINFUNC marker */
	TAC *tend; /* ENDFUNC marker */

	tlab=mk_tac(TAC_LABEL, mk_label(func->name), NULL, NULL);
	tbegin=mk_tac(TAC_BEGINFUNC, NULL, NULL, NULL);
	tend=mk_tac(TAC_ENDFUNC,   NULL, NULL, NULL);

	tbegin->prev=tlab;
	code=join_tac(args, code);
	tend->prev=join_tac(tbegin, code);

	return tend;
}

SYM *mk_tmp(void)
{
	SYM *sym;
	char *name;

	name=malloc(12);
	sprintf(name, "t%d", next_tmp++); /* Set up text */
	return mk_var(name);
}

TAC *declare_para(char *name)
{
	return mk_tac(TAC_FORMAL,mk_var(name),NULL,NULL);
}

SYM *declare_func(char *name)
{
	SYM *sym=NULL;

	sym=lookup_sym(sym_tab_global,name);

	/* name used before declared */
	if(sym!=NULL)
	{
		if(sym->type==SYM_FUNC)
		{
			error("func already declared");
			return NULL;
		}

		if(sym->type !=SYM_UNDEF)
		{
			error("func name already used");
			return NULL;
		}

		return sym;
	}
	
	
	sym=mk_sym();
	sym->type=SYM_FUNC;
	sym->name=name;
	sym->address=NULL;

	insert_sym(&sym_tab_global,sym);
	return sym;
}

TAC *do_assign(SYM *var, EXP *exp) {
    if (var->is_ref) {
        TAC *deref_assign = mk_tac(TAC_DEREF_ASSIGN, var, exp->ret, NULL);
        deref_assign->prev = exp->tac;
        
        return deref_assign;
    } else {
       TAC *code;

	if(var->type != SYM_VAR) {
		error("assignment to non-variable");
		return NULL;
	}

	// 如果类型不匹配，需要添加类型转换
	if(var->data_type != exp->ret->data_type) {
		TAC *cast = mk_tac(TAC_CAST_INT + (var->data_type - TYPE_INT), 
						  var, exp->ret, NULL);
		cast->prev = exp->tac;
		return cast;
	}

	code = mk_tac(TAC_COPY, var, exp->ret, NULL);
	code->prev = exp->tac;
	return code;
    }
}

TAC *do_input(SYM *var)
{
	TAC *code;

	if(var->type !=SYM_VAR) error("input to non-variable");

	code=mk_tac(TAC_INPUT, var, NULL, NULL);

	return code;
}

TAC *do_output(SYM *s)
{
	TAC *code;

	code=mk_tac(TAC_OUTPUT, s, NULL, NULL);

	return code;
}

EXP *do_bin(int binop, EXP *exp1, EXP *exp2)
{
	TAC *temp;
	SYM *tmp = mk_tmp();
	
	// 类型提升规则
	if(exp1->ret->data_type == TYPE_FLOAT || exp2->ret->data_type == TYPE_FLOAT) {
		tmp->data_type = TYPE_FLOAT;
		
		// 如果操作数不是float，需要转换
		if(exp1->ret->data_type != TYPE_FLOAT) {
			TAC *cast1 = mk_tac(TAC_CAST_FLOAT, exp1->ret, exp1->ret, NULL);
			cast1->prev = exp1->tac;
			exp1->tac = cast1;
		}
		if(exp2->ret->data_type != TYPE_FLOAT) {
			TAC *cast2 = mk_tac(TAC_CAST_FLOAT, exp2->ret, exp2->ret, NULL);
			cast2->prev = exp2->tac;
			exp2->tac = cast2;
		}
	} else if(exp1->ret->data_type == TYPE_INT || exp2->ret->data_type == TYPE_INT) {
		tmp->data_type = TYPE_INT;
		
		// 如果操作数是char，需要转换为int
		if(exp1->ret->data_type == TYPE_CHAR) {
			TAC *cast1 = mk_tac(TAC_CAST_INT, exp1->ret, exp1->ret, NULL);
			cast1->prev = exp1->tac;
			exp1->tac = cast1;
		}
		if(exp2->ret->data_type == TYPE_CHAR) {
			TAC *cast2 = mk_tac(TAC_CAST_INT, exp2->ret, exp2->ret, NULL);
			cast2->prev = exp2->tac;
			exp2->tac = cast2;
		}
	} else {
		tmp->data_type = TYPE_CHAR;
	}

	temp = mk_tac(binop, tmp, exp1->ret, exp2->ret);
	temp->prev = join_tac(exp1->tac, exp2->tac);

	return mk_exp(NULL, tmp, temp);
}    

EXP *do_cmp( int binop, EXP *exp1, EXP *exp2)
{
	TAC *temp; /* TAC code for temp symbol */
	TAC *ret; /* TAC code for result */

	temp=mk_tac(TAC_VAR, mk_tmp(), NULL, NULL);
	temp->prev=join_tac(exp1->tac, exp2->tac);
	ret=mk_tac(binop, temp->a, exp1->ret, exp2->ret);
	ret->prev=temp;

	exp1->ret=temp->a;
	exp1->tac=ret;

	return exp1;  
}   

EXP *do_un( int unop, EXP *exp) 
{
	TAC *temp;
	SYM *tmp = mk_tmp();
	
	// 设置结果的数据类型与操作数相同
	tmp->data_type = exp->ret->data_type;
	
	temp = mk_tac(unop, tmp, exp->ret, NULL);
	temp->prev = exp->tac;
	
	return mk_exp(NULL, tmp, temp); 
}

TAC *do_call(char *name, EXP *arglist)
{
	EXP  *alt; /* For counting args */
	TAC *code; /* Resulting code */
	TAC *temp; /* Temporary for building code */

	code=NULL;
	for(alt=arglist; alt !=NULL; alt=alt->next) code=join_tac(code, alt->tac);

	while(arglist !=NULL) /* Generate ARG instructions */
	{
		temp=mk_tac(TAC_ACTUAL, arglist->ret, NULL, NULL);
		temp->prev=code;
		code=temp;

		alt=arglist->next;
		arglist=alt;
	};

	temp=mk_tac(TAC_CALL, NULL, (SYM *)strdup(name), NULL);
	temp->prev=code;
	code=temp;

	return code;
}

EXP *do_call_ret(char *name, EXP *arglist)
{
	EXP  *alt; /* For counting args */
	SYM *ret; /* Where function result will go */
	TAC *code; /* Resulting code */
	TAC *temp; /* Temporary for building code */

	ret=mk_tmp(); /* For the result */
	code=mk_tac(TAC_VAR, ret, NULL, NULL);

	for(alt=arglist; alt !=NULL; alt=alt->next) code=join_tac(code, alt->tac);

	while(arglist !=NULL) /* Generate ARG instructions */
	{
		temp=mk_tac(TAC_ACTUAL, arglist->ret, NULL, NULL);
		temp->prev=code;
		code=temp;

		alt=arglist->next;
		arglist=alt;
	};

	temp=mk_tac(TAC_CALL, ret, (SYM *)strdup(name), NULL);
	temp->prev=code;
	code=temp;

	return mk_exp(NULL, ret, code);
}
TAC *declare_array(char *name, int size) {
    SYM *sym = mk_var(name);
    sym->is_array = 1;
    sym->array_size = size;
    
    // 为数组分配空间
    if (scope) { // 局部数组
        sym->offset = tof;  // 使用标准偏移字段
        tof += size * 4;   // 每个元素4字节
    } else { // 全局数组
        sym->offset = tos;  // 使用标准偏移字段
        tos += size * 4;
    }
    
    return mk_tac(TAC_ARRAY, sym, mk_const(size), NULL);
}

// 修改数组访问函数
EXP *do_array_access(char *name, EXP *index_exp) {
    SYM *array_sym = get_var(name);
    if (!array_sym->is_array) {
        error("'%s' is not an array", name);
        return NULL;
    }
    
    // 计算元素地址: base + index * 4
    SYM *elem_addr = mk_tmp();
    TAC *mul_tac = mk_tac(TAC_MUL, elem_addr, index_exp->ret, mk_const(4));
    mul_tac->prev = index_exp->tac;
    
    SYM *base_addr = mk_tmp();
    TAC *base_tac = mk_tac(TAC_VAR, base_addr, array_sym, NULL);
    base_tac->prev = mul_tac;
    
    TAC *addr_tac = mk_tac(TAC_ADD, elem_addr, base_addr, elem_addr);
    addr_tac->prev = base_tac;
    
    // 加载元素值
    SYM *elem_value = mk_tmp();
    TAC *load_tac = mk_tac(TAC_ARR_LOAD, elem_value, elem_addr, NULL);
    load_tac->prev = addr_tac;
    
    return mk_exp(NULL, elem_value, load_tac);
}
TAC *do_array_assign(EXP *array_access, EXP *value_exp) {
    // array_access 包含元素地址计算
    TAC *assign_tac = mk_tac(TAC_ARR_STORE, array_access->ret, value_exp->ret, NULL);
    assign_tac->prev = join_tac(array_access->tac, value_exp->tac);
    return assign_tac;
}
char *mk_lstr(int i)
{
	char lstr[10]="L";
	sprintf(lstr,"L%d",i);
	return(strdup(lstr));	
}

TAC *do_if(EXP *exp, TAC *stmt)
{
	TAC *label=mk_tac(TAC_LABEL, mk_label(mk_lstr(next_label++)), NULL, NULL);
	TAC *code=mk_tac(TAC_IFZ, label->a, exp->ret, NULL);

	code->prev=exp->tac;
	code=join_tac(code, stmt);
	label->prev=code;

	return label;
}

TAC *do_test(EXP *exp, TAC *stmt1, TAC *stmt2)
{
	TAC *label1=mk_tac(TAC_LABEL, mk_label(mk_lstr(next_label++)), NULL, NULL);
	TAC *label2=mk_tac(TAC_LABEL, mk_label(mk_lstr(next_label++)), NULL, NULL);
	TAC *code1=mk_tac(TAC_IFZ, label1->a, exp->ret, NULL);
	TAC *code2=mk_tac(TAC_GOTO, label2->a, NULL, NULL);

	code1->prev=exp->tac; /* Join the code */
	code1=join_tac(code1, stmt1);
	code2->prev=code1;
	label1->prev=code2;
	label1=join_tac(label1, stmt2);
	label2->prev=label1;
	
	return label2;
}
/* 引用声明处理 */
SYM *declare_ref(char *name, SYM *target) {
    
    
    SYM *ref_sym = mk_sym();
    ref_sym->type = SYM_VAR;
    ref_sym->name = strdup(name);
    ref_sym->is_ref = 1;
    ref_sym->ref_target = target;
    
    // 添加到符号表
    if (scope) {
        insert_sym(&sym_tab_local, ref_sym);
    } else {
        insert_sym(&sym_tab_global, ref_sym);
    }
    
    // 正确的引用声明指令：存储地址到引用变量
    TAC *decl_tac = mk_tac(TAC_REF_DECL, ref_sym, target, NULL);
    
    return decl_tac->a; // 返回符号表项
}

EXP *handle_deref(EXP *exp) {
    if (!exp || !exp->ret) {
        error("Invalid expression in handle_deref");
        return NULL;
    }
    
    if (!exp->ret->is_ref) {
        error("Cannot dereference non-reference type");
        return NULL;
    }
    
    printf("Dereferencing: %s\n", exp->ret->name);
    
    // 创建临时变量存储解引用结果
    SYM *tmp = mk_tmp();
    TAC *deref_tac = mk_tac(TAC_DEREF, tmp, exp->ret, NULL);
    deref_tac->prev = exp->tac;
    
    return mk_exp(NULL, tmp, deref_tac);
}

EXP *handle_ref(SYM *var) {
    if (!var) {
        error("Invalid variable in handle_ref");
        return NULL;
    }
    
    printf("Getting reference for: %s\n", var->name);
    
    // 创建临时变量存储引用
    SYM *tmp = mk_tmp();
    TAC *ref_tac = mk_tac(TAC_REF_OP, tmp, var, NULL);
    
    return mk_exp(NULL, tmp, ref_tac);
}

// 处理取地址操作
EXP *handle_addr_of(SYM *var) {
    if (!var) {
        error("Cannot take address of undefined variable");
        return NULL;
    }
    
    SYM *tmp = mk_tmp();
    tmp->is_ptr = 1;  // 结果是指针类型
    tmp->points_to = var;  // 记录指向的变量
    
    TAC *code = mk_tac(TAC_ADDR_OF, tmp, var, NULL);
    return mk_exp(NULL, tmp, code);
}

// 处理指针解引用操作
EXP *handle_ptr_deref(SYM *var) {
    if (!var || !var->is_ptr) {
        error("Cannot dereference non-pointer type");
        return NULL;
    }
    
    SYM *tmp = mk_tmp();
    if (var->points_to) {
        tmp->type = var->points_to->type;  // 继承被指向变量的类型
    }
    
    TAC *code = mk_tac(TAC_PTR_DEREF, tmp, var, NULL);
    
    return mk_exp(NULL, tmp, code);
}
TAC *do_while(EXP *exp, TAC *stmt) 
{
	TAC *label=mk_tac(TAC_LABEL, mk_label(mk_lstr(next_label++)), NULL, NULL);
	TAC *code=mk_tac(TAC_GOTO, label->a, NULL, NULL);

	code->prev=stmt; /* Bolt on the goto */

	return join_tac(label, do_if(exp, code));
}
TAC *do_break()
{
    // 创建一个 TAC_BREAK 指令
    return mk_tac(TAC_BREAK, NULL, NULL, NULL);
}

// 添加 for 循环的 TAC 生成
TAC* do_for(TAC* init, EXP* cond, TAC* inc, TAC* body) {
    
// 生成开始和结束标签
    TAC *start_label = mk_tac(TAC_LABEL, mk_label(mk_lstr(next_label++)), NULL, NULL);
    TAC *end_label = mk_tac(TAC_LABEL, mk_label(mk_lstr(next_label++)), NULL, NULL);
    
    // 创建条件检查指令
    TAC *cond_check = mk_tac(TAC_IFZ, end_label->a, cond->ret, NULL);
    cond_check->prev = cond->tac;
    
    // 创建循环跳转指令
    TAC *loop_back = mk_tac(TAC_GOTO, start_label->a, NULL, NULL);
    loop_back->prev = inc;
    
    // 组合所有部分:
    // 1. 初始化代码
    // 2. 开始标签
    // 3. 条件检查
    // 4. 循环体
    // 5. 增量代码
    // 6. 跳回开始
    // 7. 结束标签
    
    TAC *code = join_tac(init, start_label);
    code = join_tac(code, cond_check);
    code = join_tac(code, body);
    code = join_tac(code, loop_back);
    code = join_tac(code, end_label);
    return code;
}
SYM *get_var(char *name)
{
	SYM *sym=NULL; /* Pointer to looked up symbol */

	if(scope) sym=lookup_sym(sym_tab_local,name);

	if(sym==NULL) sym=lookup_sym(sym_tab_global,name);

	if(sym==NULL)
	{
		error("name not declared as local/global variable");
		return NULL;
	}

	if(sym->type!=SYM_VAR)
	{
		error("not a variable");
		return NULL;
	}

	return sym;
} 

EXP *mk_exp(EXP *next, SYM *ret, TAC *code)
{
	EXP *exp=(EXP *)malloc(sizeof(EXP));

	exp->next=next;
	exp->ret=ret;
	exp->tac=code;

	return exp;
}

SYM *mk_text(char *text)
{
	SYM *sym=NULL; /* Pointer to looked up symbol */

	sym=lookup_sym(sym_tab_global,text);

	/* text already used */
	if(sym!=NULL)
	{
		return sym;
	}

	/* text unseen before */
	sym=mk_sym();
	sym->type=SYM_TEXT;
	sym->name=text;
	sym->label=next_label++;

	insert_sym(&sym_tab_global,sym);
	return sym;
}

SYM *mk_const(int n)
{
	SYM *sym=NULL;

	char name[10];
	sprintf(name, "%d", n);

	sym=lookup_sym(sym_tab_global, name);
	if(sym!=NULL)
	{
		return sym;
	}

	sym=mk_sym();
	sym->type=SYM_INT;
	sym->data_type=TYPE_INT;
	sym->value.int_value=n;
	sym->name=strdup(name);
	insert_sym(&sym_tab_global,sym);

	return sym;
}     

char *to_str(SYM *s, char *str) 
{
	if(s==NULL)	return "NULL";

	switch(s->type)
	{
		case SYM_FUNC:
		case SYM_VAR:
		/* Just return the name */
		return s->name;

		case SYM_TEXT:
		/* Put the address of the text */
		sprintf(str, "L%d", s->label);
		return str;

		case SYM_INT:
		/* Convert the number to string */
		sprintf(str, "%d", s->value.int_value);
		return str;
		case SYM_CHAR:
			/* Convert char to string */
			sprintf(str, "'%c'", s->value.char_value);
			return str;

		case SYM_FLOAT:
			/* Convert float to string */
			sprintf(str, "%f", s->value.float_value);
			return str;
		default:
		/* Unknown arg type */
		error("unknown TAC arg type");
		return "?";
	}
} 

void out_str(FILE *f, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(f, format, args);
    va_end(args);
}

void out_sym(FILE *f, SYM *s)
{
	if(s==NULL) fprintf(f, "null");
	else {
		switch(s->type) {
			case SYM_VAR:
				fprintf(f, "%s", s->name);
				break;
			case SYM_INT:
				fprintf(f, "%d", s->value.int_value);
				break;
			case SYM_CHAR:
				fprintf(f, "'%c'", s->value.char_value);
				break;
			case SYM_FLOAT:
				fprintf(f, "%f", s->value.float_value);
				break;
			case SYM_TEXT:
				fprintf(f, "%s", s->name);
				break;
			case SYM_LABEL:
				fprintf(f, "%s", s->name);
				break;
			default:
				fprintf(f, "???");
		}
	}
}

void out_tac(FILE *f, TAC *i)
{
	char sa[12]; /* For text of TAC args */
	char sb[12];
	char sc[12];

	switch(i->op)
	{
		case TAC_UNDEF:
		fprintf(f, "undef");
		break;

		case TAC_ADD:
		fprintf(f, "%s = %s + %s", to_str(i->a, sa), to_str(i->b, sb), to_str(i->c, sc));
		break;

		case TAC_SUB:
		fprintf(f, "%s = %s - %s", to_str(i->a, sa), to_str(i->b, sb), to_str(i->c, sc));
		break;

		case TAC_MUL:
		fprintf(f, "%s = %s * %s", to_str(i->a, sa), to_str(i->b, sb), to_str(i->c, sc));
		break;

		case TAC_DIV:
		fprintf(f, "%s = %s / %s", to_str(i->a, sa), to_str(i->b, sb), to_str(i->c, sc));
		break;

		case TAC_EQ:
		fprintf(f, "%s = (%s == %s)", to_str(i->a, sa), to_str(i->b, sb), to_str(i->c, sc));
		break;

		case TAC_NE:
		fprintf(f, "%s = (%s != %s)", to_str(i->a, sa), to_str(i->b, sb), to_str(i->c, sc));
		break;

		case TAC_LT:
		fprintf(f, "%s = (%s < %s)", to_str(i->a, sa), to_str(i->b, sb), to_str(i->c, sc));
		break;

		case TAC_LE:
		fprintf(f, "%s = (%s <= %s)", to_str(i->a, sa), to_str(i->b, sb), to_str(i->c, sc));
		break;

		case TAC_GT:
		fprintf(f, "%s = (%s > %s)", to_str(i->a, sa), to_str(i->b, sb), to_str(i->c, sc));
		break;

		case TAC_GE:
		fprintf(f, "%s = (%s >= %s)", to_str(i->a, sa), to_str(i->b, sb), to_str(i->c, sc));
		break;

		case TAC_NEG:
		fprintf(f, "%s = - %s", to_str(i->a, sa), to_str(i->b, sb));
		break;

		case TAC_COPY:
		fprintf(f, "%s = %s", to_str(i->a, sa), to_str(i->b, sb));
		break;

		case TAC_GOTO:
		fprintf(f, "goto %s", i->a->name);
		break;

		case TAC_IFZ:
		fprintf(f, "ifz %s goto %s", to_str(i->b, sb), i->a->name);
		break;

		case TAC_ACTUAL:
		fprintf(f, "actual %s", to_str(i->a, sa));
		break;

		case TAC_FORMAL:
		fprintf(f, "formal %s", to_str(i->a, sa));
		break;

		case TAC_CALL:
		if(i->a==NULL) fprintf(f, "call %s", (char *)i->b);
		else fprintf(f, "%s = call %s", to_str(i->a, sa), (char *)i->b);
		break;

		case TAC_INPUT:
		fprintf(f, "input %s", to_str(i->a, sa));
		break;

		case TAC_OUTPUT:
		fprintf(f, "output %s", to_str(i->a, sa));
		break;

		case TAC_RETURN:
		fprintf(f, "return %s", to_str(i->a, sa));
		break;

		case TAC_LABEL:
		fprintf(f, "label %s", i->a->name);
		break;

		case TAC_VAR:
		fprintf(f, "var %s", to_str(i->a, sa));
		break;

		case TAC_BEGINFUNC:
		fprintf(f, "begin");
		break;
		case TAC_FOR:
            	fprintf(f, "for");
            	break;
            	 case TAC_ARRAY:
        fprintf(f, "array %s[%d]", to_str(i->a, sa), i->b->value);
        break;
    case TAC_ARR_LOAD:
        fprintf(f, "%s = [%s]", to_str(i->a, sa), to_str(i->b, sb));
        break;
    case TAC_ARR_STORE:
        fprintf(f, "[%s] = %s", to_str(i->a, sa), to_str(i->b, sb));
        break;
        	case TAC_BREAK:
            	fprintf(f, "break");
            	break;
case TAC_AND:
            fprintf(f, "%s = %s & %s", to_str(i->a, sa), to_str(i->b, sb), to_str(i->c, sc));
            break;
        case TAC_OR:
            fprintf(f, "%s = %s | %s", to_str(i->a, sa), to_str(i->b, sb), to_str(i->c, sc));
            break;
		case TAC_ENDFUNC:
		fprintf(f, "end");
		break;
   		case TAC_REF_DECL:
        fprintf(f, "ref %s points to %s", to_str(i->a, sa), to_str(i->b, sb));
        break;
            
        case TAC_DEREF:
            fprintf(f, "%s = *%s", to_str(i->a, sa), to_str(i->b, sb));
            break;
            
        case TAC_REF_OP:
            fprintf(f, "%s = &%s", to_str(i->a, sa), to_str(i->b, sb));
            break;
            
       case TAC_DEREF_ASSIGN:
            fprintf(f, "*%s = %s", to_str(i->a, sa), to_str(i->b, sb));
            break;
            case TAC_CAST_INT:
			fprintf(f, "%s = (int)%s\n", i->a->name, i->b->name);
			break;
		case TAC_CAST_CHAR:
			fprintf(f, "%s = (char)%s\n", i->a->name, i->b->name);
			break;
		case TAC_CAST_FLOAT:
			fprintf(f, "%s = (float)%s\n", i->a->name, i->b->name);
			break;
		case TAC_PTR_DECL:
			fprintf(f, "ptr %s", to_str(i->a, sa));
			break;
		case TAC_ADDR_OF:
			fprintf(f, "%s = &%s", to_str(i->a, sa), to_str(i->b, sb));
			break;
		case TAC_PTR_DEREF:
			fprintf(f, "%s = *%s", to_str(i->a, sa), to_str(i->b, sb));
			break;
		case TAC_PTR_ASSIGN:
			fprintf(f, "*%s = %s", to_str(i->a, sa), to_str(i->b, sb));
			break;
		default:
		error("unknown TAC opcode");
		break;
	}
}
SYM *mk_type(int type) {
    SYM *sym = mk_sym();
    sym->type = type;
    return sym;
}

SYM *mk_const_int(int n) {
    SYM *sym = mk_sym();
    sym->type = SYM_INT;
    sym->data_type = TYPE_INT;
    sym->value.int_value = n;
    return sym;
}

SYM *mk_const_char(char c) {
    SYM *sym = mk_sym();
    sym->type = SYM_CHAR;
    sym->data_type = TYPE_CHAR;
    sym->value.char_value = c;
    sym->name = malloc(4);
    sprintf(sym->name, "%c", c);
    return sym;
}

SYM *mk_const_float(float f) {
    SYM *sym = mk_sym();
    sym->type = SYM_FLOAT;
    sym->data_type = TYPE_FLOAT;
    sym->value.float_value = f;
    return sym;
}

TAC *do_declare(SYM *type, TAC *var_list) {
    TAC *curr = var_list;
    while (curr != NULL) {
        if (curr->op == TAC_VAR && curr->a != NULL) {
            curr->a->data_type = type->type;
        }
        curr = curr->prev;
    }
    return var_list;
}

EXP *do_cast(SYM *type, EXP *exp) {
    TAC *temp;
    SYM *tmp = mk_tmp();
    tmp->data_type = type->type;
    
    switch(type->type) {
        case TYPE_INT:
            temp = mk_tac(TAC_CAST_INT, tmp, exp->ret, NULL);
            break;
        case TYPE_CHAR:
            temp = mk_tac(TAC_CAST_CHAR, tmp, exp->ret, NULL);
            break;
        case TYPE_FLOAT:
            temp = mk_tac(TAC_CAST_FLOAT, tmp, exp->ret, NULL);
            break;
        default:
            error("Invalid type cast");
            return exp;
    }
    
    temp->prev = exp->tac;
    return mk_exp(NULL, tmp, temp);
}
