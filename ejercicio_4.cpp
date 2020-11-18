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
#include <time.h>

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

    unsigned int totalFiles = (unsigned int) atoi(argv[2]);
    unsigned int n = (unsigned int) atoi(argv[3]);

    DIR *carpeta;
    struct dirent *archivo;
    carpeta = opendir(argv[1]);
    if (carpeta != NULL){
        unsigned int cantTemps = n*n*totalFiles;
        int* temps = (int*) malloc(sizeof(int)*(cantTemps));
        unsigned int ptemps = 0;
        int max_temp = INT_MIN;
        

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
                    char* linea = new char [line.length()+1];
                    char* lin_respaldo = linea; // Para liberar memoria posteriormente
                    strcpy (linea, line.c_str());
                    int offset;
                    // Y cada línea contiene n enteros
                    for(unsigned int i=0; i<n && ptemps < cantTemps; i++){
                        sscanf(linea," %d%n", &temps[ptemps], &offset);
                        linea += offset;
                        //cout << temps[ptemps] << "  ";
                        // Cada entero se guarda en un arreglo
                        if(temps[ptemps] > max_temp){
                            max_temp = temps[ptemps];
                        }
                        ptemps++;
                    }
                    //cout << endl;
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

        clock_t start = clock();
        // Se crea el int_vector y su indice para llenado
        int_vector<> iv_t(cantTemps);
        unsigned int piv_t = 0;
        // Se crea el int_vector
        bit_vector bv_t = bit_vector(cantTemps, 0);

        // Llenado de las estructuras
        iv_t[piv_t++] = temps[0];
        bv_t[0] = 1;
        for(unsigned int i=1; i<cantTemps; i++){
            if(temps[i] == temps[i-1]){
                // Si la celda es igual a la anterior, sólo se marca 0 en el bitmap
                bv_t[i] = 0;
            }else{
                // Si la celda es distinta de la anterior, se marca 1 y se guarda el nuevo valor
                iv_t[piv_t++] = temps[0];
                bv_t[i] = 1;
            }
        }

        iv_t.resize(piv_t);
        int bitsPreCompress = iv_t.width();
        double mBytesPreCompress = size_in_mega_bytes(iv_t);
        util::bit_compress(iv_t);
        double time = (double)(clock() - start) / CLOCKS_PER_SEC;
        time *= 1000;        // A milisegundos

        clock_t start_rrr = clock();
        // Crear rrr_vector con valores de bit_vector y soporte rank
        rrr_vector<63> rrrb_t(bv_t);
        rrr_vector<>::rank_1_type rank_rrrb_t(&rrrb_t);
        double time_rrr = (double)(clock() - start_rrr) / CLOCKS_PER_SEC;
        time_rrr *= 1000;

        clock_t start_sd = clock();
        // Crear sd_vector con valores de bit_vector y soporte rank
        sd_vector<> sdb_t(bv_t);
        sd_vector<>::rank_1_type rank_sdb_t(&sdb_t);
        double time_sd = (double)(clock() - start_sd) / CLOCKS_PER_SEC;
        time_sd *= 1000;
        

        free(temps);
        // Mostrar información de la operación
        printf("************ int_vector ************\n");
        printf("width antes de bit_compress: %d\n", bitsPreCompress);
        cout << "int_vector in mega bytes: " << mBytesPreCompress << "[MB]" << endl;
        printf("width despues de bit_compress: %d bits por entero\n", iv_t.width());
        cout << "int_vector in mega bytes: " << size_in_mega_bytes(iv_t) << "[MB]" << endl;
        cout << "int_vector size: " << iv_t.size() << " temperaturas." << endl;
        printf("************ bit_vector ************\n");
        cout << "bit_vector in mega bytes: " << size_in_mega_bytes(bv_t) << "[MB]" << endl;
        cout << "bit_vector size: " << bv_t.size() << " temperaturas." << endl;
        printf("************ rrr_vector ************\n");
        cout << "rrr_vector in bytes: " << size_in_mega_bytes(rrrb_t) << "[MB]" << endl;
        cout << "rank_rrr_vector in bytes: " << size_in_mega_bytes(rank_rrrb_t) << "[MB]" << endl;
        printf("************ sd_vector ************\n");
        cout << "sd_vector in bytes: " << size_in_mega_bytes(sdb_t) << "[MB]" << endl;
        cout << "rank_sd_vector in bytes: " << size_in_mega_bytes(rank_sdb_t) << "[MB]" << endl;
        printf("************ TOTALES ************\n");
        // El total de cada estrcutura contiene: inv_vector + bitmap + soporte_rank
        double memoriaTotalrrr = size_in_mega_bytes(iv_t) + size_in_mega_bytes(rrrb_t) + size_in_mega_bytes(rank_rrrb_t);
        cout << "Tiempo hasta bit_vector: " << time << " [ms]" << endl;
        cout << "Tiempo construcción rrr_vector y soporte rank: " << time_rrr << " [ms] (Total: " << (time+time_rrr) << " [ms]" << endl;
        cout << "Memoria requerida en la representación con rrr_vector: " << memoriaTotalrrr << "[MB]" << endl;
        double memoriaTotalsd = size_in_mega_bytes(iv_t) + size_in_mega_bytes(sdb_t) + size_in_mega_bytes(rank_sdb_t);
        cout << "Tiempo construcción sd_vector y soporte rank: " << time_sd << " [ms] (Total: " << (time+time_sd) << " [ms]" << endl;
        cout << "Memoria requerida en la representación con sd_vector: " << memoriaTotalsd << "[MB]" << endl;
        
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
