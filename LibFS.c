#include "LibFS.h"
#include "LibDisk.h"

// global errno value here
int osErrno;

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

    // load the disk file
    if (Disk_Load(path) == -1) {
    printf("Disk_Load() failed\n");
    osErrno = E_GENERAL;
    return -1;
    }

    return 0;
}

int
FS_Sync()       // Saves the current disk (from RAM) to a file (secondary storage)
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

