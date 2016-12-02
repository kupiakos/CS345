// os345fat.c - file management system
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the CS345 projects.          **
// ** It comes "as is" and "unwarranted."  As such, when you use part   **
// ** or all of the code, it becomes "yours" and you are responsible to **
// ** understand any algorithm or method presented.  Likewise, any      **
// ** errors or problems become your responsibility to fix.             **
// **                                                                   **
// ** NOTES:                                                            **
// ** -Comments beginning with "// ??" may require some implementation. **
// ** -Tab stops are set at every 3 spaces.                             **
// ** -The function API's in "OS345.h" should not be altered.           **
// **                                                                   **
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// ***********************************************************************
//
//		11/19/2011	moved getNextDirEntry to P6
//
// ***********************************************************************
#include <stdio.h>
#include <string.h>
#include "os345.h"
#include "os345fat.h"

// ***********************************************************************
// ***********************************************************************
//	functions to implement in Project 6
//
int fmsCloseFile(int);

int fmsDefineFile(char *, int);

int fmsDeleteFile(char *);

int fmsOpenFile(char *, int);

int fmsReadFile(int, char *, int);

int fmsSeekFile(int, int);

int fmsWriteFile(int, char *, int);

// ***********************************************************************
// ***********************************************************************
//	Support functions available in os345p6.c
//
extern int fmsGetDirEntry(char *fileName, DirEntry *dirEntry);

extern int fmsGetNextDirEntry(int *dirNum, char *mask, DirEntry *dirEntry, int dir);

extern int fmsMount(char *fileName, void *ramDisk);

extern void setFatEntry(int FATindex, unsigned short FAT12ClusEntryVal, unsigned char *FAT);

extern unsigned short getFatEntry(int FATindex, unsigned char *FATtable);

extern int fmsMask(char *mask, char *name, char *ext);

extern void setDirTimeDate(DirEntry *dir);

extern int isValidFileName(char *fileName);

extern void printDirectoryEntry(DirEntry *);

extern void fmsError(int);

extern int fmsReadSector(void *buffer, int sectorNumber);

extern int fmsWriteSector(void *buffer, int sectorNumber);

// ***********************************************************************
// ***********************************************************************
// fms variables
//
// RAM disk
unsigned char RAMDisk[SECTORS_PER_DISK * BYTES_PER_SECTOR];

// File Allocation Tables (FAT1 & FAT2)
unsigned char FAT1[NUM_FAT_SECTORS * BYTES_PER_SECTOR];
unsigned char FAT2[NUM_FAT_SECTORS * BYTES_PER_SECTOR];

char dirPath[128];                            // current directory path
FDEntry OFTable[MAX_OPEN_FILES];                    // open file table

extern bool diskMounted;                    // disk has been mounted
extern TCB tcb[];                            // task control block
extern int curTask;                            // current task #


// ***********************************************************************
// ***********************************************************************
// This function closes the open file specified by fileDescriptor.
// The fileDescriptor was returned by fmsOpenFile and is an index into the open file table.
//	Return 0 for success, otherwise, return the error number.
//
int fmsCloseFile(int fileDescriptor) {
    if (!IsValidFd(fileDescriptor)) {
        return FATERR_INVALID_DESCRIPTOR;
    }
    FDEntry *fdEntry = &OFTable[fileDescriptor];
    if (fdEntry->name[0] == 0) return FATERR_FILE_NOT_OPEN;

    // TODO: Flush if buffer altered (really should just have flush function)

    fdEntry->name[0] = 0;

    return FATERR_SUCCESS;
} // end fmsCloseFile



// ***********************************************************************
// ***********************************************************************
// If attribute=DIRECTORY, this function creates a new directory
// file directoryName in the current directory.
// The directory entries "." and ".." are also defined.
// It is an error to try and create a directory that already exists.
//
// else, this function creates a new file fileName in the current directory.
// It is an error to try and create a file that already exists.
// The start cluster field should be initialized to cluster 0.  In FAT-12,
// files of size 0 should point to cluster 0 (otherwise chkdsk should report an error).
// Remember to change the start cluster field from 0 to a free cluster when writing to the
// file.
//
// Return 0 for success, otherwise, return the error number.
//
int fmsDefineFile(char *fileName, int attribute) {
    // ?? add code here
    printf("\nfmsDefineFile Not Implemented");

    return FATERR_DISK_NOT_MOUNTED;
} // end fmsDefineFile



// ***********************************************************************
// ***********************************************************************
// This function deletes the file fileName from the current director.
// The file name should be marked with an "E5" as the first character and the chained
// clusters in FAT 1 reallocated (cleared to 0).
// Return 0 for success; otherwise, return the error number.
//
int fmsDeleteFile(char *fileName) {
    // ?? add code here
    printf("\nfmsDeleteFile Not Implemented");

    return FATERR_FILE_NOT_DEFINED;
} // end fmsDeleteFile



// ***********************************************************************
// ***********************************************************************
// This function opens the file fileName for access as specified by rwMode.
// It is an error to try to open a file that does not exist.
// The open mode rwMode is defined as follows:
//    0 - Read access only.
//       The file pointer is initialized to the beginning of the file.
//       Writing to this file is not allowed.
//    1 - Write access only.
//       The file pointer is initialized to the beginning of the file.
//       Reading from this file is not allowed.
//    2 - Append access.
//       The file pointer is moved to the end of the file.
//       Reading from this file is not allowed.
//    3 - Read/Write access.
//       The file pointer is initialized to the beginning of the file.
//       Both read and writing to the file is allowed.
// A maximum of 32 files may be open at any one time.
// If successful, return a file descriptor that is used in calling subsequent file
// handling functions; otherwise, return the error number.
//
int fmsOpenFile(char *fileName, int rwMode) {
    if (!IsValidOpenMode(rwMode)) {
        return FATERR_INVALID_MODE;
    }
    int err;

    // Get the associated directory entry
    DirEntry dirEntry;
    if ((err = fmsGetDirEntry(fileName, &dirEntry)) != 0) {
        return err;
    }
    if (dirEntry.name[0] == 0 || dirEntry.name[0] == ' ') {
        return FATERR_INVALID_FILE_NAME;
    }

    // Find an available file descriptor (and ensure the file is not opened)
    // Go backwards so we can check every file but also get the lowest available fd
    int newFd = -1;
    for (int checkFd = MAX_OPEN_FILES - 1; checkFd >= 0; --checkFd) {
        FDEntry *checkEntry = &OFTable[checkFd];

        if (checkEntry->name[0] == 0) {
            newFd = checkFd;
        } else if (checkEntry->startCluster == dirEntry.startCluster) {
            return FATERR_FILE_ALREADY_OPEN;
        }
    }
    if (newFd == -1) {
        return FATERR_TOO_MANY_OPEN;
    }
    FDEntry *fdEntry = &OFTable[newFd];

    // Copy data from dirEntry into fdEntry
    // (the descriptor is now "used" as the name is set)
    memcpy(fdEntry->name, dirEntry.name, 8);
    memcpy(fdEntry->extension, dirEntry.extension, 3);
    fdEntry->attributes = dirEntry.attributes;
    // TODO: Verify
    fdEntry->directoryCluster = (uint16) CDIR;
    fdEntry->startCluster = dirEntry.startCluster;
    fdEntry->currentCluster = 0;
    fdEntry->fileSize = dirEntry.fileSize;
    fdEntry->pid = curTask;
    fdEntry->mode = (char) rwMode;
    fdEntry->flags = 0;
    fdEntry->fileIndex = 0;
    memset(fdEntry->buffer, 0, sizeof(fdEntry->buffer));

    if (rwMode == OPEN_WRITE || rwMode == OPEN_RDWR) {
        // TODO: Change modification date
    }
    if (rwMode == OPEN_WRITE) {
        // TODO: Clear file on open with write
    }
    if (rwMode == OPEN_APPEND) {
        fmsSeekFile(newFd, fdEntry->fileSize);
    }

    return newFd;
} // end fmsOpenFile



// ***********************************************************************
// ***********************************************************************
// This function reads nBytes bytes from the open file specified by fileDescriptor into
// memory pointed to by buffer.
// The fileDescriptor was returned by fmsOpenFile and is an index into the open file table.
// After each read, the file pointer is advanced.
// Return the number of bytes successfully read (if > 0) or return an error number.
// (If you are already at the end of the file, return EOF error.  ie. you should never
// return a 0.)
//
// wtf: nBytes should be a size_t or uint32!
int fmsReadFile(int fileDescriptor, char *buffer, int nBytes) {
    if (!IsValidFd(fileDescriptor)) {
        return FATERR_INVALID_DESCRIPTOR;
    }
    int error;
    uint16 nextCluster;
    FDEntry *fdEntry;
    int numBytesRead = 0;
    unsigned int bufferIndex;
    uint32 bytesLeft;
    fdEntry = &OFTable[fileDescriptor];
    if (fdEntry->name[0] == 0)
        return FATERR_FILE_NOT_OPEN;
    if ((fdEntry->mode == 1) || (fdEntry->mode == 2))
        return FATERR_ILLEGAL_ACCESS;
    while (nBytes > 0) {
        // wtf: wouldn't it make more sense to just return 0 rather than an "error"?
        if (fdEntry->fileSize == fdEntry->fileIndex)
            return (numBytesRead ? numBytesRead : FATERR_END_OF_FILE);
        bufferIndex = fdEntry->fileIndex % BYTES_PER_SECTOR;
        if ((bufferIndex == 0) && (fdEntry->fileIndex || !fdEntry->currentCluster)) {
            if (fdEntry->currentCluster == 0) {
                if (fdEntry->startCluster == 0) return FATERR_END_OF_FILE;
                nextCluster = fdEntry->startCluster;
                fdEntry->fileIndex = 0;
            } else {
                nextCluster = getFatEntry(fdEntry->currentCluster, FAT1);
                if (nextCluster == FAT_EOC) return numBytesRead;
            }
            if (fdEntry->flags & BUFFER_ALTERED) {
                // isn't it possible for sector size != cluster size?
                if ((error = fmsWriteSector(fdEntry->buffer,
                                            C_2_S(fdEntry->currentCluster))))
                    return error;
                fdEntry->flags &= ~BUFFER_ALTERED;
            }
            fdEntry->flags |= BUFFER_NOT_READ;
            fdEntry->currentCluster = nextCluster;
        }
        if (fdEntry->flags & BUFFER_NOT_READ) {
            if ((error = fmsReadSector(fdEntry->buffer, C_2_S(fdEntry->currentCluster))))
                return error;
            fdEntry->flags &= ~BUFFER_NOT_READ;
        }
        bytesLeft = BYTES_PER_SECTOR - bufferIndex;
        if (bytesLeft > nBytes) bytesLeft = (uint32) nBytes;
        if (bytesLeft > (fdEntry->fileSize - fdEntry->fileIndex))
            bytesLeft = fdEntry->fileSize - fdEntry->fileIndex;
        memcpy(buffer, &fdEntry->buffer[bufferIndex], bytesLeft);
        fdEntry->fileIndex += bytesLeft;
        numBytesRead += bytesLeft;
        buffer += bytesLeft;
        nBytes -= bytesLeft;
    }
    return numBytesRead;
} // end fmsReadFile




// ***********************************************************************
// ***********************************************************************
// This function changes the current file pointer of the open file specified by
// fileDescriptor to the new file position specified by index.
// The fileDescriptor was returned by fmsOpenFile and is an index into the open file table.
// The file position may not be positioned beyond the end of the file.
// Return the new position in the file if successful; otherwise, return the error number.
//
int fmsSeekFile(int fileDescriptor, int index) {
    if (!IsValidFd(fileDescriptor)) {
        return FATERR_INVALID_DESCRIPTOR;
    }
    FDEntry *fdEntry = &OFTable[fileDescriptor];
    if (fdEntry->name[0] == 0) return FATERR_FILE_NOT_OPEN;

    // wtf: Why can't you seek if you're writing? Makes no sense.
    if (fdEntry->mode != OPEN_READ && fdEntry->mode != OPEN_RDWR) {
        return FATERR_FILE_SEEK_ERROR;
    }
    // The index for the end of the file is fileSize.
    // Index 0 is the beginning of the file.
    // If the file is 0 bytes, index 0 is to read/write at the end.
    // If the file is n bytes, index n-1 is the last byte.
    // Therefore, it is valid for index to equal fdEntry->fileSize.
    if (index > fdEntry->fileSize) {
        // The docs don't say *what* to do when index is outside the bounds of the file.
        // It just says "the file position may not be positioned beyond the end of the file".
        // I choose to interpret this as bounding it to the end of the file.
        // Fun fact: POSIX says you can go past the end of the file,
        //           and any read from the gap will return zero bytes.
        //           This will also define a sparse file when possible.
        index = fdEntry->fileSize;
    } else if (index < 0) {
        // Nifty extra: pass in a negative index to go relative to the end of the file.
        // -1 is at the end of the file (fdEntry->fileSize)
        // -2 will position to read last byte of the file
        index = fdEntry->fileSize + 1 - index;
    }

    // # of sectors left to traverse through past the start cluster
    int sectorsLeft = index / BYTES_PER_SECTOR;
    int cluster = fdEntry->startCluster;
    while (sectorsLeft-- > 0) {
        cluster = getFatEntry(cluster, FAT1);
        if (cluster == FAT_EOC) {
            return FATERR_INVALID_FAT_CHAIN;
        } else if (cluster == FAT_BAD) {
            return FATERR_INVALID_SECTOR;
        }
    }
    fdEntry->flags |= BUFFER_NOT_READ;
    fdEntry->currentCluster = (uint16) cluster;
    // bah, index should be uint32 in the first place!
    fdEntry->fileIndex = (uint32) index;
    return index;
} // end fmsSeekFile



// ***********************************************************************
// ***********************************************************************
// This function writes nBytes bytes to the open file specified by fileDescriptor from
// memory pointed to by buffer.
// The fileDescriptor was returned by fmsOpenFile and is an index into the open file table.
// Writing is always "overwriting" not "inserting" in the file and always writes forward
// from the current file pointer position.
// Return the number of bytes successfully written; otherwise, return the error number.
//
int fmsWriteFile(int fileDescriptor, char *buffer, int nBytes) {
    // ?? add code here
    printf("\nfmsWriteFile Not Implemented");

    return FATERR_FILE_NOT_OPEN;
} // end fmsWriteFile
