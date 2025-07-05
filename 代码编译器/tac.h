/* type of symbol */
#define SYM_UNDEF 0
#define SYM_VAR 1
#define SYM_FUNC 2
#define SYM_TEXT 3
#define SYM_INT 4
#define SYM_CHAR 5
#define SYM_FLOAT 6
#define SYM_LABEL 7
/* data types */
#define TYPE_INT 1
#define TYPE_CHAR 2
#define TYPE_FLOAT 3
/* type of tac */ 
#define TAC_UNDEF 0 /* undefine */
#define TAC_ADD 1 /* a=b+c */
#define TAC_SUB 2 /* a=b-c */
#define TAC_MUL 3 /* a=b*c */
#define TAC_DIV 4 /* a=b/c */
#define TAC_EQ 5 /* a=(b==c) */
#define TAC_NE 6 /* a=(b!=c) */
#define TAC_LT 7 /* a=(b<c) */
#define TAC_LE 8 /* a=(b<=c) */
#define TAC_GT 9 /* a=(b>c) */
#define TAC_GE 10 /* a=(b>=c) */
#define TAC_NEG 11 /* a=-b */
#define TAC_COPY 12 /* a=b */
#define TAC_GOTO 13 /* goto a */
#define TAC_IFZ 14 /* ifz b goto a */
#define TAC_BEGINFUNC 15 /* function begin */
#define TAC_ENDFUNC 16 /* function end */
#define TAC_LABEL 17 /* label a */
#define TAC_VAR 18 /* int a */
#define TAC_FORMAL 19 /* formal a */
#define TAC_ACTUAL 20 /* actual a */
#define TAC_CALL 21 /* a=call b */
#define TAC_RETURN 22 /* return a */
#define TAC_INPUT 23 /* input a */
#define TAC_OUTPUT 24 /* output a */
// 新增TAC类型
#define TAC_AND 25   // 位与操作
#define TAC_OR  26   // 位或操作
#define TAC_FOR 27     // for 循环
#define TAC_BREAK 28   // break 语句
#define TAC_ARRAY     29  // 数组声明
#define TAC_ARR_LOAD  30  // 加载数组元素
#define TAC_ARR_STORE 31  // 存储数组元素
#define TAC_REF_DECL 32   /* 引用声明 */
#define TAC_DEREF    33   /* 解引用操作 */
#define TAC_REF_OP   34   /* 取引用操作 */
#define TAC_DEREF_ASSIGN 35 /* 解引用赋值 */
#define TAC_CAST_INT 36 /* int casting */
#define TAC_CAST_CHAR 37 /* char casting */
#define TAC_CAST_FLOAT 38 /* float casting */
#define TAC_PTR_DECL 39    /* 指针声明 */
#define TAC_ADDR_OF 40     /* 取地址操作 */
#define TAC_PTR_DEREF 41   /* 指针解引用 */
#define TAC_PTR_ASSIGN 42  /* 指针赋值 */
typedef struct sym
{
	/*	
		type:SYM_VAR name:abc value:98 offset:-1
		type:SYM_VAR name:bcd value:99 offset:4
		type:SYM_LABEL name:L1/max			
		type:SYM_INT value:1			
		type:SYM_FUNC name:max address:1234		
		type:SYM_TEXT name:"hello" label:10
	*/
	int type;
	int data_type; /* TYPE_INT, TYPE_CHAR, TYPE_FLOAT */
	int scope; /* 0:global, 1:local */
	char *name;
	int offset;
	union {
		int int_value;
		char char_value;
		float float_value;
	} value;
	int label;
	int is_ref;       /* 新增：标记是否为引用 */
	struct sym *ref_target;  /* 新增：引用指向的目标 */
	int is_ptr;          /* 新增：是否为指针类型 */
	struct sym *points_to;  /* 新增：指针指向的变量（可选） */
	struct tac *address; /* SYM_FUNC */	
	struct sym *next;
	void *etc;
	int is_array;       // 是否为数组
    int array_size;     // 数组大小
    int array_offset; 
} SYM;

typedef struct tac
{
	struct tac  *next;
	struct tac  *prev;
	int op;
	SYM *a;
	SYM *b;
	SYM *c;
	void *etc;
} TAC;

typedef struct exp
{
	struct exp *next; /* for argument list */
	TAC *tac; /* code */
	SYM *ret; /* return value */
	void *etc;
} EXP;

/* global var */
extern FILE *file_x, *file_s;
extern int yylineno, scope, next_tmp, next_label;
extern int loop_depth;  // 当前循环嵌套深度
extern SYM *sym_tab_global, *sym_tab_local;
extern TAC *tac_first, *tac_last;

/* function */
void tac_init();
void tac_complete();
TAC *join_tac(TAC *c1, TAC *c2);
void out_str(FILE *f, const char *format, ...);
void out_sym(FILE *f, SYM *s);
void out_tac(FILE *f, TAC *i);
SYM *mk_label(char *name);
SYM *mk_tmp(void);
SYM *mk_const(int n);
SYM *mk_const_int(int n);      // 整数常量
SYM *mk_const_char(char c);    // 字符常量
SYM *mk_const_float(float f);  // 浮点常量
SYM *mk_text(char *text);
TAC *mk_tac(int op, SYM *a, SYM *b, SYM *c);
EXP *mk_exp(EXP *next, SYM *ret, TAC *code);
char *mk_lstr(int i);
SYM *get_var(char *name); 
SYM *declare_func(char *name);
TAC *declare_var(char *name);
TAC *declare_para(char *name);
TAC *do_func(SYM *name,    TAC *args, TAC *code);
TAC *do_assign(SYM *var, EXP *exp);
TAC *do_output(SYM *var);
TAC *do_input(SYM *var);
TAC *do_call(char *name, EXP *arglist);
TAC *do_if(EXP *exp, TAC *stmt);
TAC *do_test(EXP *exp, TAC *stmt1, TAC *stmt2);
SYM *declare_ref(char *name, SYM *target);
EXP *handle_deref(EXP *exp);
EXP *handle_ref(SYM *var);
EXP *handle_addr_of(SYM *var);  /* 新增：取地址操作 */
EXP *handle_ptr_deref(SYM *var); /* 新增：指针解引用 */
TAC *do_while(EXP *exp, TAC *stmt);
TAC *do_for(TAC *init, EXP *cond, TAC *inc, TAC *body);
TAC *do_break(void);
EXP *do_bin( int binop, EXP *exp1, EXP *exp2);
EXP *do_cmp( int binop, EXP *exp1, EXP *exp2);
EXP *do_un( int unop, EXP *exp);
EXP *do_call_ret(char *name, EXP *arglist);
void error(const char *format, ...);
TAC *declare_array(char *name, int size);
EXP *do_array_access(char *name, EXP *index_exp);
TAC *do_array_assign(EXP *array_access, EXP *value_exp);


/* function declarations */
SYM *mk_type(int type);
SYM *mk_const_int(int n);
SYM *mk_const_char(char c);
SYM *mk_const_float(float f);
TAC *do_declare(SYM *type, TAC *var_list);
EXP *do_cast(SYM *type, EXP *exp);
