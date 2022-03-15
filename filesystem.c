#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>


/*
 *   ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ 
 *  |   |   |   |   |                       |   |
 *  | 0 | 1 | 2 | 3 |     .....             |127|
 *  |___|___|___|___|_______________________|___|
 *  |   \    <-----  data blocks ------>
 *  |     \
 *  |       \
 *  |         \
 *  |           \
 *  |             \
 *  |               \
 *  |                 \
 *  |                   \
 *  |                     \
 *  |                       \
 *  |                         \
 *  |                           \
 *  |                             \
 *  |                               \
 *  |                                 \
 *  |                                   \
 *  |                                     \
 *  |                                       \
 *  |                                         \
 *  |                                           \
 *  |     <--- super block --->                   \
 *  |______________________________________________|
 *  |               |      |      |        |       |
 *  |        free   |      |      |        |       |
 *  |       block   |inode0|inode1|   .... |inode15|
 *  |        list   |      |      |        |       |
 *  |_______________|______|______|________|_______|
 *
 *
 */

#define FILENAME_MAXLEN 8  // including the NULL char
#define NO_BLOCKS 128
#define NO_INODES 16
#define FREE_LST_LEN 128

// int fd;


/* 
 * inode 
 */
typedef struct
{
  	int  dir;  // boolean value. 1 if it's a directory.
  	char name[FILENAME_MAXLEN];
  	int  size;  // actual file/directory size in bytes.
  	int  blockptrs [8];  // direct pointers to blocks containing file's content.
  	int  used;  // boolean value. 1 if the entry is in use.
  	int  rsvd;  // reserved for future use
}
inode;

/* 
 * directory entry
 */
typedef struct
{
  	char name[FILENAME_MAXLEN];
  	int  namelen;  // length of entry name
  	int  inode;  // this entry inode index
}
dirent;

// super block
typedef struct
{
  	char free_block_lst[FREE_LST_LEN];
  	inode inode_table[NO_INODES]; 
}
super;

// block
typedef union
{
	//if this block is super block
  	super super_block_ptr;

	//if this block is directory
  	dirent directory_table[64];

	//if this block is file;
	// char file_data[1024];
}
block;


/*
 * functions
 */
// initialize disk
void init_disk(block*);

// create file
void CR(block*, char*);

// Delete file
void DL(block*, char*);

// Copy/move File
void CP(block*, char*, int);

// create directory
void CD(block*, char*);

// delete directory
void DD(block*, char*);

// list file info
void LL(block*, int);

//check if path exist.
void verify_path(block*, char**, int, int*);
void verify_path_2(block*, char**, int, int*);


/* 
 * main
 */
int main(int argc, char* argv[])
{
	//create and init disk.
  	block disk_block_ptr[128];
	init_disk(disk_block_ptr);

	FILE* myfs = fopen("myfs", "w");

	//open file.
    FILE * stream = fopen(argv[1], "r");
    if(stream == NULL)
    {
        fprintf(stderr, "Couldn't read file.\n");
        exit(1);
    }

    char line[1000];
	char func[3];
	char str[1000];

    //read file.
    while (fgets(line, 1000, stream) != NULL)
    {
        line[strcspn(line, "\n")] = '\0';

		strcpy(func, strtok(line, " "));
		if(strcmp(func, "LL"))
		{
			strcpy(str, strtok(NULL, "\0"));

			//call create file
			if(!strcmp(func, "CR"))
			{
				CR(disk_block_ptr, str);
			}

			//call delete file
			if(!strcmp(func, "DL"))
			{
				DL(disk_block_ptr, str);
			}

			//call copy file
			if(!strcmp(func, "CP"))
			{
				CP(disk_block_ptr, str, 0);
			}
			
			//call copy file then delete src.
			if(!strcmp(func, "MV"))
			{
				CP(disk_block_ptr, str, 1);
			}

			//call create directory
			if(!strcmp(func, "CD"))
			{
				CD(disk_block_ptr, str);
			}

			//call delete directory
			if(!strcmp(func, "DD"))
			{
				DD(disk_block_ptr, str);
			}
		}
		//call list info
		else
		{
			strcpy(str, "\0");

			LL(disk_block_ptr, 1);
		}
		
		//write to myfs.
		for (int i = 0; i < NO_BLOCKS; i++)
		{
			fwrite((disk_block_ptr + i), 1024, 1, myfs);
		}
    }

	fclose(myfs);
    fclose(stream);
	return 0;
}

void init_disk(block* block_lst_ptr)
{
	FILE* stream = fopen("myfs", "r");

	if (stream == NULL)
	{
		printf("Creating myfs.\n");
	//init inode table in super block.
	for(int i = 0; i < NO_INODES; i++)
	{
		//init first inode for root,
		if (i == 0)
		{
			strcpy(((block_lst_ptr + 0) -> super_block_ptr).inode_table[i].name, "/");
			((block_lst_ptr + 0) -> super_block_ptr).inode_table[i].dir = 1;
			((block_lst_ptr + 0) -> super_block_ptr).inode_table[i].size = 0;
			((block_lst_ptr + 0) -> super_block_ptr).inode_table[i].used = 1;
			((block_lst_ptr + 0) -> super_block_ptr).inode_table[i].rsvd = 1;

			//init first direct block pointers for root inode to 1.
			((block_lst_ptr + 0) -> super_block_ptr).inode_table[i].blockptrs[0] = 1;

			//init remaining t0 -1.
			for(int j = 1; j < 8; j++)
			{
				((block_lst_ptr + 0) -> super_block_ptr).inode_table[i].blockptrs[j] = -1;
			}
		}

		//init remaining inodes.
		else
		{
			strcpy(((block_lst_ptr + 0) -> super_block_ptr).inode_table[i].name, " ");
			((block_lst_ptr + 0) -> super_block_ptr).inode_table[i].dir = 0;
			((block_lst_ptr + 0) -> super_block_ptr).inode_table[i].size = 0;
			((block_lst_ptr + 0) -> super_block_ptr).inode_table[i].used = 0;
			((block_lst_ptr + 0) -> super_block_ptr).inode_table[i].rsvd = 0;
		

			//init direct block pointers in remaining inodes.
			for(int j = 0; j < 8; j++)
			{
				((block_lst_ptr + 0) -> super_block_ptr).inode_table[i].blockptrs[j] = -1;
			}
		}
	}

	//init free block list in super block.
	//superblock
	((block_lst_ptr + 0) -> super_block_ptr).free_block_lst[0] = '0';
	//root
	((block_lst_ptr + 0) -> super_block_ptr).free_block_lst[1] = '0';
	//remaining blocks
	for(int i = 2; i < FREE_LST_LEN; i++)
	{
		((block_lst_ptr + 0) -> super_block_ptr).free_block_lst[i] = '1';
	}
	
	//init rest of the blocks.
	for(int i = 1; i < NO_BLOCKS; i++)
	{
		for(int j = 0; j < (NO_INODES - 1); j++)
		{
			(block_lst_ptr + i) -> directory_table[j].inode = -1;
			strcpy((block_lst_ptr + i) -> directory_table[j].name, " ");
			(block_lst_ptr + i) -> directory_table[j].namelen = 0;
		}
	}	
	}

	else
	{
		printf("Reading myfs.\n");

		// fread(&((block_lst_ptr + 0) -> super_block_ptr), 1024, 1, stream);

		for (int i = 0; i < NO_BLOCKS; i++)
		{
			fread((block_lst_ptr + i), 1024, 1, stream);
		}

		fclose(stream);
	}

	return;
}

void verify_path(block* disk_block_ptr, char** arr, int last_index, int* inode_arr)
{
	//init parent inode array.
	for(int i = 0; i < last_index; i++)
	{
		inode_arr[i] = -1;
	}

	//if new file/dir in root.
	if (last_index == 1)
	{
		for (int i = 0; i < NO_INODES - 1; i++)
		{
			if ((disk_block_ptr + 1) -> directory_table[i].inode != -1 && !strcmp((disk_block_ptr + 1) -> directory_table[i].name, arr[last_index - 1]))
			{
				printf("the ");

				if (((disk_block_ptr + 0) -> super_block_ptr).inode_table[(disk_block_ptr + 1) -> directory_table[i].inode].dir == 1)
				{
					printf("directory ");
				}
				else
				{
					printf("file ");
				}
				
				printf("already exists\n");
				inode_arr[0] = -1;
				break;
			}

			else
			{
				inode_arr[0] = 0;
			}
		}
	}
	
	//check path if path not in root.
	else
	{
		int next_block = 1;
		int next_inode = -1;
		int root_block = ((disk_block_ptr + 0) -> super_block_ptr).inode_table[0].blockptrs[0];

		for(int i = 0; i < (NO_INODES - 1); i++)
		{
			if(((disk_block_ptr + root_block) -> directory_table[i].inode != -1) && !strcmp((disk_block_ptr + root_block) -> directory_table[i].name, arr[0]))
			{
				next_inode = (disk_block_ptr + root_block) -> directory_table[i].inode;
				inode_arr[0] = 0;
				inode_arr[1] = next_inode;
				break;
			}
		}

		if(next_inode == -1)
		{
			printf("The directory %s in the given path does not exit.\n", arr[0]);
			return;
		}
		for (int i = 1; i < last_index - 1; ++i)
		{ 
			next_block = ((disk_block_ptr + 0) -> super_block_ptr).inode_table[next_inode].blockptrs[0];
			next_inode = -1;

			for(int j = 0; j < (NO_INODES - 1); j++)
			{
				if(((disk_block_ptr + next_block) -> directory_table[j].inode != -1) && !strcmp((disk_block_ptr + next_block) -> directory_table[j].name, arr[i]))
				{
					next_inode = (disk_block_ptr + next_block) -> directory_table[j].inode;
					inode_arr[i+1] = next_inode;
					break;
				}
			}

			if (next_inode == -1)
			{
				printf("The directory %s in the given path does not exit.\n", arr[i]);
				inode_arr[0] = -1;
				inode_arr[1] = -1;
				break;
			}
		}

		if (next_inode != -1)
		{
			next_block = ((disk_block_ptr + 0) -> super_block_ptr).inode_table[next_inode].blockptrs[0];

			for(int j = 0; j < NO_INODES -1; j++)
			{	
				if(((disk_block_ptr + next_block) -> directory_table[j].inode != -1) && !strcmp((disk_block_ptr + next_block) -> directory_table[j].name, arr[last_index-1]))
				{
					printf("the ");

					if (((disk_block_ptr + 0) -> super_block_ptr).inode_table[(disk_block_ptr + 1) -> directory_table[j].inode].dir == 1)
					{
						printf("directory ");
					}
					else
					{
						printf("file ");
					}
					
					printf("already exists\n");

					inode_arr[0] = -1;
					inode_arr[1] = -1;
					break;
				}
			}
		}
	}

	return;
}

void verify_path_2(block* disk_block_ptr, char** arr, int last_index, int* inode_arr)
{
	//init parent inode array.
	for(int i = 0; i < last_index; i++)
	{
		inode_arr[i] = -1;
	}

	//if new file/dir in root.
	if (last_index == 1)
	{
		for (int i = 0; i < NO_INODES - 1; i++)
		{
			if ((disk_block_ptr + 1) -> directory_table[i].inode != -1 && !strcmp((disk_block_ptr + 1) -> directory_table[i].name, arr[last_index - 1]))
			{
				inode_arr[0] = 0;
				break;
			}
		}

		if (inode_arr[0] == -1)
		{
			printf("the file does not exist\n");
			return;
		}
	}
	
	else
	{
		int next_block = 1;
		int next_inode = -1;
		// go to root.
		int root_block = ((disk_block_ptr + 0) -> super_block_ptr).inode_table[0].blockptrs[0];

		for(int i = 0; i < (NO_INODES - 1); i++)
		{
			if(((disk_block_ptr + root_block) -> directory_table[i].inode != -1) && !strcmp((disk_block_ptr + root_block) -> directory_table[i].name, arr[0]))
			{
				next_inode = (disk_block_ptr + root_block) -> directory_table[i].inode;
				inode_arr[0] = 0;
				inode_arr[1] = next_inode;
				break;
			}
		}

		if(next_inode == -1)
		{
			printf("The directory %s in the given path does not exit.\n", arr[0]);
			return;
		}

		//check path.
		for (int i = 1; i < last_index - 1; ++i)
		{ 
			next_block = ((disk_block_ptr + 0) -> super_block_ptr).inode_table[next_inode].blockptrs[0];
			next_inode = -1;

			for(int j = 0; j < (NO_INODES - 1); j++)
			{	
				//dir not exists in path.
				if(((disk_block_ptr + next_block) -> directory_table[j].inode != -1) && !strcmp((disk_block_ptr + next_block) -> directory_table[j].name, arr[i]))
				{
					next_inode = (disk_block_ptr + next_block) -> directory_table[j].inode;
					inode_arr[i+1] = next_inode;
					break;
				}
			}

			if (next_inode == -1)
			{
				printf("The directory %s in the given path does not exit.\n", arr[i]);
				inode_arr[0] = -1;
				inode_arr[1] = -1;
				break;
			}
		}
	}

	return;
}

void CR(block* disk_block_ptr, char* str)
{
	printf("CR called, str:");
	printf("%s\n", str);

	char path[256];
	int file_size = 0;

	char* token = strtok(str, " ");
	strcpy(path, token);

	token = strtok(NULL, " ");
	file_size = atoi(token);

	//find no of blocks to allocate.	
	int no_blocks = (file_size/1024);
	no_blocks++;

	if (no_blocks > 8)
	{
		printf("not enough space.\n");
		return;
	}

	int data_blocks[no_blocks];
	data_blocks[0] = -1;

	int path_length = 0;

	//find path length.
	for (int i = 0; path[i]; i++)
	{
		if(path[i] == '/')
		{
			path_length++;
		}
	}

	//store directories in path in an array.
	int i = 0;
	char* arr[path_length];
	token = strtok(path, "/");
	while (token != NULL)
	{
		arr[i++] = token;
		token = strtok(NULL, "/");
	}

	//new file name
	char* file_name = arr[path_length - 1];

	// array to store inode of parents.
	int parent_inode_arr[path_length];

	verify_path(disk_block_ptr, arr, path_length, parent_inode_arr);
	

	//do only if path was ok and file does not already exist.
	if(parent_inode_arr[0] == 0)
	{
		//find free block(s).
		int count = 0;
		for(int i = 0; i < NO_BLOCKS; i++)
		{
			if(((disk_block_ptr + 0) -> super_block_ptr).free_block_lst[i] == '1')
			{
				data_blocks[count] = i;
				count++;
				((disk_block_ptr + 0) -> super_block_ptr).free_block_lst[i] = '0';

				//required number of free blocks found.
				if(count == no_blocks)
				{
					break;
				}
			}
		}

		//if no data block assigned.
		if (data_blocks[0] == -1)
		{
			printf("not enough space.\n");
			return;
		}
		
		else
		{
			//if not enough data blocks availabe. reclaim alloted data blocks.
			if (count < no_blocks)
			{
				printf("not enough space.\n");

				//reclaim allocated data blocks.
				for (int i = 0; i < count; i++)
				{
					((disk_block_ptr + 0) -> super_block_ptr).free_block_lst[data_blocks[i]] = '1';
					data_blocks[i] = -1;
				}
			}

			else
			{
				//find free inode
				int inode = -1;
				for(int i = 0; i < NO_INODES; i++)
				{
					
					if(((disk_block_ptr + 0) -> super_block_ptr).inode_table[i].used == 0)
					{
						inode = i;
						strcpy(((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode].name, file_name);
						((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode].dir = 0;
						((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode].used = 1;
						((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode].size = file_size;
						for (int j = 0; j < no_blocks; j++)
						{
							((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode].blockptrs[j] = data_blocks[j];
						}
						((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode].rsvd = 0;
						break;
					}
				}

				//free inode not found reclaim allocated data blocks.
				if (inode == -1)
				{
					printf("not enough space.\n");

					for (int i = 0; i < count; i++)
					{
						((disk_block_ptr + 0) -> super_block_ptr).free_block_lst[data_blocks[i]] = '1';
						data_blocks[i] = -1;
					}
					return;
				}

				//assign inode in parent directory.
				else
				{
					int assigned = 0;
					int parent_block = ((disk_block_ptr + 0) -> super_block_ptr).inode_table[parent_inode_arr[path_length - 1]].blockptrs[0];

					for (int i = 0; i < NO_INODES - 1; i++)
					{
						if((disk_block_ptr + parent_block) -> directory_table[i].inode == -1)
						{
							strcpy((disk_block_ptr + parent_block) -> directory_table[i].name, file_name);
							(disk_block_ptr + parent_block) -> directory_table[i].inode = inode;
							(disk_block_ptr + parent_block) -> directory_table[i].namelen = strlen(file_name);

							assigned = 1;
							break;
						}
					}
					
					//not enough space in directory
					if(assigned == 0)
					{
						printf("not enough space\n");

						//reclaim allocated inode;
						((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode].used = 0;

						//reclaim allocated blocks
						for (int i = 0; i < count; i++)
						{
							((disk_block_ptr + 0) -> super_block_ptr).free_block_lst[data_blocks[i]] = '1';
							data_blocks[i] = -1;
						}
						return;
					}

					//update sizes of all parent directories.
					else
					{					
						for(int i = 0; i < path_length; i++)
						{
							((disk_block_ptr + 0) -> super_block_ptr).inode_table[parent_inode_arr[i]].size += file_size;
						}
					}
				}
			}
		}		
	}
	// printf("exiting\n");
}

void DL(block* disk_block_ptr, char* path)
{
	printf("DL called, str:");
	printf("%s\n", path);

	int path_length = 0;

	//find path length.
	for (int i = 0; path[i]; i++)
	{
		if(path[i] == '/')
		{
			path_length++;
		}
	}

	//store directories in path in an array.
	int i = 0;
	char* arr[path_length];
	char* token = strtok(path, "/");
	while (token != NULL)
	{
		arr[i++] = token;
		token = strtok(NULL, "/");
	}

	//to delete file name
	char* file_name = arr[path_length - 1];

	// array to store inode of parents.
	int parent_inode_arr[path_length];
	
	verify_path_2(disk_block_ptr, arr, path_length, parent_inode_arr);

	if (parent_inode_arr[0] == 0)
	{
		int inode_del = -1;
		
		//go to parents data block.
		int parent_inode = parent_inode_arr[path_length - 1];
		int parent_block = ((disk_block_ptr + 0) -> super_block_ptr).inode_table[parent_inode].blockptrs[0];

		//find file to delete in parent directory.
		for (int i = 0; i < NO_INODES - 1; i++)
		{
			if ((disk_block_ptr + parent_block) -> directory_table[i].inode != -1 && !strcmp((disk_block_ptr + parent_block) -> directory_table[i].name, file_name))
			{
				inode_del = (disk_block_ptr + parent_block) -> directory_table[i].inode;

				//file found
				if (((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode_del].dir == 0)
				{
					(disk_block_ptr + parent_block) -> directory_table[i].inode = -1;
					break;
				}	
				else
				{
					inode_del = -1;
				}		
			}	
		}
		
		//if no such file exists in parent directory.
		if (inode_del == -1)
		{
			printf("the file does not exist.\n");
		}

		else
		{
			//update all parent size;
			int file_size = ((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode_del].size;
			for(int i = 0; i < path_length; i++)
			{
				((disk_block_ptr + 0) -> super_block_ptr).inode_table[parent_inode_arr[i]].size -= file_size;
			}

			//recliam allocated data blocks.
			for (int i = 0; i < 8; i++)
			{
				if (((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode_del].blockptrs[i] != -1)
				{
					// strcpy(((disk_block_ptr + ((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode_del].blockptrs[i]) -> file_data), " ");

					((disk_block_ptr + 0) -> super_block_ptr).free_block_lst[((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode_del].blockptrs[i]] = '1';

					((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode_del].blockptrs[i] = -1;
				}
			}

			//reclaim inode
			((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode_del].used = 0;
		}
	}
	
	//printf("exiting.\n");
}

void CP(block* disk_block_ptr, char* str, int mv)
{
	printf("CP called, str:");
	printf("%s\n", str);

	char src[256];
	char src_2[256];

	char dst[256];
	char dst_2[256];
	char dst_3[256];

	char* token = strtok(str, " ");
	strcpy(src, token);
	strcpy(src_2, token);

	token = strtok(NULL, " ");
	strcpy(dst, token);
	strcpy(dst_2, token);
	strcpy(dst_3, token);


	if (!strcmp(src, dst))
	{
		return;
	}

	//find src path length.
	int src_path_length = 0;
	for (int i = 0; src[i]; i++)
	{
		if(src[i] == '/')
		{
			src_path_length++;
		}
	}

	//find dst path length.
	int dst_path_length = 0;
	for (int i = 0; dst[i]; i++)
	{
		if(dst[i] == '/')
		{
			dst_path_length++;
		}
	}

	//store directories of src path in an array.
	int i = 0;
	char* src_arr[src_path_length];
	token = strtok(src, "/");
	while (token != NULL)
	{
		src_arr[i++] = token;
		token = strtok(NULL, "/");
	}

	//store directories of dst path in an array.
	i = 0;
	char* dst_arr[dst_path_length];
	token = strtok(dst, "/");
	while (token != NULL)
	{
		dst_arr[i++] = token;
		token = strtok(NULL, "/");
	}

	char* src_file = src_arr[src_path_length - 1];
	char* dst_file = dst_arr[dst_path_length - 1];
	
	// array to store inode of parents for src and dst.
	int src_parent_inode_arr[src_path_length];
	int dst_parent_inode_arr[dst_path_length];
	
	verify_path_2(disk_block_ptr, src_arr, src_path_length, src_parent_inode_arr);

	if (src_parent_inode_arr[0] == -1)
	{
		return;
	}
	//if source path ok
	else
	{
		//check if src file exists or is a dir.
		int src_exists = 0;
		int src_size = 0;
		int src_parent_inode = src_parent_inode_arr[src_path_length - 1];
		int src_parent_block = ((disk_block_ptr + 0) -> super_block_ptr).inode_table[src_parent_inode].blockptrs[0];

		for (int i = 0; i < NO_INODES - 1; i++)
		{
			if((disk_block_ptr + src_parent_block) -> directory_table[i].inode != -1 && !strcmp((disk_block_ptr + src_parent_block) -> directory_table[i].name, src_file))
			{
				//check if src is dir.
				if(((disk_block_ptr + 0) -> super_block_ptr).inode_table[(disk_block_ptr + src_parent_block) -> directory_table[i].inode].dir == 1)
				{
					printf("can't handle directories.\n");
					return;
				}
				else
				{
					src_exists = 1;
					src_size = ((disk_block_ptr + 0) -> super_block_ptr).inode_table[(disk_block_ptr + src_parent_block) -> directory_table[i].inode].size;
				}

				break;
			}
		}
		
		if(src_exists == 1)
		{
			verify_path_2(disk_block_ptr, dst_arr, dst_path_length, dst_parent_inode_arr);
			
			if (dst_parent_inode_arr[0] == -1)
			{
				return;
			}
			//if dst path ok
			else
			{
				//check if dst file exits or is a dir.
				int dst_exists = 0;
				int dst_parent_inode = dst_parent_inode_arr[dst_path_length - 1];
				int dst_parent_block = ((disk_block_ptr + 0) -> super_block_ptr).inode_table[dst_parent_inode].blockptrs[0];

				for (int i = 0; i < NO_INODES - 1; i++)
				{
					if ((disk_block_ptr + dst_parent_block) -> directory_table[i].inode != -1 && !strcmp((disk_block_ptr + dst_parent_block) -> directory_table[i].name, dst_file))
					{
						//check if dst is dir.
						if(((disk_block_ptr + 0) -> super_block_ptr).inode_table[(disk_block_ptr + dst_parent_block) -> directory_table[i].inode].dir == 1)
						{
							printf("can't handle directories.\n");
							return;
						}
						else
						{
							dst_exists = 1;
						}

						break;
					}
				}

				//overwrite
				if (dst_exists == 1)
				{					
					DL(disk_block_ptr, dst_3);
				}

				char buffer[20];
				sprintf(buffer, "%d", src_size);

				strcat(dst_2, " ");
				strcat(dst_2, buffer);

				CR(disk_block_ptr, dst_2);

				//if move file called. DL src file.
				if (mv == 1)
				{
					DL(disk_block_ptr, src_2);
				}
			}
		}
		else
		{
			printf("source file does not exist.\n");
		}			
	}
}

void CD(block* disk_block_ptr, char* path)
{
	printf("CD called, str:");
	printf("%s\n", path);

	int path_length = 0;

	//find path length.
	for (int i = 0; path[i]; i++)
	{
		if(path[i] == '/')
		{
			path_length++;
		}
	}

	//store directories in path in an array.
	int i = 0;
	char* arr[path_length];
	char* token = strtok(path, "/");
	while (token != NULL)
	{
		arr[i++] = token;
		token = strtok(NULL, "/");
	}

	//new dir name
	char* dir_name = arr[path_length - 1];

	// array to store inode of parents.
	int parent_inode_arr[path_length];
	
	verify_path(disk_block_ptr, arr, path_length, parent_inode_arr);

	//do only if path was ok and dir does not already exist.
	if(parent_inode_arr[0] == 0)
	{
		//find free block(s).
		int data_block = -1;
		for(int i = 0; i < NO_BLOCKS; i++)
		{
			if(((disk_block_ptr + 0) -> super_block_ptr).free_block_lst[i] == '1')
			{
				data_block = i;
				((disk_block_ptr + 0) -> super_block_ptr).free_block_lst[data_block] = '0';
				break;
			}
		}

		//if not enough data blocks availabe.
		if (data_block == -1)
		{
			printf("not enough space.\n");
			return;
		}
		else
		{
			//find free inode
			int inode = -1;
			for(int i = 0; i < NO_INODES; i++)
			{
				if(((disk_block_ptr + 0) -> super_block_ptr).inode_table[i].used == 0)
				{
					inode = i;
					strcpy(((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode].name, dir_name);
					((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode].dir = 1;
					((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode].used = 1;
					((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode].size = 0;
					((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode].blockptrs[0] = data_block;
					((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode].rsvd = 0;
					break;
				}
			}

			//free inode not found reclaim allocated data block.
			if (inode == -1)
			{
				printf("not enough space.\n");

				((disk_block_ptr + 0) -> super_block_ptr).free_block_lst[data_block] = '1';
			}

			//assign inode in parent directory.
			else
			{
				int assigned = 0;
				int parent_block = ((disk_block_ptr + 0) -> super_block_ptr).inode_table[parent_inode_arr[path_length - 1]].blockptrs[0];

				for (int i = 0; i < NO_INODES - 1; i++)
				{
					if((disk_block_ptr + parent_block) -> directory_table[i].inode == -1)
					{
						strcpy((disk_block_ptr + parent_block) -> directory_table[i].name, dir_name);
						(disk_block_ptr + parent_block) -> directory_table[i].inode = inode;
						(disk_block_ptr + parent_block) -> directory_table[i].namelen = strlen(dir_name);

						assigned = 1;
						break;
					}
				}
				
				//not enough space in parent directory
				if(assigned == 0)
				{
					printf("not enough space\n");

					//reclaim allocated inode;
					((disk_block_ptr + 0) -> super_block_ptr).inode_table[inode].used = 0;

					//reclaim allocated block
					((disk_block_ptr + 0) -> super_block_ptr).free_block_lst[data_block] = '1';
				}

				//update sizes of all parent directories.
				else
				{
					//update size loop doesn't run if new file in root.(e.g. pathlenght = 1)
					for(int i = 0; i < path_length; i++)
					{
						((disk_block_ptr + 0) -> super_block_ptr).inode_table[parent_inode_arr[i]].size += 1024;
					}
				}
			}
		}		
	}
	
	// printf("exiting\n");
}

void DD(block* disk_block_ptr, char* path)
{
	printf("DD called, str:");
	printf("%s\n", path);

	char path_save[256];
	char path_send[256];

	//save path for recursion.
	strcpy(path_save, path);

	int path_length = 0;

	//find path length.
	for (int i = 0; path[i]; i++)
	{
		if(path[i] == '/')
		{
			path_length++;
		}
	}

	//store directories in path in an array.
	int i = 0;
	char* arr[path_length];
	char* token = strtok(path, "/");
	while (token != NULL)
	{
		arr[i++] = token;
		token = strtok(NULL, "/");
	}

	//to delete dir name
	char* dir_name = arr[path_length - 1];

	// array to store inode of parents.
	int parent_inode_arr[path_length];

	verify_path_2(disk_block_ptr, arr, path_length, parent_inode_arr);

	if (parent_inode_arr[0] == 0)
	{
		//go to parents data block.
		int parent_inode = parent_inode_arr[path_length - 1];
		int parent_block = ((disk_block_ptr + 0) -> super_block_ptr).inode_table[parent_inode].blockptrs[0];

		int del_inode = -1;

		int un_link = -1;

		//find inode of dir to delete.
		for (int i = 0; i < NO_INODES -1; i++)
		{
			if((disk_block_ptr + parent_block) -> directory_table[i].inode != -1 && !strcmp((disk_block_ptr + parent_block) -> directory_table[i].name, dir_name))
			{
				del_inode = (disk_block_ptr + parent_block) -> directory_table[i].inode;
				un_link = i;
				
				//dir inode found
				if(((disk_block_ptr + 0) -> super_block_ptr).inode_table[del_inode].dir == 1)
				{
					break;
				}
				//dir inode not found
				else
				{
					del_inode = -1;
				}	
			}
		}
		//dir does not exit
		if (del_inode == -1)
		{
			printf("the directory does not exist.\n");
			return;
		}

		else
		{
			//go to dir block
			int dir_block = ((disk_block_ptr + 0) -> super_block_ptr).inode_table[del_inode].blockptrs[0];
			int child_del_inode = -1;
			//traverse over dir contents
			for (int i = 0; i < NO_INODES -1; i++)
			{
				if ((disk_block_ptr + dir_block) -> directory_table[i].inode != -1)
				{
					child_del_inode = (disk_block_ptr + dir_block) -> directory_table[i].inode;
			
					//create path
					strcpy(path_send, path_save);
					strcat(path_send, "/");
					strcat(path_send, ((disk_block_ptr + 0) -> super_block_ptr).inode_table[child_del_inode].name);

					//delete file.
					if (((disk_block_ptr + 0) -> super_block_ptr).inode_table[child_del_inode].dir == 0)
					{
						DL(disk_block_ptr, path_send);
					}

					//delete dir
					else
					{
						DD(disk_block_ptr, path_send);
					}
					
					(disk_block_ptr + dir_block) -> directory_table[i].inode = -1;
					child_del_inode = -1;
				}
			}

			// printf("del_inode:%d\n", del_inode);

			//unlink dir from parrent.	
			(disk_block_ptr + parent_block) -> directory_table[un_link].inode = -1;

			//free dir block.
			((disk_block_ptr + 0) -> super_block_ptr).free_block_lst[dir_block] = '1';

			//free dir inode
			((disk_block_ptr + 0) -> super_block_ptr).inode_table[del_inode].used = 0;

			//update all parent sizes.
			for (int i = 0; i < path_length; i++)
			{
				((disk_block_ptr + 0) -> super_block_ptr).inode_table[parent_inode_arr[i]].size -= 1024;
			}
		}
	}
}

void LL(block* disk_block_ptr, int block)
{
	printf("LL called\n");

	//traverse inodes in use in superblock.
	for (int i = 0; i < NO_INODES; i++)
	{
		if(((disk_block_ptr + 0) -> super_block_ptr).inode_table[i].used == 1)
		{
			printf("%s ", ((disk_block_ptr + 0) -> super_block_ptr).inode_table[i].name);
			printf("%d\n", ((disk_block_ptr + 0) -> super_block_ptr).inode_table[i].size);
		}
	}	
}