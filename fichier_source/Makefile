#le compilateur du programme ncdf
#s'il manque des fichiers *.h, il faut préciser l'adresse où se trouve le répertoire des fichiers *.h de netcdf  dans la variable HEADER_NETCDF.
#s'il manque le fichier mpi.h, il faut réinstaller netcdf avec les options "parallèles". Si c'est fait et ne fonctionne pas, il faut préciser 
#l'adresse */openmpi/include dans la variable HEADER_OPENMPI.
#s'il manque des librairies, il faut préciser l'adresse de la librairie dans LIBRARY_NETCDF. De même pour openmpi dans LIBRARY_OPENMPI si ça ne suffisait pas.

LIBRARY_NETCDF = /home/fatih/netcdf/lib
HEADER_NETCDF = /home/fatih/netcdf/include

LIBRARY_OPENMPI = /home/fatih/openmpi/lib
HEADER_OPENMPI = /home/fatih/openmpi/include


All : ncdf.c fonction_donnee.o
	gcc  -I${HEADER_NETCDF} -I${HEADER_OPENMPI} ncdf.c fonction_donnee.o -o ../programme/ncdf -L${LIBRARY_NETCDF} -L${LIBRARY_OPENMPI} -lnetcdf -lpthread 
	

fonction_donnee.o: fonction_donnee.c fonction_donnee.h
	gcc   -I${HEADER_NETCDF} -I${HEADER_OPENMPI}  -c fonction_donnee.c -o fonction_donnee.o

clear:
	rm *.o
