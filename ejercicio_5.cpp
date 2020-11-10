#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include <cstring>
#include <sdsl/int_vector_mapper.hpp>
#include <sdsl/k2_tree.hpp>
#include <iostream>

using namespace std;
using namespace sdsl;

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
        // Matrices para guardar los 0s y 1s desde la 2 matriz en adelante
        // Crear un arreglo tridimensional para las n matrices de n x n cada una.
        // Memoria para las matrices
        vector<vector<vector<int>>> matrices(totalFiles-1, vector<vector<int>>(n, vector<int>(n)));
        // Índices para navegar en las matrices
        int nxn = n*n;          // Para mirar la celda de la matriz anterior en el arreglo
        int indTemps = nxn;     // Comienza en la celda [0][0] de la 2a matriz
        
        for(unsigned int i=0; i < totalFiles-1; i++){
            for(unsigned int j=0; j< n; j++){
                for(unsigned int k=0; k<n; k++){
                    // Comparación para determinar valores 0 o 1 en cada celda
                    if(temps[indTemps] != temps[indTemps-nxn]){
                        matrices[i][j][k] = 1;
                        cout << "1  ";
                    }else{
                        matrices[i][j][k] = 0;
                        cout << "0  ";
                    }
                    indTemps++;
                }
                cout << endl;
            }
            cout << endl;
        }       
        free(temps);

        vector<k2_tree<2>> arbolesk2(matrices.size());
        for(int i=0; i<matrices.size(); i++){
            k2_tree<2> aux(matrices[i]);
            arbolesk2[i] = aux;
        }

        for(int j=0; j<arbolesk2.size(); j++){
            vector<long unsigned int> r = arbolesk2[j].neigh(0);
            for(int i=0; i<r.size(); i++){
                cout << r[i]<< endl;
            }
            cout << endl;
        }


    }else{
        perror ("Error al abrir el directorio ");
    }
}