
# gcc
COMPILER = gcc
FLAGS = -O2 -lm -Wimplicit-function-declaration
#mpi
P_COMPILER = smpicc


all: compile

compile:*.c
	clear 
	$(COMPILER) -o sequential_julia.exec sequential_julia.c  $(FLAGS) 
	$(P_COMPILER) -o 1D_parallel_julia.exec 1D_parallel_julia.c  $(FLAGS) 
	$(P_COMPILER) -o 1D_parallel_julia_no_io.exec 1D_parallel_julia_no_io.c  $(FLAGS) 
	$(P_COMPILER) -o 2D_parallel_julia.exec 2D_parallel_julia.c  $(FLAGS) 
#	$(P_COMPILER) -o temp.exec temp.c  $(FLAGS) 
	@echo "Compilaçao concluida"

clean:
	rm -rf *.exec *.bmp



