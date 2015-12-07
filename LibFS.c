#include "LibFS.h"
#include "LibDisk.h"
#include <fcntl.h>   // For checking if the file exists
#include <errno.h>   // For checking if the file exists
#include <string.h>


// global errno value here
int osErrno;

typedef struct inode {
    int size;
    int type;
    int blocks[MAX_INODE_BLOCKS];
} Inode;

typedef struct log {
    char name[16];
    int inode_number;
} Log;

typedef struct dir_data_block {
    Log logs[25];
} Dir_Data_Block;

/* CONSTANTS */
static char MAGIC_NUMBER = 42;
const int INODE_SEC_START = 5;
const int DATA_SEC_START = 255;
const int DATA_BITMAP_OFFSET = 2542;        // currently not used
const int NUM_DATA_BLOCKS = 9745;
const int NUM_INODE_BLOCKS = 250;
const int INODE_BITMAP_SEC = 1;
const int DATA_BITMAP_SEC = 2;
const int FILE_DATA_SIZE = 20;              // The size in bytes of a single file information stored in a Dir's data block
const int LOG_SIZE = 20;                    // size in bytes of a file log entry for a directory data block

/* GLOBALS */
char *filepath;
const size_t MAX_FILE_SIZE = 16;
char buf[SECTOR_SIZE];

/* FUNCTIONS */
Dir_Data_Block New_Dir_Data_Block();
int Find_Free_Inode_Block();
int Find_Free_Data_Block();
int Find_Pathname_Errors(char * file);
int Create_Inode(int type);
int Change_Bitmap_Value(int offset, int sec);
int Get_Path_Token_Count(char *pathname);
int Is_In_Directory(Inode *inode, char *token); // this is trash, delete it
//int Insert_Log(Inode *parent, char *token);
int Find_Inode(int inode_number, char *token);
int Insert_Log(int parent_inode_num, char *token);


void Debug_Testing();
void Pointer_Printing(char *token);
void Array_Printing(char arr[]);


/*      BEGIN PROGRAM       */
int
FS_Boot(char *path)     // Allocates memory in RAM for the disk file to be loaded
{
    filepath = path;
    printf("FS_Boot %s\n", path);

    // oops, check for errors
    if (Disk_Init() == -1) {            
	printf("Disk_Init() failed\n");
	osErrno = E_GENERAL;
	return -1;
    }

    // Determine if the file exists
    int f_desc = open(path, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);  // will fail, returning -1 if the file EXISTS, otherwise the file is created

    if (f_desc < 0)
    {
        if (errno == EEXIST)        // if the file exists and open has failed
        {
            // load the disk file
            if (Disk_Load(path) == -1) {
            printf("Disk_Load() failed\n");
            osErrno = E_GENERAL;
            return -1;
            }

            // Validate the magic number is correct (to check if the file is corrupt)
            Disk_Read(0, buf);
            if (buf[0] != MAGIC_NUMBER) {
                printf("File does not match disk type or it is corrupt.\n");
            } else {
                printf("DEBUG: The file loaded successfully.\n");
            }

            // Make sure the file is the correct size
            // TODO:  Should we use FSTAT for this?

        } else {
            printf("There was a problem with opening the file.\n");
            osErrno = E_GENERAL;
            return -1;
        }

        //TODO: Throw this into a Super_Init() function
    } else {  // the file has now been created and needs initial setup (it is already open)
        close(f_desc);                  // close the file, it does not need to be open
        Disk_Read(0, buf);              // read in the superblock from disk
        buf[0] = MAGIC_NUMBER;          // Assign the magic number to the first index of buffer
        Disk_Write(0, buf);             // Write the magic number to disk
        Create_Inode(DIR_FILE);         // Create the root directory inode
    }

//    Create_Inode(NORM_FILE);
//    Debug_Testing();

    return 0;
}


int
FS_Sync()       // Saves the current disk (from RAM) to a file (secondary storage)
{
    printf("FS_Sync\n");
    // Save the file
    if(Disk_Save(filepath) == -1) {
    printf("Disk_Save() failed\n");
    osErrno = E_GENERAL;
    }

    return 0;
}


int
File_Create(char *file)
{
    printf("FS_Create\n");
    char *token, *path;
    int i, inode_to_search = 0;
    int num_tokens = Get_Path_Token_Count(file);
    Inode *parent;
    char dir_buffer[SECTOR_SIZE];

    if (Find_Pathname_Errors(file) == -1) {
        osErrno = E_CREATE;
        return -1;
    }

    path = malloc(strlen(file) + 1);
    strcpy(path, file);

    token = strtok(path, "/");

    for (i = 0; i < num_tokens; i++) {
        if (i == num_tokens - 1) {      // If we are looking at the last token
            // find the first place for the file log
//            int sec = INODE_SEC_START + (inode_to_search / 4);
//            printf("INSIDE HERE, THE VALUE OF INODE_TO_SEARCH IS: %d\n", inode_to_search);
//            int offset = inode_to_search % 4;
//            Disk_Read(sec, dir_buffer);
//            parent = (Inode *) (dir_buffer + (offset * sizeof(Inode)));

//            if (parent->size > 15000) {
//                osErrno = E_NO_SPACE;
//                printf("File_Create failed, not enough space in directory.\n");
//                return -1;
//            }

//            Insert_Log(parent, token);

            Insert_Log(inode_to_search, token);

        } else {
            inode_to_search = Find_Inode(inode_to_search, token);
            printf("\tThe inode to search value is: %d\n", inode_to_search);
            if (inode_to_search != -1) {
                token = strtok(NULL, "/");
                printf("The new token is: %s\n", token);
                Find_Inode(inode_to_search, token);
            }
        }
    }

//    Disk_Write(INODE_SEC_START, dir_buffer);
//    Debug_Testing();

    free(path);
    return 0;
}

int
Find_Inode(int inode_number, char *token){
    char inodeBuf[SECTOR_SIZE];   // our buffer for this function's inode block read
    char dataBuf[SECTOR_SIZE];      // our buffer for this functions data block read
    int sec = INODE_SEC_START + (inode_number / 4); // determine the block to read for this inode value
    int offset = (inode_number % 4) * 128;  // determine the offset for this inode within the sector
    Inode *inode;
    Dir_Data_Block *data;
    Disk_Read(sec, inodeBuf);     // read in the sector from disk
    inode = (Inode *) (inodeBuf + offset);

    int i;
    for (i = 0; i < MAX_INODE_BLOCKS; i++) {
        if (inode->blocks[i] != -1) {
            Disk_Read(DATA_SEC_START + inode->blocks[i], dataBuf);
            data = (Dir_Data_Block *) dataBuf;
            int j;
            for(j = 0; j < sizeof(Log); j++) {
                if ((strncmp(token, data->logs[j].name, strlen(data->logs[j].name))) == 0) {
                    printf("The strings are the same.\n");
                    return data->logs[j].inode_number;
                }
            }
        }
    }

    return -1;
}


int
//Insert_Log(Inode *parent, char *token) {        // todo:  throw function calls into if statements for error checking
Insert_Log(int parent_inode_num, char *token) {
    Inode *parent;
    char dir_buffer[SECTOR_SIZE];

    int j, i, data_block = NULL, inode_num;
    char read_buffer[SECTOR_SIZE];
    Log log;

    inode_num = Create_Inode(NORM_FILE);
    log.inode_number = inode_num;
    printf("DEBUG: New file inode number is: %d\n", log.inode_number);
    strcpy(log.name, token);

    int sec = INODE_SEC_START + (parent_inode_num / 4);
    int offset = parent_inode_num % 4;
    Disk_Read(sec, dir_buffer);
    parent = (Inode *) (dir_buffer + (offset * sizeof(Inode)));


    for(j = 0; j < MAX_INODE_BLOCKS; j++) {
        printf("AT J=%d, the value of parent->blocks[j] is: %d\n", j, parent->blocks[j]);
        if (parent->blocks[j] == -1) {  // if there is no data block associated with this inode block pointer
            data_block = Find_Free_Data_Block();
            Change_Bitmap_Value(data_block, DATA_BITMAP_SEC);
            printf("DEBUG: This file's log is stored on data block: %d\n", data_block);
            Dir_Data_Block dir_block = New_Dir_Data_Block();
            parent->blocks[j] = data_block;
            parent->size += sizeof(Log);
            dir_block.logs[0] = log;
            Disk_Read(DATA_SEC_START + data_block, read_buffer);
            memcpy(read_buffer, &dir_block, sizeof(Log));
            Disk_Write(DATA_SEC_START + data_block, read_buffer);
            Disk_Write(sec, dir_buffer);
//            Debug_Testing();
            return data_block;

        } else {
            printf("DEBUG: Searching data block %d\n", parent->blocks[j]);
            Disk_Read(DATA_SEC_START + parent->blocks[j], read_buffer);
            Dir_Data_Block *dir_block = (Dir_Data_Block *) read_buffer;

            for (i = 0; i < sizeof(dir_block->logs); i++) {
                printf("DEBUG: Inode at log# %d   is: %d\n", i, dir_block->logs[i].inode_number);
                Array_Printing(dir_block->logs[i].name);

                if (dir_block->logs[i].inode_number == -1) {
                    dir_block->logs[i] = log;
                    parent->size += sizeof(Log);
                    printf("DEBUG: Writing to log index %d\n", i);
                    memcpy(read_buffer, dir_block, sizeof(Dir_Data_Block));
                    Disk_Write(DATA_SEC_START + parent->blocks[j], read_buffer);
                    return 0;
                }
            }
        }
    }

    return -1;
}

Dir_Data_Block
New_Dir_Data_Block()
{
    int i;
    Dir_Data_Block block;
    for (i = 0; i < SECTOR_SIZE / sizeof(Log); i++) {
        block.logs[i].inode_number = -1;
    }

    return block;
}


int
Is_In_Directory(Inode *inode, char *token)  // UNTESTED
{
    int i, offset = 0;
    for (i = 0; i < MAX_INODE_BLOCKS; i++) {
        if(inode->blocks[i] < 0) { break; }
        Disk_Read(inode->blocks[i], buf);
        while (offset < inode->size)
        {
            // see if the name of the token matches the name of a file in this block of the directory's data
            if ((strncmp(token, (buf + offset), strlen(buf + offset))) == 0) { return 1; } // File is in dir
            offset += FILE_DATA_SIZE;
        }
    }
    return -1;  // the file does not exist in the directory
}

int
Find_Pathname_Errors(char *file)
{
    char *token, *path;

    // Save the file name into a mutable string
    path = malloc(strlen(file) + 1);
    strcpy(path, file);

    if ((token = strtok(path, "/")) == NULL) {
        printf("File_Create failed.  File name cannot be null.\n");
        return -1;
    }

    do {
        // Check if the token name is too big per guidelines
        if (strlen(token) > MAX_FILE_SIZE) {
            printf("File_Create failed. Filename %s is too long.", token);
            return -1;
        }

        token = strtok(NULL, "/");

    } while (token != NULL);

    free(path);
    return 0;
}

int
Get_Path_Token_Count(char *file)
{
    int count = 0;
    // Save the file name into a mutable string
    char *pathname = malloc(strlen(file) + 1);
    strcpy(pathname, file);

    strtok(pathname, "/");
    do { count++; } while (strtok(NULL, "/") != NULL);

    free(pathname);
    return count;
}


int
File_Open(char *file)
{
    printf("FS_Open\n");
    return 0;
}

int
File_Read(int fd, void *buffer, int size)
{
    printf("FS_Read\n");
    return 0;
}

int
File_Write(int fd, void *buffer, int size)
{
    printf("FS_Write\n");
    return 0;
}

int
File_Seek(int fd, int offset)
{
    printf("FS_Seek\n");
    return 0;
}

int
File_Close(int fd)
{
    printf("FS_Close\n");
    return 0;
}

int
File_Unlink(char *file)
{
    printf("FS_Unlink\n");
    return 0;
}

// directory ops
int
Dir_Create(char *path)
{
    printf("Dir_Create %s\n", path);

    return 0;
}

int
Dir_Size(char *path)
{
    printf("Dir_Size\n");
    return 0;
}

int
Dir_Read(char *path, void *buffer, int size)
{
    printf("Dir_Read\n");
    return 0;
}

int
Dir_Unlink(char *path)
{
    printf("Dir_Unlink\n");
    return 0;
}

int
Create_Inode(int type)
{
    int offset, sec, start_pos, i;

    // Determine the inode offset value (from index 0 in the inode bitmap)
    if ((offset = Find_Free_Inode_Block()) == -1)
    {
        osErrno = E_NO_SPACE;
        printf("Create_Inode() failed, disk space full.\n");
        return -1;
    } else {
        printf("DEBUG: This inode's bitmap location is: %d\n", (unsigned) offset);
    }

    // Create and initialize the inode
    Inode node;                                     //  TODO: throw this into a New_Inode() function
    node.size = 0;
    node.type = type;
    for (i = 0; i < MAX_INODE_BLOCKS; i++) {
        node.blocks[i] = -1;
    }

    // Determine the sector and position of the free inode    TODO: get rid of magic numbers
    sec = INODE_SEC_START + (offset / 4);                     // 4 inodes per sector
    start_pos = (offset % 4) * (int) sizeof(Inode);           // Find where in the sector the free inode info starts
    Disk_Read(sec, buf);                                      // Read the free inode sector

    printf("DEBUG: This inode's sector is: %d, and its offset is: %d\n", sec, start_pos);

    // Put the inode into the buffer
    if (memcpy(buf + start_pos, &node, sizeof(Inode)) == NULL) {
        printf("Error copying inode to buffer.  Create failed.\n");
        return -1;
    }

    // Write the buffer to disk
    Disk_Write(sec, buf);

    // Mark inode allocated in the bitmap
    Change_Bitmap_Value(offset, INODE_BITMAP_SEC);

    return offset;
}

int
Change_Bitmap_Value(int offset, int sec)
{
    sec += (offset / (SECTOR_SIZE * 8));        // Data block bitmap is sprawled along 3 sectors
    offset %= SECTOR_SIZE * 8;                  // Offset redefined based on sector

    // Find which byte needs changed
    int byte_index = offset / 8;                // 8 bits per byte
    unsigned char add_val = (char) 128;         // We want to xor a bit within the byte at byte_index in the bitmap
    add_val >>= (offset % 8);                   // This will shift the 1 in 10000000 to the appropriate position

    Disk_Read(sec, buf);
    *(buf + byte_index) ^= add_val;
    Disk_Write(sec, buf);                       // Write the change

    return 0;
}


int
Find_Free_Inode_Block()
{
    int i, j, offset = 0;

    // Read the inode bitmap from disk
    Disk_Read(INODE_BITMAP_SEC, buf);

    // find the first available inode
    for (i = 0; i < NUM_INODE_BLOCKS; i++)
    {
        unsigned char current = (unsigned char) buf[i];
        unsigned char op = (char) 128;

        for (j = 0; j < 8; j++)
        {
            if ((current & op) == 0) { return offset; }
            op >>= 1;
            offset++;
        }
    }

    return -1;
}


int
Find_Free_Data_Block()
{
    int i, j, offset = 0;
    int sec = DATA_BITMAP_SEC;

    while (offset < NUM_DATA_BLOCKS) {
        // Read the data block bitmap from disk
        Disk_Read(sec, buf);

        // find the first available data block
        for (i = 0; i < SECTOR_SIZE; i++) {
            unsigned char current = (unsigned char) buf[i];
            unsigned char op = (char) 128;

            for (j = 0; j < 8; j++) {
                if ((current & op) == 0) { return offset; }
                op >>= 1;
                offset++;
                if (offset == NUM_DATA_BLOCKS) { return -1; }
            }
        }
        sec++;
    }

    return -1;
}

void
Debug_Testing()
{
//    int offset = Find_Free_Data_Block();
//    Change_Bitmap_Value(offset, DATA_BITMAP_SEC);
//    printf("DEBUG: The value of data bitmap offset is %d\n", offset);

    char buffer[SECTOR_SIZE];
    Disk_Read(DATA_SEC_START, buffer);
    Dir_Data_Block *datablock = (Dir_Data_Block *) buffer;
    printf("DEBUG TESTING:  NAME: %s\n", datablock->logs[0].name);
    printf("DEBUG TESTING:  INODE: %d\n", datablock->logs[0].inode_number);

//    Disk_Read(INODE_SEC_START, buffer);
//    Inode *root = (Inode *) buffer;
//    printf("DEBUG TESTING: Size is: %d\n", root->size);
//    printf("DEBUG TESTING: Datablock is: %d\n", root->blocks[0]);
}


void
Pointer_Printing(char *token)
{
    size_t i;
    char thestring[16];
    strcpy(thestring, token);
    printf("DEBUG: The word is: ");
    for (i = 0; i < strlen(token); i++) {
        printf("%c", thestring[i]);
    }
    printf("\n");
}

void
Array_Printing(char arr[])
{
    size_t i;
    printf("Debug: The word is: ");
    for (i = 0; i < strlen(arr); i++) {
        printf("%c", arr[i]);
    }
    printf("\n");
}