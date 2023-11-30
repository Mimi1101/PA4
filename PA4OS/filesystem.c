#include <stdio.h>
#include "filesystem.h"
#include "softwaredisk.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

void print_block_contents(unsigned long blockNum, unsigned long numbytes);
// int grab_inode_block(int potential_block, char *name);
// void print_inode_ids_in_block(int desiredID, int blockNum);
// int grab_inode_index(int potential_block, char *name);
short find_inode_entry(short inode_id);
short find_free_inode(void);
void fs_print_error(void);
// int find_free_data_index(void);
// int find_free_data_block(int data_index);
short find_free_directory_block(short dir_index);
void print_directory_confirmation(short dir_index, short dir_block);
void print_inode_confirmation(short inode_id, short i_block);
short find_free_inode_block(short inode_id);

#define MAX_FILES 512
#define DATA_BITMAP_BLOCK 0
#define INODE_BITMAP_BLOCK 1
#define FIRST_INODE_BLOCK 2
#define LAST_INODE_BLOCK 5
#define INODES_PER_BLOCK 128
#define FIRST_DIR_ENTRY_BLOCK 6
#define LAST_DIR_ENTRY_BLOCK 69 // 69 - 6 = 63;
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
    uint8_t direct_blocks[13]; // Assuming MAX_BLOCK_SIZE is the maximum size of a block
    uint8_t single_indrect;    // Stores number
    FileMode mode;
    short position;
    bool open;
} Inode;

struct FileInternals
{
    char *fsname;
    short inode_id;
} FileInternals;

uint16_t inode_bitmap[MAX_FILES];
uint16_t data_bitmap[LAST_DATA_BLOCK - FIRST_DATA_BLOCK]; // 4025 blocks avail. 70 + datamap_index Elements mapped 0 70 -> 1 -> 71 2 -> 72 .... 4025 -> 4095
Inode *inode_array[SOFTWARE_DISK_BLOCK_SIZE];
File directory_array[SOFTWARE_DISK_BLOCK_SIZE];

File create_file(char *name)
{
    // Finding free inode and flipping bit in array
    read_sd_block(inode_bitmap, INODE_BITMAP_BLOCK);
    printf("Inode Size: %lu\n", sizeof(Inode));
    printf("File: %lu\n", sizeof(FileInternals));
    short inode_id = find_free_inode();
    if (inode_id == -1)
    {
        fserror = FS_OUT_OF_SPACE; // no inode
        return NULL;
    }
    printf("Contents of Block %lu: ", INODE_BITMAP_BLOCK);
    print_block_contents(INODE_BITMAP_BLOCK, 10);
    printf("\nCREATE: Inode_id: %d\n", inode_id);

    // Create File_Handeler
    directory_array[inode_id] = malloc(sizeof(File));
    directory_array[inode_id]->fsname = strdup(name);
    directory_array[inode_id]->inode_id = inode_id;

    short dir_index = inode_id;
    short dir_block = find_free_directory_block(dir_index);
    if (dir_block == -1)
    {
        fserror = FS_OUT_OF_SPACE;
        return NULL;
    }
    printf("Looking at block: %d\n", dir_block);

    write_sd_block(directory_array, dir_block);
    print_directory_confirmation(dir_index, dir_block);

    //Create inode
    inode_array[inode_id] = malloc(sizeof(Inode));
    inode_array[inode_id]->open = false;
    inode_array[inode_id]->position = 0;
    inode_array[inode_id]->single_indrect = 1; // Change later
    inode_array[inode_id]->mode = READ_WRITE;


    short i_block = find_free_inode_block(inode_id);
    write_sd_block(inode_array, i_block);
    print_inode_confirmation(inode_id, i_block);

    printf("CREATE_SUCCESS: File{%s}, Inode_ID: %d\n",directory_array[inode_id]->fsname, directory_array[inode_id]->inode_id);
}

/*Begin: File Create helper methods*/
short find_free_inode()
{

    for (int i = 0; i < MAX_FILES; i++)
    {
        if (inode_bitmap[i] == 0)
        {
            inode_bitmap[i] = 1;
            int ret = write_sd_block(inode_bitmap, INODE_BITMAP_BLOCK);
            if (ret)
            {
                return i;
            }
        }
    }
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

short find_inode_entry(short inode_id) // inode_id refers to index not identifier
{
    printf("INODE ID: %d\n", inode_id);
    short ret = (inode_id / INODES_PER_BLOCK) + 2;

    ret = FIRST_DATA_BLOCK;

    printf("RETURN: %d\n", ret);
    if (inode_id > MAX_FILE_SIZE)
    {
        ret = -1; // Invalid inode_id
    }
    return ret;
}

short find_free_directory_block(short dir_index)
{
    short index = -1;
    if (dir_index > DIR_ENTRIES_PER_BLOCK * LAST_DIR_ENTRY_BLOCK)
    {
        fserror = FS_OUT_OF_SPACE;
        return -1;
    }
    return FIRST_DIR_ENTRY_BLOCK + (dir_index / DIR_ENTRIES_PER_BLOCK);
}

short find_free_inode_block(short inode_id)
{
    if (inode_id > MAX_FILE_SIZE)
    {
        fserror = FS_OUT_OF_SPACE;
        return -1;
    }
    return (inode_id / INODES_PER_BLOCK) + FIRST_INODE_BLOCK;
}

void print_directory_confirmation(short dir_index, short dir_block)
{
    File buffer[SOFTWARE_DISK_BLOCK_SIZE];
    read_sd_block(buffer, dir_block);
    printf("CONFIRM: File_Handler {%s} has been stored\n", buffer[dir_index]->fsname);
}

void print_inode_confirmation(short inode_id, short i_block)
{
    Inode *buffer[SOFTWARE_DISK_BLOCK_SIZE];
    read_sd_block(buffer, i_block);
    printf("CONFIRM: Inode_Handler has position of {%d}. Storage confirmed\n", buffer[inode_id]->position);
}
/*End: File Create helper methods*/


