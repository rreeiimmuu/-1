#ifndef OPCODE_H
#define OPCODE_H

/* 数据类型标识 */
#define T_INT     0x0000  // 整数类型
#define T_CHAR    0x0001  // 字符类型
#define T_FLOAT   0x0002  // 浮点类型

/* 基本操作码 */
#define I_END     0x0000  // 结束程序
#define I_NOP     0x0100  // 空操作

/* 输入输出指令 */
#define I_IN_I    0x0300  // 输入整数
#define I_IN_C    0x0310  // 输入字符
#define I_IN_F    0x0320  // 输入浮点数
#define I_OUT_I   0x0400  // 输出整数
#define I_OUT_C   0x0410  // 输出字符
#define I_OUT_F   0x0420  // 输出浮点数
#define I_OUT_S   0x0500  // 输出字符串

/* 加载指令（LOD） */
#define I_LOD_0   0x1000  // LOD Rx, #constant (int)
#define I_LOD_1   0x1100  // LOD Rx, Ry (int)
#define I_LOD_2   0x1200  // LOD Rx, Ry + #constant (int)
#define I_LOD_3   0x1300  // LOD Rx, [constant] (int)
#define I_LOD_4   0x1400  // LOD Rx, [Ry] (int)
#define I_LOD_5   0x1500  // LOD Rx, [Ry + #constant] (int)

/* 浮点加载指令（LODF） */
#define I_LODF_0  0x1600  // LODF Rx, #constant (float)
#define I_LODF_1  0x1700  // LODF Rx, Ry (float)
#define I_LODF_2  0x1800  // LODF Rx, Ry + #constant (float)
#define I_LODF_3  0x1900  // LODF Rx, [constant] (float)
#define I_LODF_4  0x1A00  // LODF Rx, [Ry] (float)
#define I_LODF_5  0x1B00  // LODF Rx, [Ry + #constant] (float)

/* 存储指令（STO） */
#define I_STO_0   0x2000  // STO [Rx], #constant (int)
#define I_STO_1   0x2100  // STO [Rx], Ry (int)
#define I_STO_2   0x2200  // STO [Rx], Ry + #constant (int)
#define I_STO_3   0x2300  // STO [Rx + #constant], Ry (int)

/* 浮点存储指令（STOF） */
#define I_STOF_0  0x2400  // STOF [Rx], #constant (float)
#define I_STOF_1  0x2500  // STOF [Rx], Ry (float)
#define I_STOF_2  0x2600  // STOF [Rx], Ry + #constant (float)
#define I_STOF_3  0x2700  // STOF [Rx + #constant], Ry (float)

/* 整数算术运算 */
#define I_ADD_0   0x3000  // ADD Rx, #constant
#define I_ADD_1   0x3100  // ADD Rx, Ry
#define I_SUB_0   0x4000  // SUB Rx, #constant
#define I_SUB_1   0x4100  // SUB Rx, Ry
#define I_MUL_0   0x5000  // MUL Rx, #constant
#define I_MUL_1   0x5100  // MUL Rx, Ry
#define I_DIV_0   0x6000  // DIV Rx, #constant
#define I_DIV_1   0x6100  // DIV Rx, Ry

/* 浮点算术运算 */
#define I_ADDF_0  0x3200  // ADDF Rx, #constant
#define I_ADDF_1  0x3300  // ADDF Rx, Ry
#define I_SUBF_0  0x4200  // SUBF Rx, #constant
#define I_SUBF_1  0x4300  // SUBF Rx, Ry
#define I_MULF_0  0x5200  // MULF Rx, #constant
#define I_MULF_1  0x5300  // MULF Rx, Ry
#define I_DIVF_0  0x6200  // DIVF Rx, #constant
#define I_DIVF_1  0x6300  // DIVF Rx, Ry

/* 类型转换指令 */
#define I_ITOF    0x7000  // ITOF Rx (int to float)
#define I_FTOI    0x7100  // FTOI Rx (float to int)
#define I_CTOI    0x7200  // CTOI Rx (char to int)
#define I_ITOC    0x7300  // ITOC Rx (int to char)

/* 按位运算 */
#define I_AND_0   0x9000  // AND Rx, #constant
#define I_AND_1   0x9100  // AND Rx, Ry
#define I_OR_0    0xA000  // OR Rx, #constant
#define I_OR_1    0xA100  // OR Rx, Ry

/* 测试与跳转 */
#define I_TST_0   0x8000  // TST Rx (int)
#define I_TSTF_0  0x8100  // TSTF Rx (float)
#define I_JMP_0   0x8200  // JMP #constant
#define I_JMP_1   0x8300  // JMP Rx
#define I_JEZ_0   0x8400  // JEZ #constant
#define I_JEZ_1   0x8500  // JEZ Rx
#define I_JLZ_0   0x8600  // JLZ #constant
#define I_JLZ_1   0x8700  // JLZ Rx
#define I_JGZ_0   0x8800  // JGZ #constant
#define I_JGZ_1   0x8900  // JGZ Rx

/* 浮点比较跳转 */
#define I_JEZF_0  0x8A00  // JEZF #constant (float equal zero)
#define I_JEZF_1  0x8B00  // JEZF Rx
#define I_JLZF_0  0x8C00  // JLZF #constant (float less than zero)
#define I_JLZF_1  0x8D00  // JLZF Rx
#define I_JGZF_0  0x8E00  // JGZF #constant (float greater than zero)
#define I_JGZF_1  0x8F00  // JGZF Rx
#define I_REF_LOAD   0xB000  // 加载引用
#define I_DEREF      0xB100  // 解引用加载
#define I_DEREF_STORE 0xB200 // 解引用存储
#define TAC_FOR_INIT 0x9000    // for 循环初始化
#define TAC_FOR_COND 0x9100    // for 循环条件判断
#define TAC_FOR_STEP 0x9200    // for 循环步长增减
#define TAC_BREAK    0x9300    // break 语句


#endif // OPCODE_H
