#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// mpirun -host localhost:4 -np 4 ./Hib_MulMatCua 1024 2
// Inicializa una matriz plana (representando nxm) con valores aleatorios
void initialize(double *m, int total_elements) {
    for (int i = 0; i < total_elements; i++) {
        m[i] = (double)rand() / RAND_MAX;
    }
}

void mul_mat_cua_secuencial(int n, int ldn, double *A, double *B, double *C){
    int i, j, k;
    double sum;

    for (i = 0; i < n; i++) {
        // Cada hilo calcula una fila completa 'i' de la matriz resultado C
        for (j = 0; j < n; j++) {
            sum = 0.0;
            // Producto punto de la fila i de A y columna j de B
            for (k = 0; k < n; k++) {
                sum += A[i * ldn + k] * B[k * ldn + j];
            }
            C[i * ldn + j] = sum;
        }
    }

}

int main(int argc, char *argv[]) {
    int p, my_rank;
    int n, t, f;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &p);   // p = número de procesos MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    //Solo en el Proceso 0 valida los argumentos/* Multiplicacion secuencial */
void mul_mat_cua_secuencial(int m,int n, int k, int lda, int ldb, int ldc, double *A, double *B, double *C) {
    int i, j,p;
    double sum;


    //schedule(static) divide las iteraciones (filas i)
    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
            sum = 0.0;
            for (p = 0; p < k; p++) {
                sum += A[i * lda + p] * B[p * ldb + j];
            }
            C[i * ldc + j] = sum;
        }
    }
    
}

void comparar_matrices(double *C1, double *C2, int m, int n, int ldc) {
    int i, j;
    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
            double diff = C1[i * ldc + j] - C2[i * ldc + j];
            if (diff < 0) diff = -diff;
            if (diff > 1e-6) { // Tolerancia para comparación
                printf("Diferencia encontrada en C[%d][%d]: %.6lf vs %.6lf\n", i, j, C1[i * ldc + j], C2[i * ldc + j]);
                return; // Matrices diferentes
            }
        }
    }
    printf("Matrices comparadas correctamente, son iguales dentro de la tolerancia.\n");
}   

    if (my_rank == 0) {
        if (argc != 3) {
            printf("\nUso: mpirun -host localhost -np <p> %s <n> <t>\n", argv[0]);
            printf("  <p>: Numero de procesos MPI (definido en mpirun)\n");
            printf("  <n>: Dimension de las matrices cuadradas (nxn)\n");
            printf("  <t>: Numero de hilos OpenMP por proceso\n\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
        n = atoi(argv[1]);
        t = atoi(argv[2]);
        
        if (n % p != 0) {
            printf("Error: La dimension N (%d) debe ser divisible por P (%d).\n", n, p);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
    }

    //Proceso 0 difunde n y t a todos los demás procesos
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&t, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //Tamaños locales
    f = n / p; // Número de filas que procesará cada proceso MPI
    int loc_elements = f * n; // Elementos en el trozo de A y C de cada proceso
    int total_elements = n * n; // Elementos totales de las matrices completas

    //Reserva de memoria
    double *A = NULL, *C = NULL;
    // loc_A almacena las f filas de A
    //  loc_C almacena el resultado local
    double *loc_A = (double*) malloc(loc_elements * sizeof(double));
    double *loc_C = (double*) calloc(loc_elements, sizeof(double));

    // Todos los procesos necesitan la matriz B completa
    double *B = (double*) malloc(total_elements * sizeof(double)); 

    //Unicamente el Proceso 0 reserva memoria para las matrices completas A y C
    if (my_rank == 0) {
        A = (double*) malloc(total_elements * sizeof(double));
        C = (double*) malloc(total_elements * sizeof(double));
        
        srand(time(NULL));
        initialize(A, total_elements);
        initialize(B, total_elements);
        printf("\nIniciando calculo hibrido (MPI + OpenMP):\n");
        printf("Matriz %dx%d | Procesos MPI: %d | Hilos OpenMP/proc: %d\n", n, n, p, t);
    }

    //---------DISTRIBUCIÓN DE DATOS-----------
    //Difundimos matriz B COMPLETA a todos los procesos
    MPI_Bcast(B, total_elements, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    //Repartimos la matriz A en trozos de loc_elements hacia loc_A
    MPI_Scatter(A, loc_elements, MPI_DOUBLE, loc_A, loc_elements, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    //-----------CÁLCULO LOCAL HÍBRIDO (OpenMP dentro de MPI)---------
    omp_set_num_threads(t); // Configuramos los hilos OpenMP
    
    // Barrera para medir el tiempo sincronizadamente
    MPI_Barrier(MPI_COMM_WORLD); // asegurar que una fase del programa paralelo haya terminado completamente de repartir todas las partes de la matriz
    double start_time = MPI_Wtime(); //medir el rendimiento de secciones de código paralelo calculando la diferencia entre dos llamadas

    /* Bucle principal: 
       schedule(static) reparte iteraciones equitativamente: g = f / t filas por hilo */
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < f; i++) {
        for (int j = 0; j < n; j++) {
            double sum = 0.0;
            for (int k = 0; k < n; k++) {
                // loc_A es de tamaño f x n (i va de 0 a f-1)
                // B es de tamaño n x n
                sum += loc_A[i * n + k] * B[k * n + j];
            }
            loc_C[i * n + j] = sum;
        }
    }

    double end_time = MPI_Wtime();
    double local_time = end_time - start_time;
    double max_time;

    
    
    //MPI_Reduce usando MPI_MAX nos saca el tiempo máximo que tardó el proceso más lento
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    //Recoleactamos el resultado con el proceso 0 todos los loc_C en la matriz C final
    MPI_Gather(loc_C, loc_elements, MPI_DOUBLE, C, loc_elements, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    //El proceso 0 recolecta los RESULTADOS y hace la LIMPIEZA
    if (my_rank == 0) {
        double operations = 2.0 * (double)n * (double)n * (double)n;
        double gflops = (operations / max_time) * 1e-9;
        
        printf("\nResultados:\n");
        printf("  Tiempo de ejecucion: %.6lf segundos\n", max_time);
        printf("  Rendimiento: %.6lf GFlops\n\n", gflops);

        free(A);
        free(C);
    }

    free(loc_A);
    free(loc_C);
    free(B);

    MPI_Finalize();
    return 0;
}