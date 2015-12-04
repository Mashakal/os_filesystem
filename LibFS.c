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

/* CONSTANTS */
static char MAGIC_NUMBER = 42;
const int INODE_SEC_START = 5;
const int DATA_SEC_START = 255;     // inode bitmap and datablock bitmap both start in sector 1
const int DATA_BITMAP_OFFSET = 2542;    // The
const int NUM_DATA_BLOCKS = 9745;
const int INODE_BITMAP_SEC = 1;
const int DATA_BITMAP_SEC = 2;

/* GLOBALS */
char *filepath;
char buf[SECTOR_SIZE];

/* FUNCTIONS */
int Find_Free_Inode_Block();
int Create_Inode(int type);
int Find_Free_Data_Block();
int Change_Bitmap_Value(int offset, int sec);
void Debug_Testing();


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

    } else {  // the file has now been created and needs initial setup (it is already open)
        close(f_desc);                  // close the file, it does not need to be open
        Disk_Read(0, buf);              // read in the superblock from disk
        buf[0] = MAGIC_NUMBER;          // Assign the magic number to the first index of buffer
        Disk_Write(0, buf);             // Write the magic number to disk
//        Disk_Read(1, buf);              // Read in the inode bitmap
//        buf[0] = (char) 128;            // set the first bit in the inode bitmap to allocated (for the root directory).  128 = 10000000 in binary.
//        Disk_Write(1, buf);             // write the inode bitmap back to the sector
//
//        // CREATE THE ROOT DIRECTORY INODE
//        Inode root;
//        root.size = 0;  // is this the size of the information in the data blocks or the info in the inode
//        root.type = DIR_TYPE;
//        Disk_Read(5, buf);
//        memcpy(buf, &root, sizeof(root)); // IS THIS RIGHT?
//        Disk_Write(5, buf);

        Disk_Save(filepath);            // Save the init file
    }

    Create_Inode(NORM_FILE);
    Debug_Testing();
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
    // check that the parent directories exist
        // split the file into tokens, separated by '/'
        // if the token cannot be found in the current directory (starting with root)
            // if the file name is no more than 16 bytes, create it
            // otherwise, return an error
        // if it can be found, move on to the next token

    printf("FS_Create\n");
    return 0;
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
Empty_Buffer() {
    int i;
    for (i = 0; i < SECTOR_SIZE; i++) {
        buf[i] = 0;
    }
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
        printf("DEBUG: The next bitmap location is: %d\n", (unsigned) offset);
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
    } else {
        printf("DEBUG: The inode copied into the buffer successfully.\n");
    }

//    Inode *ptr_inode = (Inode *) (buf + start_pos);
//    printf("This is a test to see if the ptr_inode works as expected.\n");
//    printf("\tthe inode's type is: %d\n", ptr_inode->type);
//    printf("\tthe inode's size is: %d\n", ptr_inode->size);

    // Write the buffer to disk
    Disk_Write(sec, buf);

    // Mark inode allocated in the bitmap
    Change_Bitmap_Value(offset, INODE_BITMAP_SEC);

    return 0;
}

int
Change_Bitmap_Value(int offset, int sec)
{
    sec += (offset / (SECTOR_SIZE * 8));        // Data block bitmap is sprawled along 3 sectors
    offset %= SECTOR_SIZE * 8;                  // Offset redefined based on sector

    // Find which byte needs changed
    int byte_index = offset / 8;                // 8 bits per byte
    unsigned char add_val = (char) 128;         // We want to add 1 to a bit within the byte at byte_index in the bitmap
    add_val >>= (offset % 8);                   // This will shift the 1 in 10000000 to the appropriate position

    Disk_Read(sec, buf);
    *(buf + byte_index) ^= add_val;
    Disk_Write(sec, buf);                       // Write the changes

    return 0;
}


int
Find_Free_Inode_Block()
{
    int i, j, offset = 0;

    // Read the inode bitmap from disk
    Disk_Read(1, buf);

    // find the first available inode
    for (i = 0; i < 250; i++)
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
    int sec = 2;

    while (offset < NUM_DATA_BLOCKS) {
        // Read the inode bitmap from disk
        Disk_Read(sec, buf);

        // find the first available inode
        for (i = 0; i < NUM_DATA_BLOCKS; i++) {
            unsigned char current = (unsigned char) buf[i];
            unsigned char op = (char) 128;

            for (j = 0; j < 8; j++) {
                if ((current & op) == 0) { return offset; }
                op >>= 1;
                offset++;
            }
        }
        sec += 1;
    }

    return -1;
}

void
Debug_Testing()
{
    int offset = Find_Free_Data_Block();
    Change_Bitmap_Value(offset, DATA_BITMAP_SEC);
    printf("DEBUG: The value of data bitmap offset is %d\n", offset);
}