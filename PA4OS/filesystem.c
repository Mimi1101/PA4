// Brandon Walton
// Bibhushita Baral

#include <stdio.h>
#include "filesystem.h"
#include "softwaredisk.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Find
short find_free_directory_block(short directory_index); 
short find_free_inode(void); 
short find_free_data_block(void);

// Print Confirmation
void print_directory_confirmation(short directory_index, short directory_block);
void print_inode_confirmation(short inode_id, short i_block);
void print_write_confirmation(short inode_index, unsigned long numbytes, short directory_index, short data_block_index);
void print_block_contents(unsigned long blockNum, unsigned long numbytes);

// Get Methods
short get_inode_block(short inode_id);
short get_inode_index(char *name);
short get_directory_block(char *name);
short get_directory_index(char *name);

#define MAX_FILES 512
#define DATA_BITMAP_BLOCK 0
#define INODE_BITMAP_BLOCK 1
#define FIRST_INODE_BLOCK 2
#define LAST_INODE_BLOCK 5
#define INODES_PER_BLOCK 128
#define FIRST_DIR_ENTRY_BLOCK 6
#define LAST_DIR_ENTRY_BLOCK 69 // 69 - 6 = 63;
#define DIR_ENTRIES_PER_BLOCK 8
#define FIRST_DATA_BLOCK 70  // 70 -> 325 direct blocks 70 -
#define LAST_DATA_BLOCK 4095 // 326 -> 4095 indirect blocks
#define MAX_FILENAME_SIZE 507
#define NUM_DIRECT_INODE_BLOCKS 13
#define NUM_SINGLE_INDIRECT_BLOCKS (SOFTWARE_DISK_BLOCK_SIZE / sizeof(uint16_t)) // 1
#define MAX_FILE_SIZE (NUM_DIRECT_INODE_BLOCKS + NUM_SINGLE_INDIRECT_BLOCKS) * SOFTWARE_DISK_BLOCK_SIZE
#define MAX_FILE_NAME_LENGTH 256
FSError fserror;

typedef struct Inode
{
    uint8_t direct_blocks[13]; // Assuming MAX_BLOCK_SIZE is the maximum size of a block
    uint16_t single_indrect;   // Stores number
    FileMode mode;
    uint16_t size;
    bool open;
} Inode;

struct FileInternals
{
    char *fsname;
    short inode_id;
} FileInternals;

uint16_t inode_bitmap[SOFTWARE_DISK_BLOCK_SIZE];
uint16_t data_bitmap[SOFTWARE_DISK_BLOCK_SIZE]; // 4025 blocks avail. 70 + datamap_index Elements mapped 0 70 -> 1 -> 71 2 -> 72 .... 4025 -> 4095
Inode *inode_array[SOFTWARE_DISK_BLOCK_SIZE];
File directory_array[SOFTWARE_DISK_BLOCK_SIZE];

// Helper methods

short find_free_directory_block(short directory_index)
{
    short index = -1;
    if (directory_index > DIR_ENTRIES_PER_BLOCK * LAST_DIR_ENTRY_BLOCK)
    {
        fserror = FS_OUT_OF_SPACE;
        return -1;
    }
    return FIRST_DIR_ENTRY_BLOCK + (directory_index / DIR_ENTRIES_PER_BLOCK);
}

short find_free_inode()
{
    for (short i = 0; i < MAX_FILES; i++)
    {
        if (inode_bitmap[i] == 0)
        {
            inode_bitmap[i] = 1;
            short ret = write_sd_block(inode_bitmap, INODE_BITMAP_BLOCK);
            if (ret)
            {
                return i;
            }
        }
    }
    return 0;
}

short find_free_data_block()
{
    read_sd_block(data_bitmap, DATA_BITMAP_BLOCK);
    for (short i = FIRST_DATA_BLOCK; i <= (LAST_DATA_BLOCK); i++)
    {
        if (data_bitmap[i] == 0)
        {
            data_bitmap[i] = 1;
            write_sd_block(data_bitmap, DATA_BITMAP_BLOCK);
            return i;
        }
    }
    return -1;
}

void print_directory_confirmation(short directory_index, short directory_block)
{
    File buffer[SOFTWARE_DISK_BLOCK_SIZE];
    read_sd_block(buffer, directory_block);
    printf("\nCONFIRM: File_Handler {%s} has been stored\n", buffer[directory_index]->fsname);
}

void print_inode_confirmation(short inode_id, short i_block)
{
    Inode *buffer[SOFTWARE_DISK_BLOCK_SIZE];
    read_sd_block(buffer, i_block);
    printf("CONFIRM: Inode_Handler has position of {%d}. Storage confirmed\n", buffer[inode_id]->size);
}

void print_write_confirmation(short inode_index, unsigned long numbytes, short directory_index, short data_block_index)
{

    printf("Contents of File: {%s}, Inode_ID: {%d} Direct-Block{%d}: ", directory_array[directory_index]->fsname, directory_array[directory_index]->inode_id, data_block_index);
    for (short i = 0; i < numbytes; i++)
    {
        printf("%c ", inode_array[inode_index]->direct_blocks[i]);
    }
}

void print_block_contents(unsigned long blockNum, unsigned long numbytes)
{
    uint16_t buffer[SOFTWARE_DISK_BLOCK_SIZE]; // has to be the same size as array
    memset(buffer, 0, SOFTWARE_DISK_BLOCK_SIZE);
    short read_success = read_sd_block(buffer, blockNum);
    if (read_success)
    {
        for (short i = 0; i < numbytes; i++)
        {
            printf("%d ", buffer[i]);
        }
    }
    else
    {
        printf("Error reading Block %lu\n", blockNum);
    }
}

short get_inode_block(short inode_id)
{
    if (inode_id > MAX_FILE_SIZE)
    {
        fserror = FS_OUT_OF_SPACE;
        return -1;
    }
    return (inode_id / INODES_PER_BLOCK) + FIRST_INODE_BLOCK;
}

short get_inode_index(char *name)
{
    short potential_block = FIRST_DIR_ENTRY_BLOCK;
    while (potential_block <= LAST_DIR_ENTRY_BLOCK)
    {
        read_sd_block(directory_array, potential_block);
        for (short i = 0; i < DIR_ENTRIES_PER_BLOCK * (LAST_DIR_ENTRY_BLOCK - FIRST_DIR_ENTRY_BLOCK); i++)
        {
            if (directory_array[i] != NULL && strcmp(directory_array[i]->fsname, name) == 0)
            {
                write_sd_block(directory_array, potential_block);
                return directory_array[i]->inode_id;
            }
        }
        potential_block++;
    }
    fserror = FS_OUT_OF_SPACE; // Couldn't find block
    return -1;
}

short get_directory_block(char *name)
{
    short potential_block = FIRST_DIR_ENTRY_BLOCK;
    while (potential_block <= LAST_DIR_ENTRY_BLOCK)
    {
        read_sd_block(directory_array, potential_block);
        for (short i = 0; i < DIR_ENTRIES_PER_BLOCK * (LAST_DIR_ENTRY_BLOCK - FIRST_DIR_ENTRY_BLOCK); i++)
        {
            if (directory_array[i] != NULL && strcmp(directory_array[i]->fsname, name) == 0)
            {
                write_sd_block(directory_array, potential_block);
                return potential_block;
            }
        }
        potential_block++;
    }
    fserror = FS_OUT_OF_SPACE; // Couldn't find block
    return -1;
}

short get_directory_index(char *name)
{
    short potential_block = FIRST_DIR_ENTRY_BLOCK;
    while (potential_block <= LAST_DIR_ENTRY_BLOCK)
    {
        read_sd_block(directory_array, potential_block);
        for (short i = 0; i < DIR_ENTRIES_PER_BLOCK * (LAST_DIR_ENTRY_BLOCK - FIRST_DIR_ENTRY_BLOCK); i++)
        {
            if (directory_array[i] != NULL && strcmp(directory_array[i]->fsname, name) == 0)
            {
                write_sd_block(directory_array, potential_block);
                return i;
            }
        }
        potential_block++;
    }
    fserror = FS_OUT_OF_SPACE; // Couldn't find block
    return -1;
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

//System Calls

File open_file(char *name, FileMode mode)
{
    fserror = FS_NONE;
    short success = file_exists(name);
    if (!success)
    {
        fserror = FS_FILE_NOT_FOUND;
        return NULL;
    }
    short inode_index = get_inode_index(name);
    short i_block = get_inode_block(inode_index);
    short directory_index = get_directory_index(name);
    short directory_block = get_directory_block(name);

    read_sd_block(inode_array, i_block);
    read_sd_block(directory_array, directory_block);
    if (inode_index == -1 || i_block == -1 || directory_index == -1 || directory_block == -1)
    {
        printf("Something Unexpected Happened\n");
        return NULL;
    }

    if (inode_array[inode_index]->open == true)
    {
        printf("OPEN_ERROR: File is already opened\n\n");
        fserror = FS_FILE_OPEN;
        return NULL;
    }
    inode_array[inode_index]->open = false;
    inode_array[inode_index]->mode = mode;

    write_sd_block(inode_array, i_block);
    write_sd_block(directory_array, directory_block);

    return directory_array[directory_index];
}

File create_file(char *name)
{
    fserror = FS_NONE;
    // Finding free inode and flipping bit in array
    read_sd_block(inode_bitmap, INODE_BITMAP_BLOCK);
    printf("Inode Size: %lu + File_Internal Size: %lu = %d\n", sizeof(Inode), sizeof(FileInternals), sizeof(Inode) + sizeof(FileInternals));
    short inode_id = find_free_inode();
    if (inode_id == -1)
    {
        fserror = FS_OUT_OF_SPACE; // no inode
        return NULL;
    }
    printf("Contents of Block %lu: ", INODE_BITMAP_BLOCK);
    print_block_contents(INODE_BITMAP_BLOCK, 10);
    printf("\n");
    // printf("\nCREATE: Inode_id: %d\n", inode_id);

    // Create File_Handeler
    directory_array[inode_id] = malloc(sizeof(FileInternals));
    directory_array[inode_id]->fsname = strdup(name);
    directory_array[inode_id]->inode_id = inode_id;

    short directory_index = inode_id;
    short directory_block = find_free_directory_block(directory_index);
    if (directory_block == -1)
    {
        fserror = FS_OUT_OF_SPACE;
        return NULL;
    }

    write_sd_block(directory_array, directory_block);
    // print_directory_confirmation(dir_index, dir_block);

    // Create inode
    inode_array[inode_id] = malloc(sizeof(Inode));
    inode_array[inode_id]->open = true;
    inode_array[inode_id]->size = 0;
    inode_array[inode_id]->single_indrect = 0; // Change later
    memset(inode_array[inode_id]->direct_blocks, 0, sizeof(inode_array[inode_id]->direct_blocks));
    inode_array[inode_id]->mode = READ_WRITE;

    short i_block = get_inode_block(inode_id);
    write_sd_block(inode_array, i_block);
    // print_inode_confirmation(inode_id, i_block);

    printf("CREATE_SUCCESS: File{%s}, Inode_ID: %d\n\n", directory_array[inode_id]->fsname, directory_array[inode_id]->inode_id);
    return directory_array[inode_id];
}

void close_file(File file)
{
    fserror = FS_NONE;
    short success = file_exists(file->fsname);
    if (!success)
    {
        fserror = FS_FILE_NOT_FOUND;
        return;
    }
    short inode_index = get_inode_index(file->fsname);
    short i_block = get_inode_block(file->inode_id);
    short directory_index = get_directory_index(file->fsname);
    short directory_block = get_directory_block(file->fsname);

    read_sd_block(inode_array, i_block);
    read_sd_block(directory_array, directory_block);

    printf("CLOSE: File{%s} is closed", file->fsname);
    inode_array[inode_index]->open = false; // Force close, can't close something that is already closed
    write_sd_block(inode_array, i_block);
    write_sd_block(directory_array, directory_block);
}

/*TWEAK IT UP SOME*/
unsigned long read_file(File file, void *buf, unsigned long numbytes)
{
    fserror = FS_NONE;
    short success = file_exists(file->fsname);

    if (!success)
    {
        fserror = FS_FILE_NOT_FOUND;
        return 0;
    }

    short inode_index = get_inode_index(file->fsname);
    short i_block = get_inode_block(file->inode_id);
    short dir_index = get_directory_index(file->fsname);
    short directory_block = get_directory_block(file->fsname);

    read_sd_block(directory_array, directory_block);
    read_sd_block(inode_array, i_block);

    // Check for direct blocks

    // uint8_t direct_index = get_direct_data_index(inode_index);
    // uint16_t indirect_index = get_indirect_data_index(inode_index);
    // uint16_t indirect_pointer_index = get_indirect_pointer_index(inode_index);

    // Checking if the file is open
    if (!inode_array[inode_index]->open)
    {
        fserror = FS_FILE_NOT_OPEN;
        return 0;
    }

    if (inode_array[inode_index]->size + numbytes > MAX_FILE_SIZE)
    {
        fserror = FS_EXCEEDS_MAX_FILE_SIZE;
        return 0;
    }

    for (short i = 0; i < NUM_DIRECT_INODE_BLOCKS; i++)
    {
        if (inode_array[inode_index]->direct_blocks[i] != 0)
        {
            read_sd_block(buf, (inode_array[inode_index]->direct_blocks[i] + FIRST_DATA_BLOCK) - 1);
            printf("READ BUFFER IS: %s\n", buf);
            printf("Num Bytes being written in: %d\n", strlen(buf));

            return strlen(buf);
        }
    }
}

/*TWEAK IT UP SOME*/
unsigned long write_file(File file, void *buf, unsigned long numbytes)
{
    fserror = FS_NONE;
    short is_direct = 1;
    // printf("Here\n");
    int success = file_exists(file->fsname);
    if (!success)
    {
        fserror = FS_FILE_NOT_FOUND;
        printf("File not found: %s\n", file->fsname); // Add this line

        return 0;
    }

    short inode_index = get_inode_index(file->fsname);
    short i_block = get_inode_block(file->inode_id);
    short directory_index = get_directory_index(file->fsname);
    short directory_block = get_directory_block(file->fsname);

    read_sd_block(inode_array, i_block);
    read_sd_block(directory_array, directory_block);

    if (inode_array[inode_index]->mode == READ_ONLY)
    {
        fserror = FS_FILE_READ_ONLY;
        return 0;
    }

    if (inode_array[inode_index]->open == false)
    {
        fserror = FS_FILE_NOT_OPEN;
        return 0;
    }

    short size = inode_array[inode_index]->size;
    short s_bytes = numbytes;
    // printf("Write would be: %d", size + s_bytes);
    if (size > MAX_FILE_SIZE)
    {

        fserror = FS_EXCEEDS_MAX_FILE_SIZE;
        return 0;
    }
    short offset = size % SOFTWARE_DISK_BLOCK_SIZE;

    // read_sd_block(data_bitmap, DATA_BITMAP_BLOCK);
    short data_block_index = 0;
    if (inode_array[inode_index]->single_indrect == 0)
    {
        data_block_index = find_free_data_block();
    }
    if (data_block_index == -1)
    {

        fserror = FS_IO_ERROR;
        return 0;
    }

    // Writing Data
    char buffer[numbytes];

    memcpy(buffer, buf, numbytes);
    printf("LENGTH OF THING IS: %d\n", strlen(buffer));
    if (strlen(buffer) < numbytes)
    {
        printf("INVALID!!!!\n");
        return 0;
    }

    // Find Free direct/indirect and link
    int index = -1;
    for (short i = 0; i < NUM_DIRECT_INODE_BLOCKS; i++)
    {
        if (inode_array[inode_index] != NULL && inode_array[inode_index]->direct_blocks[i] == 0 && data_block_index <= 325) // fix math sit
        {
            // printf("data - block = %d\n", data_block_index - FIRST_DATA_BLOCK);
            inode_array[inode_index]->direct_blocks[i] = (data_block_index - FIRST_DATA_BLOCK) + 1; // Read index it by minus 1
            // printf("DIRECT WRITE: {%s}'s Direct_Block[%d] -> Block[%d] -> %s\n", directory_array[directory_index]->fsname, i, inode_array[inode_index]->direct_blocks[i], buffer);
            write_sd_block(buffer, data_block_index);
            inode_array[inode_index]->size += strlen(buffer);
            printf("SIZE IS: %d <--> MAX IS: %d\n", inode_array[inode_index]->size, MAX_FILE_SIZE);
            write_sd_block(inode_array, i_block);
            // printf("SIZE OF BUFFER IS: %d\n", strlen(buffer));
            return strlen(buffer);
        }
    }

    if (inode_array[inode_index]->single_indrect == 0)
    {
        inode_array[inode_index]->single_indrect = data_block_index; // equals the actual index
        write_sd_block(inode_array, i_block);
        uint16_t single_indirect_array[SOFTWARE_DISK_BLOCK_SIZE];
        memset(single_indirect_array, (uint16_t)0, SOFTWARE_DISK_BLOCK_SIZE);
        write_sd_block(single_indirect_array, inode_array[inode_index]->single_indrect);
    }

    uint16_t single_indirect_array[SOFTWARE_DISK_BLOCK_SIZE];
    short single_indirect_entry = find_free_data_block();
    // read_sd_block(single_indirect_array, inode_array[inode_index]->single_indrect);

    for (uint16_t i = 0; i < SOFTWARE_DISK_BLOCK_SIZE; i++)
    {
        if (single_indirect_array[i] == 0)
        {
            // printf("INDEX: %u\n", i);
            single_indirect_array[i] = single_indirect_entry;
            printf("INDIRECT WRITE: {%s}'s Single_Indirect_Block[%d] -> Single_Indirect_Index[%u] -> Block[%d] -> %s\n", directory_array[directory_index]->fsname, inode_array[inode_index]->single_indrect, i, single_indirect_entry, buffer);
            write_sd_block(single_indirect_array, data_block_index);
            inode_array[inode_index]->size += strlen(buffer);
            write_sd_block(inode_array, i_block);
            return strlen(buffer);
        }
    }
    fserror = FS_OUT_OF_SPACE; // Replace
    return 0;
}

int seek_file(File file, unsigned long bytepos)
{
    fserror = FS_NONE;

    short success = file_exists(file->fsname);
    if (!success)
    {
        fserror = FS_FILE_NOT_FOUND;
    }

    char inode_index = get_inode_index(file->fsname);
    char i_block = get_inode_block(file->inode_id);
    read_sd_block(inode_array, i_block);

    if ((inode_array[inode_index]->size + bytepos) >= MAX_FILE_SIZE)
    {
        fserror = FS_EXCEEDS_MAX_FILE_SIZE;
        return 0;
    }
    printf("Position was: %d\n", inode_array[inode_index]->size);
    inode_array[inode_index]->size += bytepos;
    write_sd_block(inode_array, i_block);
    printf("Position is now: %d\n", inode_array[inode_index]->size);
    return 1;
}

int delete_file(char *name)
{
    fserror = FS_NONE;
    short success = file_exists(name);
    if (!success)
    {
        fserror = FS_FILE_NOT_FOUND;
        return 0;
    }
    short directory_index = get_directory_index(name);
    short directory_block = get_directory_block(name);
    short inode_index = get_inode_index(name);
    short i_block = get_inode_block(directory_array[directory_index]->inode_id);

    read_sd_block(inode_array, i_block);
    read_sd_block(directory_array, directory_block);

    inode_bitmap[inode_index] = 0;
    data_bitmap[directory_index] = 0;
    free(inode_array[inode_index]);
    free(directory_array[directory_index]);
    write_sd_block(inode_array, i_block);
    write_sd_block(directory_array, directory_block);
    write_sd_block(inode_bitmap, INODE_BITMAP_BLOCK);
    write_sd_block(data_bitmap, DATA_BITMAP_BLOCK);
    printf("DELETE: File{%s} has been deleted\n", name);
    return 1;
}

unsigned long file_length(File file)
{
    short success = file_exists(file->fsname);
    if (!success)
    {
        fserror = FS_FILE_NOT_FOUND;
        return 0;
    }
    short inode_index = get_inode_index(file->fsname);
    short i_block = get_inode_block(file->inode_id);

    return inode_array[i_block]->size;
}

int file_exists(char *name)
{
    // printf("being called\n");
    fserror = FS_NONE;
    // int ret = -1;
    int potential_block = FIRST_DIR_ENTRY_BLOCK;
    while (potential_block <= LAST_DIR_ENTRY_BLOCK)
    {
        read_sd_block(directory_array, potential_block);
        for (int i = 0; i < DIR_ENTRIES_PER_BLOCK * (LAST_DIR_ENTRY_BLOCK - FIRST_DIR_ENTRY_BLOCK); i++)
        {
            // printf("here\n");
            if (directory_array[i] != NULL && strcmp(directory_array[i]->fsname, name) == 0)
            {
                // printf("Exist: File{%s} exist\n", directory_array[i]->fsname);
                write_sd_block(directory_array, potential_block);
                return 1;
            }
        }
        potential_block++;
    }
    printf("NON_EXIST: File{%s} NOT FOUND!", name);
    fserror = FS_FILE_NOT_FOUND;
    return 0;
}
