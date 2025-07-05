#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opcode.h"

#define REGMAX 16
#define MEMMAX (256 * 256)
#define R_FLAG 0
#define R_IP 1
#define FLAG_EZ 0
#define FLAG_LZ 1
#define FLAG_GZ 2

int reg[REGMAX];        // 寄存器
unsigned char mem[MEMMAX]; // 内存
int op, rx, ry, constant; // 操作码、寄存器编号、立即数

/* 寄存器越界检查宏 */
#define CHECK_REG(r) \
    if (r < 0 || r >= REGMAX) { \
        fprintf(stderr, "error: invalid register R%d\n", r); \
        exit(0); \
    }

/* 内存地址越界检查宏 */
#define CHECK_ADDR(addr, size) \
    if ((addr) < 0 || (addr) + (size) > MEMMAX) { \
        fprintf(stderr, "error: memory access out of bounds at address %d\n", addr); \
        exit(0); \
    }

/* 更新标志寄存器 */
void update_flags(int value) {
    if (value == 0) {
        reg[R_FLAG] = FLAG_EZ;
    } else if (value < 0) {
        reg[R_FLAG] = FLAG_LZ;
    } else {
        reg[R_FLAG] = FLAG_GZ;
    }
}

/* 解析指令 */
void instruction(int ip) {
    op = (mem[ip] << 8) | mem[ip + 1]; // 双字节操作码（高字节在前）
    rx = mem[ip + 2];
    ry = mem[ip + 3];
    constant = (mem[ip + 4] << 24) | (mem[ip + 5] << 16) |
               (mem[ip + 6] << 8) | mem[ip + 7];
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s filename\n", argv[0]);
        exit(0);
    }

    FILE *input = fopen(argv[1], "rb");
    if (input == NULL) {
        fprintf(stderr, "error: open %s failed\n", argv[1]);
        exit(0);
    }

    /* 初始化寄存器和内存 */
    for (int i = 0; i < REGMAX; i++) reg[i] = 0;
    int i;
    for (i = 0; (i < MEMMAX) && !feof(input); i++) {
        int ch = fgetc(input);
        if (ch == EOF) break;
        mem[i] = (unsigned char)ch;
    }
    for (; i < MEMMAX; i++) mem[i] = 0;

    /* 主执行循环 */
    while (1) {
        instruction(reg[R_IP]);

        switch (op) {
            // -------------------------------
            // 基础指令
            // -------------------------------
            case I_END:
                exit(0);
            case I_NOP:
                break;
		    // -------------------------------
            // 输入输出指令
            // -------------------------------
            case I_OUT_I:
                printf("%d", reg[15]);
                break;
            case I_OUT_C:
                printf("%c", reg[15]);
                break;
            case I_OUT_S:
                CHECK_ADDR(reg[15], 0);
                printf("%s", &mem[reg[15]]);
                break;
            case I_IN_I:
                scanf("%d", &reg[15]);
                break;
         
            // -------------------------------
            // 加载指令（LOD）
            // -------------------------------
            case I_LOD_0:  // LOD Rx, #constant
                reg[rx] = constant;
                break;
            case I_LOD_1:  // LOD Rx, Ry
                CHECK_REG(rx);
                CHECK_REG(ry);
                reg[rx] = reg[ry];
                break;
            case I_LOD_2:  // LOD Rx, Ry + #constant
                CHECK_REG(rx);
                CHECK_REG(ry);
                reg[rx] = reg[ry] + constant;
                break;
            case I_LOD_3:  // LOD Rx, [constant]
                CHECK_ADDR(constant, 3);
                reg[rx] = (mem[constant] << 24) | (mem[constant + 1] << 16) |
                          (mem[constant + 2] << 8) | mem[constant + 3];
                break;
            case I_LOD_4:  // LOD Rx, [Ry] (数组元素加载)
    CHECK_REG(rx);
    CHECK_REG(ry);
    CHECK_ADDR(reg[ry], 4);
    reg[rx] = (mem[reg[ry]] << 24) | (mem[reg[ry]+1] << 16) |
             (mem[reg[ry]+2] << 8) | mem[reg[ry]+3];
    break;
            case I_LOD_5:  // LOD Rx, [Ry + #constant]
                CHECK_REG(ry);
                CHECK_ADDR(reg[ry] + constant, 3);
                reg[rx] = (mem[reg[ry] + constant] << 24) |
                          (mem[reg[ry] + constant + 1] << 16) |
                          (mem[reg[ry] + constant + 2] << 8) |
                          mem[reg[ry] + constant + 3];
                break;

            // -------------------------------
            // 存储指令（STO）
            // -------------------------------
            case I_STO_0:  // STO [Rx], #constant
                CHECK_REG(rx);
                CHECK_ADDR(reg[rx], 3);
                mem[reg[rx]] = (constant >> 24) & 0xFF;
                mem[reg[rx] + 1] = (constant >> 16) & 0xFF;
                mem[reg[rx] + 2] = (constant >> 8) & 0xFF;
                mem[reg[rx] + 3] = constant & 0xFF;
                break;
            case I_STO_1:  // STO [Rx], Ry (数组元素存储)
                CHECK_REG(rx);
                CHECK_REG(ry);
                CHECK_ADDR(reg[rx], 3);
                mem[reg[rx]]   = (reg[ry] >> 24) & 0xFF;
                mem[reg[rx]+1] = (reg[ry] >> 16) & 0xFF;
                mem[reg[rx]+2] = (reg[ry] >> 8)  & 0xFF;
                mem[reg[rx]+3] = reg[ry] & 0xFF;
                break;
            case I_STO_2:  // STO [Rx], Ry + #constant
                CHECK_REG(rx);
                CHECK_REG(ry);
                CHECK_ADDR(reg[rx], 3);
                int sto2_val = reg[ry] + constant;
                mem[reg[rx]] = (sto2_val >> 24) & 0xFF;
                mem[reg[rx] + 1] = (sto2_val >> 16) & 0xFF;
                mem[reg[rx] + 2] = (sto2_val >> 8) & 0xFF;
                mem[reg[rx] + 3] = sto2_val & 0xFF;
                break;
            case I_STO_3:  // STO [Rx + #constant], Ry
                CHECK_REG(rx);
                CHECK_REG(ry);
                CHECK_ADDR(reg[rx] + constant, 3);
                mem[reg[rx] + constant] = (reg[ry] >> 24) & 0xFF;
                mem[reg[rx] + constant + 1] = (reg[ry] >> 16) & 0xFF;
                mem[reg[rx] + constant + 2] = (reg[ry] >> 8) & 0xFF;
                mem[reg[rx] + constant + 3] = reg[ry] & 0xFF;
                break;

            // -------------------------------
            // 算术运算指令
            // -------------------------------
            case I_ADD_0:  // ADD Rx, #constant
                CHECK_REG(rx);
                reg[rx] += constant;
                update_flags(reg[rx]);
                break;
            case I_ADD_1:  // ADD Rx, Ry
                CHECK_REG(rx);
                CHECK_REG(ry);
                reg[rx] += reg[ry];
                update_flags(reg[rx]);
                break;
            case I_SUB_0:  // SUB Rx, #constant
                CHECK_REG(rx);
                reg[rx] -= constant;
                update_flags(reg[rx]);
                break;
            case I_SUB_1:  // SUB Rx, Ry
                CHECK_REG(rx);
                CHECK_REG(ry);
                reg[rx] -= reg[ry];
                update_flags(reg[rx]);
                break;
            case I_MUL_0:  // MUL Rx, #constant
                CHECK_REG(rx);
                reg[rx] *= constant;
                update_flags(reg[rx]);
                break;
                
            case I_MUL_1:  // MUL Rx, Ry
                CHECK_REG(rx);
                CHECK_REG(ry);
                reg[rx] *= reg[ry];
                update_flags(reg[rx]);
                break;
            case I_DIV_0:  // DIV Rx, #constant
                CHECK_REG(rx);
                if (constant == 0) {
                    fprintf(stderr, "error: divide by zero\n");
                    exit(0);
                }
                reg[rx] /= constant;
                update_flags(reg[rx]);
                break;
            case I_DIV_1:  // DIV Rx, Ry
                CHECK_REG(rx);
                CHECK_REG(ry);
                if (reg[ry] == 0) {
                    fprintf(stderr, "error: divide by zero\n");
                    exit(0);
                }
                reg[rx] /= reg[ry];
                update_flags(reg[rx]);
                break;

            // -------------------------------
            // 按位运算指令
            // -------------------------------
            case I_AND_0:  // AND Rx, #constant
                CHECK_REG(rx);
                reg[rx] &= constant;
                update_flags(reg[rx]);
                break;
            case I_AND_1:  // AND Rx, Ry
                CHECK_REG(rx);
                CHECK_REG(ry);
                reg[rx] &= reg[ry];
                update_flags(reg[rx]);
                break;
                
            case I_OR_0:   // OR Rx, #constant
                CHECK_REG(rx);
                reg[rx] |= constant;
                update_flags(reg[rx]);
                break;
            case I_OR_1:   // OR Rx, Ry
                CHECK_REG(rx);
                CHECK_REG(ry);
                reg[rx] |= reg[ry];
                update_flags(reg[rx]);
                break;
            case I_TST_0:  // TST Rx
                CHECK_REG(rx);
                update_flags(reg[rx]);
                break;
            case I_JMP_0:  // JMP #constant
                reg[R_IP] = constant - 8; // 补偿IP自增
                break;
            case I_JMP_1:  // JMP Rx
                CHECK_REG(rx);
                reg[R_IP] = reg[rx] - 8;
                break;
            case I_JEZ_0:  // JEZ #constant
                if (reg[R_FLAG] == FLAG_EZ) reg[R_IP] = constant - 8;
                break;
            case I_JEZ_1:  // JEZ Rx
                CHECK_REG(rx);
                if (reg[R_FLAG] == FLAG_EZ) reg[R_IP] = reg[rx] - 8;
                break;
            case I_JLZ_0:  // JLZ #constant
                if (reg[R_FLAG] == FLAG_LZ) reg[R_IP] = constant - 8;
                break;
            case I_JLZ_1:  // JLZ Rx
                CHECK_REG(rx);
                if (reg[R_FLAG] == FLAG_LZ) reg[R_IP] = reg[rx] - 8;
                break;
            case I_JGZ_0:  // JGZ #constant
                if (reg[R_FLAG] == FLAG_GZ) reg[R_IP] = constant - 8;
                break;
            case I_JGZ_1:  // JGZ Rx
                CHECK_REG(rx);
                if (reg[R_FLAG] == FLAG_GZ) reg[R_IP] = reg[rx] - 8;
                break;
            /* 在指令解析中添加 */
case I_REF_LOAD:
            reg[rx] = reg[ry]; // 加载引用（地址）
            break;
            
        // 添加解引用指令
        case I_DEREF:
            CHECK_ADDR(reg[ry], 3);
            reg[rx] = (mem[reg[ry]] << 24) | (mem[reg[ry]+1] << 16) |
                      (mem[reg[ry]+2] << 8) | mem[reg[ry]+3];
            break;
            
        // 添加解引用存储指令
        case I_DEREF_STORE:
            CHECK_ADDR(reg[rx], 3);
            mem[reg[rx]] = (reg[ry] >> 24) & 0xFF;
            mem[reg[rx] + 1] = (reg[ry] >> 16) & 0xFF;
            mem[reg[rx] + 2] = (reg[ry] >> 8) & 0xFF;
            mem[reg[rx] + 3] = reg[ry] & 0xFF;
            break;

            // -------------------------------
            // 默认错误处理
            // -------------------------------
            default:
                fprintf(stderr, "error: invalid opcode 0x%04X\n", op);
                exit(0);
        }

        reg[R_IP] += 8; // 每条指令固定8字节
    }

    fclose(input);
    return 0;
}
