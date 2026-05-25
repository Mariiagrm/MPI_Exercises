#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // Necesario para incluir la función sleep()

#define LENGTH   32
#define MAX_NODES 32

int main(int argc, char *argv[])
{
    int  np, rank, len;
    char name[MPI_MAX_PROCESSOR_NAME];
    char mssg[LENGTH], messages[MAX_NODES][LENGTH];
    
    MPI_Status  status;
    MPI_Request request, *p_requests;
    
    MPI_Init(&argc, &argv);
    MPI_Get_processor_name(name, &len);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    printf("< %s >: process %d of %d\n", name, rank, np);
    
    if (rank == 0) 
    {
        p_requests = (MPI_Request *) malloc(sizeof(MPI_Request)*(np - 1));
        for(int i=1; i<np; i++) 
        {
            //Recepción de mensajes de forma asíncrona
            MPI_Irecv(messages[i], LENGTH, MPI_CHAR, MPI_ANY_SOURCE, 10, MPI_COMM_WORLD, &p_requests[i-1]);
        }
        printf("process %d: performing other tasks while receiving messages...\n", rank);
    }
    else 
    {
        //Retardo artificial de 2*p segundos
        sleep(2 * rank); 
        
        snprintf(mssg, LENGTH, "Hello from Process %d", rank);
        MPI_Isend(mssg, strlen(mssg) + 1, MPI_CHAR, 0, 10, MPI_COMM_WORLD, &request);
        printf("\t process %d sending message to process 0\n", rank); 
    }
    
    if (rank == 0) 
    {
        for(int i=1; i<np; i++) 
        {
            int flag = 0; // Bandera para MPI_Test
            
            //Espera activa utilizando MPI_Test
            while (!flag) {
                // Comprueba el estado de la petición sin bloquear la ejecución
                MPI_Test(&p_requests[i-1], &flag, &status);
                
                if (!flag) { // Si el mensaje aún no ha llegado (flag == 0)
                    printf("Process 0 is waiting...\n");
                    sleep(1); // Espera 1 segundo antes de volver a preguntar
                }
            }
            
            // Usamos status.MPI_SOURCE porque el Irecv se hizo con MPI_ANY_SOURCE
            printf("\t\t process 0: message received from process %d: \t %s \n", status.MPI_SOURCE, messages[i]);
        }
        free(p_requests);
    }
    else 
    {
        // Los procesos > 0 deben asegurarse de que su envío asíncrono se completó antes de terminar
        MPI_Wait(&request, &status);
    }
    
    MPI_Finalize();
    return(EXIT_SUCCESS);
}