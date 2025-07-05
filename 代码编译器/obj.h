#ifndef OBJ_H
#define OBJ_H
/* register */
#define R_UNDEF -1
#define R_FLAG 0
#define R_IP 1
#define R_BP 2
#define R_JP 3
#define R_TP 4
#define R_GEN 5
#define R_OUT 15  // 输出寄存器
#define R_NUM 16
// 添加临时寄存器定义
#define R_TEMP1 6   // 临时寄存器1
#define R_TEMP2 7   // 临时寄存器2
/* frame */
#define FORMAL_OFF -4 	/* first formal parameter */
#define OBP_OFF 0 		/* dynamic chain */
#define RET_OFF 4 		/* ret address */
#define LOCAL_OFF 8 		/* local var */

#define MODIFIED 1
#define UNMODIFIED 0
#define R_TMP 15 
/* data type sizes (in bytes) */
#define SIZE_INT 4
#define SIZE_CHAR 1
#define SIZE_FLOAT 4
/* output instructions */
#define OUTN "OUTN"  /* 输出整数 */
#define OUTC "OUTC"  /* 输出字符 */
#define OUTF "OUTF"  /* 输出浮点数 */
#define OUTS "OUTS"  /* 输出字符串 */
struct rdesc /* register descriptor */
{
	struct sym *var;
	int mod;
	int data_type;  /* 添加数据类型字段 */
};
/* 寄存器分配和管理函数 */
void rdesc_clear(int r);
void rdesc_fill(int r, SYM *s, int mod);
void asm_write_back(int r);
void asm_load(int r, SYM *s);
int reg_alloc(SYM *s);

/* 类型转换相关函数 */
void asm_cast(int r, int from_type, int to_type);

/* 代码生成函数 */
void asm_bin(char *op, SYM *a, SYM *b, SYM *c);
void asm_cmp(int op, SYM *a, SYM *b, SYM *c);
void asm_cond(char *op, SYM *a, char *l);
void asm_call(SYM *a, SYM *b);
void asm_return(SYM *a);
void asm_head(void);
void asm_tail(void);
void asm_str(SYM *s);
void asm_static(void);
void asm_code(TAC *c);
extern int tos; /* top of static */
extern int tof; /* top of frame */
extern int oof; /* offset of formal */
extern int oon; /* offset of next frame */
void ensure_offset_allocated(SYM *var) ;
void tac_obj();
#endif /* OBJ_H */

