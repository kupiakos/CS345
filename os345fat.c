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
#include <assert.h>
#include <ctype.h>
#include "os345.h"
#include "os345fat.h"

// ***********************************************************************
// ***********************************************************************
//	functions to implement in Project 6
//
int fmsCloseFile(int);

int fmsDefineFile(char *, int);

int fmsRenameFile(char *fromName, char *toName);

int fmsDeleteFile(char *);

int fmsOpenFile(char *, int);

int fmsReadFile(int, char *, int);

int fmsSeekFile(int, int);

int fmsFlushFile(int);

int fmsWriteFile(int, const char *, int);

// ***********************************************************************
// ***********************************************************************
//	Support functions available in os345p6.c
//
extern int fmsGetDirEntry(char *fileName, DirEntry *dirEntry, int dir);

extern int fmsGetNextFile(int *entryNum, char *mask, DirEntry *dirEntry, int dir, uint16 *longFileName);

extern int fmsMount(char *fileName, void *ramDisk);

extern void setFatEntry(int FATindex, unsigned short FAT12ClusEntryVal, unsigned char *FAT);

extern unsigned short getFatEntry(int FATindex, unsigned char *FATtable);

extern int fmsMask(char *mask, char *name, char *ext);

extern void setDirTimeDate(DirEntry *dir);

extern int isValidFileName(char *fileName);

extern void printDirectoryEntry(DirEntry *, uint16 *longFileName);

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

    fmsFlushFile(fileDescriptor);

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
    int dir = CDIR;
    int error;
    // TODO: Modify for LFN
    if (isValidFileName(fileName) != 1) {
        return FATERR_INVALID_FILE_NAME;
    }

    char fileNameBuf[strlen(fileName) + 1];
    size_t fileNameSize = strlen(fileName);
    for (int i = 0; i < fileNameSize; ++i)
        fileNameBuf[i] = (char) toupper(fileName[i]);
    fileNameBuf[fileNameSize] = 0;
    fileName = fileNameBuf;

    DirEntry entry;
    error = fmsGetDirEntry(fileName, &entry, dir);
    if (error != FATERR_FILE_NOT_DEFINED) {
        if (error == FATERR_SUCCESS)
            return FATERR_FILE_ALREADY_DEFINED;
        return error;
    }

    // We need to find a directory entry to write to
    DirEnum dirEnum;
    error = fmsGetFirstDirEntry(dir, &dirEnum, 0);
    if (error) return error;
    while ((error = fmsGetNextDirEntry(&dirEnum, 0)) == FATERR_SUCCESS) {
        // Either unused or deleted entry
        if (dirEnum.entry.name[0] == 0 || dirEnum.entry.name[0] == 0xe5) {
            break;
        }
    }
    int dirCluster, index;
    if (error == FATERR_SUCCESS) {
        // We broke out of the loop because we found an available entry to write to
        dirCluster = dirEnum.currentCluster;
        index = dirEnum.entryNum;

        if (dir != 0) {
            // The root directory can store "past" its entries per sector, but subdirectories cannot
            index %= ENTRIES_PER_SECTOR;
        }
    } else if (error == FATERR_END_OF_DIRECTORY) {
        // There wasn't enough room in the current directory
        if (dir == 0) {
            // We can't create more room in the root directory
            return FATERR_FILE_DIRECTORY_FULL;
        }
        // Add a new cluster to make this directory larger
        error = nextFreeCluster(dirEnum.currentCluster, &dirCluster, FAT1);
        if (error) return error;
        // Completely clear the new sector
        char buffer[BYTES_PER_SECTOR];
        memset(buffer, 0, sizeof buffer);
        error = fmsWriteSector(buffer, C_2_S(dirCluster));
        if (error) return error;
        // Claim this cluster
        setFatEntry(dirCluster, FAT_EOC, FAT1);
        setFatEntry(dirCluster, FAT_EOC, FAT2);
        // Reroute the previous cluster to here
        setFatEntry(dirEnum.currentCluster, (unsigned short) dirCluster, FAT1);
        setFatEntry(dirEnum.currentCluster, (unsigned short) dirCluster, FAT2);
        // We'll now be writing to the first entry
        index = 0;
    } else {
        // A problem occurred
        return error;
    }

    // Now that we have a directory entry for our new file, create the file itself
    strToDirEntry(fileName, entry.name, entry.extension);

    entry.attributes = (uint8) attribute;
    setDirTimeDate(&entry);
    entry.startCluster = 0;
    // http://www.maverick-os.dk/FileSystemFormats/FAT16_FileSystem.html#FileSize
    // "For other entries than files then file size field should be set to 0."
    // i.e., we don't need to keep track of fileSize if we're creating a directory
    entry.fileSize = 0;

    if (attribute & DIRECTORY) {
        // Create a cluster to hold the data for the new directory
        int newCluster;
        error = nextFreeCluster(dirEnum.startCluster, &newCluster, FAT1);
        if (error) return error;
        // Completely clear the new sector
        char buffer[BYTES_PER_SECTOR];
        memset(buffer, 0, sizeof buffer);
        error = fmsWriteSector(buffer, C_2_S(newCluster));
        if (error) return error;
        // Claim this cluster
        setFatEntry(newCluster, FAT_EOC, FAT1);
        setFatEntry(newCluster, FAT_EOC, FAT2);

        entry.startCluster = (uint16) newCluster;
        // Create the default names and add them to the directory
        char *names[] = {".", ".."};
        for (int i = 0; i < sizeof(names) / sizeof(*names); ++i) {
            char *name = names[i];
            DirEntry childEntry;
            memset(childEntry.name, ' ', 8);
            memcpy(childEntry.name, name, strlen(name));
            memset(childEntry.extension, ' ', 3);
            childEntry.attributes = DIRECTORY;
            setDirTimeDate(&childEntry);
            if (i == 0)
                childEntry.startCluster = (uint16) entry.startCluster;
            else
                childEntry.startCluster = (uint16) dir;
            childEntry.fileSize = 0;
            error = fmsWriteDirEntry(newCluster, i, &childEntry);
            if (error) return error;
        }
    }

    error = fmsWriteDirEntry(dirCluster, index, &entry);
    if (error) return error;

    return FATERR_SUCCESS;
} // end fmsDefineFile


int fmsRenameFile(char *fromName, char *toName) {
    int dir = CDIR;
    int error;

    if (isValidFileName(toName) != 1)
        return FATERR_INVALID_FILE_NAME;


    int entryNum = 0;
    DirEntry entry;
    error = fmsGetNextFile(&entryNum, fromName, &entry, dir, NULL);
    --entryNum;
    if (error) return error;

    if (entry.name[0] == '.')
        return FATERR_INVALID_FILE_NAME;

    strToDirEntry(toName, entry.name, entry.extension);

    error = fmsWriteDirEntry(dir, entryNum, &entry);
    return error;
}

// ***********************************************************************
// ***********************************************************************
// This function deletes the file fileName from the current director.
// The file name should be marked with an "E5" as the first character and the chained
// clusters in FAT 1 reallocated (cleared to 0).
// Return 0 for success; otherwise, return the error number.
//
int fmsDeleteFile(char *mask) {
    int dir = CDIR;
    int error;
//    if (isValidFileName(mask) != 1) {
//        return FATERR_INVALID_FILE_NAME;
//    }
    bool any = false;

    while (1) {
        int entryNum = 0;
        DirEntry entry;
        error = fmsGetNextFile(&entryNum, mask, &entry, dir, NULL);
        --entryNum;
        if (error) {
            if (!any || error != FATERR_END_OF_DIRECTORY)
                return error;
            return FATERR_SUCCESS;
        }
        any = true;

        if (entry.attributes & DIRECTORY) {
            DirEntry childEntry;
            int childEntryNum = 0;
            while ((error = fmsGetNextFile(&childEntryNum, "*.*", &childEntry, entry.startCluster, NULL)) ==
                   FATERR_SUCCESS) {
                if (childEntry.name[0] != '.') {
                    // Not an empty directory
                    return FATERR_CANNOT_DELETE;
                }
            }
            if (error != FATERR_END_OF_DIRECTORY) {
                return error;
            }
        }

        entry.name[0] = 0xe5;
        error = fmsWriteDirEntry(dir, entryNum, &entry);
        if (error) return error;

        clearFATChain(entry.startCluster, FAT1);
        clearFATChain(entry.startCluster, FAT2);
    }

    return FATERR_SUCCESS;
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
    int entryNum = 0;
    int dir = CDIR;
    if ((err = fmsGetNextFile(&entryNum, fileName, &dirEntry, dir, NULL)) != 0) {
        if (err == FATERR_END_OF_DIRECTORY) {
            if (rwMode == OPEN_WRITE || rwMode == OPEN_RDWR) {
                err = fmsDefineFile(fileName, ARCHIVE);
                if (err) return err;
                return fmsOpenFile(fileName, rwMode);
            } else {
                return FATERR_FILE_NOT_DEFINED;
            }
        } else {
            return err;
        }
    }
    // The entryNum will be off by one as it points to the next to start with for fmsGetNextFile
    --entryNum;
    if (dirEntry.name[0] == 0 || dirEntry.name[0] == ' ') {
        return FATERR_INVALID_FILE_NAME;
    }

    if (dirEntry.attributes & READ_ONLY && rwMode != OPEN_READ) {
        return FATERR_FILE_WRITE_PROTECTED;
    }

    if (dirEntry.attributes & DIRECTORY) {
        return FATERR_ILLEGAL_ACCESS;
    }

    // Find an available file descriptor (and ensure the file is not opened)
    // Go backwards so we can check every file but also get the lowest available fd
    int newFd = -1;
    for (int checkFd = MAX_OPEN_FILES - 1; checkFd >= 0; --checkFd) {
        FDEntry *checkEntry = &OFTable[checkFd];

        if (checkEntry->name[0] == 0) {
            newFd = checkFd;
        } else if (checkEntry->directoryCluster == dir &&
                   strncasecmp((char *) checkEntry->name, (char *) dirEntry.name, 8) == 0 &&
                   strncasecmp((char *) checkEntry->extension, (char *) dirEntry.extension, 3) == 0) {
            return FATERR_FILE_ALREADY_OPEN;
        }
    }
    if (newFd == -1) {
        return FATERR_TOO_MANY_OPEN;
    }
    FDEntry *fdEntry = &OFTable[newFd];

    if (rwMode != OPEN_READ) {
        if (rwMode == OPEN_WRITE) {
            // truncate the file
            clearFATChain(dirEntry.startCluster, FAT1);
            clearFATChain(dirEntry.startCluster, FAT2);
            dirEntry.startCluster = 0;
            dirEntry.fileSize = 0;
        }
        // update the modify time/date
        setDirTimeDate(&dirEntry);
        dirEntry.attributes |= ARCHIVE;

        err = fmsWriteDirEntry(dir, entryNum, &dirEntry);
        if (err) return err;
    }

    // Copy data from dirEntry into fdEntry
    // (the descriptor is now "used" as the name is set)
    memcpy(fdEntry->name, dirEntry.name, 8);
    memcpy(fdEntry->extension, dirEntry.extension, 3);
    fdEntry->attributes = dirEntry.attributes;
    fdEntry->directoryCluster = (uint16) dir;
    fdEntry->directoryEntry = (uint16) entryNum;
    fdEntry->startCluster = dirEntry.startCluster;
    fdEntry->currentCluster = 0;
    fdEntry->fileSize = dirEntry.fileSize;
    fdEntry->pid = curTask;
    fdEntry->mode = (char) rwMode;
    fdEntry->flags = 0;
    fdEntry->fileIndex = 0;
    memset(fdEntry->buffer, 0, sizeof(fdEntry->buffer));

    if (rwMode == OPEN_APPEND) {
        err = fmsSeekFile(newFd, fdEntry->fileSize);
        if (err < 0) return err;
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
    fdEntry = &OFTable[fileDescriptor];
    if (fdEntry->name[0] == 0)
        return FATERR_FILE_NOT_OPEN;
    if ((fdEntry->mode == OPEN_WRITE) || (fdEntry->mode == OPEN_APPEND))
        return FATERR_ILLEGAL_ACCESS;
    while (nBytes > 0) {
        // wtf: wouldn't it make more sense to just return 0 rather than an "error"?
        if (fdEntry->fileSize == fdEntry->fileIndex)
            return (numBytesRead ? numBytesRead : FATERR_END_OF_FILE);
        unsigned int bufferIndex = fdEntry->fileIndex % BYTES_PER_SECTOR;
        if ((bufferIndex == 0) && (fdEntry->fileIndex || !fdEntry->currentCluster)) {
            if (fdEntry->currentCluster == 0) {
                // empty file
                if (fdEntry->startCluster == 0) return FATERR_END_OF_FILE;
                nextCluster = fdEntry->startCluster;
                fdEntry->fileIndex = 0;
            } else {
                nextCluster = getFatEntry(fdEntry->currentCluster, FAT1);
                if (nextCluster == FAT_EOC) return numBytesRead;
            }
            fmsFlushFile(fileDescriptor);
            fdEntry->flags |= BUFFER_NOT_READ;
            fdEntry->currentCluster = nextCluster;
        }
        if (fdEntry->flags & BUFFER_NOT_READ) {
            if ((error = fmsReadSector(fdEntry->buffer, C_2_S(fdEntry->currentCluster))))
                return error;
            fdEntry->flags &= ~BUFFER_NOT_READ;
        }
        uint32 bytesLeftInSector = BYTES_PER_SECTOR - bufferIndex;
        if (bytesLeftInSector > nBytes) bytesLeftInSector = (uint32) nBytes;
        if (bytesLeftInSector > (fdEntry->fileSize - fdEntry->fileIndex))
            bytesLeftInSector = fdEntry->fileSize - fdEntry->fileIndex;
        memcpy(buffer, &fdEntry->buffer[bufferIndex], bytesLeftInSector);
        fdEntry->fileIndex += bytesLeftInSector;
        numBytesRead += bytesLeftInSector;
        buffer += bytesLeftInSector;
        nBytes -= bytesLeftInSector;
    }
    return numBytesRead;
} // end fmsReadFile



// ***********************************************************************
// ***********************************************************************
// This function writes nBytes bytes to the open file specified by fileDescriptor from
// memory pointed to by buffer.
// The fileDescriptor was returned by fmsOpenFile and is an index into the open file table.
// Writing is always "overwriting" not "inserting" in the file and always writes forward
// from the current file pointer position.
// Return the number of bytes successfully written; otherwise, return the error number.
//
int fmsWriteFile(int fileDescriptor, const char *data, int nBytes) {
    if (!IsValidFd(fileDescriptor)) {
        return FATERR_INVALID_DESCRIPTOR;
    }
    int error;
    int nextCluster;
    FDEntry *fdEntry;
    int numWritten = 0;

    fdEntry = &OFTable[fileDescriptor];
    if (fdEntry->name[0] == 0)
        return FATERR_FILE_NOT_OPEN;
    if ((fdEntry->mode == OPEN_READ))
        return FATERR_ILLEGAL_ACCESS;

    while (nBytes > 0) {
        unsigned int bufferIndex = fdEntry->fileIndex % BYTES_PER_SECTOR;
        if ((bufferIndex == 0) && (fdEntry->fileIndex || !fdEntry->currentCluster)) {
            // need find the next cluster to write to
            int startCluster = S_2_C(BEG_DATA_SECTOR);
            nextCluster = 0;
            if (fdEntry->currentCluster == 0) {
                // have not started going through file
                if (fdEntry->startCluster == 0) {
                    // file is completely empty, we need to find another sector to write to
                    startCluster = S_2_C(BEG_DATA_SECTOR);
                    fdEntry->fileIndex = 0;
                } else {
                    // the file has data, but we haven't starting writing yet
                    nextCluster = fdEntry->startCluster;
                }
            } else {
                // been going through file, but just hit end of sector
                nextCluster = getFatEntry(fdEntry->currentCluster, FAT1);
                if (nextCluster == FAT_EOC) {
                    startCluster = fdEntry->currentCluster;
                    nextCluster = 0;
                }
                fmsFlushFile(fileDescriptor);
            }
            if (!nextCluster) {
                // We need to find another sector to write to
                if ((error = nextFreeCluster(startCluster, &nextCluster, FAT1))) {
                    return error;
                }
                assert(nextCluster >= 2);
                // Make the free cluster ours
                setFatEntry(nextCluster, FAT_EOC, FAT1);
                setFatEntry(nextCluster, FAT_EOC, FAT2);
                if (fdEntry->currentCluster) {
                    // We need to update the current cluster to point to the next
                    setFatEntry(fdEntry->currentCluster, (unsigned short) nextCluster, FAT1);
                    setFatEntry(fdEntry->currentCluster, (unsigned short) nextCluster, FAT2);
                } else {
                    // We need to update the directory entry to point to the start
                    fdEntry->startCluster = (uint16) nextCluster;
                    DirEntry dirEntry;
                    if ((error = fmsReadDirEntry(fdEntry->directoryCluster, fdEntry->directoryEntry, &dirEntry))) {
                        return error;
                    }
                    dirEntry.startCluster = fdEntry->startCluster;
                    if ((error = fmsWriteDirEntry(fdEntry->directoryCluster, fdEntry->directoryEntry, &dirEntry))) {
                        return error;
                    }
                }
            }
            if (fdEntry->mode == OPEN_RDWR)
                fdEntry->flags |= BUFFER_NOT_READ;
            fdEntry->currentCluster = (uint16) nextCluster;
        }
        if (fdEntry->flags & BUFFER_NOT_READ) {
            if ((error = fmsReadSector(fdEntry->buffer, C_2_S(fdEntry->currentCluster))))
                return error;
            fdEntry->flags &= ~BUFFER_NOT_READ;
        }
        uint32 bytesLeftInSector = BYTES_PER_SECTOR - bufferIndex;
        if (bytesLeftInSector > nBytes)
            bytesLeftInSector = (uint32) nBytes;
        fdEntry->flags |= BUFFER_ALTERED;
        memcpy(&fdEntry->buffer[bufferIndex], data, bytesLeftInSector);
        fdEntry->fileIndex += bytesLeftInSector;
        if (fdEntry->fileIndex > fdEntry->fileSize)
            fdEntry->fileSize = fdEntry->fileIndex;
        numWritten += bytesLeftInSector;
        data += bytesLeftInSector;
        nBytes -= bytesLeftInSector;
    }

    return numWritten;
} // end fmsWriteFile



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
    int error;
    FDEntry *fdEntry = &OFTable[fileDescriptor];
    if (fdEntry->name[0] == 0)
        return FATERR_FILE_NOT_OPEN;

    // wtf: Why can't you seek if you're writing? Makes no sense.
    if (fdEntry->mode != OPEN_READ && fdEntry->mode != OPEN_RDWR) {
        if (fdEntry->mode != OPEN_APPEND || fdEntry->currentCluster != 0) {
            // The initial seek for an append still needs to be done.
            return FATERR_FILE_SEEK_ERROR;
        }
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
    int sectorsLeft = (index - 1) / BYTES_PER_SECTOR;
    int cluster = fdEntry->startCluster;
    while (sectorsLeft--) {
        cluster = getFatEntry(cluster, FAT1);
        if (cluster == FAT_EOC) {
            return FATERR_INVALID_FAT_CHAIN;
        } else if (cluster == FAT_BAD) {
            return FATERR_INVALID_SECTOR;
        }
    }
    error = fmsFlushFile(fileDescriptor);
    if (error) return error;

    fdEntry->flags |= BUFFER_NOT_READ;
    fdEntry->currentCluster = (uint16) cluster;
    // bah, index should be uint32 in the first place!
    fdEntry->fileIndex = (uint32) index;
    return index;
} // end fmsSeekFile


// Write the current buffered data to disk
int fmsFlushFile(int fileDescriptor) {
    if (!IsValidFd(fileDescriptor)) {
        return FATERR_INVALID_DESCRIPTOR;
    }
    int error;
    FDEntry *fdEntry;

    fdEntry = &OFTable[fileDescriptor];
    if (fdEntry->name[0] == 0)
        return FATERR_FILE_NOT_OPEN;

    if (fdEntry->mode == OPEN_READ) {
        return FATERR_SUCCESS;
    }

    // Update file in directory entry
    DirEntry dirEntry;
    error = fmsReadDirEntry(fdEntry->directoryCluster, fdEntry->directoryEntry, &dirEntry);
    if (error) return error;

    dirEntry.startCluster = fdEntry->startCluster;
    dirEntry.fileSize = fdEntry->fileSize;
    setDirTimeDate(&dirEntry);

    error = fmsWriteDirEntry(fdEntry->directoryCluster, fdEntry->directoryEntry, &dirEntry);
    if (error) return error;

    if (!(fdEntry->flags & BUFFER_ALTERED))
        return FATERR_SUCCESS;

    if (!fdEntry->currentCluster)
        return FATERR_SUCCESS;

    if ((error = fmsWriteSector(fdEntry->buffer,
                                C_2_S(fdEntry->currentCluster)))) {
        return error;
    }
    fdEntry->flags &= ~BUFFER_ALTERED;

    return FATERR_SUCCESS;
}
