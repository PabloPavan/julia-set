#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define MPI_MASTER 0


int write_bmp_header(FILE *f, int width, int height){

  unsigned int row_size_in_bytes = width * 3 + ((width * 3) % 4 == 0 ? 0 : (4 - (width * 3) % 4));

  // Define all fields in the bmp header
  unsigned int filesize = 54 + (int) (row_size_in_bytes * height * sizeof(char));
  short reserved[2] = {0, 0};
  unsigned int offset = 54;

  unsigned int size = 40;
  char id[2] = "BM";
  unsigned short planes = 1;
  unsigned short bits = 24;
  unsigned int compression = 0;
  unsigned int image_size = width * height * 3 * sizeof(char);
  int x_res = 0;
  int y_res = 0;
  unsigned int ncolors = 0;
  unsigned int importantcolors = 0;

// Write the bytes to the file, keeping track of the number of written "objects"
  size_t ret = 0;
  ret += fwrite(id, sizeof(char), 2, f);
  ret += fwrite(&filesize, sizeof(int), 1, f);
  ret += fwrite(reserved, sizeof(short), 2, f);
  ret += fwrite(&offset, sizeof(int), 1, f);
  ret += fwrite(&size, sizeof(int), 1, f);
  ret += fwrite(&width, sizeof(int), 1, f);
  ret += fwrite(&height, sizeof(int), 1, f);
  ret += fwrite(&planes, sizeof(short), 1, f);
  ret += fwrite(&bits, sizeof(short), 1, f);
  ret += fwrite(&compression, sizeof(int), 1, f);
  ret += fwrite(&image_size, sizeof(int), 1, f);
  ret += fwrite(&x_res, sizeof(int), 1, f);
  ret += fwrite(&y_res, sizeof(int), 1, f);
  ret += fwrite(&ncolors, sizeof(int), 1, f);
  ret += fwrite(&importantcolors, sizeof(int), 1, f);

  // Success means that we wrote 17 "objects" successfully
  return (ret != 17);
}



int compute_julia_pixel(int x, int y, int width, int height, float tint_bias, unsigned char *rgb){
  // Check coordinates
  if((x < 0) || (x >= width) || (y < 0) || (y >= height)){
    fprintf(stderr,"Invalid (%d,%d) pixel coordinates in a %d x %d image\n", x, y, width, height);
    return -1;
  }

  // "Zoom in" to a pleasing view of the Julia set
  float X_MIN = -1.6, X_MAX = 1.6, Y_MIN = -0.9, Y_MAX = +0.9;
  float float_y = (Y_MAX - Y_MIN) * (float)y / height + Y_MIN ;
  float float_x = (X_MAX - X_MIN) * (float)x / width  + X_MIN ;

  // Point that defines the Julia set
  float julia_real = -.79;
  float julia_img = .15;

  // Maximum number of iteration
  int max_iter = 300;

  // Compute the complex series convergence
  float real=float_y, img=float_x;
  int num_iter = max_iter;
  while((img * img + real * real < 2 * 2) && (num_iter > 0)){
    float xtemp = img * img - real * real + julia_real;
    real = 2 * img * real + julia_img;
    img = xtemp;
    num_iter--;
  }

  // Paint pixel based on how many iterations were used, using some funky colors
  float color_bias = (float) num_iter / max_iter;
  rgb[0] = (num_iter == 0 ? 200 : - 500.0 * pow(tint_bias, 1.2) *  pow(color_bias, 1.6));
  rgb[1] = (num_iter == 0 ? 100 : -255.0 *  pow(color_bias, 0.3));
  rgb[2] = (num_iter == 0 ? 100 : 255 - 255.0 * pow(tint_bias, 1.2) * pow(color_bias, 3.0));

  return 0;
}

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
  unsigned char *rgb = (unsigned char *) malloc(sizeof(unsigned char) * (chunk[0] * chunk[1] * 3 ));
  
  t0 = MPI_Wtime();
  for(y = begin[0] ; y < end[0]; y++)
    for(x = begin[1] ; x < end[1]; x++)
      compute_julia_pixel(x, y, size * 2 , size, 1.0, &rgb[(y - begin[0]) * 3 * size * 2 + (x - begin[1]) * 3]);
  t1 = MPI_Wtime();

  // MPI_Gather(rgb, chunk * ( 2 * size ) * 3 , MPI_UNSIGNED_CHAR, rgb, chunk * ( 2 * size ) * 3 , MPI_UNSIGNED_CHAR, MPI_MASTER, MPI_COMM_WORLD);
   
  if(MPI_rank == MPI_MASTER){ 
    output = fopen(argv[2], "w");

    if(output == NULL){
      fprintf(stderr, "Can't open output file %s!\n", argv[2]);
      exit(1);
    }

    int return_validation = write_bmp_header(output, size * 2, size);
    if(!return_validation)
      printf("Successful Execution");
    else
      printf("PANIC! Error in write bmp");
  
    fwrite(&rgb[0], sizeof(unsigned char), (chunk[1] * 3), output);
    fclose(output);
    MPI_Send(&flag, 1, MPI_UNSIGNED_CHAR, 1, 0, MPI_COMM_WORLD);
    printf("%d - p%d sending to %d\n", k++, MPI_rank, 1);
    
    for(j = 1; j < chunk[0]; j++){
      printf("%d - p%d waiting for %d\n", k++, MPI_rank, MPI_sqrt - 1);
      MPI_Recv(&flag, 1, MPI_UNSIGNED_CHAR, MPI_sqrt - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      output = fopen(argv[2], "a");
      fwrite(&rgb[j * 3], sizeof(unsigned char), (chunk[1] * 3), output);
      fclose(output);
      printf("%d - p%d sending to %d\n", k++, MPI_rank, 1);
      MPI_Send(&flag, 1, MPI_UNSIGNED_CHAR, 1, 0, MPI_COMM_WORLD);
    }
  }else if((MPI_rank + 1) % MPI_sqrt == 0){
    for(j = 0; j < chunk[0]; j++){
      printf("%d - p%d waiting for %d\n", k++, MPI_rank, MPI_rank - 1);
      MPI_Recv(&flag, 1, MPI_UNSIGNED_CHAR, MPI_rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      output = fopen(argv[2], "a");
      fwrite(&rgb[j * 3], sizeof(unsigned char), (chunk[1] * 3), output);
      fclose(output);
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
      output = fopen(argv[2], "a");
      fwrite(&rgb[j * 3], sizeof(unsigned char), (chunk[1] * 3), output);
      fclose(output);
      printf("%d - p%d sending to %d\n", k++, MPI_rank, MPI_rank + 1);
      MPI_Send(&flag, 1, MPI_UNSIGNED_CHAR, MPI_rank + 1, 0, MPI_COMM_WORLD);
    }
  }
  
  free(rgb);

  fprintf(stderr, "%d:\t%.5lf\n", MPI_rank, t1 - t0);
  MPI_Finalize();

  return 0;
}
