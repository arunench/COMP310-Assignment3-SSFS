//Arunen Chellan - 260636717
#include "sfs_api.h"
#include <math.h>
#define SSFS_NUM_INODE_BLOCKS (SSFS_NUM_INODES * 64) / SSFS_BLOCK_SIZE + 2 //the j-node has 14 direct pointers 

superblock_t superblock; // create superblock
inode_t inode_table[SSFS_NUM_INODES]; // create inode table
directory_t root_dir[SSFS_NUM_INODES]; // make the root directory the size of all the inodes
file_descriptor_t fd_table[SSFS_NUM_INODES]; // create the file descriptor table 
unsigned char freebitmap[SSFS_NUM_BLOCKS-2]; // 1026 blocks - 1 FBM - 1 Superblock = 1024 datablocks. Bit value 1 indicates the data block is unused and 0 indicates used
int fbm_index = 0;
int NUM_ROOTDIR_BLOCKS = (sizeof(root_dir)/SSFS_BLOCK_SIZE) + 1;
int data_blocks_count = (1026 - 1 - 4 - 14 - 1);

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
        printf("The superblock was successfully written to the disk!\n");
        free(superblock_buffer);


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
        printf("The inode table was successfully written to the disk!\n");  
        free(inode_table_buffer);

        // This puts us at freebitmap[13] = 0
        // inode blocks are tracked by the FBM
        for(int i = NUM_ROOTDIR_BLOCKS; i < SSFS_NUM_INODE_BLOCKS + NUM_ROOTDIR_BLOCKS + 1; i++){
            freebitmap[i] = 0;
        }

        // 1026(the total number of blocks) - 1(the superblock) - 4(the number of blocks for the root_dir) - 14(the number of inode blocks) - 1(FBM) 
        // The FBM is not tracked by the FBM
        int data_blocks_count = (1026 - 1 - 4 - 14 - 1);

        // Write the FBM to the blocks on the disk
        unsigned char* freebitmap_buffer = malloc(sizeof(freebitmap));
        memcpy(freebitmap_buffer, &freebitmap, sizeof(freebitmap));
        write_blocks(1 + NUM_ROOTDIR_BLOCKS + SSFS_NUM_INODE_BLOCKS + data_blocks_count, 1, freebitmap_buffer);     
        printf("The FBM table was successfully written to the disk!\n");
        free(freebitmap_buffer);


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

        // Retrieve the freebitmap from the disk
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
        for (int i = 0; i < 14; i++){

        	superblock.jnode.direct[i] = 1 + NUM_ROOTDIR_BLOCKS + i;

        }

    }
    return;
}
// NOT NEEDED
int ssfs_get_next_file_name(char *fname){
    return 0;
}
// NOT NEEDED
int ssfs_get_file_size(char* path){
    return 0;
}

// A helper function i created to add a file to the root directory
// I give it an inode number and set it's size to 0
// I then recopy the tables back to the disk
directory_t add_file(char* filename){

    // Create a new directory entry type
    directory_t new_entry; 
    new_entry.filename = filename;

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

    // Re copy the root directory to the disk
    directory_t* root_dir_buffer = malloc(sizeof(root_dir));
    memcpy(root_dir_buffer, &root_dir, sizeof(root_dir));
    write_blocks(1, NUM_ROOTDIR_BLOCKS, root_dir_buffer);             
    free(root_dir_buffer);


    // Re copy the inode table to the disk
    inode_t* inode_table_buffer = malloc(sizeof(inode_table));
    memcpy(inode_table_buffer, &inode_table, sizeof(inode_table));
    write_blocks(1+NUM_ROOTDIR_BLOCKS, SSFS_NUM_INODE_BLOCKS, inode_table_buffer);      
    free(inode_table_buffer);


    // Return the directory entry
    return new_entry;


}


// A helper function i made
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

// A helper function I made
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

// A helper function I made
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
// A helper function I made
// Find the next available index in the freebitmap index
int find_free_fbm_index(){
	
	int fbm_index = 0;

	for (int i = 0; i < SSFS_NUM_BLOCKS-2; i++){

		if(freebitmap[i] == 1){
			fbm_index = i;
			break;
		}
		
	}

	return fbm_index;

}

// Search's root directory and returns the index the given filename is at
int search_root_dir(char* filename){

	int rootdir_index;
	
	for(int i = 0; i < SSFS_NUM_INODES - 1; i++){
        
        char* filename_buffer = strdup(root_dir[i].filename);
        int compare = strcmp(filename_buffer, filename);
      
        if (compare == 0){

            rootdir_index = i;
            printf("Found your file: %s, in the root directory at index %d\n", filename, rootdir_index);
            free(filename_buffer);
            break;
        }
        
        
        else{
            free(filename_buffer);
        }
    }

    return rootdir_index;

}
// Out of the 14 blocks the inode can point to, this helper function returns
// The direct pointer index where data was writting to last
// i.e. for inode3, it will return 3, meaning inode3.direct[3] = last block that was written to
int find_last_written_directpointer(inode_t inode){

	int last_written_directpointer;

	for (int i = 0; i < 14; i++){
		if(inode.direct[i] == NULL){
			last_written_directpointer = i-1;
			break;
		}
	}

	return last_written_directpointer;

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

    // If the file exists and already has a file descriptor index, return that file descriptor index 
    if (file_exists == 1){

	    int file_descriptor_index = find_unused_file_descp_index();
		printf("File descriptor index%d\n", file_descriptor_index);
		fd_table[file_descriptor_index].inode_num = file.inode_num;
		fd_table[file_descriptor_index].read_ptr = 0;
		fd_table[file_descriptor_index].write_ptr = 0;

	    return file_descriptor_index;

    }

    // If the file does not exist, create it and assign it a file descriptor index for the file 
    if (file_exists == 0){

    file = add_file(name);

    int file_descriptor_index = find_unused_file_descp_index();
	printf("File descriptor index for the file titled %s is %d\n", file.filename, file_descriptor_index);
	fd_table[file_descriptor_index].inode_num = file.inode_num;
	fd_table[file_descriptor_index].read_ptr = 0;
	fd_table[file_descriptor_index].write_ptr = 0;

    return file_descriptor_index;

    }




}

int ssfs_fclose(int fileID){

	file_descriptor_t close_file;

	if (fileID >= 0 && fileID < SSFS_NUM_INODES){

		
		if (fd_table[fileID].inode_num == -1){
			printf("The file is already closed\n");
			return -1; // The file is already closed
		}

		// If the file is not closed, close it

		fd_table[fileID].inode_num = -1;
		fd_table[fileID].read_ptr = 0;
		fd_table[fileID].write_ptr = 0;
		printf("fileID %d is now closed\n", fileID);
		return 0;

	}

    return -1;
}

int ssfs_frseek(int fileID, int loc){

	if (fileID > SSFS_NUM_INODES){
		return -1;
	}

	if (fd_table[fileID].inode_num == -1){
		return -1;
	}

	if (loc < 0){
		return -1;
	}

	if (loc > 14335){
		return -1;
	}
	if (loc > inode_table[fd_table[fileID].inode_num].size){
		return -1;
	}

	fd_table[fileID].read_ptr = loc;

    return 0;
}

int ssfs_fwseek(int fileID, int loc){

	if (fileID > SSFS_NUM_INODES){
		return -1;
	}

	if (fd_table[fileID].inode_num == -1){
		return -1;
	}

	if (loc < 0){
		return -1;
	}

	if (loc > 14335){
		return -1;
	}
	if (loc > inode_table[fd_table[fileID].inode_num].size){
		return -1;
	}
	fd_table[fileID].write_ptr = loc;

    return 0;
}



int ssfs_fwrite(int fileID, char *buf, int length){

	printf("Trying access file_descriptor %d\n", fileID);
	if (fileID > SSFS_NUM_BLOCKS-2) {return -1;}
    // If the inode does not exist in the file descriptor table, return and error
    if(fd_table[fileID].inode_num == -1){

        return -1;
    }

    // Get the file descriptor entry
    file_descriptor_t file_to_write = fd_table[fileID];

    // Get the corresponding inode
    inode_t fileinode = inode_table[file_to_write.inode_num];

    // Find how many data blocks the buffer will require
	int blocksneeded;

	
    // Write to a newly created file and the file size is under 1024 bytes
    if (fileinode.size == 0){

    	blocksneeded = length/SSFS_BLOCK_SIZE + 1;
    	int beginIndex = 0;
    	int block_index;
    	int inode_direct_index;
	    printf("Wrting a file of size %zu bytes to the disk, we will need to allocate %d blocks.\n", strlen(buf), blocksneeded);
    	for (int i = 0; i < blocksneeded; i++){

    		
	    	char* buffer = malloc(SSFS_BLOCK_SIZE);
	    	block_index = find_free_fbm_index();
	    	memcpy(buffer, buf + beginIndex, SSFS_BLOCK_SIZE);
	    	if(strlen(buffer) == 0){break;}
		    if(strlen(buffer) == 1026){ 
		        	//NULL TERMINATE
	    			buffer[1025] ='\0';
	    			buffer[1024] = '\0'; 
    		}	    	

	    	write_blocks(block_index, 1, buffer);
	    	
	    	freebitmap[block_index] = 0;
	    	fileinode.direct[i] = block_index;
	    	inode_direct_index = i;
	    	file_to_write.write_ptr = strlen(buffer) + inode_direct_index*SSFS_BLOCK_SIZE;
	    	fileinode.size = strlen(buffer) + fileinode.size;
	    	printf("Writing... Successfully wrote [%d bytes] to block number %d!\n", strlen(buffer), block_index);
	    	beginIndex = beginIndex + SSFS_BLOCK_SIZE;
	    	free(buffer);
    	}
    	int direct_block_index = (int)(file_to_write.write_ptr/ SSFS_BLOCK_SIZE);
    	int write_left_off = file_to_write.write_ptr%SSFS_BLOCK_SIZE;
    	int bytes_can_write = SSFS_BLOCK_SIZE - write_left_off; 
    	int lastindex;
    	printf("This file now has a size of %d bytes and the write pointer is at block byte %d of 14335.\n", fileinode.size, file_to_write.write_ptr);
    	for (int i = 0; i < 14; i++){
    		if(fileinode.direct[i] == '\0'){break;}
    		printf("fileinode.direct[%d] is pointing to block %d\n", i, fileinode.direct[i]);
    		lastindex = i;
    	}
    	
		printf("Block %d has %d bytes of data in it, therefore we can write %d more bytes to it.\n", fileinode.direct[lastindex], write_left_off, bytes_can_write);  

    }

    // Write to an existing file but the total length is under 1024
    // Case 2 
    else if (fileinode.size != 0){
    	printf("Write pointer is now at %d\n", file_to_write.write_ptr);
    	int direct_block_index = (int)(file_to_write.write_ptr/ SSFS_BLOCK_SIZE);
    	if (fileinode.direct[direct_block_index] == 0){
    		int free_block = find_free_fbm_index();
    		fileinode.direct[direct_block_index] = free_block;
    		freebitmap[free_block] = 0;
    	}

    	printf("fileinode.direct[%d] = %d\n", direct_block_index, fileinode.direct[direct_block_index]);

    	int write_left_off = file_to_write.write_ptr%SSFS_BLOCK_SIZE;

    	int bytes_can_write = SSFS_BLOCK_SIZE - write_left_off; 

    	char* read_buffer = malloc(SSFS_BLOCK_SIZE + length);


        read_blocks(fileinode.direct[direct_block_index], 1, read_buffer);
        strcat(read_buffer, buf);

        blocksneeded = strlen(read_buffer)/SSFS_BLOCK_SIZE + 1;

    	int beginIndex = 0;

    	int inode_direct_index;

    	if (length + fileinode.size > 14335){
    		printf("You are trying to write too much data to the file. it only has a maximum capacity of 14336 bytes\n");
    		return -1;
    	}
		fileinode.size = length + fileinode.size;

		printf("Writing a file size of %zu bytes to the disk, we need to allocate %d blocks.\n", strlen(read_buffer), blocksneeded);

    	for (int i = 0; i < blocksneeded; i++){

	    	char* buffer = malloc(SSFS_BLOCK_SIZE);
	    	memcpy(buffer, read_buffer + beginIndex, SSFS_BLOCK_SIZE);
	    	
	    	if(strlen(buffer) == 0){break;}
	    	buffer[SSFS_BLOCK_SIZE] = '\0';
		    if(strlen(buffer) == 1026){ 
		        	//NULL TERMINATE
	    			buffer[1025] ='\0';
	    			buffer[1024] = '\0'; 
    		}	

	    	write_blocks(fileinode.direct[direct_block_index], 1, buffer);
	    	file_to_write.write_ptr = strlen(buffer) + direct_block_index*SSFS_BLOCK_SIZE;  
	    	char* read_buffer = malloc(SSFS_BLOCK_SIZE);
	    	read_blocks(fileinode.direct[direct_block_index], 1, read_buffer);
	    	printf("This is the content written to block %d: %s", fileinode.direct[direct_block_index], read_buffer);  	
	    	printf("The write pointer is at block byte %d of 14335.\n", file_to_write.write_ptr);
	    	if(strlen(buffer)<1024){break;}
	    	int sum = find_free_fbm_index();
	    	printf("The next available block is number %d\n", sum);
	    	direct_block_index = direct_block_index + 1;
	    	fileinode.direct[direct_block_index] = sum;
	    	printf("fileinode.direct[%d] = %d\n", direct_block_index, fileinode.direct[direct_block_index]);
	    	freebitmap[fileinode.direct[direct_block_index]] = 0;
	    	beginIndex = beginIndex + SSFS_BLOCK_SIZE;

	    	free(buffer);
    	}
    	int lastindex;
    	printf("This file now has a size of %d bytes and the write pointer is at block byte %d of 14335.\n", fileinode.size, file_to_write.write_ptr);
    	for (int i = 0; i < 14; i++){
    		if(fileinode.direct[i] == '\0'){break;}
    		printf("fileinode.direct[%d] is pointing to block %d\n", i, fileinode.direct[i]);
    		lastindex = i;
    	}
    	write_left_off = file_to_write.write_ptr%SSFS_BLOCK_SIZE;
    	bytes_can_write = SSFS_BLOCK_SIZE - write_left_off; 
		printf("Block %d has %d bytes of data in it, therefore we can write %d more bytes to it.\n", fileinode.direct[lastindex], write_left_off, bytes_can_write);     
		//free(read_buffer);
    }


    fd_table[fileID] = file_to_write;
    inode_table[file_to_write.inode_num] = fileinode;

    // Re copy the inode table to the disk
    inode_t* inode_table_buffer = malloc(sizeof(inode_table));
    memcpy(inode_table_buffer, &inode_table, sizeof(inode_table));
    write_blocks(1+NUM_ROOTDIR_BLOCKS, SSFS_NUM_INODE_BLOCKS, inode_table_buffer);       
    free(inode_table_buffer);

    // Re copy the FBM to the blocks on the disk
    unsigned char* freebitmap_buffer = malloc(sizeof(freebitmap));
    memcpy(freebitmap_buffer, &freebitmap, sizeof(freebitmap));
    write_blocks(1 + NUM_ROOTDIR_BLOCKS + SSFS_NUM_INODE_BLOCKS + data_blocks_count, 1, freebitmap_buffer);
    free(freebitmap_buffer);


    return length;
}

int ssfs_fread(int fileID, char *buf, int length){

   	file_descriptor_t file_to_read = fd_table[fileID];
    inode_t fileinode = inode_table[file_to_read.inode_num];

    // If the inode does not exist in the file descriptor table, return and error
    if(fd_table[fileID].inode_num == -1){
        return -1;
    }

    if (length == 0){
    	return -1;
    }

    

 	
    int block_index;
    int beginIndex = 0;
    char* temp = malloc(14335);
	int direct_block_index = (int)(file_to_read.read_ptr/ SSFS_BLOCK_SIZE);
	int blocksneeded;
    int write_left_off = file_to_read.read_ptr%SSFS_BLOCK_SIZE;
    int bytes_can_write = SSFS_BLOCK_SIZE - write_left_off; 

    int read = length;
    if (fileinode.size != 0){
    	// Read the content at each direct pointer
    	for (int i = 0; i < 14; i++){

    		if (fileinode.direct[i] == '\0'){break;}
    		char* read_buffer = malloc(SSFS_BLOCK_SIZE);
    		//printf("Block %d contains the following string %s\n", fileinode.direct[i]);
    		read_blocks(fileinode.direct[i], 1, read_buffer);
    		//strcat(temp, read_buffer);
    		//Please comment the following line when running the MANUAL tests
    		memcpy(buf, read_buffer, length);
    		printf("Block %d contains the following string %s\n", fileinode.direct[i], read_buffer);
    		file_to_read.read_ptr = strlen(read_buffer) + i*SSFS_BLOCK_SIZE;
    		free(read_buffer);

    	}
 // Attempt at reading mutiple block algorithm for a specific read_ptr

    	// blocksneeded = length/SSFS_BLOCK_SIZE + 1;
    	// int start = 0;
    	// int toread = length;

    	// for (int i = 0; i < blocksneeded; i++){

    	// 		block_index = fileinode.direct[direct_block_index];

    	// 		if(block_index == '\0'){break;}
		    	
		   //      char* read_buffer = malloc(SSFS_BLOCK_SIZE);
		   //      read_blocks(block_index, 1, read_buffer);
		   //      read_buffer[strlen(read_buffer)] = '\0';
		        
		   //      if(strlen(read_buffer) == 1026){ 
		   //      	//NULL TERMINATE
	    // 			read_buffer[1025] ='\0';
	    // 			read_buffer[1024] = '\0'; 
    	// 		}

    	// 		// if(strlen(read_buffer) == 0){break;}

    	// 		// if (strlen(buf) + strlen(read_buffer) > length){
    	// 		// 	int copyamt = length - strlen(buf);
    	// 		// 	char* newone; 
    	// 		// 	strncpy(newone, read_buffer, copyamt);
    	// 		// 	strcat(buf, newone);

    	// 		// }
    	// 		// else{
    	// 		// 	strcat(buf, read_buffer);
    	// 		// }
    	// 		//memcpy(buf+beginIndex, read_buffer, length);


    	// 		// memcpy(buf, temp, length);

    	// 		// Store all the content from the blocks
    	// 		// Then truncate later
    	// 		// if (toread < 1024){
    	// 		// 	memcpy(buf+start, read_buffer+beginIndex, toread);
    	// 		// 	file_to_read.read_ptr = strlen(read_buffer) + direct_block_index*SSFS_BLOCK_SIZE;
    	// 		// 	break;
    	// 		// }


    	// 		// else if (toread > 1024){
    	// 		// 	toread = toread - 1024;
    	// 		// 	memcpy(buf+start, read_buffer+beginIndex, 1024);
    	// 		// 	start = start + 1024;
    	// 		// 	beginIndex = beginIndex + 1024;
    	// 		// 	toread = toread - 1024;
    	// 		// }
    			
    	// 		memcpy(buf, read_buffer, length);

    	// 		// Update the read pointer
    	// 		file_to_read.read_ptr = strlen(read_buffer) + direct_block_index*SSFS_BLOCK_SIZE;
		   //      printf("Reading... Successfully read %s from block %d\n", read_buffer, block_index);
		   //      direct_block_index = direct_block_index + 1;
		   //      beginIndex = beginIndex + length;
		   //      free(read_buffer);
        	
    	// }
    	// int cutoff = strlen(temp) - length;

    	// printf("%zu\n", strlen(temp));
    	// char* substring = temp + length - cutoff;
    	// printf("%zu\n", strlen(temp));
    	// strcpy(buf, substring);
    	// printf("%zu\n", strlen(buf));
    	//collect the string from the amount of blocks that had to be read
    	//then truncate it to the proper size and 
    	//memcpy(buf, temp, length);
    }

    else if (fileinode.size == 0){
    	printf("The file is empty!\n");
    }


    fd_table[fileID] = file_to_read;


    return length;
}

int ssfs_remove(char *file){

	// Get the file's root_dir index
	int root_dir_index = search_root_dir(file);
	// Get the directory entry at that index
	directory_t directory_entry = root_dir[root_dir_index];
	// Store the inode number
	int inode_to_delete = directory_entry.inode_num;
	int block_index;

	// Set all the pointers of the corresponding inode to NULL
	// Update the free bit map to indicate that those blocks are unused
	for(int i = 0; i < 14; i++){
		freebitmap[inode_table[directory_entry.inode_num].direct[i]] = 1;
		inode_table[directory_entry.inode_num].direct[i] = NULL;
	}

	// Remove that inode from the inode table, it's size becomes -1 indicating unusedd
	inode_table[directory_entry.inode_num].size = -1; 

	// If the file wasn't closed already, remove its entry from the file descriptor and set it's pointers to 0
    	for(int i = 0; i < SSFS_NUM_INODES; i++){
    		if(fd_table[i].inode_num == inode_to_delete){
    			fd_table[i].inode_num = -1;
    			fd_table[i].read_ptr = 0;
    			fd_table[i].write_ptr = 0;
    			break;
    		}
    	}

    // Set the directory entry at that index to empty
	root_dir[root_dir_index].inode_num = -1;
	root_dir[root_dir_index].filename = "";

    // Re copy the root directory to the disk
    directory_t* root_dir_buffer = malloc(sizeof(root_dir));
    memcpy(root_dir_buffer, &root_dir, sizeof(root_dir));
    write_blocks(1, NUM_ROOTDIR_BLOCKS, root_dir_buffer);             
    free(root_dir_buffer);

    // Re copy the inode table to the disk
    inode_t* inode_table_buffer = malloc(sizeof(inode_table));
    memcpy(inode_table_buffer, &inode_table, sizeof(inode_table));
    write_blocks(1+NUM_ROOTDIR_BLOCKS, SSFS_NUM_INODE_BLOCKS, inode_table_buffer);      
    free(inode_table_buffer);

    // Re copy the FBM to the blocks on the disk
    unsigned char* freebitmap_buffer = malloc(sizeof(freebitmap));
    memcpy(freebitmap_buffer, &freebitmap, sizeof(freebitmap));
    write_blocks(1 + NUM_ROOTDIR_BLOCKS + SSFS_NUM_INODE_BLOCKS + data_blocks_count, 1, freebitmap_buffer);
    free(freebitmap_buffer);


	printf("Removing... Your file titled %s is now deleted\n", file);


    return 0;
}

// Uncomment for manual testing
// int main(int argc, char *argv[]){

//     mkssfs(1);

//     // Test case 1: opening a new file  
//     int fd_index1 = ssfs_fopen("arunen");
//     int fd_index2 = ssfs_fopen("alex");
//     int fd_index3 = ssfs_fopen("oliver");

//     // Test case 2: Write to a newly created file, string under 1024 bytes
// 	char *buffer1 = "This is a test, let's see if it works!";
// 	ssfs_fwrite(fd_index2,buffer1,strlen(buffer1));

//     // ssfs_fread(2,buffer1,strlen(buffer1));

//     // Test case 3: Write to an existing block where the file size is under 1024 bytes
//     char* appending1 = "Adding this to file 2";
//     ssfs_fwrite(fd_index2, appending1, strlen(appending1));
//     // ssfs_fread(2, appending1, strlen(appending1));
//     char* onechar = "okay";

//     // Test case 4
//     // Write more than 1024 bytes to a file, must use multiple data blocks
//     // String size: 1917
//     char* buffer2 = "Atlassian Austin, TX Dear Atlassian, I am a third-year Software Engineering (Minor in Musical Technology) student at McGill University and I am applying for the Summer 2017 Software Development Internship. I should be grateful for the opportunity as an intern because I believe this position will give me an chance to apply the programming skills that I have learned at school as well as acquire a great deal of knowledge and hands on experience. I am a quick learner and I pay attention to detail. I am passionate about programming (Java, JavaScript, Python), quality software, and music (composition and performance). I have a great deal of experience in the software development field since I did an 8-month co-op at Nuance Communications working in the Professional Services team where we deployed embedded speech dialog systems in vehicles and I performed software testing and requirements validation. I have a great interest in Software collaboration tools. Whether I am working on a project at school or work, I always find myself using software collaboration tools such as, GitHub, JIRA, Basecamp, Slack, etc, and I am very impressed with the software and business impact they make. I am greatly passionate about technology, and I aspire to work for a company that can use technology to make a powerful development impact. I enjoy approaching challenging problems and coming up with elegant solutions and I believe I will be an asset to the Atlassian team. I have heard many great things about working at Atlassian, such as a dynamic work environment, challenging problems, and great benefits. I believe my skill set matches what Atlassian is looking for. Feel free to contact me at 514-951-4128 or email me at arunen.chellan@mail.mcgill.ca if you would like to further discuss this position. I have the enthusiasm and determination to make a success of this opportunity. Thank you for your time Best, Arunen\n";
// 	ssfs_fwrite(fd_index2, buffer2, strlen(buffer2));

// 	char* copy = "Atlassian Austin, TX Dear Atlassian, I am a third-year Software Engineering (Minor in Musical Technology) student at McGill University and I am applying for the Summer 2017 Software Development Internship. I should be grateful for the opportunity as an intern because I believe this position will give me an chance to apply the programming skills that I have learned at school as well as acquire a great deal of knowledge and hands on experience. I am a quick learner and I pay attention to detail. I am passionate about programming (Java, JavaScript, Python), quality software, and music (composition and performance). I have a great deal of experience in the software development field since I did an 8-month co-op at Nuance Communications working in the Professional Services team where we deployed embedded speech dialog systems in vehicles and I performed software testing and requirements validation. I have a great interest in Software collaboration tools. Whether I am working on a project at school or work, I always find myself using software collaboration tools such as, GitHub, JIRA, Basecamp, Slack, etc, and I am very impressed with the software and business impact they make. I am greatly passionate about technology, and I aspire to work for a company that can use technology to make a powerful development impact. I enjoy approaching challenging problems and coming up with elegant solutions and I believe I will be an asset to the Atlassian team. I have heard many great things about working at Atlassian, such as a dynamic work environment, challenging problems, and great benefits. I believe my skill set matches what Atlassian is looking for. Feel free to contact me at 514-951-4128 or email me at arunen.chellan@mail.mcgill.ca if you would like to further discuss this position. I have the enthusiasm and determination to make a success of this opportunity. Thank you for your time Best, Arunen At creation, mkssfs() sets up super block, FBM, i-node file (the hidden file containing all the i-nodes), and the root directory. Note that the root directory is not holding any files at startup. You still need to allocate the i-node 0 to point towards the data blocks (note we are not restricting the root directory to a";
//     ssfs_fwrite(fd_index3, copy, strlen(copy));
// 	//ssfs_fread(fd_index3, copy, strlen(copy));

// 	// Test case 5: Write to an existing file where adding the buffer will overflow into another block 
//     char* appending2 = "At creation, mkssfs() sets up super block, FBM, i-node file (the hidden file containing all the i-nodes), and the root directory. Note that the root directory is not holding any files at startup. You still need to allocate the i-node 0 to point towards the data blocks (note we are not restricting the root directory to a";
//     //ssfs_fwrite(3, appending2, strlen(appending2));
//     ssfs_fwrite(fd_index2, buffer2, strlen(buffer2));
//     // Reading a multiple block file
//     ssfs_fread(fd_index2, buffer2, strlen(buffer2));

//     // Test case 6: write a file
//     // Then change the seek pointer to 5000
//     // Then write again and check the length
//     ssfs_fwrite(fd_index1, appending1, strlen(appending1));
//     ssfs_fwseek(fd_index1, 5000);
//     ssfs_fwrite(fd_index1, onechar, strlen(onechar));
//     ssfs_fread(fd_index1, appending1, strlen(appending1));


//     return 0;

// }
