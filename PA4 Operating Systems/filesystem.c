# include <stdio.h>;
#include "filesystem.h"
#include "softwaredisk.h"
#include "softwaredisk.c"
#include <stdint.h>


//
// Created By mimic on 11/15/2023.
//
// copied from her word document
#define MAX_FILES                   512                       
#define DATA_BITMAP_BLOCK           0       // max SOFTWARE_DISK_BLOCK_SIZE*8 bits
#define INODE_BITMAP_BLOCK          1       // max SOFTWARE_DISK_BLOCK_SIZE*8 bits
#define FIRST_INODE_BLOCK           2
#define LAST_INODE_BLOCK            5       // 128 inodes per blocks,
                                            // max of 4*128 = 512 inodes, thus 512 files
#define INODES_PER_BLOCK            128     
#define FIRST_DIR_ENTRY_BLOCK       6
#define LAST_DIR_ENTRY_BLOCK        69
#define DIR_ENTRIES_PER_BLOCK       8       // 8 DE per block                                                   // max 0f 64*8 = 512 Directory entries corresponding to max file for single-level directory structure
#define FIRST_DATA_BLOCK            70  
#define LAST_DATA_BLOCK             4095
#define MAX_FILENAME_SIZE           507//I need 5 bytes for the extension
#define NUM_DIRECT_INODE_BLOCKS     13
#define NUM_SINGLE_INDIRECT_BLOCKS   (SOFTWARE_DISK_BLOCK_SIZE/sizeof(uint16_t))     //(SOFTWARE_DISK_BLOCK_SIZE / sizeof(uint16_t))
#define MAX_FILE_SIZE   (NUM_DIRECT_INODE_BLOCKS + NUM_SINGLE_INDIRECT_BLOCKS) * SOFTWARE_DISK_BLOCK_SIZE
#define MAX_FILE_NAME_LENGTH        256
FSError fserror;


typedef struct Inode{
 uint64_t size;
    uint16_t undirect[NUM_DIRECT_INODE_BLOCKS];
    uint16_t indirect;
} Inode;

typedef struct InodeBlock {
 // this is this or 
 Inode inodes[SOFTWARE_DISK_BLOCK_SIZE / sizeof(Inode)];
 // Inode inode[SOFTWARE_DISK_BLOCK_SIZE];
}InodeBlock;

typedef struct InodeDirectBlock{
     uint16_t data_blocks[NUM_DIRECT_INODE_BLOCKS];
}InodeDirectBlock;

typedef struct DirEntry{
char fsname[MAX_FILE_NAME_LENGTH];
uint16_t num_inode;
}DirEntry;

typedef struct DirEntryBlock{
//this is this or
DirEntry entries[SOFTWARE_DISK_BLOCK_SIZE / sizeof(DirEntry)];
//DirEntry entries[DIR_ENTRIES_PER_BLOCK];
}DirEntryBlock;

typedef struct FreeBitMap{
 uint8_t bits[SOFTWARE_DISK_BLOCK_SIZE];
}FreeBitMap;

//basically FileInternal
typedef struct Files{
char filename[MAX_FILENAME_SIZE];  // filename of the file
    Inode inode;                       // inode structure for file metadata
    unsigned long file_position;       // current file position in bytes
    FileMode mode;                     // access mode (READ_ONLY or READ_WRITE)
}Files;
