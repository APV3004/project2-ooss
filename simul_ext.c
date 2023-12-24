#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include "headers.h"

#define LONGITUD_COMANDO 100

void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps);
int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2);
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup, FILE *file);
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

int main() {
	char comando[LONGITUD_COMANDO];
    char orden[LONGITUD_COMANDO];
    char argumento1[LONGITUD_COMANDO];
    char argumento2[LONGITUD_COMANDO];

    int i, j;
    unsigned long int m;
    EXT_SIMPLE_SUPERBLOCK ext_superblock;
    EXT_BYTE_MAPS ext_bytemaps;
    EXT_BLQ_INODOS ext_blq_inodos;
    EXT_ENTRADA_DIR directorio[MAX_FICHEROS];
    EXT_DATOS memdatos[MAX_BLOQUES_DATOS];
    EXT_DATOS datosfich[MAX_BLOQUES_PARTICION];
    int entradadir;
    int grabardatos = 0;
    FILE *fent;

    // We open the file for reading and writing
    fent = fopen("C:/Users/adrip/OneDrive - U-tad/SSOO/project2-ooss/particion.bin", "r+b");
    if (fent == NULL) {
        perror("Error opening the file");
        return 1;
    }

    // Read the entire file into the memory
    fread(&datosfich, SIZE_BLOQUE, MAX_BLOQUES_PARTICION, fent);

    // We copy the data to the variables
    memcpy(&ext_superblock, (EXT_SIMPLE_SUPERBLOCK *)&datosfich[0], SIZE_BLOQUE);
    memcpy(&directorio, (EXT_ENTRADA_DIR *)&datosfich[3], SIZE_BLOQUE);
    memcpy(&ext_bytemaps, (EXT_BLQ_INODOS *)&datosfich[1], SIZE_BLOQUE);
    memcpy(&ext_blq_inodos, (EXT_BLQ_INODOS *)&datosfich[2], SIZE_BLOQUE);
    memcpy(&memdatos, (EXT_DATOS *)&datosfich[4], MAX_BLOQUES_DATOS * SIZE_BLOQUE);

    // Command processing loop
    for (;;) {
        do {
            printf(">> ");
            fflush(stdin);
            fgets(comando, LONGITUD_COMANDO, stdin);
        } while (ComprobarComando(comando, orden, argumento1, argumento2) != 0);

        if (strcmp(orden, "dir") == 0) {
            Directorio(&directorio, &ext_blq_inodos);
            continue;
        }

        if (strcmp(orden, "info") == 0) {
            // Command: info
            LeeSuperBloque(&ext_superblock, fent);
        }
        else if (strcmp(orden, "bytemaps") == 0) {
            Printbytemaps(&ext_bytemaps);
        }
        else if (strcmp(orden, "rename") == 0) {
            if (Renombrar(&directorio, &ext_blq_inodos, argumento1, argumento2) == -1) {
                printf("Error: Unable to rename. File not found.\n");
            }
        }
        else if (strcmp(orden, "imprimir") == 0) {
            if (Imprimir(&directorio, &ext_blq_inodos, &memdatos, argumento1) == -1) {
                printf("Error: Unable to print. File not found.\n");
            }
        }
        else if (strcmp(orden, "remove") == 0) {
            if (Borrar(&directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, argumento1, fent) == -1) {
                printf("Error: Unable to remove. File not found.\n");
            }
            else {
                grabardatos = 1; // Set the flag to write data after metadata updates
            }
        }
        else if (strcmp(orden, "copy") == 0) {
            int result = Copiar(directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, &memdatos, argumento1, argumento2, fent);
            if (result == -1) {
                printf("Error: Unable to copy. No free inodes available.\n");
            }
            else if (result == -2) {
                printf("Error: Unable to copy. No free blocks available.\n");
            }
            else if (result == -3) {
                printf("Error: Unable to copy. Source file not found or destination file already exists.\n");
            }
            else {
                grabardatos = 1; // Set the flag to write data after metadata updates
            }
        }
        else if (strcmp(orden, "exit") == 0) {
            printf("Exiting the program.\n");
            GrabarDatos(&memdatos, fent);  // Write the remaining data
            fclose(fent);  // Closes the file
            return 0; 
        }
        else {
            printf("Error: Unknown command. Please enter a valid command.\n");
        }

        // Writes the metadata in rename, remove, copy commands
        Grabarinodosydirectorio(&directorio, &ext_blq_inodos, fent);
        GrabarByteMaps(&ext_bytemaps, fent);
        GrabarSuperBloque(&ext_superblock, fent);
        if (grabardatos) {
            GrabarDatos(&memdatos, fent);
            grabardatos = 0;
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
    printf("Directory listing:\n");
    printf("%-20s %-10s %-10s %-10s\n", "File Name", "Inode", "Size", "Blocks");

    for (int i = 0; i < MAX_FICHEROS; ++i) {
        if (directorio[i].dir_nfich[0] != '\0' && directorio[i].dir_inodo != 0xFFFF) {
            int inodeNumber = directorio[i].dir_inodo;
            printf("%-20s %-10d %-10u ", directorio[i].dir_nfich, inodeNumber, inodos[inodeNumber].blq_inodos);

            printf("Blocks: ");
            for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
                int blockNumber = inodos[inodeNumber].blq_relleno[j];
                if (blockNumber != 0xFFFF) {
                    printf("%d ", blockNumber);
                }
            }

            printf("\n");
        }
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
            int blockNumber = inodos[inodeNumber].blq_relleno[i];

            // Check if the block number is valid (not 0xFFFF)
            if (blockNumber != 0xFFFF) {
                // Print the content of the block
                printf("Block Number: %d\n", blockNumber);
                for (int j = 0; j < SIZE_BLOQUE; ++j) {
                    printf("%c", memdatos->dato[blockNumber * SIZE_BLOQUE + j]);
                }
                printf("\n");
            }
        }

        return 0; // Success
    }

    return -1; // File not found
}


int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre, FILE *fich) {
    // Delete a file
    int index = BuscaFich(directorio, inodos, nombre);

    if (index != -1) {
        int inodeNumber = directorio[index].dir_inodo;

        // Mark the inode and free blocks in the bytemaps
        ext_bytemaps->bmap_inodos[inodeNumber] = 0;

        for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; ++i) {
            int blockNumber = inodos->blq_relleno[i];
            ext_bytemaps->bmap_relleno[blockNumber] = 0;
        }

        // Put size 0 in the freed inode
        inodos->blq_relleno[inodeNumber] = 0;

        // Mark the 7 block pointers of that inode with the value 0xFFFF
        for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; ++i) {
            inodos->blq_relleno[i] = 0xFFFF;
        }

        // Delete the directory entry
        strcpy(directorio[index].dir_nfich, "");
        directorio[index].dir_inodo = 0xFFFF;

        return 0; // Success
    }

    return -1; // File not found
}


int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino,  FILE *fich) {
    int freeInode = -1;
    // Check if the source file exists
    int indexOrigen = BuscaFich(directorio, inodos, nombreorigen);
    if (indexOrigen == -1) {
        return -1; // Source file not found
    }

    // Check if the destination file exists
    int indexDestino = BuscaFich(directorio, inodos, nombredestino);
    if (indexDestino != -1) {
        return -2; // Destination file already exists
    }

    // Open the source file for reading
    FILE *sourceFile = fopen(nombreorigen, "rb");
    if (sourceFile == NULL) {
        return -3; // Unable to open the source file
    }

    // Open the destination file for writing
    FILE *destFile = fopen(nombredestino, "wb");
    if (destFile == NULL) {
        fclose(sourceFile);
        return -4; // Unable to create the destination file
    }

    // Copy the file data using a buffer
    char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), sourceFile)) > 0) {
        fwrite(buffer, 1, bytesRead, destFile);
    }

    // Close the files
    fclose(sourceFile);
    fclose(destFile);

   int freeDirEntry = -1;
    for (int i = 0; i < MAX_FICHEROS; ++i) {
        if (directorio[i].dir_inodo == 0) {
            freeDirEntry = i;
            break;
        }
    }
    if (freeDirEntry != -1) {
        strcpy(directorio[freeDirEntry].dir_nfich, nombredestino);
        directorio[freeDirEntry].dir_inodo = freeInode;
    } else {
        // No free directory entry available
        return -4;
    }

    // Write the changes to the disk
    GrabarSuperBloque(ext_superblock, fich);
    GrabarByteMaps(ext_bytemaps, fich);
    Grabarinodosydirectorio(directorio, inodos, fich);

    return 0; // Success
}




void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup, FILE *file) {
    // Read the superblock from the file
    fseek(file, 0, SEEK_SET);
    fread(psup, SIZE_BLOQUE, 1, file);

     // Check if the superblock is read successfully
    if (psup->s_blocks_count > 0) {
        printf("Superblock information:\n");
        printf("Inodes count: %u\n", psup->s_inodes_count);
        printf("Blocks count: %u\n", psup->s_blocks_count);
        printf("Free blocks count: %u\n", psup->s_free_blocks_count);
        printf("Free inodes count: %u\n", psup->s_free_inodes_count);
        printf("First data block: %u\n", psup->s_first_data_block);
        printf("Block size: %u bytes\n", psup->s_block_size);
        printf("\n");
    } else {
        printf("Error reading the superblock.\n");
    }
}

void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich) {
    // This is to write the inode list and directory to the file
    fseek(fich, SIZE_BLOQUE * 3, SEEK_SET);
    fwrite(inodos, SIZE_BLOQUE, 1, fich);
    fwrite(directorio, SIZE_BLOQUE, 1, fich);
}

void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich) {
    // this is to write the bytemaps to the file
    fseek(fich, SIZE_BLOQUE, SEEK_SET);
    fwrite(ext_bytemaps, SIZE_BLOQUE, 1, fich);
}

void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich) {
    // Write the superblock to the file
    fseek(fich, 0, SEEK_SET);
    fwrite(ext_superblock, SIZE_BLOQUE, 1, fich);
}

void GrabarDatos(EXT_DATOS *memdatos, FILE *fich){
    // this is to write the data of the bocks to the file
    fseek(fich, SIZE_BLOQUE * 4, SEEK_SET);
    fwrite(memdatos, SIZE_BLOQUE, MAX_BLOQUES_DATOS, fich);
}
// repushing it 
