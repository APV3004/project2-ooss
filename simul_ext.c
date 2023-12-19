#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include "headers.h"

#define LONGITUD_COMANDO 100

void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps);
int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2);
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup);
int BuscaFich(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, 
              char *nombre);
void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos);
int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, 
              char *nombreantiguo, char *nombrenuevo);
int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, 
             EXT_DATOS *memdatos, char *nombre);
int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos,
           EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock,
           char *nombre,  FILE *fich);
int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos,
           EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock,
           EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino,  FILE *fich);
void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich);
void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich);
void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich);
void GrabarDatos(EXT_DATOS *memdatos, FILE *fich);

int main()
{
	 char *comando[LONGITUD_COMANDO];
	 char *orden[LONGITUD_COMANDO];
	 char *argumento1[LONGITUD_COMANDO];
	 char *argumento2[LONGITUD_COMANDO];
	 
	 int i,j;
	 unsigned long int m;
     EXT_SIMPLE_SUPERBLOCK ext_superblock;
     EXT_BYTE_MAPS ext_bytemaps;
     EXT_BLQ_INODOS ext_blq_inodos;
     EXT_ENTRADA_DIR directorio[MAX_FICHEROS];
     EXT_DATOS memdatos[MAX_BLOQUES_DATOS];
     EXT_DATOS datosfich[MAX_BLOQUES_PARTICION];
     int entradadir;
     int grabardatos;
     FILE *fent;
     
     // Lectura del fichero completo de una sola vez
     
     //code in here...
     
     fent = fopen("particion.bin","r+b");
     fread(&datosfich, SIZE_BLOQUE, MAX_BLOQUES_PARTICION, fent);    
     
     
     memcpy(&ext_superblock,(EXT_SIMPLE_SUPERBLOCK *)&datosfich[0], SIZE_BLOQUE);
     memcpy(&directorio,(EXT_ENTRADA_DIR *)&datosfich[3], SIZE_BLOQUE);
     memcpy(&ext_bytemaps,(EXT_BLQ_INODOS *)&datosfich[1], SIZE_BLOQUE);
     memcpy(&ext_blq_inodos,(EXT_BLQ_INODOS *)&datosfich[2], SIZE_BLOQUE);
     memcpy(&memdatos,(EXT_DATOS *)&datosfich[4],MAX_BLOQUES_DATOS*SIZE_BLOQUE);
     
     // Buce de tratamiento de comandos
     for (;;){
		 do {
		 printf (">> ");
		 fflush(stdin);
		 fgets(comando, LONGITUD_COMANDO, stdin);
		 } while (ComprobarComando(comando,orden,argumento1,argumento2) !=0);
	     if (strcmp(orden,"dir")==0) {
            Directorio(&directorio,&ext_blq_inodos);
            continue;
            }
         
         //code in here...

         // Escritura de metadatos en comandos rename, remove, copy     
         Grabarinodosydirectorio(&directorio,&ext_blq_inodos,fent);
         GrabarByteMaps(&ext_bytemaps,fent);
         GrabarSuperBloque(&ext_superblock,fent);
         if (grabardatos)
           GrabarDatos(&memdatos,fent);
         grabardatos = 0;
         //Si el comando es salir se habrán escrito todos los metadatos
         //faltan los datos y cerrar
         if (strcmp(orden,"salir")==0){
            GrabarDatos(&memdatos,fent);
            fclose(fent);
            return 0;
         }
     }
}

void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps) {
   // This is to print block bytemap
    printf("Block bytemap:\n");
    for (int i = 0; i < MAX_BLOQUES_PARTICION; ++i) {
        printf("%d ", ext_bytemaps->bmap_bloques[i]);
    }
    // To print the inode bytemap
    printf("\nInode bytemap:\n");
    for (int i = 0; i < MAX_INODOS; ++i) {
        printf("%d ", ext_bytemaps->bmap_inodos[i]);
    }
    // new line
    printf("\n");
}

int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2) {
    // It parses the command into its components
    int result = sscanf(strcomando, "%s %s %s", orden, argumento1, argumento2);

    if (result == EOF || result == 0) {
        // The command found is not valid
        return -1;
    }

    return 0;
}


int BuscaFich(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombre) {
    // it looks for the file in the directory
    for (int i = 0; i < MAX_FICHEROS; ++i) {
        if (strcmp(directorio[i].dir_nfich, nombre) == 0) {
            return i; // if it´s found returns the index
        }
    }

    return -1; // and if it´s not found 
}

void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos) {
    // Display the directory contents
    for (int i = 0; i < MAX_FICHEROS; ++i) {
        printf("Name: %s, Inode: %d\n", directorio[i].dir_nfich, directorio[i].dir_inodo);
    }
}


int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo) {
    // this is to rename the file in the directory
    int index = BuscaFich(directorio, inodos, nombreantiguo);
    if (index != -1) {
        strcpy(directorio[index].dir_nfich, nombrenuevo);
        return 0; // if it´s found
    }

    return -1; // if it´s not found
}

int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre) {
    // Print the contents of a file
    int index = BuscaFich(directorio, inodos, nombre);

    if (index != -1) {
        int inodeNumber = directorio[index].dir_inodo;
        for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; ++i) {
            int blockNumber = inodos->bmap_bloques[i];
            printf("%s", memdatos->datos[blockNumber]);
        }

        return 0; // This is that success
    }

    return -1; // File not found
}


int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre, FILE *fich) {
    // Dlete a file
    int index = BuscaFich(directorio, inodos, nombre);

    if (index != -1) {
        int inodeNumber = directorio[index].dir_inodo;

        // Mark the inode and free blocks in the bytemaps
        ext_bytemaps->bmap_inodos[inodeNumber] = 0;

        for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; ++i) {
            int blockNumber = inodos->bmap_bloques[i];
            ext_bytemaps->bmap_bloques[blockNumber] = 0;
        }

        // Put size 0 in the freed inode
        inodos->size_fichero = 0;

        // Mark the 7 block pointers of that inode with the value 0xFFFF
        for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; ++i) {
            inodos->bmap_bloques[i] = 0xFFFF;
        }

        // Delete the directory entry
        strcpy(directorio[index].dir_nfich, "");
        directorio[index].dir_inodo = 0xFFFF;

        return 0; // Success
    }

    return -1; // File not found
}
