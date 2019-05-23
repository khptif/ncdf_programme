/*MIT License

Copyright (c) 2019 Sarikaya Fatih

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef H_FONCTION_DONNEE
#define H_FONCTION_DONNEE

#define DIVISION_LATITUDE 360
#define DIVISION_LONGITUDE 720
#define COMMENT_FILE "comment.txt"

#include<netcdf.h>
#include<netcdf_par.h>
#include<stdio.h>
#include<stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

struct tableau_donnee
{
    float choix_niveau[13];
    int nombre_jour;
    float* temperature;

};
struct arg_thread_nc_put
{
    int id_nc;
    char* adresse;
    int* index_lat;
    int* index_lon;
    int numero_index;
};

struct arg_thread_put
{
    int id_file;
    int id_var;
    float* donnee;
    float* donnees_transmettre;
    int taille_index;
    size_t jour;
    int nombre_jour;
    int* lat_i;
    int* lon_i;
    int* thread_tableau;
    int index_thread;
};

struct arg_remplir_tabl
{
    float* tableau_a_remplir;
    float* tableau;
    int debut_tableau_remplir;
    int debut_tableau;
    int nombre_remplir;
    int* thread_tableau;
    int index_thread;
};

struct ncdf_var_info
{
    char* nom;
    nc_type type;
    int nombre_dimension;
    int* dimension;
    void* defaut_valeur;
    int nombre_titre;
    char** titre;
    char** attribut;
    int chunk_vrai;
    size_t* chunk;
    int compression_vrai;
    int compression_niveau;
};

typedef struct ncdf_var_info ncdf_var_info;
typedef struct arg_thread_put arg_put;
typedef struct tableau_donnee tableau_donnee;
typedef struct arg_thread_nc_put args;
typedef struct arg_remplir_tabl arg_remplir_tabl;

void ncdf_var_info_define(ncdf_var_info* variable,char* nom,nc_type type,int nombre_dimension,int* dimension,void* defaut_valeur,int nombre_titre,char** titre,char** attribut,int chunk_vrai,size_t* chunk, int compression_vrai,int compression_niveau);
void ncdf_var_integrer(ncdf_var_info* variable,int id_fichier_ncdf,int* variable_id);
void handle_error(int status);
void printComment(const char* commentaire);
void get_jour_decennie(const char* adresse,const int annee,const int nb_decennie,int* nombre_jour,int* numero_premier_jour);

int create_fichier_nc(char* nom,int nombre_jour,int numero_jour);
char* get_donnne(const char* adresse,int* taille);// lire les donnees d'un fichier dat

void create_tableau_donnee(const char* adresse,const int nombre_jour,tableau_donnee* retour_tableau);
void affiche_tableau(const tableau_donnee* tableau);
void free_tableau(tableau_donnee* tableau);

void jour_decennie(const char* adresse);

void put_donnee(int id_nc_file, char* repertoire,int* index_lat,int* index_lon, int index_taille,int nombre_jour,int annee,int thread_nombre);

void nc_put(void* arguments);

void thread_remplir_tab(void* arg);
#endif
