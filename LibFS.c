#include "LibFS.h"
#include "LibDisk.h"
#include <fcntl.h>   // For checking if the file exists
#include <errno.h>   // For checking if the file exists
#include <string.h>


// global errno value here
int osErrno;


/*
 *   These should probably be moved somewhere else
 */
typedef struct inode {
    int size;
    int type;
    int blocks[MAX_INODE_BLOCKS];
} Inode;

static char MAGIC_NUMBER = 42;
char *filepath;

char buf[SECTOR_SIZE];

/*
 * IMPORTANT:  in the bitmap, 0 means NOT ALLOCATED and 1 means ALLOCATED
 */


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
        Disk_Read(0, buf);              // read in the superblock from RAM
        buf[0] = MAGIC_NUMBER;          // Assign the magic number to the first index of buffer
        Disk_Write(0, buf);             // Write the magic number to disk
        Disk_Read(1, buf);              // Read in the inode bitmap
        buf[0] = (char) 128;            // set the first bit in the inode bitmap to allocated (for the root directory).  128 = 10000000 in binary.
        Disk_Write(1, buf);             // write the inode bitmap back to the sector
        Disk_Save(filepath);            // Save the init file
    }

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
//int
//File_Init(int fd)
//{
//    int i;
//    const char AVAILABLE = 1;
//
//    if(ftruncate(fd, DISK_FILE_SIZE) < 0) {
//        printf("ftruncate() failed\n");
//        return -1;
//    }
//
//    // superblock only needs the magic number
//    if (write(fd, &MAGIC_NUMBER, sizeof(int)) < 0) {
//        printf("File_Init() failed\n");
//        osErrno = E_CREATE;                     // TODO:  change this error code
//        return -1;
//    }
//
//    // block 1, inode bitmap
//    fseek(fd, SECTOR_SIZE, SEEK_SET);           // start at block #1
//
//    for (i = 0; i < 125; i++)                   // 1000 inodes / 8 bits per byte = 125 bytes in this bitmap
//    {
//        fwrite(AVAILABLE, 1, 1, fd);            // fwrite(ptr to data, size in bytes, num elements of size size bytes, file descriptor)
//    }
//
//    // block 2-4, data block bitmap             // 512 bytes * 8 bites/byte = 4096 bits per sector, 9745 needed
//    fseek(fd, SECTOR_SIZE * 2, SEEK_SET);       // start at block #2
//
//    for (i = 0; i < 1219; i++)                  // 9745 / 8 = 1218.125, round up = 1219
//    {
//        fwrite(AVAILABLE, 1, 1, fd);
//    }
//
//    return 0;
//}

