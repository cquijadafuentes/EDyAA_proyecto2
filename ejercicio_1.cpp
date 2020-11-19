#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include <cstring>
#include <sdsl/int_vector_mapper.hpp>
#include <sdsl/suffix_trees.hpp>
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

    int totalFiles = (int) atoi(argv[2]);
    int n = (int) atoi(argv[3]);

    DIR *carpeta;
    struct dirent *archivo;
    carpeta = opendir(argv[1]);
    if (carpeta != NULL){
    	int cantTemps = n*n*totalFiles;
    	int* temps = (int*) malloc(sizeof(int)*(cantTemps));
    	int ptemps = 0;
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
                        //cout << temps[ptemps] << "  ";
                        // Cada entero se guarda en un arreglo
                    	ptemps++;
                    }
                    //cout << endl;
                    delete(lin_respaldo);
                }
                myfile.close();
            }

        }
        closedir(carpeta);
        
        // Se crea el bit_vector con los valores desde temps
        int_vector<> iv_t(cantTemps);
        for(int i=0; i<cantTemps; i++){
            iv_t[i] = temps[i];
        }
        free(temps);
        
		// Calcular y mostrar la entropia de orden 0
        cst_sct3<csa_wt<wt_int<rrr_vector<>>>> cst;
		construct_im(cst, iv_t);
		double h0 = get<0>(Hk(cst, 0));
		
		printf("%s %1.5f\n", argv[1], h0);

    }else{
        perror ("Error al abrir el directorio ");
    }
}

