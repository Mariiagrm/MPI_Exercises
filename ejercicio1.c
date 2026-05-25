#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LENGTH    64  // para que quepa "Bye..."
#define MAX_NODES 4

int main(int argc, char *argv[])
{
    int  np, rank, len;
    char name[MPI_MAX_PROCESSOR_NAME];
    char mssg[LENGTH], messages[MAX_NODES][LENGTH];
    
    MPI_Status  status;
    
    MPI_Init(&argc, &argv); // Inicializa el entorno MPI
    MPI_Get_processor_name(name, &len); // Obtiene el nombre del procesador
    MPI_Comm_size(MPI_COMM_WORLD, &np); // Obtiene el número total de procesos
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Obtiene el rango (ID) del proceso actual
    
    printf("< %s >: process %d of %d\n", name, rank, np);
    
    if (rank > 0) { 
        snprintf(mssg, LENGTH, "Hello from Process %d", rank);
        MPI_Send(mssg, strlen(mssg) + 1, MPI_CHAR, 0, 10, MPI_COMM_WORLD); 
        printf("process %d sending message to process 0\n", rank); 
        
        // 2. Esperan a recibir el mensaje de vuelta del proceso 0 (tag 20)
        MPI_Recv(mssg, LENGTH, MPI_CHAR, 0, 20, MPI_COMM_WORLD, &status);
        printf("process %d receiving message \"%s\"\n", rank, mssg);
    }
    else {
        for(int i=1; i<np; i++) {
            MPI_Recv(messages[i], LENGTH, MPI_CHAR, MPI_ANY_SOURCE, 10, MPI_COMM_WORLD, &status);
            printf("process 0 receiving message \"%s\"\n", messages[i]);
        }
        
        // 2. El proceso 0 envía el mensaje de vuelta a cada proceso p > 0 (tag 20)
        for(int p=1; p<np; p++) {
            snprintf(mssg, LENGTH, "Bye from Process 0 to Process %d", p);
            MPI_Send(mssg, strlen(mssg) + 1, MPI_CHAR, p, 20, MPI_COMM_WORLD);
        }
    }
    
    MPI_Finalize();
    return(EXIT_SUCCESS);
}