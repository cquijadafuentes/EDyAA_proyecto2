#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <cstring>
#include <sdsl/int_vector_mapper.hpp>
#include <sdsl/bit_vectors.hpp>
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
    	int max_temp = INT_MIN;
        int temp_ant = INT_MIN;
        bit_vector bv_t = bit_vector(cantTemps, 0);
        int pbv_t = 0;

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
                    // En temps (arreglo de enteros) se insertan valores que sólo son distintos del anterior
                    // En bit_vector si inserta 1 si el valor es distinto, y 0 si es igual
                    for(int i=0; i<n && ptemps < cantTemps; i++){
                        sscanf(linea," %d%n", &temps[ptemps], &offset);
                        linea += offset;
                    //    cout << temps[ptemps] << "  ";
                        if(temps[ptemps] > max_temp){
                    		max_temp = temps[ptemps];
                    	}
                        if(temps[ptemps] != temp_ant){
                            bv_t[pbv_t] = 1;
                            temp_ant = temps[ptemps];
                            ptemps++;
                        }else{
                            bv_t[pbv_t] = 0;
                        }
                        pbv_t++;
                    }
                   // cout << endl;
                    delete(lin_respaldo);
                }
                myfile.close();
            }

        }
        closedir(carpeta);
        printf("Cantidad de temperaturas guardadas: %d/%d\n", ptemps,cantTemps);
        printf("max_temp: %d\n", max_temp);
        int bits = 0;
        int aux = 1;
        while(aux <= max_temp){
            aux = aux << 1;
            bits++;
        }
        printf("Bits necesarios: %d\n", bits);
        // Crear bit_vector con los valores desde temps
        int_vector<> iv_t(ptemps);
        for(int i=0; i<ptemps; i++){
            iv_t[i] = temps[i];
        }
        free(temps);
        // Crear rrr_vector con valores de bit_vector y soporte rank
        rrr_vector<63> rrrb_t(bv_t);
        rrr_vector<>::rank_1_type rank_rrrb_t(&rrrb_t);
        sd_vector<> sdb_t(bv_t);
        sd_vector<>::rank_1_type rank_sdb_t(&sdb_t);

        // Mostrar información de la operación
        printf("************ int_vector ************\n");
        printf("width antes de bit_compress: %d\n", iv_t.width());
        cout << "int_vector in mega bytes: " << size_in_bytes(iv_t) << "[B]" << endl;
        util::bit_compress(iv_t);
        printf("width despues de bit_compress: %d\n", iv_t.width());
        cout << "int_vector in mega bytes: " << size_in_bytes(iv_t) << "[B]" << endl;
/*        
        for(int i=0; i<iv_t.size(); i++){
            cout << iv_t[i];
        }
        cout << endl;
*/
        printf("************ bit_vector ************\n");
        cout << "int_vector in bytes: " << size_in_bytes(bv_t) << "[B]" << endl;
        printf("************ rrr_vector ************\n");
        cout << "rrr_vector in bytes: " << size_in_bytes(rrrb_t) << "[B]" << endl;
        cout << "rank_rrr_vector in bytes: " << size_in_bytes(rank_rrrb_t) << "[B]" << endl;
        printf("************ sd_vector ************\n");
        cout << "sd_vector in bytes: " << size_in_bytes(sdb_t) << "[B]" << endl;
        cout << "rank_sd_vector in bytes: " << size_in_bytes(rank_sdb_t) << "[B]" << endl;
        
/*
        for(int i=0; i<ptemps; i++){
            cout << iv_t[i] << "  ";
        }
        cout << endl;
        for(int i=0; i<bv_t.size(); i++){
            cout << bv_t[i];
        }
        cout << endl;
        for(int i=0; i<bv_t.size(); i++){
            cout << rank_rrrb_t.rank(i) << "  ";
        }
        cout << endl;

*/
    }else{
        perror ("Error al abrir el directorio ");
    }
}
