#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "disk_emu.h"

#define SSFS_MAX_FILENAME 16
#define SSFS_BLOCK_SIZE 1024
#define SSFS_MAGIC_NUM 4
#define SSFS_FD_ENTRIES 1024

typedef struct{
	unsigned char magic[SSFS_MAGIC_NUM];
	int block_size;
	int fs_size;
	int root_inode_num;
	int inode_table_len;
} superblock_t;

typedef struct{
	int size;
	int direct[14];
	int indirect;
	int mode;
} inode_t;

typedef struct{
	uint8_t unsigned char bytes [SSFS_BLOCK_SIZE];
} block_t;

typedef struct{
	int size;
	int	inodes_used;
	char *inodes_unused;
	inode_t *inode;
}	inode_table;

typedef struct{
	int size;
	fd_entry entry[SSFS_FD_ENTRIES];
} fd_table;

typedef struct{
	int size;
	fd_entry entry[SSFS_FD_ENTRIES];
	int inode_num;
	int read_ptr;
	int write_ptr;
} fd_entry;

typedef struct{
	directory_entry *entry;
	int num_directories; 
} directory_table;

typedef struct{
	int inode_num;
	char filename[SSFS_MAX_FILENAME];
} directory_table;

//Functions you should implement. 
//Return -1 for error besides mkssfs
void mkssfs(int fresh);
int ssfs_get_next_file_name(char *fname);
int ssfs_get_file_size(char* path);
int ssfs_fopen(char *name);
int ssfs_fclose(int fileID);
int ssfs_frseek(int fileID, int loc);
int ssfs_fwseek(int fileID, int loc);
int ssfs_fwrite(int fileID, char *buf, int length);
int ssfs_fread(int fileID, char *buf, int length);
int ssfs_remove(char *file);
int ssfs_commit();
int ssfs_restore(int cnum);