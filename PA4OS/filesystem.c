#include <stdio.h>
#include "filesystem.h"
#include "softwaredisk.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

void print_block_contents(unsigned long blockNum, unsigned long numbytes);
int grab_inode_block(int potential_block, char *name);
void print_inode_ids_in_block(int desiredID, int blockNum);
int grab_inode_index(int potential_block, char *name);
int find_inode_entry(int inode_id);
int find_free_inode(void);
void fs_print_error(void);

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
#define NUM_SINGLE_INDIRECT_BLOCKS (SOFTWARE_DISK_BLOCK_SIZE / sizeof(uint16_t)) // 1
#define MAX_FILE_SIZE (NUM_DIRECT_INODE_BLOCKS + NUM_SINGLE_INDIRECT_BLOCKS) * SOFTWARE_DISK_BLOCK_SIZE
#define MAX_FILE_NAME_LENGTH 256
FSError fserror;

typedef struct Inode
{
    uint16_t direct_blocks[13];
    uint16_t single_indrect;
    File file_md;
} Inode;

struct FileInternals
{
    FileMode mode;     /* used to store the mode in which the file is opened, such as read mode, write mode, or append mode. */
    int position;      // current position
    int current_block; // current block
    bool open;         // Temp bool used to track open or closed
    char *fsname;
    int inode_id;
} FileInternals;

uint16_t inode_bitmap[MAX_FILES];
uint16_t data_bitmap[LAST_DATA_BLOCK - FIRST_DATA_BLOCK];
Inode inode_array[INODES_PER_BLOCK];
// Think about adding DataBitmap block

// Throw the following erros: File_Already_Exist
File create_file(char *name)
{

    fserror = FS_NONE;
    int ret;

    // Inode_bitmap Handler --> finding free inode
    read_sd_block(inode_bitmap, INODE_BITMAP_BLOCK);
    int inode_id = find_free_inode();
    int i_entry = find_inode_entry(inode_id); // Grabbing Recent Inode Array
    read_sd_block(inode_array, i_entry);

    if (inode_id == -1)
    {
        fserror = FS_OUT_OF_SPACE;
        return NULL; // No Free Inode
    }
    printf("Contents of Block %lu: ", INODE_BITMAP_BLOCK);
    print_block_contents(INODE_BITMAP_BLOCK, 10);
    printf("...\n");

    printf("IN_BMP: Free Inode at inode_bitmap[%d] has been found and updated in array/blocks\n", inode_id);

    // Creating file_metadata

    // Creating Inode Structure
    Inode *inode = &inode_array[inode_id];
    if (inode == NULL)
    {
        return NULL; // Something bad happened
    }
    memset(inode->direct_blocks, 0, NUM_DIRECT_INODE_BLOCKS);
    inode->single_indrect = 1;
    inode->file_md = malloc(sizeof(File));
    inode->file_md->fsname = name;
    inode->file_md->current_block = 0;
    inode->file_md->mode = READ_WRITE;
    inode->file_md->position = 0;
    inode->file_md->open = true;
    inode->file_md->inode_id = inode_id + 1;
    // write_sd_block(inode_array, i_entry);
    // read_sd_block(inode_array,i_entry);

    // Storing meta data in inode
    printf("MD_STORED: File{%s} has been stored inside Inode_ID: %d\n", inode->file_md->fsname, inode->file_md->inode_id);
    printf("IN_STORED: ");
    print_inode_ids_in_block(inode->file_md->inode_id, i_entry); // Confirms Storage

    printf("CREATE SUCCESS: File{%s}, Inode_ID: %d\n", inode->file_md->fsname, inode->file_md->inode_id);
    printf("\n");

    write_sd_block(inode_array, i_entry);             // COMEON!! must update
    write_sd_block(inode_bitmap, INODE_BITMAP_BLOCK); // COMEON!! must update
    // free(file);
    return inode->file_md;
}

File open_file(char *name, FileMode mode)
{
    int exist = file_exists(name);
    if (!file_exists)
    {
        fserror = FS_FILE_NOT_FOUND;
        return NULL;
    }
    int block = grab_inode_block(FIRST_INODE_BLOCK, name);
    int index = grab_inode_index(FIRST_INODE_BLOCK, name);
    if (index == -1 || block == -1)
    {
        fserror = FS_FILE_NOT_FOUND;
        return NULL;
    }

    if (inode_array[index].file_md->open == true)
    {
        printf("OPEN_ERROR: File IS OPEN ALREADY\n");
        fserror = FS_FILE_OPEN;
        write_sd_block(inode_array, block);
        return inode_array[index].file_md;
    }
    else
    {
        inode_array[index].file_md->open = false;
        inode_array[index].file_md->mode = mode;
        write_sd_block(inode_array, block);
        return inode_array[index].file_md;
    }
}

unsigned long write_file(File file, void *buf, unsigned long numbytes)
{
    int ret = file_exists(file->fsname);
    if (!ret)
    {
        fserror = FS_FILE_NOT_FOUND;
        return numbytes;
    }

    int block = grab_inode_block(FIRST_INODE_BLOCK, file->fsname);
    int index = grab_inode_index(FIRST_INODE_BLOCK, file->fsname);
    printf("WRITE_FOUND: Got File{%s}\n", inode_array[index].file_md->fsname);


}

// Throw Error If it Doesnt Exist
void close_file(File file)
{
    int ret = file_exists(file->fsname);
    int index = grab_inode_index(INODE_BITMAP_BLOCK, file->fsname);
    if (ret)
    {
        printf("CLOSE: File is Closed\n");
        inode_array[index].file_md->open = false;
    }
}

int file_exists(char *name)
{
    int potential_block = 2;
    while (potential_block < 6)
    {
        // printf("Block is: %d\n", potential_block);
        int ret = read_sd_block(inode_bitmap, potential_block);
        // printf("ret is: %d\n", ret);
        for (int i = 0; i < INODES_PER_BLOCK; i++)
        {
            if ((inode_array[i].file_md != NULL) && (inode_array[i].file_md->fsname == name))
            {
                printf("EXIST: File{%s} exist \n", name);
                return 1;
            }
            // printf("%d\n", i);
        }
        potential_block++;
    }
    fserror = FS_FILE_NOT_FOUND;
    printf("NON_EXIST: File{%s} doesn't exist\n", name);
    return 0;
}

void print_block_contents(unsigned long blockNum, unsigned long numbytes)
{
    uint16_t buffer[SOFTWARE_DISK_BLOCK_SIZE]; // has to be the same size as array
    memset(buffer, 0, SOFTWARE_DISK_BLOCK_SIZE);
    int read_success = read_sd_block(buffer, blockNum);
    if (read_success)
    {
        for (int i = 0; i < numbytes; i++)
        {
            printf("%d ", buffer[i]);
        }
    }
    else
    {
        printf("Error reading Block %lu\n", blockNum);
    }
}

int find_free_inode()
{

    for (int i = 0; i < MAX_FILES; i++)
    {
        if (inode_bitmap[i] == 0)
        {
            inode_bitmap[i] = 1;
            // printf("INDEX %d IS FREE\n", i);
            int ret = write_sd_block(inode_bitmap, INODE_BITMAP_BLOCK);
            read_sd_block(inode_bitmap, INODE_BITMAP_BLOCK);
            if (ret)
            {
                return i;
            }
        }
    }
    return -1;
}

int find_inode_entry(int inode_id)
{
    int ret = FIRST_INODE_BLOCK + (inode_id - 1) / INODES_PER_BLOCK;
    if (inode_id > MAX_FILE_SIZE)
    {
        ret = -1; // Invalid inode_id
    }
    return ret;
}

void print_inode_ids_in_block(int desiredID, int blockNum)
{
    // printf("Hey the first entry is: %s\n", inode_array[0].file_md->fsname);
    for (int i = 0; i < INODES_PER_BLOCK; i++)
    {
        if (inode_array[i].file_md != NULL && inode_array[i].file_md->inode_id == desiredID)
        {
            printf("Inode_ID %d is located in inode_array[%d] inside of block %d\n", desiredID, i, blockNum);
            break;
        }
    }
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
        break;

    case FS_FILE_NOT_OPEN:
        // Attempted read/write/close/etc. on a file that isn't open
        printf("ERROR: File not open\n");
        break;

    case FS_FILE_OPEN:
        // File is already open. Concurrent opens are not supported, and neither is deleting a file that is open.
        printf("ERROR: File already open\n");
        break;

    case FS_FILE_NOT_FOUND:
        // Attempted open or delete of a file that doesn’t exist
        printf("ERROR: File not found\n");
        break;

    case FS_FILE_READ_ONLY:
        // Attempted write to a file opened for READ_ONLY
        printf("ERROR: File is read-only\n");
        break;

    case FS_FILE_ALREADY_EXISTS:
        // Attempted creation of a file with an existing name
        printf("ERROR: File already exists\n");
        break;

    case FS_EXCEEDS_MAX_FILE_SIZE:
        // Seek or write would exceed the maximum file size
        printf("ERROR: Exceeds max file size\n");
        break;

    case FS_ILLEGAL_FILENAME:
        // Filename begins with a null character
        printf("ERROR: Illegal filename, Filename begins with a null character\n");
        break;

    case FS_IO_ERROR:
        // Something really bad happened
        printf("ERROR: I/O error\n");
        break;

    default:
        // Handle unknown error code (optional)
        printf("Unknown error\n");
        break;
    }
}

int grab_inode_index(int potential_block, char *name)
{
    while (potential_block < 6)
    {
        read_sd_block(inode_bitmap, potential_block); // read from first block
        for (int i = 0; i < INODES_PER_BLOCK; i++)
        {
            if (inode_array[i].file_md->fsname == name)
            {
                write_sd_block(inode_bitmap, potential_block);
                return i; // gives index where file is found
            }
            potential_block++;
        }
    }
    return -1; // Something BAD HAPPENED
}

int grab_inode_block(int potential_block, char *name)
{
    while (potential_block < 6)
    {
        read_sd_block(inode_bitmap, potential_block); // read from first block
        for (int i = 0; i < INODES_PER_BLOCK; i++)
        {
            if (inode_array[i].file_md->fsname == name)
            {
                return potential_block; // gives index where file is found
            }
            potential_block++;
        }
    }
    return -1; // Something BAD HAPPENED
}