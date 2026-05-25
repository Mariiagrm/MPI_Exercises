#!/bin/bash

# Nombre del archivo de salida
ARCHIVO_CSV="resultados_rendimiento.csv"

# Escribir la cabecera en el archivo CSV
echo "Tamano_N,Procesos_MPI_p,Hilos_OMP_t,Tiempo_s,GFlops" > $ARCHIVO_CSV

# Definir los arrays de parámetros
TAMANOS=(512 1024 2048)
PROCESOS=(1 4 8)
HILOS=(1 4 8)

echo "Iniciando bateria de pruebas..."
echo "Los resultados se guardaran en: $ARCHIVO_CSV"
echo "---------------------------------------------------"

# Bucle anidado para recorrer todas las combinaciones
for tam in "${TAMANOS[@]}"; do
    for p in "${PROCESOS[@]}"; do
        for t in "${HILOS[@]}"; do
            
            echo -n "Ejecutando -> Matriz: ${tam}x${tam} | MPI (p): $p | OpenMP (t): $t ... "
            
            # Ejecutar el programa y capturar la salida de consola en una variable
            # (Se usa -host localhost, y se asignan los valores de p, tam y t)
            # mpitun -host localhost -np 1 ./Hib_MulMatCua 1024  $1
            # mpitun -host localhost:4 -np 4 ./Hib_MulMatCua 2048  4
            SALIDA=$(mpirun -host localhost:$p -np $p ./Hib_MulMatCua $tam $t 2>/dev/null)
            
            # Extraer los valores numéricos usando grep y awk
            # Busca la línea que contiene la palabra clave y extrae la columna correspondiente
            TIEMPO=$(echo "$SALIDA" | grep "Tiempo de ejecucion" | awk '{print $4}')
            GFLOPS=$(echo "$SALIDA" | grep "Rendimiento" | awk '{print $2}')
            
            # Control de errores (por si la ejecución falla o devuelve vacío)
            if [ -z "$TIEMPO" ]; then
                TIEMPO="ERROR"
                GFLOPS="ERROR"
                echo "FALLO"
            else
                echo "OK (${TIEMPO}s)"
            fi
            
            # Añadir la fila de datos al archivo CSV
            echo "$tam,$p,$t,$TIEMPO,$GFLOPS" >> $ARCHIVO_CSV
            
        done
    done
done

echo "---------------------------------------------------"
echo "¡Pruebas finalizadas exitosamente!"