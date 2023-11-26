#include <stdio.h>
#include "filesystem.h"
#include "softwaredisk.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

//
// Created By mimic on 11/15/2023.
//
// copied from her word document in PA4
#define MAX_FILES 512
#define DATA_BITMAP_BLOCK 0
#define INODE_BITMAP_BLOCK 1
#define FIRST_INODE_BLOCK 2
#define LAST_INODE_BLOCK 5
#define INODES_PER_BLOCK 128
#define FIRST_DIR_ENTRY_BLOCK 6
#define LAST_DIR_ENTRY_BLOCK 69
#define DIR_ENTRIES_PER_BLOCK 8
#define FIRST_DATA_BLOCK 70
#define LAST_DATA_BLOCK 4095
#define MAX_FILENAME_SIZE 507
#define NUM_DIRECT_INODE_BLOCKS 13
#define NUM_SINGLE_INDIRECT_BLOCKS (SOFTWARE_DISK_BLOCK_SIZE / sizeof(uint16_t))
#define MAX_FILE_SIZE (NUM_DIRECT_INODE_BLOCKS + NUM_SINGLE_INDIRECT_BLOCKS) * SOFTWARE_DISK_BLOCK_SIZE
#define MAX_FILE_NAME_LENGTH 256
FSError fserror;

// Represents an Inode, a data structure that stores information about a file.
// tracks the size of the file and blocks that are used

typedef struct Inode{

uint16_t direct_blocks[13];
uint16_t indirect_block;

} Inode;

typedef struct FileDirectory{
 int id;
 //Inode id
 char fsname[MAX_FILE_NAME_LENGTH];
} FileDirectory;

typedef struct FileInternal{
    FileDirectory direntry;      // A DirEntry structure providing file name and inode id.
    Inode i_data;           // An Inode structure holding file-related information.
    FileMode mode;          /* used to store the mode in which the file is opened, such as read mode, write mode, or append mode. */
    uint32_t position;      // current position
    uint16_t current_block; // current block
}FileInternal;

//Global variables
uint8_t blockBitMap[SOFTWARE_DISK_BLOCK_SIZE];
uint8_t inodeBitMap[512];
FileDirectory directory[MAX_FILES];

//THROW ERRORS!!
File create_file( char *name){

    //find free bitmap for available inode 
    int inode_id = -1;
    for (int i = 0; i < sizeof(inodeBitMap); i++) {
        if (inodeBitMap[i] == 0) {
            inode_id = i;
            inodeBitMap[i] = 1;
            break;
        }
    }
    // If no inode is available, return an error
    if (inode_id == -1) {
        printf("Error: Unable to find a free inode.\n");
        fserror = FS_OUT_OF_SPACE;
        return NULL;
    }

   // Setting up file directory internals
    FileInternal *file = malloc(sizeof(FileInternal));
    if (file == NULL) {
        printf("Error: Memory allocation failed.\n");
        fserror = FS_OUT_OF_SPACE;
        return NULL;
    }

    // Initialize the file structure
    memset(file, 0, sizeof(FileInternal));

    // Set the file name
    strncpy(file->direntry.fsname, name, MAX_FILE_NAME_LENGTH - 1);

    // Set the inode id
    file->direntry.id = inode_id;

    // Set the initial mode
    file->mode = READ_WRITE;

    // Set the initial position
    file->position = 0;

    // Set the initial block
    file->current_block = 0;
   // so i did try to run with the test function and it does create a file but its not showing up in our folder
   // because i think when we create a file we have to write the directory information such as file name and size and all that to the software disk
   // I am trying that here

   // Add the new file to the directory
    for (int i = 0; i < MAX_FILES; i++) {
        if (directory[i].id == 0) {
            directory[i] = file->direntry;
            break;
        }
    }

    // Write the updated inode bitmap to the software disk
    write_sd_block(inodeBitMap, INODE_BITMAP_BLOCK);

    // Create and write the inode structure to the software disk
    Inode *inode = malloc(sizeof(Inode));
    if (inode == NULL) {
        printf("Error: Memory allocation failed for inode.\n");
        fserror = FS_OUT_OF_SPACE;
        free(file);
        return NULL;
    }

    // Initialize the inode structure
    memset(inode, 0, sizeof(Inode));

    // Write the inode structure to the software disk
    write_sd_block(inode, FIRST_INODE_BLOCK + inode_id);

    // Write the updated directory to the software disk
    write_sd_block(directory, FIRST_DIR_ENTRY_BLOCK);

    return (File)file;
}

File open_file(char *name, FileMode mode){

}
