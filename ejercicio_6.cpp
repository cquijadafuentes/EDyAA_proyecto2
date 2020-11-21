#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include <cstring>
#include <sdsl/int_vector_mapper.hpp>
#include <sdsl/k2_tree.hpp>
#include <sdsl/enc_vector.hpp>
#include <iostream>
#include <math.h>

using namespace std;
using namespace sdsl;

typedef struct matrixsByK2tree{
    int minimaDiferencia;           // Se utiliza para desplazar los valores en valoresD y comenzar desde 0
    int minimaTemperatura;          // Se utiliza para desplazar los valores en valoresC y comenzar desde 0
    unsigned int totalMatrices;     // Cantidad de matrices representadas en la estructura
    unsigned int n;                 // Dimensión de las matrices de la estructura
    int_vector<> valoresC;          // Matrices representadas completamente
    int_vector<> valoresD;          // Valores distintos de las matrices
    int_vector<> pos;               // Posiciones donde comienzan los valores distintos de valoresD
    vector<k2_tree<2>> matricesk2;  // Matrices de k2-trees
}MATRIXK2TREE;

// Primera aproximación de ED para guardar las matrices en k2tree
// Desventaja: obtener el valor de una celda, en el peor caso, explora todas las celdas.
void mktA_crear(int * temps, unsigned int n, unsigned int totalFiles);


// Segunda aproximación
// Cada log_2(n) matrices, se almacena una versión completa
// obtener el valor de una celda, en el peor caso, explora log_2(n) celdas
void mktB_crear(int * temps, unsigned int n, unsigned int totalFiles);


// Tercera aproximación
// Misma estructura que Segunda aproximación
// En vez de comparar con la anterior, se compara con el inicio del bloque
void mktC_crear(int * temps, unsigned int n, unsigned int totalFiles);


// MORTON DECODE 
// http://asgerhoedt.dk/?p=276

unsigned int CompactBy1(unsigned int x) {
    x &= 0x55555555;                  // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
    x = (x ^ (x >>  1)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
    x = (x ^ (x >>  2)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
    x = (x ^ (x >>  4)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
    x = (x ^ (x >>  8)) & 0x0000ffff; // x = ---- ---- ---- ---- fedc ba98 7654 3210
    return x;
}

pair<unsigned int, unsigned int> MortonDecode2(unsigned int c){
    unsigned int x = CompactBy1(c >> 1);
    unsigned int y = CompactBy1(c);
    return pair<unsigned int, unsigned int>(x,y);
}

int main(int argc, char const *argv[]){
    if(argc < 4){
        printf("Error! Faltan argumentos.\n");
        printf("Uso: %s <carpeta> <total_archivos> <n_matriz>\n", argv[0]);
        printf("Donde:\n");
        printf("\tcarpeta es la ruta a la carpeta con los archivos de datos.\n");
        printf("\ttotal_archivos es la cantidad de archivos de datos en la carpeta.\n");
        printf("\tn_matriz es la dimensión de las matrices cuadradas en cada archivo.\n");
        return -1;
    }

    unsigned int const totalFiles = (unsigned int) atoi(argv[2]);
    unsigned int const n = (unsigned int) atoi(argv[3]);
    if(n==0 || totalFiles == 0){
        return -1;
    }
    cout << "n: " << n << endl;
    DIR *carpeta;
    struct dirent *archivo;
    carpeta = opendir(argv[1]);
    if (carpeta != NULL){
        int cantTemps = n*n*totalFiles;
        // Lectura de todas las temperaturas y se almacenan en un arreglo
        int* temps = (int*) malloc(sizeof(int)*(cantTemps));
        int ptemps = 0;
        int max_temp = 0;
    
        while ((archivo = readdir(carpeta))) {
            // Se explora el directorio archivo por archivo
            string line;
            char filename[strlen(archivo->d_name) + strlen(argv[1]) + 2];
            strcpy(filename, argv[1]);
            strcat(filename, "/");
            strcat(filename, archivo->d_name);
            ifstream myfile(filename);
            if (myfile.is_open()){
                // Por cada archivo se exploran las líneas
                while (getline(myfile,line)){
                    char* linea_op = new char [line.length()+1];    // Puntero original que se debe liberar
                    char* linea = linea_op;                         // Para moverse en la linea
                    strcpy (linea, line.c_str());
                    int offset;
                    // Y cada línea contiene n enteros
                    for(unsigned int i=0; i<n && ptemps < cantTemps; i++){
                        sscanf(linea," %d%n", &temps[ptemps], &offset);
                        linea += offset;
                        // Cada entero se guarda en un arreglo
                        if(temps[ptemps] > max_temp){
                            max_temp = temps[ptemps];
                        }
                        ptemps++;
                    }
                    delete(linea_op);
                }
                myfile.close();
            }

        }
        closedir(carpeta);
cout << "n: " << n << endl;
        mktA_crear(temps, n, (unsigned int)totalFiles);
        mktB_crear(temps, n, (unsigned int)totalFiles);
        mktC_crear(temps, n, (unsigned int)totalFiles);

        free(temps);


    }else{
        perror ("Error al abrir el directorio ");
    }
}


void mktA_crear(int * temps, unsigned int n, unsigned int totalFiles){
    cout << "*** PROPUESTA A ***" << endl;
    clock_t start = clock();
    MATRIXK2TREE edcMK2T;
    edcMK2T.n = n;

    // Matrices para guardar los 0s y 1s desde la 2 matriz en adelante
    // Crear un arreglo tridimensional para las n matrices de n x n cada una.
    // Memoria para las matrices
    vector<vector<vector<int>>> matrices;
    // Índices para navegar en las matrices
    unsigned int nxn = n*n;          // Para mirar la celda de la matriz anterior en el arreglo
    unsigned int indTemps = nxn;     // Comienza en la celda [0][0] de la 2a matriz
    unsigned int cantMaxVD = nxn*(totalFiles-1);    // Máxima cantidad de valores distintos posibles
    edcMK2T.valoresD = int_vector<> (cantMaxVD);
    edcMK2T.valoresC = int_vector<> (nxn);
    edcMK2T.pos = int_vector<> (totalFiles-1);     // Indica dónde comienzan los valores para cada matriz rep x k2tree
    edcMK2T.totalMatrices = totalFiles;
    edcMK2T.minimaTemperatura = temps[0];
    edcMK2T.minimaDiferencia = temps[1] - temps[0];
    unsigned int cantVD = 0;
    unsigned int x = 0;
    unsigned int y = 0;
    unsigned int pbase = 0;
    
    // Las matrices están almacenadas en un arreglo de valores de tamaño nxn*(totalFiles-1)
    // La estrategia es calcular generar la decodificación del j-ésimo código Morton, que indica
    // la posición x,y en la matriz que luego será representada en k2-tree y esa celda se corresponde con
    // la posición pbase+(x*n)+y en el arreglo de valores
    // pbase toma valores en nxn * i (implícitamente);

    // Primero se guarda toda la primera matriz en el arreglo de valores diferentes
    // Se considera la primera matriz como toda diferente porque no tiene con qué compararse
    for(unsigned int i=0; i<nxn; i++){
        edcMK2T.valoresC[i]=temps[i];
        if(temps[i] < edcMK2T.minimaTemperatura){
            edcMK2T.minimaTemperatura = temps[i];
        }
    }
    cantVD = 0;
    for(unsigned int i=0; i < totalFiles-1; i++){
        edcMK2T.pos[i] = cantVD;
        vector<vector<int>> auxMat(n, vector<int>(n,0));
        pbase += nxn;
        for(unsigned int j=0; j< nxn; j++){
            pair<unsigned int, unsigned int> xy = MortonDecode2(j);
            x = xy.first;
            y = xy.second;
            indTemps = pbase + x*n + y;
            if(temps[indTemps] != temps[indTemps-nxn]){
                auxMat[x][y] = 1;
                edcMK2T.valoresD[cantVD++] = temps[indTemps] - temps[indTemps-nxn];
                if(temps[indTemps] - temps[indTemps-nxn] < edcMK2T.minimaDiferencia){
                    edcMK2T.minimaDiferencia = temps[indTemps] - temps[indTemps-nxn];
                }
            }
        }

        matrices.push_back(auxMat);
    }
    // Ajustar tamaño del arreglo de valores según los encontrados.
    edcMK2T.valoresD.resize(cantVD);

    double sizeAntes = size_in_mega_bytes(edcMK2T.valoresD) + size_in_mega_bytes(edcMK2T.valoresC);
    sizeAntes += size_in_mega_bytes(edcMK2T.pos);

    // Ajustar valores de diferencias y temperaturas para que comiencen desde 0 y 
    // así mejorar la comresión del int_vector
    for(unsigned int i=0; i < edcMK2T.valoresD.size(); i++){
        edcMK2T.valoresD[i] -= edcMK2T.minimaDiferencia;
    }
    util::bit_compress(edcMK2T.valoresD);
    for(unsigned int i=0; i < edcMK2T.valoresC.size(); i++){
        edcMK2T.valoresC[i] -= edcMK2T.minimaTemperatura;
    }
    util::bit_compress(edcMK2T.valoresC);
    util::bit_compress(edcMK2T.pos);

    double sizeDespues = size_in_mega_bytes(edcMK2T.valoresD) + size_in_mega_bytes(edcMK2T.valoresC);
    sizeDespues += size_in_mega_bytes(edcMK2T.pos);

    double sizeK2trees = 0.0;
    edcMK2T.matricesk2 = vector<k2_tree<2>>(matrices.size());
    for(unsigned int i=0; i<matrices.size(); i++){
        /*
        cout << "Mat[" << i << "]" << endl;
        for(unsigned int j = 0; j<n; j++){
            for(unsigned int k=0; k<n; k++){
                cout << matrices[i][j][k] << " ";
            }
            cout << endl;
        }
        */
        edcMK2T.matricesk2[i] = k2_tree<2>(matrices[i]);
        sizeK2trees += size_in_mega_bytes(edcMK2T.matricesk2[i]);
    }
    double time = (double)(clock() - start) / CLOCKS_PER_SEC;
    time *= 1000;        // A milisegundos

    cout << "Tamaño de int_vectors antes de bit_compress: " << sizeAntes << " [MB]" << endl;
    cout << "Tamaño de int_vectors despues de bit_compress: " << sizeDespues << " [MB]" << endl;

    cout << "Temperatura mínima: " << edcMK2T.minimaTemperatura << endl;
    cout << "Diferencia mínima: " << edcMK2T.minimaDiferencia << endl;

    cout << "Cantidad de valores diferentes: " << edcMK2T.valoresD.size() - nxn << endl;
    cout << "k2_trees size: " << sizeK2trees << " [MB]" << endl;
    
    cout << "************ TOTALES ************" << endl;
    cout << "Tiempo de la construcción: " << time << " [ms]" << endl;
    cout << "Tamaño de la estructura: " << (sizeDespues+sizeK2trees) << " [MB]" << endl;

}


void mktB_crear(int * temps, unsigned int n, unsigned int totalFiles){
    cout << "*** PROPUESTA B ***" << endl;
    clock_t start = clock();
    MATRIXK2TREE edcMK2T;
    edcMK2T.n = n;

    // Matrices para guardar los 0s y 1s desde la 2 matriz en adelante
    // Crear un arreglo tridimensional para las n matrices de n x n cada una.
    // Memoria para las matrices
    vector<vector<vector<int>>> matrices;
    // Índices para navegar en las matrices
    unsigned int nxn = n*n;          // Para mirar la celda de la matriz anterior en el arreglo
    unsigned int log2n = (unsigned int) log2(totalFiles);   // Tamaño de cada bloque
    unsigned int cantBloques = totalFiles / log2n;  // Cant de bloques en la estructura
    unsigned int cantMaxVD = nxn*(totalFiles - cantBloques);    // Máxima cantidad de valores distintos posibles
    edcMK2T.valoresD = int_vector<> (cantMaxVD);
    edcMK2T.valoresC = int_vector<> (cantBloques*nxn);
    edcMK2T.pos = int_vector<> (totalFiles - cantBloques);     // Indica dónde comienzan los valores para cada matriz rep x k2tree
    unsigned int posMD = 0;
    edcMK2T.totalMatrices = totalFiles;
    edcMK2T.minimaTemperatura = temps[0];
    edcMK2T.minimaDiferencia = temps[1] - temps[0];
    unsigned int cantVD = 0;
    unsigned int cantVC = 0;
    unsigned int x = 0;
    unsigned int y = 0;
    unsigned int pbase = 0;
    unsigned int indTemps;
    unsigned int indTempsCompara;

    // Similar al proceso anterior
    for(unsigned int i=0; i < totalFiles; i++){
        bool inicioBloque = i%log2n == 0;
        if(!inicioBloque){
            // Si no es un inicio de bloque, se hace como la primera propuesta A
            edcMK2T.pos[posMD++] = cantVD;
            vector<vector<int>> auxMat(n, vector<int>(n,0));
            for(unsigned int j=0; j< nxn; j++){
                pair<unsigned int, unsigned int> xy = MortonDecode2(j);
                x = xy.first;
                y = xy.second;
                indTemps = pbase + x*n + y;
                indTempsCompara = indTemps-nxn;
                if(temps[indTemps] != temps[indTempsCompara]){
                    auxMat[x][y] = 1;
                    edcMK2T.valoresD[cantVD++] = temps[indTemps] - temps[indTempsCompara];
                    if(temps[indTemps] - temps[indTempsCompara] < edcMK2T.minimaDiferencia){
                        edcMK2T.minimaDiferencia = temps[indTemps] - temps[indTempsCompara];
                    }
                }
            }
            matrices.push_back(auxMat);
        }else{
            // En inicio de bloque se copian todas las celdas en el arreglo de valores completos
            for(unsigned int j=0; j< nxn; j++){
                indTemps = pbase + j;
                edcMK2T.valoresC[cantVC++] = temps[indTemps];
                if(temps[indTemps] < edcMK2T.minimaTemperatura){
                    edcMK2T.minimaTemperatura = temps[indTemps];
                }
            }
        }
        pbase += nxn;
    }
    // Ajustar tamaño del arreglo de valores según los encontrados.
    edcMK2T.valoresD.resize(cantVD);

    double sizeAntes = size_in_mega_bytes(edcMK2T.valoresD) + size_in_mega_bytes(edcMK2T.valoresC);
    sizeAntes += size_in_mega_bytes(edcMK2T.pos);

    // Ajustar valores de diferencias y temperaturas para que comiencen desde 0 y 
    // así mejorar la comresión del int_vector
    for(unsigned int i=0; i < edcMK2T.valoresD.size(); i++){
        edcMK2T.valoresD[i] -= edcMK2T.minimaDiferencia;
    }
    util::bit_compress(edcMK2T.valoresD);
    for(unsigned int i=0; i < edcMK2T.valoresC.size(); i++){
        edcMK2T.valoresC[i] -= edcMK2T.minimaTemperatura;
    }
    util::bit_compress(edcMK2T.valoresC);
    util::bit_compress(edcMK2T.pos);

    double sizeDespues = size_in_mega_bytes(edcMK2T.valoresD) + size_in_mega_bytes(edcMK2T.valoresC);
    sizeDespues += size_in_mega_bytes(edcMK2T.pos);

    double sizeK2trees = 0.0;
    edcMK2T.matricesk2 = vector<k2_tree<2>>(matrices.size());
    
    for(unsigned int i=0; i<matrices.size(); i++){
        /*
        cout << "Mat[" << i << "]" << endl;
        for(unsigned int j = 0; j<n; j++){
            for(unsigned int k=0; k<n; k++){
                cout << matrices[i][j][k] << " ";
            }
            cout << endl;
        }
        */
        edcMK2T.matricesk2[i] = k2_tree<2>(matrices[i]);
        sizeK2trees += size_in_mega_bytes(edcMK2T.matricesk2[i]);
    }
    double time = (double)(clock() - start) / CLOCKS_PER_SEC;
    time *= 1000;        // A milisegundos

    cout << "Tamaño de int_vectors antes de bit_compress: " << sizeAntes << " [MB]" << endl;
    cout << "Tamaño de int_vectors despues de bit_compress: " << sizeDespues << " [MB]" << endl;

    cout << "Temperatura mínima: " << edcMK2T.minimaTemperatura << endl;
    cout << "Diferencia mínima: " << edcMK2T.minimaDiferencia << endl;

    cout << "Cantidad de valores diferentes: " << edcMK2T.valoresD.size() - nxn << endl;
    cout << "k2_trees size: " << sizeK2trees << " [MB]" << endl;
    
    cout << "************ TOTALES ************" << endl;
    cout << "Tiempo de la construcción: " << time << " [ms]" << endl;
    cout << "Tamaño de la estructura: " << (sizeDespues+sizeK2trees) << " [MB]" << endl;
}


void mktC_crear(int * temps, unsigned int n, unsigned int totalFiles){
    cout << "*** PROPUESTA C ***" << endl;
    clock_t start = clock();
    MATRIXK2TREE edcMK2T;
    edcMK2T.n = n;

    // Matrices para guardar los 0s y 1s desde la 2 matriz en adelante
    // Crear un arreglo tridimensional para las n matrices de n x n cada una.
    // Memoria para las matrices
    vector<vector<vector<int>>> matrices;
    // Índices para navegar en las matrices
    unsigned int nxn = n*n;          // Para mirar la celda de la matriz anterior en el arreglo
    unsigned int log2n = (unsigned int) log2(totalFiles);   // Tamaño de cada bloque
    unsigned int cantBloques = totalFiles / log2n;  // Cant de bloques en la estructura
    unsigned int cantMaxVD = nxn*(totalFiles - cantBloques);    // Máxima cantidad de valores distintos posibles
    edcMK2T.valoresD = int_vector<> (cantMaxVD);
    edcMK2T.valoresC = int_vector<> (cantBloques*nxn);
    edcMK2T.pos = int_vector<> (totalFiles - cantBloques);     // Indica dónde comienzan los valores para cada matriz rep x k2tree
    unsigned int posMD = 0;
    edcMK2T.totalMatrices = totalFiles;
    edcMK2T.minimaTemperatura = temps[0];
    edcMK2T.minimaDiferencia = temps[1] - temps[0];
    unsigned int cantVD = 0;
    unsigned int cantVC = 0;
    unsigned int x = 0;
    unsigned int y = 0;
    unsigned int pbase = 0;
    unsigned int indTemps;
    unsigned int indTempsCompara;
    unsigned int indBloque = 0;

    // Similar al proceso anterior
    for(unsigned int i=0; i < totalFiles; i++){
        bool inicioBloque = i%log2n == 0;
        if(!inicioBloque){
            // Si no es un inicio de bloque, se hace como la primera propuesta A
            edcMK2T.pos[posMD++] = cantVD;
            vector<vector<int>> auxMat(n, vector<int>(n,0));
            for(unsigned int j=0; j< nxn; j++){
                pair<unsigned int, unsigned int> xy = MortonDecode2(j);
                x = xy.first;
                y = xy.second;
                indTemps = pbase + x*n + y;
                // Ahora la comparación es contra la primera matriz en el bloque
                indTempsCompara = (indBloque * nxn) + indTemps % nxn;
                if(temps[indTemps] != temps[indTempsCompara]){
                    auxMat[x][y] = 1;
                    edcMK2T.valoresD[cantVD++] = temps[indTemps] - temps[indTempsCompara];
                    if(temps[indTemps] - temps[indTempsCompara] < edcMK2T.minimaDiferencia){
                        edcMK2T.minimaDiferencia = temps[indTemps] - temps[indTempsCompara];
                    }
                }
            }
            matrices.push_back(auxMat);
        }else{
            // En inicio de bloque se copian todas las celdas en el arreglo de valores completos
            for(unsigned int j=0; j< nxn; j++){
                indTemps = pbase + j;
                edcMK2T.valoresC[cantVC++] = temps[indTemps];
                if(temps[indTemps] < edcMK2T.minimaTemperatura){
                    edcMK2T.minimaTemperatura = temps[indTemps];
                }
            }
            indBloque++;
        }
        pbase += nxn;
    }
    // Ajustar tamaño del arreglo de valores según los encontrados.
    edcMK2T.valoresD.resize(cantVD);

    double sizeAntes = size_in_mega_bytes(edcMK2T.valoresD) + size_in_mega_bytes(edcMK2T.valoresC);
    sizeAntes += size_in_mega_bytes(edcMK2T.pos);

    // Ajustar valores de diferencias y temperaturas para que comiencen desde 0 y 
    // así mejorar la comresión del int_vector
    for(unsigned int i=0; i < edcMK2T.valoresD.size(); i++){
        edcMK2T.valoresD[i] -= edcMK2T.minimaDiferencia;
    }
    util::bit_compress(edcMK2T.valoresD);
    for(unsigned int i=0; i < edcMK2T.valoresC.size(); i++){
        edcMK2T.valoresC[i] -= edcMK2T.minimaTemperatura;
    }
    util::bit_compress(edcMK2T.valoresC);
    util::bit_compress(edcMK2T.pos);

    double sizeDespues = size_in_mega_bytes(edcMK2T.valoresD) + size_in_mega_bytes(edcMK2T.valoresC);
    sizeDespues += size_in_mega_bytes(edcMK2T.pos);

    double sizeK2trees = 0.0;
    edcMK2T.matricesk2 = vector<k2_tree<2>>(matrices.size());
    
    for(unsigned int i=0; i<matrices.size(); i++){
        /*
        cout << "Mat[" << i << "]" << endl;
        for(unsigned int j = 0; j<n; j++){
            for(unsigned int k=0; k<n; k++){
                cout << matrices[i][j][k] << " ";
            }
            cout << endl;
        }
        */
        edcMK2T.matricesk2[i] = k2_tree<2>(matrices[i]);
        sizeK2trees += size_in_mega_bytes(edcMK2T.matricesk2[i]);
    }
    double time = (double)(clock() - start) / CLOCKS_PER_SEC;
    time *= 1000;        // A milisegundos

    cout << "Tamaño de int_vectors antes de bit_compress: " << sizeAntes << " [MB]" << endl;
    cout << "Tamaño de int_vectors despues de bit_compress: " << sizeDespues << " [MB]" << endl;

    cout << "Temperatura mínima: " << edcMK2T.minimaTemperatura << endl;
    cout << "Diferencia mínima: " << edcMK2T.minimaDiferencia << endl;

    cout << "Cantidad de valores diferentes: " << edcMK2T.valoresD.size() - nxn << endl;
    cout << "k2_trees size: " << sizeK2trees << " [MB]" << endl;
    
    cout << "************ TOTALES ************" << endl;
    cout << "Tiempo de la construcción: " << time << " [ms]" << endl;
    cout << "Tamaño de la estructura: " << (sizeDespues+sizeK2trees) << " [MB]" << endl;
}