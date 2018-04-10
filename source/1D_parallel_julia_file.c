#include <stdio.h>
#include <stdlib.h>
#include "tempo.h"
#include <math.h>
#include <mpi.h>

#define ROOT_PROCESS 0 

int write_bmp_header(MPI_File fh, int width, int height) {

MPI_Status s;

  unsigned int row_size_in_bytes = width * 3 + 
	  ((width * 3) % 4 == 0 ? 0 : (4 - (width * 3) % 4));

  // Define all fields in the bmp header
  unsigned int filesize = 54 + (int)(row_size_in_bytes * height * sizeof(char));
  short reserved[2] = {0,0};
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

	
  // Write the bytes to the file, keeping track of the
  // number of written "objects"

  size_t ret = 0;

  ret += MPI_File_write(fh, &id,2,MPI_CHAR,&s);
  ret += MPI_File_write(fh, &filesize,1,MPI_INT,&s);
  ret += MPI_File_write(fh, &reserved,2,MPI_SHORT,&s);
  ret += MPI_File_write(fh, &offset,2,MPI_SHORT,&s);
  ret += MPI_File_write(fh, &size,1,MPI_INT,&s);
  ret += MPI_File_write(fh, &width,1,MPI_INT,&s);
  ret += MPI_File_write(fh, &height,1,MPI_INT,&s);
  ret += MPI_File_write(fh, &planes,1,MPI_SHORT,&s);
  ret += MPI_File_write(fh, &bits,1,MPI_SHORT,&s);
  ret += MPI_File_write(fh, &compression,1,MPI_INT,&s);
  ret += MPI_File_write(fh, &image_size,1,MPI_INT,&s);
  ret += MPI_File_write(fh, &x_res,1,MPI_INT,&s);
  ret += MPI_File_write(fh, &y_res,1,MPI_INT,&s);
  ret += MPI_File_write(fh, &ncolors,1,MPI_INT,&s);
  ret += MPI_File_write(fh, &importantcolors,1,MPI_INT,&s);
  // ret += fwrite(id, sizeof(char), 2, f);
  // ret += fwrite(&filesize, sizeof(int), 1, f);
  // ret += fwrite(reserved, sizeof(short), 2, f);
  // ret += fwrite(&offset, sizeof(int), 1, f);
  // ret += fwrite(&size, sizeof(int), 1, f);
  // ret += fwrite(&width, sizeof(int), 1, f);
  // ret += fwrite(&height, sizeof(int), 1, f);
  // ret += fwrite(&planes, sizeof(short), 1, f);
  // ret += fwrite(&bits, sizeof(short), 1, f);
  // ret += fwrite(&compression, sizeof(int), 1, f);
  // ret += fwrite(&image_size, sizeof(int), 1, f);
  // ret += fwrite(&x_res, sizeof(int), 1, f);
  // ret += fwrite(&y_res, sizeof(int), 1, f);
  // ret += fwrite(&ncolors, sizeof(int), 1, f);
  // ret += fwrite(&importantcolors, sizeof(int), 1, f);

  // Success means that we wrote 17 "objects" successfully
  return (ret != 17);
}



int compute_julia_pixel(int x, int y, int width, int height, float tint_bias, unsigned char *rgb) {

  // Check coordinates
  if ((x < 0) || (x >= width) || (y < 0) || (y >= height)) {
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
  while (( img * img + real * real < 2 * 2 ) && ( num_iter > 0 )) {
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


// main

int main(int argc, char *argv[]){
        
  if(argc == 1) { 
    printf("\n ./set_julia.exec TAM FILE \n");
    exit(0);
  }
	
  int myrank,allranks;

  int size = 0;
  int i = 0;

  MPI_File fh;
  MPI_Status s;


  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  MPI_Comm_size(MPI_COMM_WORLD,&allranks);

  // FILE *input = fopen(argv[2], "w");
  // if (input == NULL) {
  //   fprintf(stderr, "Can't open output file %s!\n",argv[2]);
  //   exit(1);
  // }

  size = atoi(argv[1]);

  // if (myrank == ROOT_PROCESS){

  // int return_validation =  write_bmp_header(input,size*2,size);
  // if(!return_validation){
  //   printf("Successful Execution");
  // }else{
  //   printf("PANIC! Error in write bmp");
  // }
  // fclose(input);
  // }
  
  //unsigned char *rgb = malloc(sizeof(unsigned char) * (size * ( 2 * size ) * 3 ));


printf("hello %d = %d\n", myrank, size);
// exit(1);
//MPI_Bcast((void *)&size, 1, MPI_INT, ROOT_PROCESS, MPI_COMM_WORLD);
 // todo mundo faz aqui pra baixo 

int parte = size/allranks;
int inicio = myrank * parte;
int fim = inicio + parte;
if(myrank == allranks)
  fim = size;

unsigned char *tmp = malloc(sizeof(unsigned char) * (size * 2 * size * 3 ));

//printf ("%d:  size = %d -- parte = %d -- inicio = %d -- fim = %d \n", myrank,size,parte,inicio,fim);
  
  int x = 0;
  int y = 0;
  for(y = inicio ; y < fim ; y++){
    for(x = 0 ; x < size * 2 ; x++){
      //printf ("%d:  y = %d -- x = %d -- largura= %d -- autura = %d \n", myrank,y,x, size * 2 , fim - inicio + (parte*myrank));
      //compute_julia_pixel(x , y , size * 2 , fim - inicio + (parte * myrank) , 1.0 , &tmp[y * 3 * parte * 2  + x * 3]);
      compute_julia_pixel(x , y , size * 2 , size , 1.0 , &tmp[y * 3 * size * 2  + x * 3]);
     } 
  }

MPI_File_open(MPI_COMM_WORLD, argv[2], MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);

if (myrank == ROOT_PROCESS){

 write_bmp_header(fh,size*2,size);
 
}

MPI_File_write_ordered(fh, &tmp,(size * 2 * size * 3 ),MPI_CHAR,&s);

MPI_File_close(&fh);
// for (int i =0; i < parte*size*2*3; i++ )
//   printf("tmp do rank %d , %d \n", myrank, tmp[i]);


//MPI_Gather((void *)&tmp, parte * size * 2 * 3, MPI_CHAR, (void *)rgb, size * size * 2 * 3 * allranks, MPI_CHAR, ROOT_PROCESS, MPI_COMM_WORLD);

//  if(myrank == 1) {
//  // for (int i =0; i < parte*size*2; i++ )
//  //  printf("tmp do rank %d , %d \n", myrank, tmp[i]);

//   //printf("rgb do rank %d , %c \n", myrank, rgb );

  

//   fwrite(tmp, sizeof(unsigned char),(size*2*size*3), input);
// }
   // int yx = 0;
   // for(yx = 0; yx < size * size * 2; yx++){
   //  x = yx % (2 * size);
  	// y = yx /  size / 2;
   //     	compute_julia_pixel(x ,y ,size*2 ,size ,1.0 ,&rgb[y*3*size*2+x*3]);
   //   }
 
  
  
  

  MPI_Finalize();   
 // tempoFinal("mili segundos", argv[0], MSGLOG);

  return 0;
}
