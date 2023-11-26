#include <stdio.h>
#include <stdlib.h>
#include "softwaredisk.h"
#include "filesystem.h"
int main(int argc, char *argv[]) {
        init_software_disk();
    // printf("checking data structure sizes and alignments\n");
    // if(check_structure_alignment() != 1){
    //     printf("check failed Filesystem not initialized and should not be used\n");

    // }
    // else{
    //     printf("check succeeded\n");
    //     printf("initializing filesystem\n");
    //     init_software_disk();
    //     printf("Done.\n");
    // }
    return 0;
}