#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h> // Libreria encargada del tema de los directorios
#include <string.h>
#include <sdsl/bit_vectors.hpp>
#include <iostream>

using namespace std;
using namespace sdsl;

int main(int argc, char const *argv[]){
    if(argc < 2){
        printf("Error! Faltan argumentos.\n");
        printf("Usage: %s <data_folder> <total_files> <n_matrix>\n", argv[0]);

        return -1;
    }

    DIR *carpeta;
    struct dirent *archivo;
    carpeta = opendir(argv[1]);
    if (carpeta != NULL){
        while ((archivo = readdir(carpeta))) {
            cout << "-" << archivo->d_name << endl;

            string line;
            char filename[strlen(archivo->d_name) + strlen(argv[1] + 2)];
            strcpy(filename, argv[1]);
            strcat(filename, "/");
            strcat(filename, archivo->d_name);
            cout << "Filename: " << filename << endl;
            ifstream myfile(filename);
            if (myfile.is_open())            {
                while (getline(myfile,line)){
                    cout << line << endl;
                }
                myfile.close();
            }
            else{
                cout << "Unable to open file";    
            } 

        }
        closedir(carpeta);
    }else{
        perror ("Error al abrir el directorio ");
    }

    printf("Ok.\n");
}
