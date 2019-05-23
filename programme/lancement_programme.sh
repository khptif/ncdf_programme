#! /bin/bash

ADRESSE=/home/fatih/Documents/projet_netcdf/fichier_c/run5/t
ANNEE=1913
NOMBRE_DECENNIE=1
THREAD=8
ADRESSE_RETOUR=/home/fatih/Documents/projet_netcdf/fichier_c/testRepertoire/
NOM=test_%d_%d.nc4

LIB="adresse où se trouve les librairies netcdf"
LIBMPI="adresse où se trouve le répértoire openmpi/lib "

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${LIB}
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${LIBMPI}

./ncdf ${ADRESSE} ${ANNEE} ${NOMBRE_DECENNIE} ${THREAD} ${ADRESSE_RETOUR} ${NOM}
