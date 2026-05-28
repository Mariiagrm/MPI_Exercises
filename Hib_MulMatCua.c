#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// mpirun -host localhost:4 -np 4 ./Hib_MulMatCua 1024 2

void initialize(double *m, int total_elements) {
    for (int i = 0; i < total_elements; i++) {
        m[i] = (double)rand() / RAND_MAX;
    }
}

void mul_mat_cua_secuencial(int n, int ldn, double *A, double *B, double *C) {
    int i, j, k;
    double sum;

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            sum = 0.0;
            for (k = 0; k < n; k++) {
                sum += A[i * ldn + k] * B[k * ldn + j];
            }
            C[i * ldn + j] = sum;
        }
    }
}

void comparar_matrices(double *C1, double *C2, int m, int n, int ldc) {
    int i, j;
    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
            double diff = C1[i * ldc + j] - C2[i * ldc + j];
            if (diff < 0) diff = -diff;
            if (diff > 1e-6) {
                printf("Diferencia encontrada en C[%d][%d]: %.6lf vs %.6lf\n",
                       i, j, C1[i * ldc + j], C2[i * ldc + j]);
                return;
            }
        }
    }
    printf("Matrices comparadas correctamente, son iguales dentro de la tolerancia.\n");
}

int main(int argc, char *argv[]) {
    int p, my_rank;
    int n, t, f;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    // Solo el Proceso 0 valida los argumentos
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

    // Proceso 0 difunde n y t a todos los demás procesos
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&t, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Tamaños locales
    f = n / p;
    int loc_elements = f * n;
    int total_elements = n * n;

    // Reserva de memoria
    double *A = NULL, *C = NULL;
    double *loc_A = (double*) malloc(loc_elements * sizeof(double));
    double *loc_C = (double*) calloc(loc_elements, sizeof(double));
    // Todos los procesos necesitan la matriz B completa
    double *B = (double*) malloc(total_elements * sizeof(double));

    // Únicamente el Proceso 0 reserva memoria para las matrices completas A y C
    if (my_rank == 0) {
        A = (double*) malloc(total_elements * sizeof(double));
        C = (double*) malloc(total_elements * sizeof(double));

        srand(time(NULL));
        initialize(A, total_elements);
        initialize(B, total_elements);
        printf("\nIniciando calculo hibrido (MPI + OpenMP):\n");
        printf("Matriz %dx%d | Procesos MPI: %d | Hilos OpenMP/proc: %d\n", n, n, p, t);
    }

    // Distribución de datos
    MPI_Bcast(B, total_elements, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatter(A, loc_elements, MPI_DOUBLE, loc_A, loc_elements, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Cálculo local híbrido (OpenMP dentro de MPI)
    omp_set_num_threads(t);

    // Barrera para medir el tiempo sincronizadamente
    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();

    /* schedule(static) reparte iteraciones equitativamente: g = f / t filas por hilo */
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < f; i++) {
        for (int j = 0; j < n; j++) {
            double sum = 0.0;
            for (int k = 0; k < n; k++) {
                sum += loc_A[i * n + k] * B[k * n + j];
            }
            loc_C[i * n + j] = sum;
        }
    }

    double end_time = MPI_Wtime();
    double local_time = end_time - start_time;
    double max_time;

    // MPI_MAX saca el tiempo del proceso más lento
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    // Recolectamos todos los loc_C en la matriz C final
    MPI_Gather(loc_C, loc_elements, MPI_DOUBLE, C, loc_elements, MPI_DOUBLE, 0, MPI_COMM_WORLD);

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
