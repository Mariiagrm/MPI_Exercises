#include <mpi.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void init_array(int n, double *v)
{
    srand(time(NULL) + getpid());
    
    for(int i = 0; i < n; i++) {
        v[i] = (double) rand() / RAND_MAX + 1; // Valores aleatorios
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
    double *v = NULL, *r = NULL;
    double *loc_v;
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
                v = (double *) calloc(n,  sizeof(double));
                r = (double *) calloc(np, sizeof(double));
                init_array(n, v);
                
                #ifdef DEBUG
                    printf("Original array:");
                    print_array(n, v);
                    printf("\n");
                #endif
            }
        }
    }
    
    // El proceso 0 envía la variable de estado retv al resto de procesos
    MPI_Bcast(&retv, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if(retv < 0) 
    {
        MPI_Finalize();
        return(EXIT_FAILURE);
    }
    
    // El proceso 0 envía el tamaño total N a todos
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    int my_size = n / np;
    loc_v = (double *) malloc(sizeof(double) * my_size);
    
    // Reparto del array v desde el proceso 0 hacia loc_v en todos los procesos
    MPI_Scatter(v, my_size, MPI_DOUBLE, loc_v, my_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    
    //--------CALCULO DEL MINIMO GLOBAL EN TODOS LOS PROCESOS -----------------
    double local_min = loc_v[0];
    for(int i = 1; i < my_size; i++) 
    {
        if (loc_v[i] < local_min) {
            local_min = loc_v[i];
        }
    }
    printf("P%d: local_min = %g\n", my_rank, local_min);
    
    // Recolección de los mínimos locales en el array r del proceso 0
    MPI_Gather(&local_min, 1, MPI_DOUBLE, r, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    
    // -------------CÁLCULO DEL MÍNIMO GLOBAL------------
    if(my_rank == 0) 
    {
        double global_min = r[0];
        for(int i = 1; i < np; i++) 
        {
            if (r[i] < global_min) {
                global_min = r[i];
            }
        }
        
        printf("El mínimo general sin MPI_Reduce es: %g\n", global_min);
        
        free(r); 
        free(v);
    }
    
    free(loc_v);
    MPI_Finalize();
    return(EXIT_SUCCESS);
}