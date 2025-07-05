# 编译动态库
LIBRARIES = libtac.so libobj.so

all: mini asm machine

libtac.so: tac.c tac.h
	gcc -fPIC -shared -o $@ tac.c

libobj.so: obj.c obj.h
	gcc -fPIC -shared -o $@ obj.c

# 生成可执行文件并链接动态库
mini: main.c mini.l mini.y $(LIBRARIES)
	lex -o mini.l.c mini.l
	yacc -d -o mini.y.c mini.y
	gcc -g3 main.c mini.l.c mini.y.c -L. -ltac -lobj -Wl,-rpath='$$ORIGIN' -o mini

clean:
	rm -fr *.l.c *.y.c *.y.h *.s *.x *.o core mini *.so
asm: asm.l asm.y opcode.h
	lex -o asm.l.c asm.l
	yacc -d -o asm.y.c asm.y
	gcc -g3 asm.l.c asm.y.c -o asm

machine: machine.c opcode.h
	gcc -g3 machine.c -o machine
.PHONY: all clean
