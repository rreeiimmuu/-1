#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "tac.h"
#include "obj.h"

/* global var */
int tos; /* top of static */
int tof; /* top of frame */
int oof; /* offset of formal */
int oon; /* offset of next frame */
struct rdesc rdesc[R_NUM];
void ensure_offset_allocated(SYM *var) {
    if (!var) return;
    
    if (var->offset == -1) {
        if (scope) {
            var->offset = tof;
            tof += (var->data_type == TYPE_FLOAT) ? SIZE_FLOAT : SIZE_INT;
        } else {
            var->offset = tos;
            tos += (var->data_type == TYPE_FLOAT) ? SIZE_FLOAT : SIZE_INT;
        }
    }
}
void rdesc_clear(int r)    
{
	rdesc[r].var = NULL;
	rdesc[r].mod = 0;
}

void rdesc_fill(int r, SYM *s, int mod)
{
	int old;
	for(old=R_GEN; old < R_NUM; old++)
	{
		if(rdesc[old].var==s)
		{
			rdesc_clear(old);
		}
	}

	rdesc[r].var=s;
	rdesc[r].mod=mod;
	rdesc[r].data_type = s->data_type;
}     
int reg_alloc(SYM *s);
void asm_write_back(int r)
{
	if((rdesc[r].var!=NULL) && rdesc[r].mod)
	{
		if(rdesc[r].var->scope==1) /* local var */
		{
			switch(rdesc[r].var->data_type) {
				case TYPE_FLOAT:
					out_str(file_s, "	STOF (R%u+%u),R%u\n", R_BP, rdesc[r].var->offset, r);
					break;
				default:
					out_str(file_s, "	STO (R%u+%u),R%u\n", R_BP, rdesc[r].var->offset, r);
			}
		}
		else /* global var */
		{
			out_str(file_s, "	LOD R%u,STATIC\n", R_TP);
			switch(rdesc[r].var->data_type) {
				case TYPE_FLOAT:
					out_str(file_s, "	STOF (R%u+%u),R%u\n", R_TP, rdesc[r].var->offset, r);
					break;
				default:
					out_str(file_s, "	STO (R%u+%u),R%u\n", R_TP, rdesc[r].var->offset, r);
			}
		}
		rdesc[r].mod=UNMODIFIED;
	}
}

void asm_load(int r, SYM *s) 
{
 char name[12];
    
    if(s == NULL) return;
	/* already in a reg */
	for(int i=R_GEN; i < R_NUM; i++)  
	{
		if(rdesc[i].var==s)
		{
			/* load from the reg */
			out_str(file_s, "	LOD R%u,R%u\n", r, i);

			/* update rdesc */
			// rdesc_fill(r, s, rdesc[i].mod);
			return;
		}
	}
	
	/* not in a reg */
	switch(s->type)
	{
		case SYM_INT:
		out_str(file_s, "	LOD R%u,%u\n", r, s->value);
		break;

		case SYM_VAR:
		if(s->scope==1) /* local var */
		{
			if((s->offset)>=0) out_str(file_s, "	LOD R%u,(R%u+%d)\n", r, R_BP, s->offset);
			else out_str(file_s, "	LOD R%u,(R%u-%d)\n", r, R_BP, -(s->offset));
		}
		else /* global var */
		{
			out_str(file_s, "	LOD R%u,STATIC\n", R_TP);
			if(s->offset>=0)
                out_str(file_s, "	LOD R%u,(R%u+%d)\n", r, R_TP, s->offset);
            else
                out_str(file_s, "	LOD R%u,(R%u-%d)\n", r, R_TP, -(s->offset));
		}
		break;


		case SYM_TEXT:
		out_str(file_s, "	LOD R%u,L%u\n", r, s->label);
		break;
	}

	// rdesc_fill(r, s, UNMODIFIED);
}   

int reg_alloc(SYM *s)
{
	int r; 

	/* already in a register */
	for(r=R_GEN; r < R_NUM; r++)
	{
		if(rdesc[r].var==s)
		{
			return r;
		}
	}

	/* empty register */
	for(r=R_GEN; r < R_NUM; r++)
	{
		if(rdesc[r].var==NULL)
		{
			asm_load(r, s);
			rdesc_fill(r, s, UNMODIFIED);
			return r;
		}

	}
	
	/* unmodifed register */
	for(r=R_GEN; r < R_NUM; r++)
	{
		if(!rdesc[r].mod)
		{
			asm_load(r, s);
			rdesc_fill(r, s, UNMODIFIED);
			return r;
		}
	}

	/* random register */
	srand(time(NULL));
	int random = (rand() % (R_NUM - R_GEN)) + R_GEN; 
	asm_write_back(random);
	asm_load(random, s);
	rdesc_fill(random, s, UNMODIFIED);
	return random;
}

void asm_bin(char *op, SYM *a, SYM *b, SYM *c) {
    int reg_b=-1, reg_c=-1; 

	while(reg_b == reg_c)
	{
		reg_b = reg_alloc(b); 
		reg_c = reg_alloc(c); 
	}
        // 处理类型转换
    if (a->data_type == TYPE_FLOAT) {
        if (b->data_type != TYPE_FLOAT) {
            asm_cast(reg_b, b->data_type, TYPE_FLOAT);
        }
        if (c->data_type != TYPE_FLOAT) {
            asm_cast(reg_c, c->data_type, TYPE_FLOAT);
        }
    }

    // 执行运算
    if (strstr(op, "F") != NULL) {  // 浮点运算
        out_str(file_s, "	%s R%u,R%u\n", op, reg_b, reg_c);
    } else {  // 整数运算
        if (strcmp(op, "AND") == 0) {
            out_str(file_s, "	AND R%u,R%u\n", reg_b, reg_c);
        } else if (strcmp(op, "OR") == 0) {
            out_str(file_s, "	OR R%u,R%u\n", reg_b, reg_c);
        } else {
            out_str(file_s, "	%s R%u,R%u\n", op, reg_b, reg_c);
        }
    }

    rdesc_fill(reg_b, a, MODIFIED);
}
void asm_cmp(int op, SYM *a, SYM *b, SYM *c)
{
	int reg_b=-1, reg_c=-1; 

	while(reg_b == reg_c)
	{
		reg_b = reg_alloc(b); 
		reg_c = reg_alloc(c); 
	}

	out_str(file_s, "	SUB R%u,R%u\n", reg_b, reg_c);
	out_str(file_s, "	TST R%u\n", reg_b);

	switch(op)
	{		
		case TAC_EQ:
		out_str(file_s, "	LOD R3,R1+40\n");
		out_str(file_s, "	JEZ R3\n");
		out_str(file_s, "	LOD R%u,0\n", reg_b);
		out_str(file_s, "	LOD R3,R1+24\n");
		out_str(file_s, "	JMP R3\n");
		out_str(file_s, "	LOD R%u,1\n", reg_b);
		break;
		
		case TAC_NE:
		out_str(file_s, "	LOD R3,R1+40\n");
		out_str(file_s, "	JEZ R3\n");
		out_str(file_s, "	LOD R%u,1\n", reg_b);
		out_str(file_s, "	LOD R3,R1+24\n");
		out_str(file_s, "	JMP R3\n");
		out_str(file_s, "	LOD R%u,0\n", reg_b);
		break;
		
		case TAC_LT:
		out_str(file_s, "	LOD R3,R1+40\n");
		out_str(file_s, "	JLZ R3\n");
		out_str(file_s, "	LOD R%u,0\n", reg_b);
		out_str(file_s, "	LOD R3,R1+24\n");
		out_str(file_s, "	JMP R3\n");
		out_str(file_s, "	LOD R%u,1\n", reg_b);
		break;
		
		case TAC_LE:
		out_str(file_s, "	LOD R3,R1+40\n");
		out_str(file_s, "	JGZ R3\n");
		out_str(file_s, "	LOD R%u,1\n", reg_b);
		out_str(file_s, "	LOD R3,R1+24\n");
		out_str(file_s, "	JMP R3\n");
		out_str(file_s, "	LOD R%u,0\n", reg_b);
		break;
		
		case TAC_GT:
		out_str(file_s, "	LOD R3,R1+40\n");
		out_str(file_s, "	JGZ R3\n");
		out_str(file_s, "	LOD R%u,0\n", reg_b);
		out_str(file_s, "	LOD R3,R1+24\n");
		out_str(file_s, "	JMP R3\n");
		out_str(file_s, "	LOD R%u,1\n", reg_b);
		break;
		
		case TAC_GE:
		out_str(file_s, "	LOD R3,R1+40\n");
		out_str(file_s, "	JLZ R3\n");
		out_str(file_s, "	LOD R%u,1\n", reg_b);
		out_str(file_s, "	LOD R3,R1+24\n");
		out_str(file_s, "	JMP R3\n");
		out_str(file_s, "	LOD R%u,0\n", reg_b);
		break;
	}

	/* Delete c from the descriptors and insert a */
	rdesc_clear(reg_b);
	rdesc_fill(reg_b, a, MODIFIED);
}   

void asm_cond(char *op, SYM *a,  char *l)
{
	for(int r=R_GEN; r < R_NUM; r++) asm_write_back(r);

	if(a !=NULL)
	{
		int r;

		for(r=R_GEN; r < R_NUM; r++) /* Is it in reg? */
		{
			if(rdesc[r].var==a) break;
		}

		if(r < R_NUM) out_str(file_s, "	TST R%u\n", r);
		else out_str(file_s, "	TST R%u\n", reg_alloc(a)); /* Load into new register */
	}

	out_str(file_s, "	%s %s\n", op, l); 
} 

void asm_call(SYM *a, SYM *b)
{
	int r;
	for(int r=R_GEN; r < R_NUM; r++) asm_write_back(r);
	for(int r=R_GEN; r < R_NUM; r++) rdesc_clear(r);
	out_str(file_s, "	STO (R2+%d),R2\n", tof+oon);	/* store old bp */
	oon += 4;
	out_str(file_s, "	LOD R4,R1+32\n"); 				/* return addr: 4*8=32 */
	out_str(file_s, "	STO (R2+%d),R4\n", tof+oon);	/* store return addr */
	oon += 4;
	out_str(file_s, "	LOD R2,R2+%d\n", tof+oon-8);	/* load new bp */
	out_str(file_s, "	JMP %s\n", (char *)b);			/* jump to new func */
	if(a != NULL)
	{
		r = reg_alloc(a);
		out_str(file_s, "	LOD R%u,R%u\n", r, R_TP);	
		rdesc[r].mod = MODIFIED;
	}
	oon=0;
}

void asm_return(SYM *a)
{
	for(int r=R_GEN; r < R_NUM; r++) asm_write_back(r);
	for(int r=R_GEN; r < R_NUM; r++) rdesc_clear(r);

	if(a!=NULL)	 /* return value */
	{
		asm_load(R_TP, a);
	}

	out_str(file_s, "	LOD R3,(R2+4)\n");	/* return address */
	out_str(file_s, "	LOD R2,(R2)\n");	/* restore bp */
	out_str(file_s, "	JMP R3\n");			/* return */
}   

void asm_head()
{
	char head[]=
	"	# head\n"
	"	LOD R2,STACK\n"
	"	STO (R2),0\n"
	"	LOD R4,EXIT\n"
	"	STO (R2+4),R4\n";

	out_str(file_s, "%s", head);
}

void asm_tail()
{
	char tail[]=
	"\n	# tail\n"
	"EXIT:\n"
	"	END\n";

	out_str(file_s, "%s", tail);
}

void asm_str(SYM *s)
{
	char *t=s->name; /* The text */
	int i;

	out_str(file_s, "L%u:\n", s->label); /* Label for the string */
	out_str(file_s, "	DBS "); /* Label for the string */

	for(i=1; t[i + 1] !=0; i++)
	{
		if(t[i]=='\\')
		{
			switch(t[++i])
			{
				case 'n':
				out_str(file_s, "%u,", '\n');
				break;

				case '\"':
				out_str(file_s, "%u,", '\"');
				break;
			}
		}
		else out_str(file_s, "%u,", t[i]);
	}

	out_str(file_s, "0\n"); /* End of string */
}

void asm_static(void)
{
	int i;

	SYM *sl;

	for(sl=sym_tab_global; sl !=NULL; sl=sl->next)
	{
		if(sl->type==SYM_TEXT) asm_str(sl);
	}

	out_str(file_s, "STATIC:\n");
	out_str(file_s, "	DBN 0,%u\n", tos);				
	out_str(file_s, "STACK:\n");
}
void asm_ref(int r, SYM *s) {
    if (s->is_ref) {
        // 处理引用变量
        out_str(file_s, "	LOD R%u, (R%u)\n", r, reg_alloc(s->ref_target));
    } else {
        // 原有加载逻辑
        asm_load(r, s);
    }
}

void asm_deref(int r, SYM *s) {
    out_str(file_s, "	LOD R%u, (R%u)\n", r, reg_alloc(s));
}

void asm_addr(int r, SYM *s) {
    if (s->scope == 1) { // 局部变量
        out_str(file_s, "	LOD R%u, R%u+%d\n", r, R_BP, s->offset);
    } else { // 全局变量
        out_str(file_s, "	LOD R%u, STATIC+%d\n", r, s->offset);
    }
}
void asm_code(TAC *c)
{
	int r;

	switch(c->op)
	{
		case TAC_UNDEF:
		error("cannot translate TAC_UNDEF");
		return;

		case TAC_ADD:
			if (c->a->data_type == TYPE_FLOAT || c->b->data_type == TYPE_FLOAT || c->c->data_type == TYPE_FLOAT) {
				asm_bin("ADDF", c->a, c->b, c->c);
			} else {
				asm_bin("ADD", c->a, c->b, c->c);
			}
			return;

		case TAC_SUB:
			if (c->a->data_type == TYPE_FLOAT || c->b->data_type == TYPE_FLOAT || c->c->data_type == TYPE_FLOAT) {
				asm_bin("SUBF", c->a, c->b, c->c);
			} else {
				asm_bin("SUB", c->a, c->b, c->c);
			}
			return;

		case TAC_MUL:
			if (c->a->data_type == TYPE_FLOAT || c->b->data_type == TYPE_FLOAT || c->c->data_type == TYPE_FLOAT) {
				asm_bin("MULF", c->a, c->b, c->c);
			} else {
				asm_bin("MUL", c->a, c->b, c->c);
			}
			return;


		case TAC_DIV:
			if (c->a->data_type == TYPE_FLOAT || c->b->data_type == TYPE_FLOAT || c->c->data_type == TYPE_FLOAT) {
				asm_bin("DIVF", c->a, c->b, c->c);
			} else {
				asm_bin("DIV", c->a, c->b, c->c);
			}
			return;

		case TAC_NEG:
			{
				int r = reg_alloc(c->b);
				if (c->b->data_type == TYPE_FLOAT) {
					out_str(file_s, "    NEGF R%u\n", r);
				} else {
					out_str(file_s, "    NEG R%u\n", r);
				}
				rdesc_fill(r, c->a, MODIFIED);
			}
			return;

		case TAC_EQ:
		case TAC_NE:
		case TAC_LT:
		case TAC_LE:
		case TAC_GT:
		case TAC_GE:
		asm_cmp(c->op, c->a, c->b, c->c);
		return;

		case TAC_COPY:
			{
				int r = reg_alloc(c->b);
				if (c->a->data_type != c->b->data_type) {
					asm_cast(r, c->b->data_type, c->a->data_type);
				}
				rdesc_fill(r, c->a, MODIFIED);
			}
			return;
case TAC_AND:
    asm_bin("AND", c->a, c->b, c->c);
    return;
case TAC_OR:
    asm_bin("OR", c->a, c->b, c->c);
    return;
		case TAC_INPUT:
		r=reg_alloc(c->a);
		out_str(file_s, "	IN\n");
		out_str(file_s, "	LOD R%u,R15\n", r);
		rdesc[r].mod = MODIFIED;
		return;
 case TAC_ARRAY: {
            if(scope) {
                tof += 4 * (c->a->array_size - 1);
            } else {
                tos += 4 * (c->a->array_size - 1);
            }
            break;
        }
      case TAC_ARR_LOAD: {
    int reg_addr = reg_alloc(c->b);
    int reg_dest = reg_alloc(c->a);
    
    if (c->b->scope == 1) { // 局部数组
        out_str(file_s, "	LOD R%u,(R%u+%d)\n", reg_dest, R_BP, c->b->offset);
    } else { // 全局数组
        out_str(file_s, "	LOD R%u,STATIC\n", R_TP);
        if(c->b->offset>=0){
        out_str(file_s, "	LOD R%u,(R%u+%d)\n", reg_dest, R_TP, c->b->offset);}
        else{ out_str(file_s, "	LOD R%u,(R%u-%d)\n", reg_dest, R_TP, -(c->b->offset));}
    }
    rdesc_fill(reg_dest, c->a, MODIFIED);
    break;
}

// 修改数组存储汇编生成
case TAC_ARR_STORE: {
    int reg_addr = reg_alloc(c->a);
    int reg_value = reg_alloc(c->b);
    
    if (c->a->scope == 1) { // 局部数组
        out_str(file_s, "	STO (R%u+%d),R%u\n", R_BP, c->a->offset, reg_value);
    } else { // 全局数组
        out_str(file_s, "	LOD R%u,STATIC\n", R_TP);
        if(c->a->offset>=0){
        out_str(file_s, "	STO (R%u+%d),R%u\n", R_TP, c->a->offset, reg_value);}
        else{out_str(file_s, "	STO (R%u-%d),R%u\n", R_TP, -(c->a->offset), reg_value);}
    }
    break;
}

		case TAC_OUTPUT:
			 if (c->a->is_ref) {
        // 处理引用类型输出
        int r_addr = reg_alloc(c->a);
        out_str(file_s, "    LOD R%d, (R%d) # 解引用%s\n",
                R_OUT, r_addr, c->a->name);
        out_str(file_s, "    OUTN\n");
    } else
			if(c->a->type == SYM_VAR || c->a->type == SYM_CHAR)
			{
				r=reg_alloc(c->a);
				switch(c->a->data_type) {
					case TYPE_CHAR:
						out_str(file_s, "    LOD R15,R%u\n", r);
						out_str(file_s, "    OUTC\n");  // 使用OUTC指令输出字符
						break;
					case TYPE_FLOAT:
						out_str(file_s, "    LOD R15,R%u\n", r);
						out_str(file_s, "    OUTF\n");  // 使用OUTF指令输出浮点数
						break;
					default:  // TYPE_INT
						out_str(file_s, "    LOD R15,R%u\n", r);
						out_str(file_s, "    OUTN\n");  // 使用OUTN指令输出整数
				}
			} 
			else if(c->a->type == SYM_TEXT)
			{
				r=reg_alloc(c->a);
				out_str(file_s, "    LOD R15,R%u\n", r);
				out_str(file_s, "    OUTS\n");
			}
			return;
					
			
		case TAC_GOTO:
		asm_cond("JMP", NULL, c->a->name);
		return;

		case TAC_IFZ:
		asm_cond("JEZ", c->b, c->a->name);
		return;

		case TAC_LABEL:
		for(int r=R_GEN; r < R_NUM; r++) asm_write_back(r);
		for(int r=R_GEN; r < R_NUM; r++) rdesc_clear(r);
		out_str(file_s, "%s:\n", c->a->name);
		return;

		case TAC_ACTUAL:
		r=reg_alloc(c->a);
		out_str(file_s, "	STO (R2+%d),R%u\n", tof+oon, r);
		oon += 4;
		return;

		case TAC_CALL:
		asm_call(c->a, c->b);
		return;

		case TAC_BEGINFUNC:
		/* We reset the top of stack, since it is currently empty apart from the link information. */
		scope=1;
		tof=LOCAL_OFF;
		oof=FORMAL_OFF;
		oon=0;
		return;

		case TAC_FORMAL:
		c->a->scope=1; /* parameter is special local var */
		c->a->offset=oof;
		oof -=4;
		return;
		case TAC_CAST_INT:
			{
				int r = reg_alloc(c->b);
				asm_cast(r, c->b->data_type, TYPE_INT);
				rdesc_fill(r, c->a, MODIFIED);
			}
			return;

		case TAC_CAST_CHAR:
			{
				int r = reg_alloc(c->b);
				asm_cast(r, c->b->data_type, TYPE_CHAR);
				rdesc_fill(r, c->a, MODIFIED);
			}
			return;

		case TAC_CAST_FLOAT:
			{
				int r = reg_alloc(c->b);
				asm_cast(r, c->b->data_type, TYPE_FLOAT);
				rdesc_fill(r, c->a, MODIFIED);
			}
			return;

		case TAC_VAR:
			if(c->a->scope==0) /* global */
			{
				c->a->offset=tos;
				tos += (c->a->data_type == TYPE_CHAR) ? SIZE_CHAR : 
					  (c->a->data_type == TYPE_FLOAT) ? SIZE_FLOAT : SIZE_INT;
			}
			else /* local */
			{
				c->a->offset=tof;
				tof += (c->a->data_type == TYPE_CHAR) ? SIZE_CHAR : 
					  (c->a->data_type == TYPE_FLOAT) ? SIZE_FLOAT : SIZE_INT;
			}
			return;

		case TAC_RETURN:
		asm_return(c->a);
		return;

		case TAC_ENDFUNC:
		asm_return(NULL);
		scope=0;
		return;
		 case TAC_FOR:
{
    // 通过 prev/next 链遍历 FOR 结构
    TAC *init = c->prev;          // 初始化代码（通过 prev 链查找）
    TAC *start_label = c;         // 开始标签（当前 TAC_LABEL）
    TAC *cond_check = c->next;    // 条件检查（TAC_IFZ）
    TAC *body = cond_check->next; // 循环体
    TAC *loop_back = body->next;  // 跳回开始（TAC_GOTO）
    TAC *end_label = loop_back->next; // 结束标签（TAC_LABEL）

    // 生成汇编代码
    if (init) asm_code(init);
    out_str(file_s, "%s:\n", start_label->a->name);
    if (cond_check) asm_code(cond_check);
    if (body) asm_code(body);
    if (loop_back) asm_code(loop_back);
    out_str(file_s, "%s:\n", end_label->a->name);
    break;
}

case TAC_BREAK:
{
    // 查找最近的ifz指令使用的标签
    TAC *cur = c;
    while (cur->next) {
        cur = cur->next;
        if (cur->op == TAC_IFZ) {
            // 这是循环的退出条件判断
            out_str(file_s, "\tJMP %s\n", cur->a->name);
            break;
        }
        if (cur->op == TAC_LABEL && 
            (strcmp(cur->a->name, "L3") == 0 || 
            strcmp(cur->a->name, "L_loop_end") == 0)) {
            // 直接匹配已知的循环结束标签
            out_str(file_s, "\tJMP %s\n", cur->a->name);
            break;
        }
    }
    if (!cur->next) error("break outside loop");
    break;
}
		case TAC_REF_DECL: {
    // 确保变量偏移量已分配
    ensure_offset_allocated(c->a); // 引用变量b
    ensure_offset_allocated(c->b); // 目标变量a
    
    // 使用临时寄存器R_TEMP1计算地址
    if (c->b->scope == 1) { // 局部变量
        // 先加载偏移量到R_TEMP1
        out_str(file_s, "    LOD R%d, %d # 加载%s的偏移量\n", 
                R_TEMP1, c->b->offset, c->b->name);
        // 再将R_TEMP1加到R_BP上
        out_str(file_s, "    ADD R%d, R%d # 加上R_BP\n", 
                R_TEMP1, R_BP);
    } else { // 全局变量
        // 先加载静态地址到R_TEMP1
        out_str(file_s, "    LOD R%d, STATIC # 加载静态区基地址\n", R_TEMP1);
        // 再加上变量的偏移量
        out_str(file_s, "    ADD R%d, %d # 加上%s的偏移量\n", 
                R_TEMP1, c->b->offset, c->b->name);
    }
    
    // 存储地址到引用变量b
    if (c->a->scope == 1) { // 局部引用变量
        out_str(file_s, "    STO (R%d+%d), R%d # 存储到引用变量%s\n", 
                R_BP, c->a->offset, R_TEMP1, c->a->name);
    } else { // 全局引用变量
        // 加载静态地址到R_TEMP2
        out_str(file_s, "    LOD R%d, STATIC # 加载静态区基地址\n", R_TEMP2);
        // 存储地址到静态区
        out_str(file_s, "    STO (R%d+%d), R%d # 存储到引用变量%s\n", 
                R_TEMP2, c->a->offset, R_TEMP1, c->a->name);
    }
    break;
}
        case TAC_DEREF: {
            // 解引用操作：从引用变量指向的地址加载值
            int r_ref = reg_alloc(c->b);
            int r_temp = reg_alloc(c->a);
            
            out_str(file_s, "    LOD R%u, (R%u)\n", r_temp, r_ref);
            rdesc_fill(r_temp, c->a, MODIFIED);
            break;
        }
        
        case TAC_REF_OP: {
            // 取引用操作：获取变量的地址
            int r_var = reg_alloc(c->b);
            int r_ref = reg_alloc(c->a);
            
            // 如果变量是局部变量
            if (c->b->scope == 1) {
                out_str(file_s, "    LOD R%u, R%u+%d\n", r_ref, R_BP, c->b->offset);
            } 
            // 如果变量是全局变量
            else {
                out_str(file_s, "    LOD R%u, STATIC+%d\n", r_ref, c->b->offset);
            }
            
            rdesc_fill(r_ref, c->a, MODIFIED);
            break;
        }
        
        case TAC_DEREF_ASSIGN: {
            // 解引用赋值：*ref = value
            int r_ref = reg_alloc(c->a);
            int r_value = reg_alloc(c->b);
            
            out_str(file_s, "    STO (R%u), R%u\n", r_ref, r_value);
            break;
        }

        // 指针相关的TAC操作
        case TAC_PTR_DECL:
            // 指针声明处理
            if(scope) {
                c->a->scope=1; /* local pointer */
                c->a->offset=tof;
                tof +=4;
            } else {
                c->a->scope=0; /* global pointer */
                c->a->offset=tos;
                tos +=4;
            }
            return;
            
        case TAC_ADDR_OF: {
            // 取地址操作：p = &a
            int r_addr = reg_alloc(c->a);
            
            if (c->b->scope == 1) {
                // 局部变量：地址 = BP + offset
                out_str(file_s, "    LOD R%u, R%u\n", r_addr, R_BP);
                out_str(file_s, "    ADD R%u, %d\n", r_addr, c->b->offset);
            } else {
                // 全局变量：地址 = STATIC + offset
                out_str(file_s, "    LOD R%u, STATIC\n", r_addr);
                out_str(file_s, "    ADD R%u, %d\n", r_addr, c->b->offset);
            }
            
            rdesc_fill(r_addr, c->a, MODIFIED);
            return;
        }
        
        case TAC_PTR_DEREF: {
            // 指针解引用：a = *p
            int r_ptr = reg_alloc(c->b);
            int r_value = reg_alloc(c->a);
            
            out_str(file_s, "    LOD R%u, (R%u)\n", r_value, r_ptr);
            rdesc_fill(r_value, c->a, MODIFIED);
            return;
        }
        
        case TAC_PTR_ASSIGN: {
            // 指针赋值：*p = value
            int r_ptr = reg_alloc(c->a);
            int r_value = reg_alloc(c->b);
            
            out_str(file_s, "    STO (R%u), R%u\n", r_ptr, r_value);
            return;
        }

		default:
		/* Don't know what this one is */
		error("unknown TAC opcode to translate");
		return;
	}
}
void asm_cast(int r, int from_type, int to_type)
{
    if (from_type == to_type) {
        return;  // 相同类型不需要转换
    }

    switch(to_type) {
        case TYPE_INT:
            switch(from_type) {
                case TYPE_CHAR:
                    // 字符到整数的转换是直接的，不需要特殊处理
                    break;
                case TYPE_FLOAT:
                    out_str(file_s, "    FTOI R%u,R%u\n", r, r);
                    break;
            }
            break;
            
        case TYPE_CHAR:
            switch(from_type) {
                case TYPE_INT:
                    out_str(file_s, "    AND R%u,255\n", r); // 只保留低8位
                    break;
                case TYPE_FLOAT:
                    out_str(file_s, "    FTOI R%u,R%u\n", r, r);
                    out_str(file_s, "    AND R%u,255\n", r);
                    break;
            }
            break;
            
        case TYPE_FLOAT:
            switch(from_type) {
                case TYPE_INT:
                case TYPE_CHAR:
                    out_str(file_s, "    ITOF R%u,R%u\n", r, r);
                    break;
            }
            break;
    }
}
void tac_obj()
{
	tof=LOCAL_OFF; /* TOS allows space for link info */
	oof=FORMAL_OFF;
	oon=0;

	for(int r=0; r < R_NUM; r++) rdesc[r].var=NULL;
	
	asm_head();

	TAC * cur;
	for(cur=tac_first; cur!=NULL; cur=cur->next)
	{
		out_str(file_s, "\n	# ");
		out_tac(file_s, cur);
		out_str(file_s, "\n");
		asm_code(cur);
	}
	asm_tail();
	asm_static();
}
