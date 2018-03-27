
# gcc
COMPILER = gcc
FLAGS = -O2 -lm -Wimplicit-function-declaration

all: compile

compile:sequential_julia.c 
	clear 
	$(COMPILER) -o set_julia.exec sequential_julia.c  $(FLAGS) 
	@echo "Compilaçao concluida"

clean:
	rm -rf *.exec *.bmp



