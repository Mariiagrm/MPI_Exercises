#include <mpi.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


//Uso: mpirun -np 4 ./ejercicio3ReduceD 16
void init_array(int n, double *v)
{
    srand(time(NULL) + getpid());
    
    for(int i = 0; i < n; i++) {
        v[i] = (double) rand() / RAND_MAX + 1;
    }
}

void print_array(int n, double *v)
{
    printf("\n\n");
    for(int i = 0; i < n; i++) { printf("%g ", v[i]); }
    printf("\n\n");
}

int main(int argc, char *argv[])
{
    double *v = NULL; 
    double *loc_v;
    double local_min, global_min; // Variables para el mínimo local y global
    int n, np, my_rank, len, retv = 0;
    char name[MPI_MAX_PROCESSOR_NAME];
    
    MPI_Init(&argc, &argv);
    MPI_Get_processor_name(name, &len);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    
    if (my_rank == 0) {
        if (argc < 2) 
        {
            printf("\n\n  Usage: %s size \n\n\n", argv[0]);
            retv = -1;
        }
        else 
        {
            n = atoi(argv[1]);
            if((n % np) != 0) 
            {
                printf("\n\n\n N must be divisible by NP...\n\n");
                retv = -2;
            }
            else 
            {
                // Ya no necesitamos reservar memoria para el array 'r'
                v = (double *) calloc(n,  sizeof(double));
                init_array(n, v);
                
                #ifdef DEBUG
                    printf("Original array:");
                    print_array(n, v);
                    printf("\n");
                #endif
            }
        }
    }
    
    MPI_Bcast(&retv, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if(retv < 0) 
    {
        MPI_Finalize();
        return(EXIT_FAILURE);
    }
    
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    int my_size = n / np;
    loc_v = (double *) malloc(sizeof(double) * my_size);
    
    MPI_Scatter(v, my_size, MPI_DOUBLE, loc_v, my_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    
ç    local_min = loc_v[0];
    for(int i = 1; i < my_size; i++) 
    {
        if (loc_v[i] < local_min) {
            local_min = loc_v[i];
        }
    }
    printf("P%d: local_min = %g\n", my_rank, local_min);
    
    MPI_Reduce(&local_min, &global_min, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    
    if(my_rank == 0) 
    {
       
        printf("El minimo general con MPI_Reduce es: %g\n", global_min);
        
        free(v); 
    }
    
    free(loc_v);
    MPI_Finalize();
    return(EXIT_SUCCESS);
}