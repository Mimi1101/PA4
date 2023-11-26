#include <stdio.h>
#include "filesystem.h"
#include "softwaredisk.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Created By mimic on 11/15/2023.
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


typedef struct Inode
{
    
    uint16_t direct_blocks[13];
    uint16_t indirect_block;
   uint32_t size;   // this is come in handy coz every inode has 13 direct and one indirect and our inode has to be 32 bytes according to gombe


} Inode;


// typedef struct FileDirectory
// {
//     int id;
//     char *fsname;
// } FileDirectory;


struct FileInternals
{
    // FileDirectory direntry; // A DirEntry structure providing file name and inode id.
    Inode i_data;           // An Inode structure holding file-related information.
    FileMode mode;          /* used to store the mode in which the file is opened, such as read mode, write mode, or append mode. */
    uint32_t position;      // current position
    uint16_t current_block; // current block
    bool open;              // Temp bool used to track open or closed
    int inode_id;
    char *fsname;


} FileInternals;


// typedef struct FileInternal *File;


// Global variables
uint8_t blockBitMap[SOFTWARE_DISK_BLOCK_SIZE];
uint8_t inodeBitMap[512];
File directory[MAX_FILES];
Inode inodes[512];


// THROW ERRORS!!
File create_file(char *name)
{
    // find free bitmap for available inode
     printf("Size of Inode: %lu bytes\n", sizeof(Inode));
    int inode_id = -1;
    for (int i = 0; i < sizeof(inodeBitMap); i++)
    {
        if (inodeBitMap[i] == 0)
        {
            inode_id = i;
            inodeBitMap[i] = 1;
            break;
        }
    }
    // If no inode is available, return an error
    if (inode_id == -1)
    {
        printf("Error: Unable to find a free inode.\n");
        fserror = FS_OUT_OF_SPACE;
        return NULL;
    }


    // Setting up file directory internals
    File file = malloc(sizeof(FileInternals));


    if (file == NULL)
    {
        printf("Error: Memory allocation failed.\n");
        fserror = FS_OUT_OF_SPACE;
        return NULL;
    }


    // Initialize the file structure
    memset(file, 0, sizeof(FileInternals));


    //Initializing file fields
    file->mode = READ_WRITE;
    file->position = 0;
    file->current_block = 0;
    file->open = true;
    file->fsname = name;
    file->inode_id = inode_id;


    // Add the new file to the directory
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (directory[i] == NULL)
        {
            directory[i] = file;
            break;
        }
    }




    // Write the updated inode bitmap to the software disk
    write_sd_block(inodeBitMap, INODE_BITMAP_BLOCK);


    // Create and write the inode structure to the software disk
    Inode *inode = malloc(sizeof(Inode));
    if (inode == NULL)
    {
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
    fserror = FS_NONE;


    // removed the (File) cast might throw an error
    printf("ID: %d, Name: %s\n", file->inode_id, file->fsname);
    return file;
}


// THROW ERRORS!!
File open_file(char *name, FileMode mode)
{
    int length = sizeof(directory) / sizeof(directory[0]);
    // Searching For Name
    for (int i = 0; i < length; i++)
    { // Found Name and setting up mode
        if (directory[i]->fsname == name)
        {
            File file = directory[i];
            if (!file->open)
            {
                file->mode = mode;
                fserror = FS_NONE;
                return file;
            }
            else
            {
                fserror = FS_FILE_OPEN;
                return NULL;
            }
        }
    }
    fserror = FS_FILE_NOT_FOUND;
    printf("FILE NOT FOUND\n");
    return NULL; // Check this
}


// THROW ERRORS!!
void close_file(File file)
{
    file->open = false;
    fserror = FS_NONE;
}


// THROW ERRORS!!
int file_exists(char *name)
{
    int length = sizeof(directory) / sizeof(directory[0]);
    for (int i = 0; i < length; i++)
    {
        if (directory[i]->fsname == name)
        {
            fserror = FS_NONE;
            printf("FOUND FILE: %s\n", name);
            return 1;
        }
    }
    fserror = FS_FILE_NOT_FOUND;
    return 0;
}


void fs_print_error(void)
{
    switch (fserror)
    {
    case FS_NONE:
        printf("NO ERROR\n");
        break;


    case FS_OUT_OF_SPACE:
        // The operation caused the software disk to fill up
        printf("ERROR: Disk is out of space\n");
        exit(0);
        break;


    case FS_FILE_NOT_OPEN:
        // Attempted read/write/close/etc. on a file that isn't open
        printf("ERROR: File not open\n");
        exit(0);
        break;


    case FS_FILE_OPEN:
        // File is already open. Concurrent opens are not supported, and neither is deleting a file that is open.
        printf("ERROR: File already open\n");
        exit(0);
        break;


    case FS_FILE_NOT_FOUND:
        // Attempted open or delete of a file that doesn’t exist
        printf("ERROR: File not found\n");
        exit(0);
        break;


    case FS_FILE_READ_ONLY:
        // Attempted write to a file opened for READ_ONLY
        printf("ERROR: File is read-only\n");
        exit(0);
        break;


    case FS_FILE_ALREADY_EXISTS:
        // Attempted creation of a file with an existing name
        printf("ERROR: File already exists\n");
        exit(0);
        break;


    case FS_EXCEEDS_MAX_FILE_SIZE:
        // Seek or write would exceed the maximum file size
        printf("ERROR: Exceeds max file size\n");
        exit(0);
        break;


    case FS_ILLEGAL_FILENAME:
        // Filename begins with a null character
        printf("ERROR: Illegal filename, Filename begins with a null character\n");
        exit(0);
        break;


    case FS_IO_ERROR:
        // Something really bad happened
        printf("ERROR: I/O error\n");
        exit(0);
        break;


    default:
        // Handle unknown error code (optional)
        printf("Unknown error\n");
        exit(0);
        break;
    }
}




