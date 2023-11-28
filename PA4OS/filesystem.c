#include <stdio.h>
#include "filesystem.h"
#include "softwaredisk.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

<<<<<<< Updated upstream
//
=======
int find_freeinode(void);
int find_freedirspace(void);
void init_file(File file, char *name, int inode_id);
int find_inode_block(int inode_id);
int find_dirblock(int dir_entry);

>>>>>>> Stashed changes
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

<<<<<<< Updated upstream
// Represents an Inode, a data structure that stores information about a file.
// tracks the size of the file and blocks that are used

typedef struct Inode{

uint16_t direct_blocks[13];
uint16_t indirect_block;
=======
typedef struct Inode
{
>>>>>>> Stashed changes

} Inode;

<<<<<<< Updated upstream
typedef struct FileDirectory{
 int id;
 //Inode id
 char fsname[MAX_FILE_NAME_LENGTH];
} FileDirectory;

typedef struct FileInternal{
    FileDirectory direntry;      // A DirEntry structure providing file name and inode id.
    Inode i_data;           // An Inode structure holding file-related information.
=======
struct FileInternals
{
    // FileDirectory direntry; // A DirEntry structure providing file name and inode id.
    Inode *i_data;          // An Inode structure holding file-related information.
>>>>>>> Stashed changes
    FileMode mode;          /* used to store the mode in which the file is opened, such as read mode, write mode, or append mode. */
    uint32_t position;      // current position
    uint16_t current_block; // current block
}FileInternal;

<<<<<<< Updated upstream
//Global variables
=======
// Global variables
>>>>>>> Stashed changes
uint8_t blockBitMap[SOFTWARE_DISK_BLOCK_SIZE];
uint8_t inodeBitMap[512];
FileDirectory directory[MAX_FILES];

<<<<<<< Updated upstream
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
=======
// THROW ERRORS!!
File create_file(char *name)
{

    // find free bitmap for available inode
    printf("Size of Inode: %lu bytes\n", sizeof(Inode));

    int inode_id = find_freeinode();
    if (inode_id == -1)
    {
>>>>>>> Stashed changes
        printf("Error: Unable to find a free inode.\n");
        fserror = FS_OUT_OF_SPACE;
        return NULL;
    }

<<<<<<< Updated upstream
   // Setting up file directory internals
    FileInternal *file = malloc(sizeof(FileInternal));
    if (file == NULL) {
=======
    /*Begin: Creating File*/
    File file = malloc(sizeof(FileInternals));
    if (file == NULL)
    {
>>>>>>> Stashed changes
        printf("Error: Memory allocation failed.\n");
        fserror = FS_OUT_OF_SPACE;
        return NULL;
    }
<<<<<<< Updated upstream

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
=======
    memset(file, 0, sizeof(FileInternals));
    init_file(file, name, inode_id);
    /*End: Creating File*/

    int dir_id = find_freedirspace();
    directory[dir_id] = file;

    /*Begin: Creating Inode*/
>>>>>>> Stashed changes
    Inode *inode = malloc(sizeof(Inode));
    if (inode == NULL) {
        printf("Error: Memory allocation failed for inode.\n");
        fserror = FS_OUT_OF_SPACE;
        free(file);
        return NULL;
    }
<<<<<<< Updated upstream

    // Initialize the inode structure
    memset(inode, 0, sizeof(Inode));
=======
    for (int i = 0; i < NUM_DIRECT_INODE_BLOCKS; i++)
    {
        inode->direct_blocks[i] = 0;
    }
    file->i_data = inode;
    /*End: Creating Inode*/
>>>>>>> Stashed changes

    /*Begin: Writing to Inode Block*/
    int inode_blockentry = find_inode_block(inode_id);
    int inode_write = write_sd_block(file->i_data, inode_blockentry);
    if (inode_write == 0 || inode_blockentry == -1)
    { // Write Failed or Out of Space
        return NULL;
    }
    printf("Inode_ID: %d Has been succesfully writtin in block: %d\n", inode_id, inode_blockentry);
    /*End: Writitng to Inode Block*/

<<<<<<< Updated upstream
    // Write the updated directory to the software disk
    write_sd_block(directory, FIRST_DIR_ENTRY_BLOCK);
=======
    /*Begin: Writitng to Directory Block*/
    int dir_blockentry = find_dirblock(dir_id);
    int dir_write = write_sd_block(directory[dir_id], dir_blockentry);
    if (dir_write == 0 || dir_blockentry == -1)
    { // Write Failed or Out of Space
        return NULL;
    }
    printf("Entry_ID: %d has been successfully written in block: %d\n", dir_id, dir_write);
    /*End: Writing to Directory Block*/
>>>>>>> Stashed changes

    return (File)file;
}

<<<<<<< Updated upstream
File open_file(char *name, FileMode mode){

=======
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

unsigned long write_file(File file, void *buf, unsigned long numbytes)
{
    fserror = FS_NONE;
    unsigned long remaining_space = SOFTWARE_DISK_BLOCK_SIZE - numbytes;
    // Over Writing Error
    if (numbytes > remaining_space)
    {
        fserror = FS_OUT_OF_SPACE;
        return 0;
    }
    // Search for free Block
    int blockNum = -1;
    for (int i = FIRST_DATA_BLOCK; i <= LAST_DATA_BLOCK; i++)
    {
        if (blockBitMap[i] == 0)
        {
            blockNum = i;
            // Setting Free Block to Direct Block
            for (int j = 0; j < NUM_DIRECT_INODE_BLOCKS; j++)
            {
                if (file->i_data->direct_blocks[j] == 0)
                {
                    file->i_data->direct_blocks[j] = blockNum;
                    break;
                }
                /*WORK ON INDIRECT BLOCKS*/
            }
            break;
        }
    }
    // Block not found
    if (blockNum == -1)
    {
        fserror = FS_OUT_OF_SPACE; // or another appropriate error code
        return 0;
    }
    printf("Current Position is now: %d and Desired Block is: %d\n", file->position, blockNum);
    char padded_buf[SOFTWARE_DISK_BLOCK_SIZE];       // Setting Padded_Buffer to 4096
    memset(padded_buf, 0, SOFTWARE_DISK_BLOCK_SIZE); // Initilizing to 0
    memcpy(padded_buf, buf, numbytes);               // Copying into padded_buffer
    int ret = write_sd_block(padded_buf, blockNum);  // writing padded_buffer into sd
    printf("Result from write: %d", ret);
    if (ret == 0)
    {
        sd_print_error();
        return 0;
    }
    printf("Disk Writing Passed!");
    file->position += numbytes;
    return numbytes;
>>>>>>> Stashed changes
}

/*Helper Methods*/
int find_freeinode()
{
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
    return inode_id;
}

void init_file(File file, char *name, int inode_id)
{
    // Initializing file fields
    file->mode = READ_WRITE;
    file->position = 0;
    file->current_block = 0;
    file->open = true;
    file->fsname = name;
    file->inode_id = inode_id;
}

int find_freedirspace()
{
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (directory[i] == NULL)
        {
            return i;
        }
    }
    return -1;
}

int find_inode_block(int inode_id)
{
    int ret = FIRST_INODE_BLOCK + (inode_id - 1) / INODES_PER_BLOCK;
    if (inode_id > 512)
    {
        ret = -1; // Invalid inode_id
    }
    return ret;
}

int find_dirblock(int dir_entry){
    int dir_index = FIRST_DIR_ENTRY_BLOCK + (dir_entry - 1) / DIR_ENTRIES_PER_BLOCK;
    if (dir_index >= LAST_DATA_BLOCK)
    {
        return -1;
    }
    return dir_index;
}