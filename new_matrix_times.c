#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define min(x, y) ((x)<(y)?(x):(y))


//MPI_Send(data , count  , datatype , destination , tag , communicator?)
//
//When the root process (in our example, it was process zero) calls MPI_Bcast, 
//the data variable will be sent to all other processes.
//
//When all of the receiver processes call MPI_Bcast,
//the data variable will be filled in with the data from the root process.
//
//MPI_Bcast is how all of the processes have matrix b[] in their memory

int main(int argc, char* argv[])
{
  int nrows, ncols;
  double *aa;
  double *b;//this is now the matrix we will use when multiplying I will set their values = aa  

  double *c; //when we receive from process we store the received value here. this is the result matrix from the multiplication
  double *buffer, ans; //buffer is where you store the row you will be sending to the process. ans is a temp location for the answer that will be stored in c
  double *times;
  double total_times;
  int run_index;
  int nruns;
  int myid, master, numprocs;
  double starttime, endtime;
  MPI_Status status;
  int rownum, row; //since this is matrix * matrix : row should be seen as element. ex: row=13 is b[13] when we get to slave code
  int i, j, numsent, sender;//numsent is a number to keep track of how many times we've sent out to be calculated. this can also be viewed as element. ex: numsent=13 is b[13]
  srand(time(0));
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);


  if (argc > 1) {
    //if more than one processor
    nrows = atoi(argv[1]);
    ncols = nrows;
    aa = (double*)malloc(sizeof(double) * nrows * ncols);

    b = (double*)malloc(sizeof(double) * nrows*ncols);
    c = (double*)malloc(sizeof(double) * nrows*ncols);
    buffer = (double*)malloc(sizeof(double) * ncols);
    master = 0;
    if (myid == master) {
	// Master Code goes here

        //Matrix A
        //Create random Matrix
        for (i = 0; i < nrows; i++) {
            for (j = 0; j < ncols; j++) {
              //aa[i*ncols + j] = (double)rand()/RAND_MAX;//creates  
              //printf("Inserting element:%d\n" , i*nrows + j);
              aa[i*ncols + j] = (i*nrows+j);
              //printf("\nArray A\n row:%d \n col:%d \n is %f" , i , j  , aa[i*ncols + j] );
              b[i*ncols + j] = aa[i*nrows + j];//I added this, this makes the matrix = aa
              //printf("\nArray B\n row:%d \n col:%d \n is %f" , i , j  , aa[i*ncols + j] );
              //printf("\nB Element \n row:%d \n col:%d \n is %f" , i , j  , b[i*ncols + j] );
              //printf("B element:%f\n" , b[i*nrows + j]);
            }
        }
        //Matrix A
        
        //This starts a timer 
        starttime = MPI_Wtime();
        numsent = 0;

        //MPI_Bcast(data , count  , datatype , root ,  communicator?)
        MPI_Bcast(b, ncols*nrows, MPI_DOUBLE, master, MPI_COMM_WORLD);

        //Matrix B
        //This takes rows and sends each row to a process 
        for (i = 0; i < min(numprocs - 1, nrows*ncols); i++) {
            for (j = 0; j < ncols; j++) {
                //loads the buffer with the row
                //we want to send the same row multiple times
                //thread should choose the appropiate col
                buffer[j] = aa[i/ncols + j]; 

               // printf("\nLoading this into buffer(first run):%f\n" , buffer[j]);
                //buffer goes to MPI send
            }  
         
            ////gives row to process i while there are still rows/processes
            //MPI_Send(data , count  , datatype , destination , tag , communicator?)
            MPI_Send(buffer, ncols, MPI_DOUBLE, i+1, i+1, MPI_COMM_WORLD);//i+1 because we are in process 0 so we want to start at 1
            numsent++;//keep track of which rows we sent out, then when we cycle again we know which row we left off on
            //we should cycle until numsent = nrows*ncols <--since that's how many elements we need to calculate
        }
        //Matrix B


        //Matrix C (Resulting matrix)
        //A process has finished calculating his row(since he sent a MPI_send).
        for (i = 0; i < nrows*ncols; i++) {
          //MPI_Recv(data , count  , datatype , source , tag , communicator?)
    	    //printf("\ni:%d\n", i);
            MPI_Recv(&ans, 1, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            sender = status.MPI_SOURCE;
            rownum = status.MPI_TAG;
            //printf("\n\tnumsent:%d \n \n\t rownum:%d \n" , numsent , rownum);
            //c[numsent*i + (rownum-1)] = ans;
            c[rownum-1] = ans;
            //c[rownum-1 * i + numsent] = ans;//insert calculation answer into c[]

        ////////////////////////////////////////////////////////////////
        //This means there was more rows than processes from the above portion 
        //It means we need to start cycling to give out rows again 
        //now that the receive happened we know that process is done so we can give him a new row
        if (numsent < nrows*ncols) {
          for (j = 0; j < ncols; j++) {
            //load up the row we left off at into buffer
            buffer[j] = aa[((numsent/nrows))*nrows + j]; //numsent/nrows will give you the row we will be sending. ex: 13/4 = 3 so we would be sending the third row in a 4x4 matrix. aka aa[12] - aa[15]

            //printf("\n%d/%d:%d\n" ,numsent , nrows , numsent/nrows);
            //printf("\nLoading this into buffer:%f\n" , buffer[j]);
          }  

          //MPI_Send(data , count  , datatype , destination , tag , communicator?)
          MPI_Send(buffer, ncols, MPI_DOUBLE, sender, numsent+1, MPI_COMM_WORLD);
          numsent++;
        }
        ///////////////////////////////////////////////////////////////
        
        /////////////////////////////////////////////////////////////////////////////////////////
        //We're done since all rows were consumed. Send a 0 to notify consumers to stop consuming
        else {
          //MPI_Send(data , count  , datatype , destination , tag , communicator?)
            	//printf("\n\n\t\tSending kill signal \n\n");
		//printf("\n\n\t\there is i: %d here is numsent %d \n\n", i, numsent);
		MPI_Send(MPI_BOTTOM, 0, MPI_DOUBLE, sender, 0, MPI_COMM_WORLD);
            }
        ////////////////////////////////////////////////////////////////////////////////////////
        } 
        //Matrix C (Resulting matrix)

        //////////////////////////////////////
        //This is the last thing to execute
	/*
	for(int i=0; i<nrows*ncols; i++){
            printf("\n\nc%d:%f\n\n" , i , c[i]);
         }*/
	endtime = MPI_Wtime();
        printf("\n\nTime to calculate is: %f seconds\n\n",(endtime - starttime));
        /////////////////////////////////////
    }
   
    else { // Slave Code goes here
     //MPI_Bcast(data , count  , datatype , root , communicator?)
      MPI_Bcast(b, ncols*nrows, MPI_DOUBLE, master, MPI_COMM_WORLD);
        
      //if I'm not the last row to be calculated. Actually I can be the last row too.
        if (myid <= nrows) {
            ///////////
            //This loop ends when sender sends 0 --->when is that? found it look up there ^
            while(1) {
                //printf("\n\tCalculation\n");
              MPI_Recv(buffer, ncols, MPI_DOUBLE, master, MPI_ANY_TAG,MPI_COMM_WORLD, &status);//receive from root

              for(int bcheck = 0; bcheck < nrows*ncols; bcheck++){
              //  printf("\n\tBcheck element%d thread row%d :%f\n" ,bcheck , row ,  b[bcheck]);
              //  this can be ignored it was for debugging purposes
              }

              if (status.MPI_TAG == 0){
                break;
                }
              row = status.MPI_TAG;//this is to store which row I'm calculating
              ans = 0.0;

              ////////////////////
              //This is where the actual math happens(matrix row(aa) * matrix column(b))
              for (j = 0; j < ncols; j++) {
                //printf("\nHere is the b row:%i col:%i(aka:%d)I think:%f\n" , j, row-1, j*nrows + row -1 , b[j*nrows + row - 1]);
                //printf("\nGoing to multiply buffer%d,%d:%f * b%d,%d:%f\n" , j ,row-1, buffer[j] , j , row-1 , b[j*nrows + row - 1]);
                //printf("\nelement:%d here:%f\n" , row , b[j*nrows + (row%ncols)]);//row%ncols = what col you're in. then j*nrows gives you each element in the col
                //printf("\nelement:%d here:%f\n" , row , b[j*nrows + (row)]);
                //
                ans += buffer[j] * b[j*nrows + ((row-1)%ncols)];//(row-1%ncols) finds which col to do the math on

                //ans += buffer[j] * b[j*nrows];
                //printf("\nI am thread row %d My calculated answer: %f\n" , row ,ans );
              }
              //MPI_Send(data , count  , datatype , destination , tag , communicator?)
              printf("\nSending this ans: %f\n" , ans);
              MPI_Send(&ans, 1, MPI_DOUBLE, master, row, MPI_COMM_WORLD);
            }
        }
    }//end of slave code 
  }
  else {
    fprintf(stderr, "Usage matrix_times_vector <size>\n");
  }
  MPI_Finalize();
  return 0;
}
