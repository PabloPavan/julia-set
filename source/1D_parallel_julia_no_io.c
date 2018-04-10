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
 
  if(argc == 1){ 
    fprintf(stderr, "Usage: mpirun -np <NP> ./%s <TAM> <FILE.bmp> \n", argv[0]);
    exit(1);
  }
 
  int x = 0 , y = 0;
  int size = atoi(argv[1]);
 
  if(size % MPI_size != 0){
    fprintf(stderr, "Error: N must be divisible by number of processes\n");
    exit(1);
  }
 
  int chunk = size / MPI_size;
  int begin = MPI_rank * chunk;
  int end = (MPI_rank + 1) * chunk;
 
  printf("[Process %d out of %d]: I should compute pixel rows %d to %d, for a total of %d rows\n", MPI_rank, MPI_size, begin, end - 1, chunk);
 
  FILE *output;  
  unsigned char *rgb;
  if(MPI_rank == MPI_MASTER)
    rgb = malloc(sizeof(unsigned char) * (size * ( 2 * size ) * 3 ));
  else
    rgb = malloc(sizeof(unsigned char) * (chunk * ( 2 * size ) * 3 ));    
   
  t0 = MPI_Wtime();
  for(y = begin ; y < end ; y++)
    for(x = 0 ; x < size * 2 ; x++)
      compute_julia_pixel(x , y , size*2 , size , 1.0 , &rgb[(y - begin) * 3 * size * 2 + x * 3]);
  t1 = MPI_Wtime();
 
  MPI_Gather(rgb, chunk * ( 2 * size ) * 3 , MPI_UNSIGNED_CHAR, rgb, chunk * ( 2 * size ) * 3 , MPI_UNSIGNED_CHAR, MPI_MASTER, MPI_COMM_WORLD);
    
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
     
    fwrite(rgb, sizeof(unsigned char),(size * (2 * size) * 3), output);
    fclose(output);
  }
  free(rgb);
 
  fprintf(stderr, "%d:\t%.5lf\n", MPI_rank, t1 - t0);
  MPI_Finalize();
 
  return 0;
}