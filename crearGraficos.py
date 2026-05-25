import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from io import StringIO

# 1. Definimos los datos
datos_csv = "./resultados_rendimiento.csv"

# 2. Cargar los datos
df = pd.read_csv(datos_csv, sep=',', decimal='.')

# 3. Preparación para las etiquetas del gráfico
df['Procesos_MPI_p'] = df['Procesos_MPI_p'].astype(str) + ' MPI'
df['Tamano_N'] = 'N = ' + df['Tamano_N'].astype(str)

# 4. Configurar estilo visual
sns.set_theme(style="whitegrid")

# 5. Crear la gráfica de líneas con Seaborn (relplot)
g = sns.relplot(
    data=df, 
    x='Hilos_OMP_t', 
    y='GFlops', 
    hue='Procesos_MPI_p', 
    col='Tamano_N', 
    kind='line', 
    marker='o',         # Añade puntos marcadores en los datos exactos
    linewidth=2.5,      # Hace las líneas un poco más gruesas
    markersize=8,       # Tamaño de los puntos
    palette='tab10',    # Paleta de colores clara
    height=5,           # Altura de cada panel
    aspect=0.9          # Proporción de anchura
)

# 6. Añadir los textos y forzar que el eje X no invente números
g.fig.suptitle('--Evolución de GFlops según Hilos OMP y Procesos MPI--', y=1.05, fontsize=16)
g.set_axis_labels("Hilos (OpenMP)", "Rendimiento (GFlops)")
g.set(xticks=[1, 4, 8]) # Restringe el eje X a los hilos reales testeados

# 7. Guardar la figura
g.savefig('grafica_lineas_gflops.png', dpi=300, bbox_inches='tight')

print("Gráfica de líneas generada y guardada como 'grafica_lineas_gflops.png'")

sns.set_theme(style="whitegrid")
plt.figure(figsize=(14, 7)) # Hacemos la gráfica ancha



# 3. Preparación de datos para la gráfica
# Unificamos Procesos MPI e Hilos OMP en una sola etiqueta
df['Configuracion'] = df['Procesos_MPI_p'].astype(str) + ' MPI / ' + df['Hilos_OMP_t'].astype(str) + ' OMP'
# Convertimos Tamano_N en texto (categoría) para que los colores funcionen correctamente
df['Tamano_N'] = df['Tamano_N'].astype(str)


# 5. Dibujar la gráfica agrupada
ax = sns.barplot(
    data=df, 
    x='Configuracion', 
    y='GFlops', 
    hue='Tamano_N', 
    palette='Set2'  # Paleta de colores suaves
)

# 6. Estilizar y guardar
plt.title('--Comparación de GFlops por Tamaño (N) y Configuración (MPI / OMP)--', fontsize=16, pad=20)
plt.xlabel('Configuración (Procesos MPI / Hilos OpenMP)', fontsize=12)
plt.ylabel('Rendimiento (GFlops)', fontsize=12)

# Inclinamos las etiquetas de abajo a 45 grados para que no se superpongan
plt.xticks(rotation=45, ha='right')

# Sacamos la leyenda fuera de las barras para que no estorbe
plt.legend(title='Tamaño (N)', bbox_to_anchor=(1.05, 1), loc='upper left')

plt.tight_layout()
plt.savefig('comparacion_gflops_tamanos.png', dpi=300)

print("Gráfica generada y guardada como 'comparacion_gflops_tamanos.png'")