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

#include "fonction_donnee.h"

int numero_ligne = 1;

pthread_mutex_t mutex= PTHREAD_MUTEX_INITIALIZER;

void ncdf_var_info_define(ncdf_var_info* variable,char* nom,nc_type type,int nombre_dimension,int* dimension,void* defaut_valeur,int nombre_titre,char** titre,char** attribut,int chunk_vrai,size_t* chunk, int compression_vrai,int compression_niveau)
{
    variable->nom=nom;
    variable->type=type;
    variable->nombre_dimension=nombre_dimension;
    variable->dimension=dimension;
    variable->defaut_valeur=defaut_valeur;
    variable->nombre_titre=nombre_titre;
    variable->titre=titre;
    variable->attribut=attribut;
    variable->chunk_vrai=chunk_vrai;
    variable->chunk=chunk;
    variable->compression_niveau=compression_niveau;
    variable->compression_vrai=compression_vrai;
}

void ncdf_var_integrer(ncdf_var_info* variable,int id_fichier_ncdf,int* variable_id)
{
    handle_error(nc_def_var(id_fichier_ncdf,variable->nom,variable->type,variable->nombre_dimension,variable->dimension,variable_id));
    handle_error(nc_def_var_fill(id_fichier_ncdf,*variable_id,NC_FILL,variable->defaut_valeur));
    for(int i=0;i<variable->nombre_titre;i++)
    {
        handle_error(nc_put_att(id_fichier_ncdf,*variable_id,variable->titre[i],NC_CHAR,strlen(variable->attribut[i]),variable->attribut[i]));
    }
    if(variable->chunk_vrai)
    {
        handle_error(nc_def_var_chunking(id_fichier_ncdf,*variable_id,NC_CHUNKED,variable->chunk));
    }

    if(variable->compression_vrai)
    {
        handle_error(nc_def_var_deflate(id_fichier_ncdf,*variable_id,0,1,variable->compression_niveau));
    }
}

void handle_error(int status)
{
    if (status != NC_NOERR)
     {
        fprintf(stderr, "%s\n", nc_strerror(status));
        printf("%d\n");
        exit(-1);
    }
}

void printComment(const char* commentaire)
{
    int fd = open(COMMENT_FILE,O_WRONLY|O_CREAT,S_IRWXU|S_IRGRP|S_IROTH);
    if(fd < 0)
    {
        printf("Erreur lors de l'ecriture de %s dans le fichier commentaire\n",commentaire);
        exit(EXIT_FAILURE);
    }
    char c[100];
    lseek(fd,0,SEEK_END);
    sprintf(c,"%d- ",numero_ligne++);
    write(fd,c,strlen(c));
    write(fd,commentaire,strlen(commentaire));
    write(fd,"\n",1);
    if(close(fd)<0)
    {
        printf("Erreur lors de la fermeture du fichier commentaire\n");
        exit(EXIT_FAILURE);
    }
}

void get_jour_decennie(const char* adresse,const int annee,const int nb_decennie,int* nombre_jour,int* numero_premier_jour)
{
    printComment("Extraction des nombres de jours et des numeros de jours des decennies");
    // Ouverture du fichier

    int fd = open(adresse,O_RDONLY);
    if(fd<0)
    {
        printComment("Erreur lors de l'ouverture du fichier jour_decennie");
        exit(EXIT_FAILURE);
    }
    off_t taille =lseek(fd,0,SEEK_END);
    lseek(fd,0,SEEK_SET);
    //Lecture du fichier et fermeture
    char* donnee=calloc(taille+1,1);
    read(fd,donnee,taille);
    if(close(fd))
    {
        printComment("Erreur lors de la fermeture du fichier jour_decennie");
    }

    // recherche des donnees souhaitées
    char* c1=donnee;
    char* c2=NULL;
    int vrai_boucle =1;

    while(vrai_boucle)
    {

        if(annee == strtol(c1,&c2,10)-1)
        {
            for(int i=0;i<nb_decennie;i++)
            {
                c1=c2;
                nombre_jour[i]=strtol(c1,&c2,10);//le nombre de jour dans la decennie
                c1=c2;
                numero_premier_jour[i]=strtol(c1,&c2,10);//le numero du premier jour de la decennie
                c1=c2;
                strtol(c1,&c2,10);//le numero du dernier jour de la decennie
                c1=c2;
                strtol(c1,&c2,10);//la decennie suivante
            }
            vrai_boucle=0;
        }
        else
        {
            for(int i=0;i<3;i++)
            {
                c1=c2;
                strtol(c1,&c2,10);//on passe les 3 valeurs
            }
            c1=c2;
        }
    }
    free(donnee);
    printComment("Fin de l'extraction");
}

int create_fichier_nc(char* nom,int nombre_jour,int numero_jour)
{
    // les id du fichiers .nc4
    int id_fichier =0;
    int dim_longitude =0;
    int dim_latitude =0;
    int dim_nb_levlak=0;
    int dim_temps =0;

    ncdf_var_info variable[6];//0=lon, 1=lat, 2=nb_levlak, 3=levlak, 4=time, 5=watertemps

    // les valeurs par defaut
    float default_valueF = 1e20f;
    int default_valueI = 0;
    int niveau_prof = 13;
    int id_var_LevLak = 0;
    int compression_niveau=5;

    //creer le fichier
    handle_error(nc_create(nom,NC_CLOBBER|NC_NETCDF4,&id_fichier));

    //definie les dimensions
    handle_error(nc_def_dim(id_fichier,"lon",DIVISION_LONGITUDE,&dim_longitude));
    handle_error(nc_def_dim(id_fichier,"lat",DIVISION_LATITUDE,&dim_latitude));
    handle_error(nc_def_dim(id_fichier,"nb_Levlak",niveau_prof,&dim_nb_levlak));
    handle_error(nc_def_dim(id_fichier,"time",nombre_jour,&dim_temps));

    //definie la variable longitude
    int id_longitude=0;
    char* nom_lon="lon";
    nc_type type_lon=NC_DOUBLE;
    int nombre_dimension_lon=1;
    int dimension_lon[]={dim_longitude};
    double defaut_valeur_lon = 1e20;
    int nombre_titre_lon=4;
    char* titre_lon[]={"standard_name","long_name","units","axis"};
    char* attribut_lon[]={"longitude","longitude","degrees_east","X"};
    int chunk_vrai_lon=1;
    size_t chunk_lon[]={DIVISION_LONGITUDE};
    int compression_vrai_lon=1;
    int compression_niveau_lon=compression_niveau;
    ncdf_var_info_define(&variable[0],nom_lon,type_lon,nombre_dimension_lon,dimension_lon,&defaut_valeur_lon,nombre_titre_lon,titre_lon,attribut_lon,chunk_vrai_lon,chunk_lon,compression_vrai_lon,compression_niveau_lon);
    ncdf_var_integrer(&variable[0],id_fichier,&id_longitude);

    //definie la variable latitude
    int id_latitude=0;
    char* nom_lat="lat";
    nc_type type_lat=NC_DOUBLE;
    int nombre_dimension_lat=1;
    int dimension_lat[]={dim_latitude};
    double defaut_valeur_lat = 1e20;
    int nombre_titre_lat=4;
    char* titre_lat[]={"standard_name","long_name","units","axis"};
    char* attribut_lat[]={"latitude","latitude","degrees_north","Y"};
    int chunk_vrai_lat=1;
    size_t chunk_lat[]={DIVISION_LATITUDE};
    int compression_vrai_lat=1;
    int compression_niveau_lat=compression_niveau;
    ncdf_var_info_define(&variable[1],nom_lat,type_lat,nombre_dimension_lat,dimension_lat,&defaut_valeur_lat,nombre_titre_lat,titre_lat,attribut_lat,chunk_vrai_lat,chunk_lat,compression_vrai_lat,compression_niveau_lat);
    ncdf_var_integrer(&variable[1],id_fichier,&id_latitude);

    //define nb_levlak
    int id_nb_levlak=0;
    char* nom_nblev="nb_levlak";
    nc_type type_nblev=NC_FLOAT;
    int nombre_dimension_nblev=1;
    int dimension_nblev[]={dim_nb_levlak};
    float defaut_valeur_nblev = 1e20f;
    int nombre_titre_nblev=0;
    char** titre_nblev;
    char** attribut_nblev;
    int chunk_vrai_nblev=1;
    size_t chunk_nblev[]={niveau_prof};
    int compression_vrai_nblev=1;
    int compression_niveau_nblev=compression_niveau;
    ncdf_var_info_define(&variable[2],nom_nblev,type_nblev,nombre_dimension_nblev,dimension_nblev,&defaut_valeur_nblev,nombre_titre_nblev,titre_nblev,attribut_nblev,chunk_vrai_nblev,chunk_nblev,compression_vrai_nblev,compression_niveau_nblev);
    ncdf_var_integrer(&variable[2],id_fichier,&id_nb_levlak);

    int id_levlak=0;
    char* nom_lev="levlak";
    nc_type type_lev=NC_FLOAT;
    int nombre_dimension_lev=3;
    int dimension_lev[]={dim_nb_levlak,dim_latitude,dim_longitude};
    float defaut_valeur_lev = 1e20f;
    int nombre_titre_lev=3;
    char* titre_lev[]={"long_name","units","axis"};
    char* attribut_lev[]={"vertical water layer depth","m","Z"};
    int chunk_vrai_lev=1;
    size_t chunk_lev[]={1,DIVISION_LATITUDE,DIVISION_LONGITUDE};
    int compression_vrai_lev=1;
    int compression_niveau_lev=compression_niveau;
    ncdf_var_info_define(&variable[3],nom_lev,type_lev,nombre_dimension_lev,dimension_lev,&defaut_valeur_lev,nombre_titre_lev,titre_lev,attribut_lev,chunk_vrai_lev,chunk_lev,compression_vrai_lev,compression_niveau_lev);
    ncdf_var_integrer(&variable[3],id_fichier,&id_levlak);

    //define temps
    int id_time=0;
    char* nom_t="time";
    nc_type type_t=NC_DOUBLE;
    int nombre_dimension_t=1;
    int dimension_t[]={dim_temps};
    double defaut_valeur_t = 1e20;
    int nombre_titre_t=4;
    char* titre_t[]={"standard_name","long_name","units","calendar"};
    char* attribut_t[]={"time","time","days since 1661-1-1 00:00:00","proleptic_gregorian"};
    int chunk_vrai_t=1;
    size_t chunk_t[]={nombre_jour};
    int compression_vrai_t=1;
    int compression_niveau_t=compression_niveau;
    ncdf_var_info_define(&variable[4],nom_t,type_t,nombre_dimension_t,dimension_t,&defaut_valeur_t,nombre_titre_t,titre_t,attribut_t,chunk_vrai_t,chunk_t,compression_vrai_t,compression_niveau_t);
    ncdf_var_integrer(&variable[4],id_fichier,&id_time);

    //define watertemp
    int id_watertemp=0;
    char* nom_wt="watertemp";
    nc_type type_wt=NC_FLOAT;
    int nombre_dimension_wt=4;
    int dimension_wt[]={dim_temps,dim_nb_levlak,dim_latitude,dim_longitude};
    float defaut_valeur_wt = 1e20f;
    int nombre_titre_wt=3;
    char* titre_wt[]={"standard_name","long_name","missing_value"};
    char* attribut_wt[]={"lake temperature","K","1.e+20f"};
    int chunk_vrai_wt=1;
    size_t chunk_wt[]={1,niveau_prof,DIVISION_LATITUDE,DIVISION_LONGITUDE};
    int compression_vrai_wt=1;
    int compression_niveau_wt=compression_niveau;
    ncdf_var_info_define(&variable[5],nom_wt,type_wt,nombre_dimension_wt,dimension_wt,&defaut_valeur_wt,nombre_titre_wt,titre_wt,attribut_wt,chunk_vrai_wt,chunk_wt,compression_vrai_wt,compression_niveau_wt);
    ncdf_var_integrer(&variable[5],id_fichier,&id_watertemp);


    //definir les attributs

    int id_var[]={NC_GLOBAL};
    int id_var_taille=1;

    int taille_titre[]={6};
    char *titre[]={"source","comment","institution","contact","feedback","reference"};

    char *attribut[]={"SIMSTRAT lake model","Data prepared for ISIMIP2b","University of Geneva",
                       "Marjorie Perroud (marjorie.perroud@unige.ch)","Please contact us in case you notice anything suspicious in the data",
                       "When using the outputs, please cite: Goudsmit et al.(2002), Application of k‐ε turbulence models to enclosed basins: The role of internal seiches, J. Geophys. Res., 107(C12), 3230, doi:10.1029/2001JC000954; see also https://github.com/Eawag-AppliedSystemAnalysis/Simstrat"};

    int iterateur =0;
    for(int i=0;i<id_var_taille;++i)
    {
        for(int j=0;j<taille_titre[i];++j)
        {

            handle_error(nc_put_att(id_fichier,id_var[i],titre[iterateur],NC_CHAR,strlen(attribut[iterateur]),attribut[iterateur]));
            iterateur++;
        }
    }

    handle_error(nc_enddef(id_fichier));

    //on définit les valeurs qui seront mis dans la variable levlak et time
    float niv[13];
    double jour[nombre_jour];
    for(int i=0;i<13;i++)
    {
        niv[i]=i;
    }
    for(int i=1;i<nombre_jour+1;i++)
    {
        jour[i-1]=i + numero_jour;
    }

     handle_error(nc_put_var_float(id_fichier,id_nb_levlak,niv));
     handle_error(nc_put_var_double(id_fichier,id_time,jour));

    return id_fichier;

}



char* get_donnne(const char* adresse,int* taille)
{

    int fd = open(adresse,O_RDONLY);
    off_t taille_file = lseek(fd,0,SEEK_END);
    lseek(fd,0,SEEK_SET);
    char* donnee = calloc(taille_file+2,sizeof(char));
    ssize_t taille_utilise=0;
    ssize_t result=0;
    while(taille_utilise<taille_file - 1)
    {
        result=read(fd,donnee,sizeof(char)*(taille_file-taille_utilise));
        if(result<=0)
        {
            exit(EXIT_FAILURE);
        }
        taille_utilise += result;
    }

    close(fd);
    donnee[taille_file -1]='\0';
    *taille = taille_file;
    return donnee;
}

void create_tableau_donnee(const char* adresse,const int nombre_jour,tableau_donnee* retour_tableau)
{
    int nombre_ligne=1;
    int taille=0;
    char* donnee = get_donnne(adresse,&taille);// obtenir les donnees dans un tableau de caractère
    for(int i = 0; i<taille;++i)// on cherche le nombre de ligne afin de connaitre la taille des futurs tableau
    {
        if(donnee[i]=='\n')
        {
            ++nombre_ligne;
        }
    }
    float* temperature = calloc(nombre_ligne,sizeof(float));//les tableaux qui contiendront les donnees
    float* profondeur = calloc(nombre_ligne,sizeof(float));

    char* c1=donnee;
    char* c2;
    for(int i=0;i<nombre_ligne;++i)//convertie les char en float
    {
        strtof(c1,&c2);
        c1=c2;
        temperature[i]=strtof(c1,&c2);
        c1=c2;
        profondeur[i]=strtof(c1,&c2);
        if(profondeur[i]==1e20f || temperature[i]==1e20f)// onvérifie s'il existe une valeur Nan
        {
            printf("Erreur, une valeur NAN detecté\n");
            char erreur[100];
            erreur[0]='\0';
            sprintf(erreur,"Erreur, une valeur NAN à la ligne %d du fichier %s",i,adresse);
            printComment(erreur);
        }
        c1 = c2;
    }

    int boucle = 1;
    int nombre_profondeur = 1;
    int iter_prof=0;
    int tableau_nombre_prof_par_jour[3653];// Nous mettons le nombre de ligne que nous trouvons par jour en cas où il manquerait une ligne.
    tableau_nombre_prof_par_jour[3652]=-1;
    int test_profondeur=1;

    for(int i=0;i<nombre_ligne;i++)
    {
        if(iter_prof == profondeur[i])
        {
            iter_prof++;
        }
        else if(iter_prof != profondeur[i] && profondeur[i] == 0.0f)
        {
            if(nombre_profondeur<profondeur[i-1])
            {
                nombre_profondeur=profondeur[i-1];
            }
            iter_prof=1;
        }
        else
        {
            printf("Erreur, il manque une ligne dans un fichier dat3 \n");
            char erreur [100];
            sprintf(erreur,"Erreur, il manque une ligne dans le fichier %s à la ligne %d \n",adresse,i);
        }
    }

    //nombre_profondeur = tableau_nombre_prof_par_jour[0];
    int numero_prof_choisi=0;

    float choix_profondeur[13];//selon la profondeur, nous choisissons les niveaux
    int index[13];
    choix_profondeur[12]=nombre_profondeur-1;
    index[12]=nombre_profondeur-1;

    if(nombre_profondeur<12)
    {
        for(int i=0;i<nombre_profondeur;++i)
        {
            choix_profondeur[i]=i;
            index[i]=i;
        }
        for(int i=nombre_profondeur;i<13;++i)
        {
            choix_profondeur[i]=1e20;
            index[i]=-1;
        }

    }
    else if(nombre_profondeur<15)
    {
        for(int i=0;i<11;++i)
        {
            choix_profondeur[i]=i;
            index[i]=i;
        }

        choix_profondeur[11]= 12.0f;
        index[11]=12;

    }
    else if(nombre_profondeur<20)
    {
        for(int i=0;i<9;i++)
        {
            choix_profondeur[i]=i;
            index[i]=i;
        }

        choix_profondeur[9]= 10.0f;
        choix_profondeur[10]=12.0f;
        choix_profondeur[11]=15.0f;
        index[9]=10;
        index[10]=12;
        index[11]=15;

    }
    else if(nombre_profondeur<30)
    {
        for(int i=0;i<7;++i)
        {
            choix_profondeur[i]=i;
            index[i]=i;
        }
        choix_profondeur[7]=8.0f;
        choix_profondeur[8]=10.0f;
        choix_profondeur[9]=12.0f;
        choix_profondeur[10]=15.0f;
        choix_profondeur[11]=20.0f;

        index[7]=8;
        index[8]=10;
        index[9]=12;
        index[10]=15;
        index[11]=20;

    }
    else if(nombre_profondeur<40)
    {
        for(int i=0;i<6;++i)
        {
            choix_profondeur[i]=i;
            index[i]=i;
        }
        choix_profondeur[6]=8.0f;
        choix_profondeur[7]=10.0f;
        choix_profondeur[8]=12.0f;
        choix_profondeur[9]=15.0f;
        choix_profondeur[10]=20.0f;
        choix_profondeur[11]=30.0f;

        index[6]=8;
        index[7]=10;
        index[8]=12;
        index[9]=15;
        index[10]=20;
        index[11]=30;

    }
    else if(nombre_profondeur<50)
    {
        for(int i=0;i<6;++i)
        {
            choix_profondeur[i]=i;
            index[i]=i;
        }
        choix_profondeur[6]=8.0f;
        choix_profondeur[7]=10.0f;
        choix_profondeur[8]=12.0f;
        choix_profondeur[9]=15.0f;
        choix_profondeur[10]=20.0f;
        choix_profondeur[11]=30.0f;

        index[6]=8;
        index[7]=10;
        index[8]=12;
        index[9]=15;
        index[10]=20;
        index[11]=30;

    }
    else if(nombre_profondeur<100)
    {
        for(int i=0;i<6;++i)
        {
            choix_profondeur[i]=i;
            index[i]=i;
        }
        choix_profondeur[6]=8.0f;
        choix_profondeur[7]=10.0f;
        choix_profondeur[8]=15.0f;
        choix_profondeur[9]=20.0f;
        choix_profondeur[10]=30.0f;
        choix_profondeur[11]=50.0f;

        index[6]=8;
        index[7]=10;
        index[8]=15;
        index[9]=20;
        index[10]=30;
        index[11]=50;

    }
    else if(nombre_profondeur<500)
    {
        choix_profondeur[0]=0.0f;
        choix_profondeur[1]=1.0f;
        choix_profondeur[2]=2.0f;
        choix_profondeur[3]=3.0f;
        choix_profondeur[4]=5.0f;
        choix_profondeur[5]=8.0f;
        choix_profondeur[6]=10.0f;
        choix_profondeur[7]=15.0f;
        choix_profondeur[8]=20.0f;
        choix_profondeur[9]=30.0f;
        choix_profondeur[10]=50.0f;
        choix_profondeur[11]=100.0f;

        index[0]=0;
        index[1]=1;
        index[2]=2;
        index[3]=3;
        index[4]=5;
        index[5]=8;
        index[6]=10;
        index[7]=15;
        index[8]=20;
        index[9]=30;
        index[10]=50;
        index[11]=100;

    }
    else
    {
        choix_profondeur[0]=0.0f;
        choix_profondeur[1]=2.0f;
        choix_profondeur[2]=4.0f;
        choix_profondeur[3]=6.0f;
        choix_profondeur[4]=8.0f;
        choix_profondeur[5]=10.0f;
        choix_profondeur[6]=15.0f;
        choix_profondeur[7]=20.0f;
        choix_profondeur[8]=30.0f;
        choix_profondeur[9]=50.0f;
        choix_profondeur[10]=100.0f;
        choix_profondeur[11]=500.0f;

        index[0]=0;
        index[1]=2;
        index[2]=4;
        index[3]=6;
        index[4]=8;
        index[5]=10;
        index[6]=15;
        index[7]=20;
        index[8]=30;
        index[9]=50;
        index[10]=100;
        index[11]=500;
    }

    for(int i=0;i<13;++i)
    {
        retour_tableau->choix_niveau[i]=choix_profondeur[i];
    }

    float* tableau_temperature=calloc(nombre_jour*13,sizeof(float));
    for(int i=0;i<13;i++)//choisir les températures
    {
        for(int j=0;j<nombre_jour;++j)
        {
            if(index[i]<0)
            {
                tableau_temperature[i*nombre_jour + j]=1e20f;
            }
            else
            {
                tableau_temperature[i*nombre_jour + j]= temperature[nombre_profondeur*(j+1) - index[i] - 1];
               // printf(" jour : %d. profondeur: %f. temp: %f \n",j,choix_profondeur[i],temperature[nombre_profondeur*(j+1) - index[i] - 1]);
            }


        }
    }

    retour_tableau->nombre_jour=nombre_jour;
    retour_tableau->temperature=tableau_temperature;

    free(temperature);
    free(profondeur);
    free(donnee);


}


void affiche_tableau(const tableau_donnee* tableau)
{
    printf("Tableau de donnee: \n");
    printf("nombre de jour: %d \n",tableau->nombre_jour);
    printf("niveau prof. : ");
    for(int i=0;i<13;++i)
    {
      printf(" %f ",tableau->choix_niveau[i]);
    }
    printf("\n");

    for(int k = 0;k<100/*tableau->nombre_jour*/;++k)
    {
        printf("Jour %d \n",k);
       for(int i=0;i<13;++i)
        {
            printf("prof. %f: temp. %f \n ",tableau->choix_niveau[i],tableau->temperature[i*tableau->nombre_jour + k]);
        }
        printf("\n");
    }
}

void free_tableau(tableau_donnee* tableau)
{

    free(tableau->temperature);
    free(tableau);
}



void jour_decennie(const char* adresse)
{
    int fd = open(adresse,O_RDONLY);
    off_t taille =lseek(fd,0,SEEK_END)-1;
    lseek(fd,0,SEEK_SET);

    char* texte = calloc(taille ,sizeof(char));
    int char_lu=0;
    while(char_lu<taille)
    {
        char_lu += read(fd,texte + char_lu,taille - char_lu);
    }

    int nombre_ligne =-1;
    for(int i =0;i<taille;++i)
    {
        if(texte[i]=='\n')
        {
            ++nombre_ligne;
        }
    }
    close(fd);
    printf("ligne nombre %d \n",nombre_ligne);
    printf("TEST0 \n");
    int* tableau_nombre = calloc(nombre_ligne*4,sizeof(int));
    printf("TEST0 \n");
    char* current=texte + 30;
    printf(current);
    char* prochain;
    printf("TEST0 \n");
    for(int i=0;i<nombre_ligne;++i)
    {
        printf("test ligne %d \n",i);
        for(int k =0;k<4;++k)
        {
        //printf("test %d \n",4*i + k);
           tableau_nombre[4*i + k] = strtol(current,&prochain,0);
           tableau_nombre[4*i + k] = atoi(prochain);

           current = prochain;
        }
    }

    for(int i=0;i<nombre_ligne;++i)
    {
        printf("%d %d %d %d \n",tableau_nombre[i*4],tableau_nombre[i*4 + 1],tableau_nombre[i*4 + 2],tableau_nombre[i*4 + 3]);
    }
    free(texte);
    free(tableau_nombre);
}




void put_donnee(int id_nc_file, char* repertoire,int* index_lat,int* index_lon, int index_taille,int nombre_jour,int annee,int nombre_thread)
{
    int niveau = 13;

    float* donnees=calloc(nombre_jour*niveau*index_taille + 1,sizeof(float));// contiendra toutes les températures à écrire pour la décennie
   float* profondeur=calloc((niveau)*360*720,sizeof(float));//contiendra toutes les profondeurs des cellules lac à écrire

    for(int i=0;i<(niveau)*360*720;i++)
    {
        profondeur[i]=1e20f;
    }
    for(int i=0;i<nombre_jour*niveau*index_taille + 1;i++)
    {
        donnees[i]=1e20f;
    }

    size_t un= 1;
    int bool1=1;
    int i=0;
    int j=0;

    pthread_t thread[nombre_thread];//tableau des threads
    int thread_utilisee=0;//le thread courant

    arg_remplir_tabl arguments[nombre_thread];//les arguments pour l'extraction des données
    tableau_donnee* donnees_provisoir[nombre_thread];//les données extraites

    int index_thread[nombre_thread];//le tableau qui indiquera si un thread est diponible
    for(int i=0;i<nombre_thread;i++)
    {
        index_thread[i]=0;
        donnees_provisoir[i] = calloc(1,sizeof(tableau_donnee));
    }

    int pile[nombre_thread];//la pile qui contiendra les index des threads libres
    int pile_index=-1;

    printf("Extraction des données dat3 \n");
    printComment("Extraction des données dat3");
    // cree des threads pour extraire les données des fichiers .dat3 et les mettre dans donnees.
    for(int iter_index=0;iter_index<index_taille;iter_index++)
    {
        //on choisie le fichier dat3 qui sera ouvert et on vérifie s'il est ouvert
        char adresse[100];
        char nom_fichier[100];
        sprintf(nom_fichier,"/Tdec_%d_%d.dat3",annee,iter_index+1);
        adresse[0]='\0';
        strcat(adresse,repertoire);
        strcat(adresse,nom_fichier);
        int fd = open(adresse,O_RDONLY);

        if(fd<0)
        {
            close(fd);
            char* erreur=malloc(100);
            erreur[0]='\0';
            //erreur[0]='/0';
            sprintf(erreur,"Erreur, le fichier Tdec_%d_%d.dat3 n'existe pas",annee,iter_index+1);
            printComment(erreur);
            free(erreur);
            printf("\nErreur, fichier manquant");
            continue;
        }
        close(fd);

        //on utise une pile pour vérifier les threads disponibles. si index_pil = -1 -> pile vide
        if(pile_index==-1)//quand on a lancé tous les threads disponibles, on vérifie les disponibilités
        {
            while(pile_index == -1)//on reste dans la boucle tant qu'on ne trouve pas de thread disponoble
            {
                pthread_mutex_lock(&mutex);
                for(int iter=0;iter<nombre_thread;iter++)
                {
                    if(index_thread[iter]==0)
                    {
                        pile[++pile_index]=iter;//on ajoute à la pile, l'index du thread disponible et incrémente avant
                    }
                }
                pthread_mutex_unlock(&mutex);
                if(pile_index==-1)
                {
                    usleep(100);//si aucun thread n'est disponible, on attends 10 microseconds
                }
            }
        }

        thread_utilisee = pile[pile_index--];// on récupère l'index d'un thread disponible de la pile et décremente l'index de la pile
        create_tableau_donnee(adresse,nombre_jour,donnees_provisoir[thread_utilisee]);//on récupère les données du fichiers dat3

        //on définit les arguments
        arguments[thread_utilisee].tableau_a_remplir=donnees;
        arguments[thread_utilisee].tableau=donnees_provisoir[thread_utilisee]->temperature;
        arguments[thread_utilisee].debut_tableau_remplir = iter_index*13*nombre_jour;
        arguments[thread_utilisee].debut_tableau=0;
        arguments[thread_utilisee].nombre_remplir= 13*nombre_jour;
        arguments[thread_utilisee].index_thread= thread_utilisee;
        arguments[thread_utilisee].thread_tableau=index_thread;

        //on lance le thread disponible
        index_thread[thread_utilisee]=1;
        pthread_create(&thread[thread_utilisee],NULL,thread_remplir_tab,&arguments[thread_utilisee]);
        pthread_detach(thread[thread_utilisee]);

        //on récupère les données sur les profondeurs
        for(int prof=0;prof<13;prof++)
        {
            profondeur[ index_lat[iter_index] + index_lon[iter_index]*360 + prof*360*720]=donnees_provisoir[thread_utilisee]->choix_niveau[prof];
        }


        printf("\rProgression extraction %d sur %d",iter_index,index_taille);
        fflush(stdout);
    }

    while(pile_index!=-1)// on attend que les derniers threads se terminent
    {
        usleep(10);
        pile_index=-1;
        for(int iter=0;iter<nombre_thread;iter++)
        {
            if(index_thread[iter]==1)
            {
                pile_index=0;
            }
        }
    }

    for(i=0;i<nombre_thread;i++)// on libère les mémoires qui ne sont plus nécessaires
    {
        free_tableau(donnees_provisoir[i]);
    }
        printComment("fin extraction des donnees");
        printf("\nfin extraction donnee \n");
        printf("debut ecriture donnee \n");
        printComment("debut ecriture des donnees");

    //on extrait les id des variables
    int id_levlak=0;
    int id_waterTemp=0;
    handle_error(nc_inq_varid(id_nc_file,"watertemp",&id_waterTemp));
    handle_error(nc_inq_varid(id_nc_file,"levlak",&id_levlak));

    // la taille des données qui seront écris à la fois
    int taille_chunk=13*360*720;


    // contient le tableau avec les données pour un jour spécifique
    float* donnee_transmettre[nombre_thread];
    for(int iter=0;iter<nombre_thread;iter++)
    {

        donnee_transmettre[iter]=calloc(taille_chunk,sizeof(float));
    }

    arg_put argument_put[nombre_thread];//le tableau des arguments pour l'écriture des données

   for(int jour=0;jour<nombre_jour;jour++)
    {
        if(pile_index==-1)//quand on a lancé tous les threads disponibles, on vérifie les disponibilités
        {
            while(pile_index == -1)//on reste dans la boucle tant qu'on ne trouve pas de thread disponoble
            {
                pthread_mutex_lock(&mutex);
                for(int iter=0;iter<nombre_thread;iter++)
                {
                    if(index_thread[iter]==0)
                    {
                        pile[++pile_index]=iter;//on ajoute à la pile, l'index du thread disponible et incrémente avant
                    }
                }
                pthread_mutex_unlock(&mutex);
                if(pile_index==-1)
                {
                    usleep(100);//si aucun thread n'est disponible, on attends 10 microseconds
                }
            }
        }

        thread_utilisee = pile[pile_index--];//dépile l'index d'un thread disponible

        //mettre aux valeurs par défaut le tableau à écrire
        for(int p=0;p<taille_chunk;p++)
        {
            donnee_transmettre[thread_utilisee][p]=1e20f;
        }

        for(int iter_index=0;iter_index<index_taille;iter_index++)
        {
            for(int iter_prof=0;iter_prof<13;iter_prof++)
            {
                //donnee_transmettre [thread_utilisee][iter_prof + index_lat[iter_index]*13 + index_lon[iter_index]*13*360] = donnees[jour + iter_prof*nombre_jour + iter_index*13*nombre_jour];
                donnee_transmettre [thread_utilisee][index_lat[iter_index] + index_lon[iter_index]*360 + iter_prof*720*360]=donnees[jour + iter_prof*nombre_jour + iter_index*nombre_jour*13 ];
            //printf("1. jour %d , valeurs %f profondeurs %d \n",arg->jour,arg->donnee[arg->jour + j*arg->nombre_jour + i*13*arg->nombre_jour],j);
            //printf("1.45 jour %d , valeurs %f profondeurs %d \n",arg->jour,donnees_transmettre[j + arg->lat_i[i]*13 + arg->lon_i[i]*13*360],j);

            }
        }


        //définir les arguments
        argument_put[thread_utilisee].id_file=id_nc_file;
        argument_put[thread_utilisee].id_var=id_waterTemp;
        argument_put[thread_utilisee].jour=jour;
        argument_put[thread_utilisee].lat_i=index_lat;
        argument_put[thread_utilisee].lon_i=index_lat;
        argument_put[thread_utilisee].nombre_jour=nombre_jour ;
        argument_put[thread_utilisee].taille_index=index_taille;
        argument_put[thread_utilisee].donnee=donnees;
        argument_put[thread_utilisee].donnees_transmettre=donnee_transmettre[thread_utilisee];
        argument_put[thread_utilisee].index_thread=thread_utilisee;
        argument_put[thread_utilisee].thread_tableau=index_thread;

        //le thread est lancé
        index_thread[thread_utilisee]=1;
        pthread_create(&thread[thread_utilisee],NULL,nc_put,&argument_put[thread_utilisee]);
        pthread_detach(thread[thread_utilisee]);




       printf("\rProgression ecriture %d sur %d",jour,nombre_jour);
       fflush(stdout);

    }
    printf("\n");
    pile_index=0;
    while(pile_index!=-1)// on attend que les derniers threads se terminent
    {
        usleep(10);
        pile_index=-1;
        for(int iter=0;iter<nombre_thread;iter++)
        {
            if(index_thread[iter]==1)
            {
                pile_index=0;
            }
        }
    }

    printComment("Fin ecriture des donnees");
    printf("Fin ecriture des donnees \n");
    for(int iter=0;iter<nombre_thread;iter++)
    {
        free(donnee_transmettre[iter]);//on libère la mémoire
    }


    //on écrit les profondeurs
    handle_error(nc_put_var_float(id_nc_file,id_levlak,profondeur));


    free(donnees);
    free(profondeur);
}




void nc_put(void* arguments)
{
   arg_put* arg = (arg_put*)arguments;

    size_t start[]={arg->jour,0,0,0};
    size_t compte[]={1,13,360,720};

    handle_error(nc_put_vara_float(arg->id_file,arg->id_var,start,compte,arg->donnees_transmettre));

    arg->thread_tableau[arg->index_thread]=0;
    pthread_exit(NULL);


}





void thread_remplir_tab(void* arg)
{

    arg_remplir_tabl* arguments = (arg_remplir_tabl*) arg;

    for(int i=0;i<arguments->nombre_remplir;i++)
    {

        arguments->tableau_a_remplir[arguments->debut_tableau_remplir + i]=arguments->tableau[arguments->debut_tableau + i];
    }
    arguments->thread_tableau[arguments->index_thread]=0;
    pthread_exit(NULL);
}
