#include "LibFS.h"
#include "LibDisk.h"
#include <fcntl.h>   // For checking if the file exists
#include <errno.h>   // For checking if the file exists
#include <unistd.h>
#include <sys/types.h>

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

static int MAGIC_NUMBER = 342;
static off_t DISK_FILE_SIZE = 5120000;




int 
FS_Boot(char *path)     // Allocates memory in RAM for the disk file to be loaded
{
    printf("FS_Boot %s\n", path);

    // oops, check for errors
    if (Disk_Init() == -1) {            
	printf("Disk_Init() failed\n");
	osErrno = E_GENERAL;
	return -1;
    }

    int f_desc = open(path, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);  // open the file, will fail if it exists
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
        }

    } else {                     // the file has now been created and needs initial setup (it is already open)

        File_Init(f_desc);
        close(f_desc);
        if (Disk_Load(path) == -1) {
        printf("Disk_Load() failed\n");
        osErrno = E_GENERAL;
        return -1;
        }
    }

    return 0;
}


int
FS_Sync(char *file)       // Saves the current disk (from RAM) to a file (secondary storage)
{
    printf("FS_Sync\n");
    // Save the file
    if(Disk_Save(file) == -1) {         // TODO:  Find out where 'file' comes from (should it be passed in?)
    printf("Disk_Save failed\n");
    osErrno = E_GENERAL;                // TODO:  is this the correct error code?
    }
}


int
File_Create(char *file)
{
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
File_Init(int fd)
{
    // superblock only needs the magic number
    if (write(fd, &MAGIC_NUMBER, sizeof(int)) < 0)
    {
        printf("File_Init() failed\n");
        osErrno = E_CREATE;                     // TODO: is this the correct error code?
        return -1;
    }

    // block 1, inode bitmap
    fseek(fd, SECTOR_SIZE, SEEK_SET);           // start at block #1

    int i;
    for (i = 0; i < 125; i++)                   // 1000 inodes / 8 bits per byte = 125 bytes in this bitmap
    {                                           
        const char AVAILABLE = 0;
        fwrite(AVAILABLE, 1, 1, fd);            // fwrite(ptr to data, size in bytes, num elements of size size bytes, file descriptor)
    }

    // block 2, data block bitmap
    fseek(fd, SECTOR_SIZE * 2, SEEK_SET);       // start at block #2

    for (i = 0; i < 125; i++)
    {
        const char AVAILABLE = 0;
        fwrite(AVAILABLE, 1, 1, fd);
    }

    f

    return 0;
}