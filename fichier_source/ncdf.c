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

#include<netcdf.h>
#include<stdio.h>
#include<stdlib.h>
#include "fonction_donnee.h"
#include <string.h>
#include <time.h>

#define ADRESSE_PCT_LAKE "pctlake.nc4"
#define ADRESSE_NOMBRE_JOUR "day_of_the_decade_nc.txt"
#define ANNEE 1911
#define MAX_THREAD 32
#define MIN_THREAD 1
#define DEFAULT_NAME_FILE "simstrat-uog_gfdl-esm2m_ewembi_historical_2005soc_co2_watertemp_global_daily_%d_%d.nc4"
#define DEFAULT_ARGUMENT '-'


int main (int argc,char** argv)
{

    // Les arguments du programmes sont :
    //1. adresse du répertoire contenant les dossiers TYYYY0101-TYYYY12
    //2. une année qui déterminera la première décennie à traiter
    //3. le nombre de décennie à traiter
    //4. le nombre de thread utilisé Valeur par defaut 8
    //5. l'adresse du répertoire qui contiendra le fichier final par défaut le dossier contenant le programme
    //6. le shema de nom pour les fichiers finaux. Le nom doit contenir deux '%d'. La valeur par défaut est définie par DEFAULT_NAME_FILE
    //1-3 sont obligatoires. Pour avoir les valeurs par défaut des autres, écrire '-' au bonne emplacement ou ne rien écrire s'il est après le dernier argument nécessaire

    remove("comment.txt");
    printf("Debut du programme \n");
    printComment("Debut du programme");
    // Verification du nombre d'arguments ////
    if(argc<4)
    {
        printf("Erreur, il manque des arguments. %d arguments attendu \n",3);
        exit(EXIT_FAILURE);
    }
    const char* adresse_rep =argv[1];
    const int annee_depart = atoi(argv[2]) - atoi(argv[2])%10;
    const int nombre_decennie = atoi(argv[3]);

    int nombre_thread=8;
    char* repertoire_fichier_final ="";
    char* nom_fichier_final=DEFAULT_NAME_FILE;
	
    if(annee_depart<1600)
	{
		printf("Erreur, l'année est trop petite. Minimum 1660 \n");
		printComment("Erreur, l'année est trop petite. Minimum 1660");
		exit(EXIT_FAILURE);
	}

    if(argc > 4)
    {
        if(atoi(argv[4])>32)
        {
            nombre_thread=MAX_THREAD;
        }
        else if(atoi(argv[4])<1)
        {
            nombre_thread=MIN_THREAD;
        }
        else
        {
            if(argv[4][0]!=DEFAULT_ARGUMENT)
            {
                nombre_thread=atoi(argv[4]);
            }
        }

        if(argc > 5)
        {
            if(argv[5][0]!=DEFAULT_ARGUMENT)
            {
                repertoire_fichier_final=argv[5];
            }
        }

        if(argc > 6)
        {
            if(argv[6][0]!=DEFAULT_ARGUMENT)
            {
                nom_fichier_final = argv[6];
            }

        }
    }

    ///// Récupération de donnée sur des fichiers .nc4 et texte ////
    printf("Extraction des données \n");
    printComment("Extraction des données");
    // les coordonnées: [0]=latitude,[1]=longitude
    float* pct_lake_data[2];
    pct_lake_data[0] = calloc(DIVISION_LATITUDE,sizeof(float));
    pct_lake_data[1] = calloc(DIVISION_LONGITUDE,sizeof(float));
    // les valeurs qui permettent de savoir s'il existe un lac
    double* pctLake = calloc(DIVISION_LATITUDE*DIVISION_LONGITUDE,sizeof(double));
    //les id des variable du fichier .nc4
    int id_pct_lake=0;
    int var_pct_lake_id[3];

    //ouverture du fichier pct_lake.nc4
    handle_error(nc_open(ADRESSE_PCT_LAKE,NC_WRITE,&id_pct_lake));
    //extraction des id variables
    handle_error(nc_inq_varid(id_pct_lake,"lsmlat",var_pct_lake_id));
    handle_error(nc_inq_varid(id_pct_lake,"lsmlon",var_pct_lake_id + 1));
    handle_error(nc_inq_varid(id_pct_lake,"PCT_LAKE",var_pct_lake_id + 2));
    //extraction des donnees
    for(int i=0;i<2;++i)
    {
        handle_error(nc_get_var_float(id_pct_lake,var_pct_lake_id[i],pct_lake_data[i]));
    }
    handle_error(nc_get_var_double(id_pct_lake,var_pct_lake_id[2],pctLake));
    //on ferme le fichier
    handle_error(nc_close(id_pct_lake));

    //on compte le nombre de cellules considérées comme un lac
    int nombre_cellule_lac = 0;
    for(int i= 0;i<DIVISION_LONGITUDE;++i)
    {
        for(int j=0;j<DIVISION_LATITUDE;++j)
        {
            if(pctLake[i*DIVISION_LATITUDE + j]>0)
            {
                nombre_cellule_lac++;
            }
        }
    }


   // les données servant à compter les jours
    int nombre_jour[nombre_decennie];//nombre de jour d'une décennie
    int premier_jour[nombre_decennie];//numero du premier jour de la décennie
    get_jour_decennie(ADRESSE_NOMBRE_JOUR,annee_depart,nombre_decennie,nombre_jour,premier_jour);


    //on souhaite travailler qu'avec les cellules lac
    //index_lat et index lon nous retourne dans l'ordre les coodonnées de la ième cellule.

    float lat_lac[nombre_cellule_lac];//les valeurs dans -180 ->180
    float lon_lac[nombre_cellule_lac];//les valeurs dans -360 -> 360
    int index_lat[nombre_cellule_lac];//0->359 valeurs possibles
    int index_lon[nombre_cellule_lac];//0->720 valeurs possibles

    //on extrait les données nécessaires;
    int increm =0;
    for(int i=0;i<DIVISION_LONGITUDE;++i)
    {
        for(int j =0;j<DIVISION_LATITUDE;++j)
        {
            if(pctLake[i*DIVISION_LATITUDE+ j]>0)
            {
                lat_lac[increm]=pct_lake_data[0][j];
                lon_lac[increm]=pct_lake_data[1][i];
                index_lat[increm]=j;
                index_lon[increm]=i;
                increm++;
            }
        }
    }
    printf("Fin de l'extraction des données \n");
    printComment("Fin de l'extraction des données");
///// FIN extraction des données //////

//// Creation des fichiers nc4
   for(int i=0;i<nombre_decennie;i++)
    {
        int debut_annee =annee_depart+10*i +1;
        int fin_annee=annee_depart+10*(i+1);
        char nom_fichier[100];
        char fichier_inter[100];
        nom_fichier[0]='\0';
        fichier_inter[0]='\0';
        sprintf(fichier_inter,nom_fichier_final,debut_annee,fin_annee);
        strcat(nom_fichier,repertoire_fichier_final);
        strcat(nom_fichier,fichier_inter);

        create_fichier_nc(nom_fichier,nombre_jour[i],premier_jour[i]);

        printf("Creation du fichier ncdf annee %d - %d\n",debut_annee,fin_annee);
        printComment("creation d'un fichier .nc4");
        printComment(nom_fichier);

    //les id des differentes variables
        int id_fichier_nc =0;
        int id_var_waterTemp = 0;
        int id_var_levlak =0;
        int id_var_lat = 0;
        int id_var_lon = 0;
        int id_var_Levlak = 0;
        int id_var_temps = 0;


    //ouverture du fichier nc4
        handle_error(nc_open(nom_fichier,NC_WRITE,&id_fichier_nc));

        handle_error(nc_inq_varid(id_fichier_nc,"lon",&id_var_lon));
        handle_error(nc_inq_varid(id_fichier_nc,"lat",&id_var_lat));

    //ecriture des variables lat et lon
        handle_error(nc_put_var_float(id_fichier_nc,id_var_lat,pct_lake_data[0]));
        handle_error(nc_put_var_float(id_fichier_nc,id_var_lon,pct_lake_data[1]));

        char adresse[100];
        adresse[0]='\0';
        char repertoire[20];
        sprintf(repertoire,"/T%d0101-%d1231",debut_annee,fin_annee);
        strcat(adresse,adresse_rep);
        strcat(adresse,repertoire);

        put_donnee(id_fichier_nc,adresse,index_lat,index_lon,nombre_cellule_lac,nombre_jour[i],debut_annee,nombre_thread);

        handle_error(nc_close(id_fichier_nc));

    }

   for(int i=0;i<2;++i)
    {
     free(pct_lake_data[i]);
    }
    free(pctLake);


}



