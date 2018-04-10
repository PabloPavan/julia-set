#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define MPI_MASTER 0


int main(int argc, char *argv[]){
  double t0, t1;

  int MPI_rank, MPI_size;

  /* Initialize the MPI environment */
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &MPI_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &MPI_rank);
  unsigned char flag = 'y';

  if(argc == 1){ 
    fprintf(stderr, "Usage: mpirun -np <NP> ./%s <TAM> <FILE.bmp> \n", argv[0]);
    exit(1);
  }

  int x = 0 , y = 0, j;
  int size = atoi(argv[1]);
  int MPI_sqrt = (int) sqrtf(MPI_size);

  if(MPI_sqrt * MPI_sqrt != MPI_size){
    fprintf(stderr, "Error: the total number of processes must be a perfect square\n");
    exit(1);    
  }

  if(size % MPI_sqrt != 0){
    fprintf(stderr, "Error: N must be divisible by square root of number of processes\n");
    exit(1);
  }

   
  int rank[2] = {MPI_rank / MPI_sqrt, MPI_rank % MPI_sqrt};
  int chunk[2] = {size / MPI_sqrt, 2 * size / MPI_sqrt};
  int begin[2] = {rank[0] * chunk[0], rank[1] * chunk[1]};
  int end[2] = {(rank[0] + 1) * chunk[0], (rank[1] + 1) * chunk[1]};
  int k = 0;
  printf("[Process rank %d]: my 2-D rank is (%d, %d), my tile is [%d:%d, %d:%d] [%d %d] [%d]\n", MPI_rank, rank[0], rank[1], begin[0], end[0] - 1, begin[1], end[1] - 1, chunk[0], chunk[1], MPI_sqrt);

  FILE *output;
  
  t0 = MPI_Wtime();
  t1 = MPI_Wtime();

  // MPI_Gather(rgb, chunk * ( 2 * size ) * 3 , MPI_UNSIGNED_CHAR, rgb, chunk * ( 2 * size ) * 3 , MPI_UNSIGNED_CHAR, MPI_MASTER, MPI_COMM_WORLD);
   
  if(MPI_rank == MPI_MASTER){ 
    MPI_Send(&flag, 1, MPI_UNSIGNED_CHAR, 1, 0, MPI_COMM_WORLD);
    printf("%d - p%d sending to %d\n", k++, MPI_rank, 1);
    
    for(j = 1; j < chunk[0]; j++){
      printf("%d - p%d waiting for %d\n", k++, MPI_rank, MPI_sqrt - 1);
      MPI_Recv(&flag, 1, MPI_UNSIGNED_CHAR, MPI_sqrt - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      printf("%d - p%d sending to %d\n", k++, MPI_rank, 1);
      MPI_Send(&flag, 1, MPI_UNSIGNED_CHAR, 1, 0, MPI_COMM_WORLD);
    }
  }
  else if(rank[1] == 0){ 
    MPI_Send(&flag, 1, MPI_UNSIGNED_CHAR, 1, 0, MPI_COMM_WORLD);
    printf("%d - p%d sending to %d\n", k++, MPI_rank, 1);
    
    for(j = 1; j < chunk[0]; j++){
      printf("%d - p%d waiting for %d\n", k++, MPI_rank, (rank[0] + 1) * MPI_sqrt - 1);
      MPI_Recv(&flag, 1, MPI_UNSIGNED_CHAR, (rank[0] + 1) * MPI_sqrt - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      printf("%d - p%d sending to %d\n", k++, MPI_rank, 1);
      MPI_Send(&flag, 1, MPI_UNSIGNED_CHAR, 1, 0, MPI_COMM_WORLD);
    }
  }else if(rank[1] == MPI_sqrt - 1){
    for(j = 0; j < chunk[0]; j++){
      printf("%d - p%d waiting for %d\n", k++, MPI_rank, MPI_rank - 1);
      MPI_Recv(&flag, 1, MPI_UNSIGNED_CHAR, MPI_rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      if(j == chunk[0] - 1){
        printf("%d - p%d sending to %d\n", k++, MPI_rank, MPI_rank + 1);
        MPI_Send(&flag, 1, MPI_UNSIGNED_CHAR, MPI_rank + 1, 0, MPI_COMM_WORLD);
      }
      else{
        printf("%d - p%d sending to %d\n", k++, MPI_rank, MPI_rank + 1 - MPI_sqrt);
        MPI_Send(&flag, 1, MPI_UNSIGNED_CHAR, MPI_rank + 1 - MPI_sqrt, 0, MPI_COMM_WORLD);
      }
    }
  }else{
    for(j = 0; j < chunk[0]; j++){
      printf("%d - p%d waiting for %d\n", k++, MPI_rank, MPI_rank - 1);
      MPI_Recv(&flag, 1, MPI_UNSIGNED_CHAR, MPI_rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      printf("%d - p%d sending to %d\n", k++, MPI_rank, MPI_rank + 1);
      MPI_Send(&flag, 1, MPI_UNSIGNED_CHAR, MPI_rank + 1, 0, MPI_COMM_WORLD);
    }
  }

  fprintf(stderr, "%d:\t%.5lf\n", MPI_rank, t1 - t0);
  MPI_Finalize();

  return 0;
}