# Guía del proyecto: DBSCAN paralelo para detectar ruido (OpenMP)

Proyecto Apertura de Cómputo Paralelo y en la Nube. Entrega: **martes 20 de octubre de 2026, en clase**.
Equipo de 2 personas: experimento completo y los dos escritos.

---

## 1. ¿Qué es el proyecto?

Tenemos miles de puntos en 2D (x, y), generados al azar alrededor de 4 centros, más algunos puntos
sueltos. Hay que decidir qué puntos son **ruido** (outliers) usando la versión ingenua de DBSCAN,
con dos parámetros:

- `EPS = 0.03`: radio de la vecindad de cada punto.
- `MIN_SAMPLES = 10`: mínimo de puntos dentro de ese radio (contando al propio punto) para que la zona sea densa.

El algoritmo tiene dos pasos:

| Paso | Qué hace | Etiqueta |
|---|---|---|
| 1 | Para cada punto, contar cuántos puntos hay a distancia <= EPS. Si son >= MIN_SAMPLES, es **core de primer orden**; si no, por ahora es **ruido**. | `1` o `0` |
| 2 | Cada punto de ruido que esté a <= EPS de un core de primer orden se re-etiqueta como **core de segundo orden**. | `2` |

Ambos pasos comparan cada punto contra todos los demás, así que el trabajo es **O(n²)**: con
200 000 puntos son unas 40 000 millones de comparaciones. Por eso vale la pena paralelizarlo.

El PDF pide:

1. Una **versión serial**.
2. Dos **versiones paralelas** con OpenMP:
   - **P1:** la matriz de puntos completa, sin dividir.
   - **P2:** el espacio dividido en `k` bloques, respondiendo "¿cómo unir los resultados para mantener consistencia?".
3. Un **experimento de rendimiento** con:
   - k = {2, 4, 8, 16, 32, 64};
   - n = {20 000 … 200 000};
   - hilos = {1, cores/2, cores, cores·2};
   - 10 repeticiones por configuración;
   - gráficas de speedup contra la serial.
4. **Entrada y salida** en csv, compatibles con la libreta `DBSCAN_noise.ipynb` del profe.

---

## 2. Mapa de archivos

```
computoparalelo2026b/
├── DBSCAN_noise.ipynb      ← libreta del profe (adaptada): genera datos, corre el C++, grafica y valida
└── dbscan/
    ├── common.hpp          ← código compartido por las 3 versiones          ┐
    ├── dbscan_serial.cpp   ← versión serial                                 │ CÓDIGO A ENTREGAR
    ├── dbscan_p1.cpp       ← paralela P1 (matriz indivisible)               │ (lo que pide el PDF)
    ├── dbscan_p2.cpp       ← paralela P2 (k bloques + halo)                 ┘
    ├── Makefile            ← compila las 3 versiones con -O2 y OpenMP
    ├── generate_data.py    ← genera los csv de entrada del experimento
    ├── check.py            ← compara un resultado contra sklearn
    ├── run_experiments.py  ← corre todo el experimento → results/experiments.csv
    ├── plot_speedup.py     ← promedios, speedups y gráficas → results/
    ├── README.md           ← instrucciones cortas (en inglés)
    ├── GUIA_PROYECTO.md    ← esta guía
    ├── data/               ← csv de entrada y salida (se generan; no van en git)
    └── results/            ← tiempos medidos, summary.csv y gráficas (sí se entregan)
```

| Archivo | ¿Para qué sirve? | ¿Por qué es necesario? | ¿Se entrega? | ¿Entenderlo a fondo? |
|---|---|---|---|---|
| `common.hpp` | Parámetros, etiquetas, distancia, lectura/escritura de csv, argumentos | Las 3 versiones usan exactamente lo mismo, así que los resultados y los tiempos son comparables | Sí | **Sí** |
| `dbscan_serial.cpp` | El algoritmo sin paralelizar | Es la referencia: speedup = tiempo serial / tiempo paralelo | Sí | **Sí** |
| `dbscan_p1.cpp` | `parallel for` sobre los puntos | Primera estrategia que pide el PDF | Sí | **Sí** |
| `dbscan_p2.cpp` | Cuadrícula de k bloques + halo, `parallel for` sobre los bloques | Segunda estrategia que pide el PDF | Sí | **Sí, la más importante** |
| `Makefile` | Compila con `-O2` y OpenMP (en Mac: `clang++` + `libomp`) | Todas las mediciones usan los mismos flags | Opcional | Saber qué hace |
| `DBSCAN_noise.ipynb` | Genera los puntos, corre el C++, grafica y valida contra sklearn | El PDF exige que la entrada salga de esta libreta y la salida se pueda graficar con ella | Sí (para la demo) | Saber usarla |
| `generate_data.py` | Mismo generador que la libreta, pero para todos los tamaños del experimento | La libreta genera un tamaño a la vez | No es obligatorio | Saber qué hace |
| `check.py` | Compara un csv de resultados contra sklearn, etiqueta por etiqueta | Demuestra que las 3 versiones son correctas | No es obligatorio | Saber qué hace |
| `run_experiments.py` | Corre las 232 configuraciones × 10 repeticiones (2 320 ejecuciones) | Automatiza el experimento | No es obligatorio | Saber qué hace |
| `plot_speedup.py` | Promedia, calcula speedups y dibuja 3 gráficas | El PDF pide gráficas de speedup | No es obligatorio | Saber qué hace |
| `results/experiments.csv` | Todos los tiempos medidos | El PDF pide "archivo con los datos de los experimentos" | **Sí** | — |
| `results/*.png`, `summary.csv` | Gráficas y tabla de promedios | Van en el escrito | **Sí** | Saber interpretarlas |

Los archivos `emptyfloydwarshall.*` y `mpi_hello_world.*` de la carpeta del curso son de otras
clases y **no** forman parte del proyecto.

---

## 3. Cómo se conectan los archivos

```
  DBSCAN_noise.ipynb (celda 1) ──┐
                                 ├──►  data/N_data.csv         (una línea "x,y" por punto)
  generate_data.py ──────────────┘           │
                                             ▼
                          ┌──── dbscan_serial ─┐
            Makefile ───► ├──── dbscan_p1 ─────┤  (los tres incluyen common.hpp)
                          └──── dbscan_p2 ─────┘
                                 │        │
                stdout: tiempo ◄─┘        └─►  data/N_results.csv  (una línea "x,y,etiqueta" por punto)
                     │                               │
                     ▼                               ▼
          run_experiments.py               DBSCAN_noise.ipynb (celdas 3 y 4)  /  check.py
                     │                      gráfica + comparación contra sklearn
                     ▼
          results/experiments.csv
                     │
                     ▼
          plot_speedup.py ──► results/summary.csv + speedup_p1.png + speedup_p2.png + speedup_threads.png
```

Hay tres **contratos** que conectan todo. Si se rompe uno, se rompe la cadena:

1. **Formato csv de entrada:** `x,y` con 3 decimales, un punto por línea. Lo escriben la libreta y
   `generate_data.py` (con el mismo generador y la misma semilla, `random_state=11`), y lo lee `readCSV`
   en `common.hpp`.
2. **Formato csv de salida:** `x,y,etiqueta`, en el **mismo orden** que la entrada. Lo escribe
   `writeCSV`. El orden importa, porque la libreta y `check.py` comparan punto por punto contra sklearn.
3. **Línea de comandos y stdout:**
   - Los programas reciben `input.csv output.csv [hilos] [k]`.
   - Imprimen en stdout **sólo el tiempo** en segundos, y `run_experiments.py` lo lee como número. Por eso
     cualquier aviso extra se manda a `cerr`.
   - Sin argumentos usan valores por defecto, `data/10000_data.csv → data/10000_results.csv`, para que funcione
     el botón ▶ de VS Code.

Todas las rutas apuntan a `dbscan/data/`:

- **Programas C++:** usan rutas relativas a `dbscan/`. Ahí los ejecutan Code Runner y los scripts.
- **Scripts de Python:** calculan la ruta a partir de su propio archivo (`__file__`).
- **Libreta:** busca la carpeta `dbscan/` desde donde se abra.

---

## 4. Cómo correr todo, en orden

Todos los comandos se corren desde `computoparalelo2026b/dbscan/`.

### a) Probar rápido que todo funciona (1 minuto)

1. Abre `DBSCAN_noise.ipynb` en VS Code.
2. Elige el kernel **Python 3.13.2** (`/Library/Frameworks/...`). Es el que tiene sklearn.
3. Usa **Run All**. La libreta:
   - genera 4000 puntos;
   - compila el C++;
   - corre las tres versiones;
   - grafica el resultado;
   - imprime `mismatches vs sklearn: 0  OK` para cada versión.

### b) El experimento completo (para el escrito)

```bash
make                                       # 1. compilar (run_experiments.py también recompila solo)
python3 generate_data.py                   # 2. generar los 8 csv de entrada (20 000 … 200 000)
python3 check.py data/20000_data.csv data/20000_results.csv   # opcional: validar tras una corrida
caffeinate -i python3 run_experiments.py   # 3. experimento: ~1.5 h, deja la Mac conectada y sin otras apps
python3 plot_speedup.py                    # 4. summary.csv + 3 gráficas en results/
```

- `run_experiments.py --quick` es una prueba de 10 segundos. Escribe `results/experiments_quick.csv`,
  que **no** se entrega.
- Si interrumpes el experimento, lo medido hasta ese momento queda guardado, porque el script escribe línea por línea.

### c) Correr una versión a mano

```bash
./dbscan_serial data/20000_data.csv data/20000_results.csv
./dbscan_p1     data/20000_data.csv data/20000_results.csv 8        # 8 hilos
./dbscan_p2     data/20000_data.csv data/20000_results.csv 8 16     # 8 hilos, k = 16
```

### d) Botón ▶ de VS Code

- **Qué hace:** sobre cualquiera de los tres `.cpp`, compila y corre con los valores por defecto (10000 puntos).
- **Limitación:** compila **sin optimizar** (`g++-15 -g`). Sirve para probar, nunca para medir tiempos.
  `run_experiments.py` siempre recompila con el Makefile antes de medir, así que no hay riesgo.

---

## 5. Qué entregarle al profe

Checklist contra la diapositiva "¿Qué debo entregar?":

- [ ] **Todo el código fuente:** `common.hpp`, `dbscan_serial.cpp`, `dbscan_p1.cpp`, `dbscan_p2.cpp`, más el
      `Makefile`, la libreta y los scripts, para que el profe lo pueda reproducir.
- [ ] **Escrito 1, descripción del código y de las estrategias de paralelización.** Usa las secciones
      6 y 7 de esta guía y los comentarios del código.
      - P1: `parallel for` sobre los puntos.
      - P2: cuadrícula, dueño único, halo y barrera, respondiendo explícitamente "¿cómo unir los resultados?".
- [ ] **Escrito 2, evaluación experimental de desempeño**, con:
  - [ ] **Definición del experimento:** valores de n, hilos (1, 4, 8, 16) y k, 10 repeticiones, qué se mide
        (sólo el algoritmo, sin leer ni escribir csv) y speedup = tiempo serial medio / tiempo paralelo medio.
  - [ ] **Hardware:** MacBook Air con Apple M3, 8 cores (4 performance + 4 efficiency, sin hyper-threading),
        16 GB de RAM.
        **Software:** macOS 26.7, Apple clang 17.0.0 con `-O2 -Xpreprocessor -fopenmp`, `libomp` 21.1.8
        (Homebrew) y Python 3.13.2 con scikit-learn 1.9.1.
        La MacBook Air no tiene ventilador: si se calienta, baja la frecuencia del CPU durante corridas
        largas. Vale la pena mencionarlo como posible fuente de variación.
  - [ ] **Interpretación de resultados:** por qué una versión paralela fue mejor que la otra (ver sección 7).
  - [ ] **Sección "Uso responsable y ético de IA Generativa"**, según los lineamientos de IEEE y Elsevier del PDF:
        qué herramienta se usó, para qué y cómo verificaron el resultado. El PDF advierte que usar un LLM
        no exime de entender el código al 100%.
  - [ ] **Gráficas:** `results/speedup_p1.png`, `speedup_p2.png` y `speedup_threads.png`.
  - [ ] **Archivo con los datos:** `results/experiments.csv`, y `summary.csv` como tabla resumen.

Notas del PDF que afectan la calificación:

| Nota | Qué dice | Cómo estamos |
|---|---|---|
| NOTA 1 | Al menos una versión paralela con speedup > 1.5, o −50% | En la prueba rápida, P1 llega a ~4.9x y P2 a ~13x. Confírmalo en `summary.csv`. |
| NOTA 3 y 4 | −20% por día de retraso; entregar después de la hora cuenta como un día | Entregar el martes 20 en clase |
| NOTA 5 | **−80% si no pueden explicar cualquier parte del programa** | Lean la sección 6 y los comentarios de los 4 archivos de código |

---

## 6. Qué código hay que entender (en este orden)

Los comentarios en español de cada archivo explican cada decisión. Léelos en este orden:

1. **`common.hpp`:**
   - parámetros y etiquetas;
   - `dist2`, la distancia al cuadrado sin raíz;
   - `readCSV` y `writeCSV`, con la matriz `double**` al estilo del curso;
   - `getArgs`, los valores por defecto.
2. **`dbscan_serial.cpp`:** los dos pasos del algoritmo tal cual. Si entiendes este, ya entiendes el 70% de los otros dos.
3. **`dbscan_p1.cpp`:** el mismo código con `#pragma omp parallel for`. Lo importante es **por qué no hay
   condiciones de carrera** y por qué `schedule(dynamic, 64)`.
4. **`dbscan_p2.cpp`:** cuadrícula, `owned`, `candidates` (halo) y por qué no hace falta unir resultados.

### Preguntas que probablemente haga el profe

**¿Por qué no usan `critical` ni `atomic`?**
- `points` sólo se lee.
- La iteración i (en P2, el bloque dueño de i) sólo escribe `isCore[i]` y `labels[i]`, posiciones que nadie más toca.
- `neighbors` se declara dentro del ciclo, así que es privada.

No hay datos compartidos que se escriban. Con `critical` el trabajo se volvería serial.

**¿Por qué el paso 2 lee `isCore` y no `labels`?**
- `isCore` sólo tiene los cores del paso 1. Si se leyera `labels`, un punto recién convertido en CORE2
  podría convertir a otros, y el resultado dependería del orden en que los hilos procesan los puntos.
- Así, además, el resultado coincide exactamente con los puntos "border" de sklearn.

**¿Qué pasa entre el paso 1 y el paso 2?**
- Al final de cada `parallel for` hay una **barrera implícita**: ningún hilo empieza el paso 2 hasta que
  todos terminan el paso 1.
- Es indispensable, porque el paso 2 lee `isCore[j]` de puntos que procesó otro hilo.

**¿Por qué `dist2` y `EPS2`?**
- d <= EPS es lo mismo que d² <= EPS².
- Así se evita calcular la raíz cuadrada unas n² veces.

**¿Por qué `schedule(dynamic, 64)` en P1?**
- El M3 tiene cores rápidos y lentos, y en el paso 2 algunos puntos terminan inmediatamente (cores o `break`)
  y otros recorren los n puntos. Con `dynamic`, el hilo que termina antes toma más trabajo.
- Con 64 se amortiza el costo de pedir trabajo, y 64 `bool` consecutivos llenan una línea de caché, lo que
  evita el false sharing.

**¿Por qué `schedule(dynamic, 1)` en P2?**
- Los bloques tienen cargas muy distintas: uno sobre un cluster denso tiene miles de puntos, uno en una esquina casi ninguno.
- Hay pocas tareas (k <= 64), así que repartirlas de una en una no cuesta nada.

**En P2, ¿cómo unen los resultados para mantener la consistencia?** Es la pregunta del enunciado.
No hay paso de unión, gracias a tres reglas:

1. **Dueño único:** cada punto pertenece a un solo bloque (`owned`) y sólo ese bloque escribe su etiqueta,
   directamente en el arreglo global. Ese arreglo ya es el resultado unido.
2. **Halo:** cada bloque lee también los puntos de los bloques vecinos que están a <= EPS de su borde
   (`candidates`). Así, un punto en la frontera ve a todos sus vecinos y obtiene la misma etiqueta que en la
   serial. El `1e-9` extra cubre errores de redondeo.
3. **Barrera** entre el paso 1 y el paso 2: los `isCore` del halo, que pertenecen a otros bloques, ya son finales.

Prueba de que funciona: las tres versiones producen archivos **idénticos byte por byte**. Se verificó
con 87 combinaciones de n, hilos y k, y coinciden 100% con sklearn.

**¿Por qué la cuadrícula es lo más cuadrada posible?**
- Para la misma área, un cuadrado tiene menos perímetro que una tira, y el halo crece con el perímetro.
- Menos halo significa menos comparaciones repetidas.

**¿Por qué no se mide la lectura del csv?**
- Es igual en las tres versiones y no se paraleliza.
- Medirla escondería el speedup real del algoritmo.

---

## 7. ¿Por qué una versión paralela es mejor que la otra?

Esto lo pide el escrito explícitamente. Lo que se ve en la prueba rápida (n = 20 000), que hay que
confirmar con el experimento completo:

| Speedup vs serial | P1 | P2 k=2 | P2 k=16 | P2 k=64 |
|---|---|---|---|---|
| 1 hilo | 1.0 | 1.5 | 2.4 | 3.2 |
| 4 hilos | 3.6 | 1.9 | 5.0 | 8.5 |
| 8 hilos | 4.7 | 2.4 | 6.1 | 13.5 |
| 16 hilos | 4.9 | 2.2 | 6.0 | 11.5 |

Puntos para el análisis:

- **P2 gana porque hace menos trabajo, no sólo porque reparte mejor.**
  - P1 compara cada punto contra los n puntos.
  - P2 compara contra ~n/k puntos más el halo. Por eso P2 con **1 hilo** ya es más rápido que la serial.
  - El speedup de P2 combina dos efectos: menos comparaciones y paralelismo. Eso explica speedups mayores
    que el número de hilos (13.5x con 8 hilos).
- **k pequeño limita a P2:** con k = 2 sólo hay 2 tareas, así que como máximo trabajan 2 hilos aunque se
  pidan 16. En general, si k < hilos, sobran hilos sin trabajo.
- **k grande ayuda, pero tiene un límite:** al crecer k los bloques se achican y el halo (EPS por lado) pesa
  cada vez más. Además, la carga se concentra en los bloques sobre los clusters.
- **P1 no llega a 8x con 8 hilos:**
  - 4 de los 8 cores del M3 son de eficiencia (más lentos).
  - Otras posibles causas que pueden discutir: el acceso a memoria (`double**`, cada punto es un arreglo
    aparte en el heap) y que la MacBook Air baja su frecuencia al calentarse.
- **16 hilos no ayuda:** el M3 tiene 8 hilos de hardware (sin hyper-threading). Con 16, los hilos compiten
  por los mismos cores y sólo se agrega el costo de cambiar de contexto.
- **El speedup crece con n:** con más puntos hay más trabajo por hilo y el costo fijo de crear hilos y
  repartir tareas pesa menos.

---

## 8. Pendientes

1. Correr el experimento completo (sección 4b): unas 1.5 h con la Mac conectada y sin otras apps.
2. Revisar `results/summary.csv` y las 3 gráficas. Confirmar NOTA 1 (speedup > 1.5).
3. Escribir los dos escritos (sección 5), incluyendo la sección de IA generativa.
4. Que **los dos** integrantes lean los comentarios de los 4 archivos de código (NOTA 5).
