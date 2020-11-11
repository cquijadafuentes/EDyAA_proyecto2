#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include <cstring>
#include <sdsl/int_vector_mapper.hpp>
#include <sdsl/k2_tree.hpp>
#include <iostream>
#include <math.h>

using namespace std;
using namespace sdsl;

typedef struct matrixsByK2treeA{
    unsigned int n;
    int_vector<> valores;           // Valores distintos de las matrices
    int_vector<> pos;               // Posiciones donde comienzan los valores distintos de cada matriz
    vector<k2_tree<2>> matricesk2;    // Matrices de k2-trees
}MATRIXK2TREE;

// Primera aproximación de ED para guardar las matrices en k2tree
// Desventaja: obtener el valor de una celda, en el peor caso, explora todas las celdas.
void mktA_crear(int * temps, int n, unsigned int totalFiles);


// Segunda aproximación
// Cada log_2(n) matrices, se almacena una versión completa
// obtener el valor de una celda, en el peor caso, explora log_2(n) celdas
void mktB_crear(int * temps, int n, unsigned int totalFiles);


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

    int const totalFiles = (int) atoi(argv[2]);
    int const n = (int) atoi(argv[3]);

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
            char filename[strlen(archivo->d_name) + strlen(argv[1] + 2)];
            strcpy(filename, argv[1]);
            strcat(filename, "/");
            strcat(filename, archivo->d_name);
            ifstream myfile(filename);
            if (myfile.is_open()){
                // Por cada archivo se exploran las líneas
                while (getline(myfile,line)){
                    char* linea = new char [line.length()+1];
                    char* lin_respaldo = linea; // Para liberar memoria posteriormente
                    strcpy (linea, line.c_str());
                    int offset;
                    // Y cada línea contiene n enteros
                    for(int i=0; i<n && ptemps < cantTemps; i++){
                        sscanf(linea," %d%n", &temps[ptemps], &offset);
                        linea += offset;
                        // Cada entero se guarda en un arreglo
                        if(temps[ptemps] > max_temp){
                            max_temp = temps[ptemps];
                        }
                        ptemps++;
                    }
                    delete(lin_respaldo);
                }
                myfile.close();
            }

        }
        closedir(carpeta);
        
        mktA_crear(temps, n, (unsigned int)totalFiles);
        mktB_crear(temps, n, (unsigned int)totalFiles);

        free(temps);


    }else{
        perror ("Error al abrir el directorio ");
    }
}


void mktA_crear(int * temps, int n, unsigned int totalFiles){
    cout << "*** PROPUESTA A ***" << endl;
    MATRIXK2TREE edcMK2T;
    edcMK2T.n = n;

    // Matrices para guardar los 0s y 1s desde la 2 matriz en adelante
    // Crear un arreglo tridimensional para las n matrices de n x n cada una.
    // Memoria para las matrices
    vector<vector<vector<int>>> matrices;
    // Índices para navegar en las matrices
    unsigned int nxn = n*n;          // Para mirar la celda de la matriz anterior en el arreglo
    unsigned int indTemps = nxn;     // Comienza en la celda [0][0] de la 2a matriz
    unsigned int cantMaxVD = nxn*(totalFiles);    // Máxima cantidad de valores distintos posibles
    edcMK2T.valores = int_vector<> (cantMaxVD);
    edcMK2T.pos = int_vector<> (totalFiles);     // Indica dónde comienzan los valores para cada matriz rep x k2tree
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
        edcMK2T.valores[i]=temps[i];
    }
    cantVD = nxn;
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
                edcMK2T.valores[cantVD++] = temps[indTemps];
            }
        }
        matrices.push_back(auxMat);
    }
    // Ajustar tamaño del arreglo de valores según los encontrados.
    edcMK2T.valores.resize(cantVD);

    int sizeInBytesK2trees = 0;
    edcMK2T.matricesk2 = vector<k2_tree<2>>(matrices.size());
    
    for(unsigned int i=0; i<matrices.size(); i++){
        cout << "Mat[" << i << "]" << endl;
        for(int j = 0; j<n; j++){
            for(int k=0; k<n; k++){
                cout << matrices[i][j][k] << " ";
            }
            cout << endl;
        }
        edcMK2T.matricesk2[i] = k2_tree<2>(matrices[i]);
        sizeInBytesK2trees += size_in_bytes(edcMK2T.matricesk2[i]);
    }

    cout << "Cantidad de valores diferentes: " << edcMK2T.valores.size() - nxn << endl;

    cout << "k2_trees size in KiloBytes: " << sizeInBytesK2trees/1024 << " [KB]" << endl;
}


void mktB_crear(int * temps, int n, unsigned int totalFiles){
    cout << "*** PROPUESTA B ***" << endl;
    MATRIXK2TREE edcMK2T;
    edcMK2T.n = n;

    // Matrices para guardar los 0s y 1s desde la 2 matriz en adelante
    // Crear un arreglo tridimensional para las n matrices de n x n cada una.
    // Memoria para las matrices
    vector<vector<vector<int>>> matrices;
    // Índices para navegar en las matrices
    unsigned int nxn = n*n;          // Para mirar la celda de la matriz anterior en el arreglo
    unsigned int indTemps = nxn;     // Comienza en la celda [0][0] de la 2a matriz
    unsigned int cantMaxVD = nxn*(totalFiles);    // Máxima cantidad de valores distintos posibles
    edcMK2T.valores = int_vector<> (cantMaxVD);
    edcMK2T.pos = int_vector<> (totalFiles);     // Indica dónde comienzan los valores para cada matriz rep x k2tree
    unsigned int cantVD = 0;
    unsigned int x = 0;
    unsigned int y = 0;
    unsigned int pbase = 0;

    // Similar al proceso anterior
    unsigned int log2n = (unsigned int) log2(totalFiles);
    for(unsigned int i=0; i < totalFiles; i++){
        bool inicioBloque = i%log2n == 0;
        edcMK2T.pos[i] = cantVD;
        if(!inicioBloque){
            // Si no es un inicio de bloque, se hace como la primera propuesta A
            vector<vector<int>> auxMat(n, vector<int>(n,0));
            for(unsigned int j=0; j< nxn; j++){
                pair<unsigned int, unsigned int> xy = MortonDecode2(j);
                x = xy.first;
                y = xy.second;
                indTemps = pbase + x*n + y;
                if(temps[indTemps] != temps[indTemps-nxn]){
                    auxMat[x][y] = 1;
                    edcMK2T.valores[cantVD++] = temps[indTemps];
                }
            }
            matrices.push_back(auxMat);
        }else{
            // En inicio de bloque se copian todas las celdas como si fuera un valor
            // diferente de la matriz anterior
            // Se debe mantener el recorrido de Morton
            for(unsigned int j=0; j< nxn; j++){
                pair<unsigned int, unsigned int> xy = MortonDecode2(j);
                x = xy.first;
                y = xy.second;
                indTemps = pbase + x*n + y;
                edcMK2T.valores[cantVD++] = temps[indTemps];
            }
        }
        pbase += nxn;
    }
    // Ajustar tamaño del arreglo de valores según los encontrados.
    edcMK2T.valores.resize(cantVD);

    int sizeInBytesK2trees = 0;
    edcMK2T.matricesk2 = vector<k2_tree<2>>(matrices.size());
    
    for(unsigned int i=0; i<matrices.size(); i++){
        cout << "Mat[" << i << "]" << endl;
        for(int j = 0; j<n; j++){
            for(int k=0; k<n; k++){
                cout << matrices[i][j][k] << " ";
            }
            cout << endl;
        }
        edcMK2T.matricesk2[i] = k2_tree<2>(matrices[i]);
        sizeInBytesK2trees += size_in_bytes(edcMK2T.matricesk2[i]);
    }

    cout << "Cantidad de valores diferentes: " << edcMK2T.valores.size() - nxn << endl;

    cout << "k2_trees size in KiloBytes: " << sizeInBytesK2trees/1024 << " [KB]" << endl;
}