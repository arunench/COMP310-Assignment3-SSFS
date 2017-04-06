//Arunen Chellan - 260636717
#include "sfs_api.h"

#define SSFS_NUM_INODE_BLOCKS (SSFS_NUM_INODES * 64) / SSFS_BLOCK_SIZE + 2 //the j-node has 14 direct pointers 

superblock_t superblock; // create superblock
inode_t inode_table[SSFS_NUM_INODES]; // create inode table
directory_t root_dir[SSFS_NUM_INODES]; // make the root directory the size of all the inodes
file_descriptor_t fd_table[SSFS_NUM_INODES]; // create the file descriptor table 
unsigned char freebitmap[SSFS_NUM_BLOCKS-2]; // 1026 blocks - 1 FBM - 1 Superblock = 1024 datablocks. Bit value 1 indicates the data block is unused and 0 indicates used
int fbm_index = 0;
int NUM_ROOTDIR_BLOCKS = (sizeof(root_dir)/SSFS_BLOCK_SIZE) + 1;


// Create the file system 
void mkssfs(int fresh){

	if (fresh){

        init_fresh_disk(SSFS_NAME, SSFS_BLOCK_SIZE, SSFS_NUM_BLOCKS);

        for (int i = 0; i < SSFS_NUM_BLOCKS-2; i++){
            freebitmap[i] = 1; //set all the blocks to unused
        }

        // initialize superblock
        superblock.magic = SSFS_MAGIC_NUM;
        superblock.block_size = SSFS_BLOCK_SIZE;
        superblock.fs_size = SSFS_NUM_BLOCKS * SSFS_BLOCK_SIZE;
        superblock.root_inode_num = 0;
        superblock.inode_table_len = SSFS_NUM_INODES;
        superblock.jnode.size = 0; //indicates used
        
        //Superblock takes up block 0, 1st block
        //Superblock is not tracked by the FBM
        superblock_t* superblock_buffer = malloc(SSFS_BLOCK_SIZE);
        memcpy(superblock_buffer, &superblock, sizeof(superblock_t));
        // Write the superblock to the blocks, start at address 0
        // Inserting only 1 block
        // pass the superblock bufferr
        write_blocks(0, 1, superblock_buffer);
        printf("Writing the filesystem to the disk emulator...\n");
        read_blocks(0, 1, superblock_buffer);
        //printf("This is the block size, being read from the block you wrote to: %i\n", superblock_buffer->root_inode_num);
        printf("The superblock was successfully written to the disk!\n");
        free(superblock_buffer);

        //printf("Size checker: %d\n", NUM_ROOTDIR_BLOCKS);

        // initialize root directory 
        // Make the file names empty strings
        // Make the inode index 0
        for (int i = 0; i < SSFS_NUM_INODES - 1; i++){
            root_dir[i].filename = "";
            root_dir[i].inode_num = -1; //equivalent to NULL
        }

        // Initialize the inode table
        // inode has a size indicator; -1 unused, 0 used
        for (int j = 0; j < SSFS_NUM_INODES - 1; j++){
            inode_table[j].size = -1;  
        }
        // Initialize the file descriptor table
        // Make the inode index -1
        for (int k = 0; k < SSFS_NUM_INODES - 1; k++){
            fd_table[k].inode_num = -1;  //equivalent to NULL
            fd_table[k].read_ptr = 0;
            fd_table[k].write_ptr = 0;
        }
        //Add jnode to file descriptor, root nood has inode position 0. jnode = rootnode
        root_dir[0].filename = "arunenroot";
        root_dir[0].inode_num = 0;
        fd_table[0].inode_num = 0; 
        fd_table[0].read_ptr = 0;
        fd_table[0].write_ptr = 0;
        inode_table[0].size = 0; //inode 0 is now used

        // Copy the root directory into block 1 on the disk
        directory_t* root_dir_buffer = malloc(sizeof(root_dir));
        memcpy(root_dir_buffer, &root_dir, sizeof(root_dir));
        write_blocks(1, NUM_ROOTDIR_BLOCKS, root_dir_buffer);
        read_blocks(1, NUM_ROOTDIR_BLOCKS, root_dir_buffer);
        //printf("This is the content of the block you wrote to %s\n", root_dir_buffer[1].filename); 
        printf("The root directory was successfully written to the disk!\n");               
        free(root_dir_buffer);
        // The root takes up 4 blocks, it is tracked by the FBM
        // The freebitmap has 1024 entries, bit value 1 indicates the data block is unused and 0 indicated used
        for(int i = 0; i < NUM_ROOTDIR_BLOCKS; i++){
            freebitmap[i] = 0;
        }


        // Write the 14 inode blocks on the disk
        inode_t* inode_table_buffer = malloc(sizeof(inode_table));
        memcpy(inode_table_buffer, &inode_table, sizeof(inode_table));
        write_blocks(1+NUM_ROOTDIR_BLOCKS, SSFS_NUM_INODE_BLOCKS, inode_table_buffer);
        inode_t* inode_table_buffer_read = malloc(sizeof(inode_table));;
        read_blocks(1+NUM_ROOTDIR_BLOCKS, SSFS_NUM_INODE_BLOCKS, inode_table_buffer_read);
        //printf("This is the content of the block you wrote to %d\n", inode_table_buffer_read[1].size);          
        printf("The inode table was successfully written to the disk!\n");  
        free(inode_table_buffer);
        free(inode_table_buffer_read);

        // This puts us at freebitmap[13] = 0
        // inode blocks are tracked by the FBM
        for(int i = NUM_ROOTDIR_BLOCKS; i < SSFS_NUM_INODE_BLOCKS + NUM_ROOTDIR_BLOCKS; i++){
            freebitmap[i] = 0;
        }
        // 1026(the total number of blocks) - 1(the superblock) - 4(the number of blocks for the root_dir) - 14(the number of inode blocks) - 1(FBM) 
        // The FBM is not tracked by the FBM
        int data_blocks_count = (1026 - 1 - 4 - 14 - 1);
        //printf("Size checker: %i\n", sizeof(freebitmap)/sizeof(freebitmap[0]));
        // Write the FBM to the blocks on the disk
        unsigned char* freebitmap_buffer = malloc(sizeof(freebitmap));
        memcpy(freebitmap_buffer, &freebitmap, sizeof(freebitmap));
        write_blocks(1 + NUM_ROOTDIR_BLOCKS + SSFS_NUM_INODE_BLOCKS + data_blocks_count, 1, freebitmap_buffer);
        unsigned char* freebitmap_buffer_read = malloc(sizeof(freebitmap));
        read_blocks(1 + NUM_ROOTDIR_BLOCKS + SSFS_NUM_INODE_BLOCKS + data_blocks_count, 1, freebitmap_buffer_read);
        // printf("This is the content of the block you wrote to %d\n", freebitmap_buffer_read[2]);         
        printf("The FBM table was successfully written to the disk!\n");
        free(freebitmap_buffer);
        free(freebitmap_buffer_read);

  



    }

    else{

        for (int k = 0; k < SSFS_NUM_INODES - 1; k++){
            fd_table[k].inode_num = -1;  
            fd_table[k].read_ptr = 0;
            fd_table[k].write_ptr = 0;
        }

        // Retrieve the superblock data from the first block
        superblock_t* superblock_buffer = malloc(SSFS_BLOCK_SIZE);
        read_blocks(0, 1, superblock_buffer);
        memcpy(&superblock, superblock_buffer, sizeof(superblock_t));        
        printf("This is the block size, being read from the block you wrote to: %i\n", superblock.root_inode_num);
        free(superblock_buffer);

        // Retrieve the root directory data
        directory_t* root_dir_buffer = malloc(sizeof(root_dir));
        read_blocks(1, NUM_ROOTDIR_BLOCKS, root_dir_buffer);
        memcpy(&root_dir, root_dir_buffer, sizeof(root_dir));        
        printf("This is the content of the block you wrote to %s\n", root_dir[1].filename);        
        free(root_dir_buffer);

        // Retrieve the inode table data
        inode_t* inode_table_buffer = malloc(sizeof(inode_table));
        read_blocks(1+NUM_ROOTDIR_BLOCKS, SSFS_NUM_INODE_BLOCKS, inode_table_buffer);
        memcpy(&inode_table, inode_table_buffer, sizeof(inode_table));        
        printf("This is the content of the block you wrote to %d\n", inode_table[1].size);          
        free(inode_table_buffer);

        int data_blocks_count = 1026 - 1 - NUM_ROOTDIR_BLOCKS - 14 - 1;
        unsigned char* freebitmap_buffer = malloc(sizeof(freebitmap));
        read_blocks(1 + NUM_ROOTDIR_BLOCKS + SSFS_NUM_INODE_BLOCKS + data_blocks_count, 1, freebitmap_buffer);
        memcpy(&freebitmap, freebitmap_buffer, sizeof(freebitmap));        
        printf("This is the content of the block you wrote to %d\n", freebitmap[2]);         
        free(freebitmap_buffer);

        // Check how many files are in the file system
        int file_count = 0;
        // Start from inode 1 since inode 0 is the root
        for (int i = 1; i < SSFS_NUM_INODES; i++){
            // If the inode has a used indicator, update our file count
            if(inode_table[i].size == 0){
                file_count++;
            }
        }
        printf("There are %d files in the filesystem!\n", file_count);

        int free_blocks = 0;
        for (int i = 0; i < SSFS_NUM_BLOCKS; i++){
            if(freebitmap[i] == 1){
                free_blocks++;
            }
        }
        printf("There are %d free blocks to write to in the filesystem!\n", free_blocks);


    }
}
// NOT NEEDED
int ssfs_get_next_file_name(char *fname){
    return 0;
}
// NOT NEEDED
int ssfs_get_file_size(char* path){
    return 0;
}

directory_t add_file(char* filename){
    printf("Your file will now be added to the root directory and assigned an inode number\n");

    // Create a new directory entry type
    directory_t new_entry; 
    new_entry.filename = filename;
    printf("This is the filename we assigned to the new_entry: %s\n", new_entry.filename);

    // Create an inode associated with the file    
    inode_t new_inode;
    new_inode.size = 0;
    int inode_table_index = find_unused_inode_table_index();
    int rootdir_index = find_unused_root_dir_index();

    // Assign the new inode an available inode number
    new_entry.inode_num = inode_table_index;
    // Add that inode to the inode table
    inode_table[inode_table_index].size = 0;
    // Add the directory entry to the next available position in the root directory
    root_dir[rootdir_index] = new_entry;
    printf("The file %s has been added to the inode table at index %d and at index %d in the root directory\n", new_entry.filename, inode_table_index, rootdir_index);

    // Re copy the root directory to the disk
    directory_t* root_dir_buffer = malloc(sizeof(root_dir));
    memcpy(root_dir_buffer, &root_dir, sizeof(root_dir));
    write_blocks(1, NUM_ROOTDIR_BLOCKS, root_dir_buffer);   
    directory_t* root_dir_buffer_read = malloc(sizeof(root_dir));
    read_blocks(1, NUM_ROOTDIR_BLOCKS, root_dir_buffer_read);
    printf("The file %s was successfully copied to the disk\n", root_dir_buffer_read[rootdir_index].filename);           
    free(root_dir_buffer);
    free(root_dir_buffer_read);

    // Re copy the inode table to the disk
    inode_t* inode_table_buffer = malloc(sizeof(inode_table));
    memcpy(inode_table_buffer, &inode_table, sizeof(inode_table));
    write_blocks(1+NUM_ROOTDIR_BLOCKS, SSFS_NUM_INODE_BLOCKS, inode_table_buffer);
    inode_t* inode_table_buffer_read = malloc(sizeof(inode_table));;
    read_blocks(1+NUM_ROOTDIR_BLOCKS, SSFS_NUM_INODE_BLOCKS, inode_table_buffer_read);
    printf("The inode number %d was successfully copied to the disk, it is now in use, it's size is now %d\n", inode_table_index, inode_table_buffer_read[inode_table_index].size);       
    free(inode_table_buffer);
    free(inode_table_buffer_read);

    // Return the directory entry
    return new_entry;


}

// Find the next available inode index
int find_unused_inode_table_index(){

	int inode_table_index;
    // Find the inode index where you can insert this inode into the inodetable
    for (int i = 0; i < SSFS_NUM_INODES; i++){         
        if(inode_table[i].size == -1){
            inode_table_index = i;
            break;
        }
    }

    return inode_table_index;

}
// Find the next available root directory index
int find_unused_root_dir_index(){

    int rootdir_index;
    for (int i = 0; i < SSFS_NUM_INODES; i++){
        
        int compare = strcmp(root_dir[i].filename, "");
        if (compare == 0){        
            rootdir_index = i;
            break;
        }
    }    	

    return rootdir_index;

}

// Find the next available index in the file descriptor table
int find_unused_file_descp_index(){

    int file_descriptor_index;
    for (int i = 0; i < SSFS_NUM_INODES; i++){

        // If the inode number is -1 that means it is free
        if (fd_table[i].inode_num == -1){        
            file_descriptor_index = i;
            break;
        }
    }    	

    return file_descriptor_index;

}

int search_root_dir(char* filename){

		int file_exists = 0;
		int rootdir_index;
	   	for(int i = 0; i < SSFS_NUM_INODES - 1; i++){
        
        char* filename_buffer = strdup(root_dir[i].filename);
        int compare = strcmp(filename_buffer, filename);
        if (compare == 0){

            rootdir_index = i;
            printf("%d\n", i);
            printf("Found your file: %s, in the root directory at index %d\n", filename, rootdir_index);
            file_exists = 1;
            break;
        }
        
        
        else{
            free(filename_buffer);
        }
    }

    return file_exists;

}
// Opens a file and returns an integer that corresponds to the index of the entry
// for the newly opened file in the file descriptor table.
// If the file does not exist, it creates a new file and sets its size to 0.
// If the file exists, the file is opened in append mode (set the write file pointer to the end of the file, read to the begining)
int ssfs_fopen(char *name){

    // If the file name is larger than the max file name of 16 characters
    if (strlen(name) > SSFS_MAX_FILENAME){
        printf("Your file is too long!!!\n");
        return -1;
    }

    // If it is 16 characters and under, continue
    // Find out if the file exists, if not create it

    int file_exists = 0;
    int rootdir_index;
    directory_t file;
        
    printf("Let's see if the file titled %s exists or must be created\n", name);
    // Find if the file is already in the root_dir
   	for(int i = 0; i < SSFS_NUM_INODES - 1; i++){
        
        char* filename_buffer = strdup(root_dir[i].filename);
        int compare = strcmp(filename_buffer, name);
        if (compare == 0){

            rootdir_index = i;
            printf("Found your file: %s, in the root directory at index %d\n", name, rootdir_index);
            file_exists = 1;
            file = root_dir[i];
            break;
        }
        
        
        else{
            free(filename_buffer);
        }
    }

    if (file_exists == 0){
        printf("Your file: %s, does not exist, must be added to the root directory\n", name);
        file = add_file(name);
    }

    if (file_exists == 1){

    }

	int file_descriptor_index = find_unused_file_descp_index();
	fd_table[file_descriptor_index].inode_num = file.inode_num;
	fd_table[file_descriptor_index].read_ptr = 0;
	fd_table[file_descriptor_index].write_ptr = 0;

    return file_descriptor_index;
}

int ssfs_fclose(int fileID){
    return 0;
}

int ssfs_frseek(int fileID, int loc){
    return 0;
}

int ssfs_fwseek(int fileID, int loc){
    return 0;
}

int ssfs_fwrite(int fileID, char *buf, int length){
    return 0;
}

int ssfs_fread(int fileID, char *buf, int length){
    return 0;
}

int ssfs_remove(char *file){
    return 0;
}

int main(int argc, char *argv[]){
    mkssfs(1);
    // Adding a file for testing     
    int fd_index = ssfs_fopen("arunen");
    printf("The fd_index for arunen is %d\n", fd_index);  
    return 0;
}
