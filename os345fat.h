// os345fat.h - file management system equates
#ifndef __os345fat_h__
#define __os345fat_h__
// ***************************************************************************************

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

//	size_t	uintptr_t

#define SECTORS_PER_DISK   2880
#define BYTES_PER_SECTOR   512
#define NUM_FAT_SECTORS    9
#define BEG_ROOT_SECTOR    19
#define BEG_DATA_SECTOR    33
#define CLUSTERS_PER_DISK  SECTORS_PER_DISK-BEG_DATA_SECTOR

#define MAX_OPEN_FILES       32            // # of valid open files

#define READ_ONLY 0x01
#define HIDDEN    0x02
#define SYSTEM    0x04
#define VOLUME    0x08                    // this is the volume label entry
#define DIRECTORY 0x10
#define ARCHIVE   0x20                    // same as file
#define LONGNAME  (READ_ONLY | HIDDEN | SYSTEM | VOLUME)

#define OPEN_READ        0                    // read only
#define OPEN_WRITE       1                    // write only
#define OPEN_APPEND      2                    // append
#define OPEN_RDWR        3                    // read/write
#define IsValidOpenMode(x) (((x) >= OPEN_READ) && ((x) <= OPEN_RDWR))

#define ENTRIES_PER_SECTOR 16
#define FAT_UNUSED 0
#define FAT_EOC   4095
#define FAT_BAD   4087
#define C_2_S(c) ((c) + BEG_DATA_SECTOR - 2)
#define S_2_C(s) ((s) - BEG_DATA_SECTOR + 2)
#define IsValidFd(x) (((x) >= 0) && ((x) < MAX_OPEN_FILES))

#define BigEndian(v) 1?v:((((v)>>8)&0x00ff))|((v)<<8)
#define lLE(v) LITTLE?v:((BigEndian(v)<<16))|(BigEndian((v)>>16))

#define FILE_SEP ("\\/")

typedef struct {
    uint16 free;                // # of sectors free
    uint16 used;                // # of sectors used
    uint16 bad;                // # of bad sectors
    uint16 size;                // Total # of sectors in RAM disk
} DiskSize;


#pragma pack(push,1)						// BYTE align in memory (no padding)
typedef struct {
    uint8 name[8];            // file name
    uint8 extension[3];        // extension
    uint8 attributes;            // file attributes code
    uint16 directoryCluster;    // directory cluster
    uint16 startCluster;        // first cluster of the file
    uint16 currentCluster;        // current cluster in buffer
    uint32 fileSize;            // file size in bytes
    int pid;                // process who opened file
    char mode;                // access mode (read, read-only, write, append)
    char flags;                // flags
    //   x80 = file altered
    //   x40 = buffer altered
    //   x20 = locked
    //   x10 =
    //   x08 = write protected
    //   x04 = contiguous
    //   x02 =
    //   x01 =
    uint32 fileIndex;            // next character position (from beg of file)
    uint16 directoryEntry;       // position in directory entry
    char buffer[BYTES_PER_SECTOR];    // file buffer
} FDEntry;
#pragma pack(pop)							// End of strict alignment

#define FILE_ALTERED       0x80
#define BUFFER_ALTERED     0x40
#define BUFFER_NOT_READ    0x20

#pragma pack(push,1)						// BYTE align in memory (no padding)
typedef struct {
    uint8 BS_jmpBoot[3];        // Jump instruction to the boot code
    uint8 BS_OEMName[8];        // Name of system that formatted the volume
    uint16 BPB_BytsPerSec;        // How many bytes in a sector (should be 512)
    uint8 BPB_SecPerClus;        // How many sectors are in a cluster (1)
    uint16 BPB_RsvdSecCnt;        // Number of sectors that are reserved (1)
    uint8 BPB_NumFATs;        // The number of FAT tables on the disk (2)
    uint16 BPB_RootEntCnt;        // Maximum # of directory entries in root directory
    uint16 BPB_TotSec16;        // FAT-12 total number of sectors on the disk
    uint8 BPB_Media;            // Code for media type {fixed, removable, etc.}
    uint16 BPB_FATSz16;        // FAT-12 # of sectors that each FAT occupies (9)
    uint16 BPB_SecPerTrk;        // Number of sectors in one cylindrical track
    uint16 BPB_NumHeads;        // Number of heads (2 heads for 1.4Mb 3.5" floppy)
    uint32 BPB_HiddSec;        // Number of preceding hidden sectors (0)
    uint32 BPB_TotSec32;        // FAT-32 number of sectors on the disk (0)
    uint8 BS_DrvNum;            // A drive number for the media (OS specific)
    uint8 BS_Reserved1;        // Reserved space for Windows NT (set to 0)
    uint8 BS_BootSig;            // Indicates following 3 fields are present (0x29)
    uint32 BS_VolID;            // Volume serial number (for tracking this disk)
    uint8 BS_VolLab[11];        // Volume label (matches rdl or "NO NAME    ")
    uint8 BS_FilSysType[8];    // Deceptive FAT type Label
    uint8 BS_fill[450];
} BSStruct;
#pragma pack(pop)							// End of strict alignment


// this struct may need to change for big endian
#pragma pack(push,1)			// BYTE align in memory (no padding)
typedef struct {                                // (total 16 bits--a uint16)
    uint16 sec: 5;                // low-order 5 bits are the seconds
    uint16 min: 6;                // next 6 bits are the minutes
    uint16 hour: 5;                // high-order 5 bits are the hour
} FATTime;
#pragma pack(pop)				// End of strict alignment


// this struct may need to change for big endian
#pragma pack(push,1)			// BYTE align in memory (no padding)
typedef struct {                                // (total 16 bits--a uint16)
    uint16 day: 5;                // low-order 5 bits are the day
    uint16 month: 4;                // next 4 bits are the month
    uint16 year: 7;                // high-order 7 bits are the year
} FATDate;
#pragma pack(pop)				// End of strict alignment

#pragma pack(push,1)			// BYTE align in memory (no padding)
typedef struct {
    uint8 name[8];            // File name
    uint8 extension[3];        // Extension
    uint8 attributes;            // Holds the attributes code
    uint8 reserved[10];        // Reserved
    FATTime time;                // Time of last write
    FATDate date;                // Date of last write
    uint16 startCluster;        // Pointer to the first cluster of the file.
    uint32 fileSize;            // File size in bytes
} DirEntry;
#pragma pack(pop)				// End of strict alignment

typedef struct {
    int error;
    char error_msg[32];
} FMSERROR;

typedef struct {
    DirEntry entry;
    int entryNum;
    int startCluster;
    int currentCluster;
    int clusterNum;
    char buffer[BYTES_PER_SECTOR];
} DirEnum;


// ***************************************************************************************
//	Prototypes
//
void dumpRAMDisk(char *, int, int);

void printDirectoryEntry(DirEntry *, uint16 *longFileName);

void printFatEntries(unsigned char *, int, int);

void setFatEntry(int FATindex, unsigned short FAT12ClusEntryVal, unsigned char *FAT);

unsigned short getFatEntry(int FATindex, unsigned char *FATtable);

int fmsMask(char *mask, char *name, char *ext);

void setDirTimeDate(DirEntry *dir);

void lfnToBuffer(uint16 *inBuffer, char *outBuffer, size_t outSize);

uint8 calcLfnChecksum(const uint8 name[8], const uint8 extension[3]);

void dirEntryToStr(const uint8 name[8], const uint8 extension[3], char *fileName);

void strToDirEntry(const char *fileName, uint8 name[8], uint8 extension[3]);

int isValidFileName(char *fileName);

int fmsCanUndeleteEntry(DirEntry *entry);

int fmsChangeDir(char *);

int fmsGetDirEntry(char *, DirEntry *, int dir);

int fmsGetNextFile(int *, char *, DirEntry *, int, uint16 *longFileName);

int fmsGetFirstDirEntry(int dir, DirEnum *dirEnum, int skip);

int fmsGetNextDirEntry(DirEnum *dirEnum, int skip);

int fmsGetPathDir(const char *path, int startDir, int *resultDir, char *tail);

int fmsCloseFile(int);

int fmsDefineFile(char *, int);

int fmsDeleteFile(char *);

int fmsRenameFile(char *, char *);

int fmsOpenFile(char *, int);

int fmsReadFile(int, char *, int);

int fmsSeekFile(int, int);

int fmsWriteFile(int, const char *, int);

int fmsLoadFile(char *, void *, int);

int fmsMount(char *, void *);

int getDirSector(int dir, int skip, int *outSector);

int fmsReadSector(void *, int);

int fmsWriteSector(void *, int);

int nextFreeCluster(int startCluster, int *outCluster, unsigned char *FAT);

int fmsUnMount(char *, void *);

int clearFATChain(int dirCluster, unsigned char *FAT);

int fmsReadDirEntry(int dir, int entryNum, DirEntry *dirEntry);

int fmsWriteDirEntry(int dir, int entryNum, const DirEntry *dirEntry);

void fmsError(int);

int fmsDiskStats(DiskSize *dskSize);

// ***************************************************************************************
//	FMS Errors

// WTF WTF WTF: Why would anyone EVER define errors as virtually unreadable macros?
// At least POSIX's errno.h errors are mostly readable, if terse.
// NT got this part right IMO: errors should be immediately readable in code, even if verbose.

#define NUM_ERRORS   28

#define FATERR_INVALID_FILE_NAME       -50
#define FATERR_INVALID_FILE_TYPE       -51
#define FATERR_INVALID_DESCRIPTOR      -52
#define FATERR_INVALID_SECTOR          -53
#define FATERR_INVALID_FAT_CHAIN       -54
#define FATERR_INVALID_DIRECTORY       -55
// Added by kupiakos
#define FATERR_INVALID_MODE            -56

#define FATERR_FILE_ALREADY_DEFINED    -60
#define FATERR_FILE_NOT_DEFINED        -61
#define FATERR_FILE_ALREADY_OPEN       -62
#define FATERR_FILE_NOT_OPEN           -63
#define FATERR_FILE_DIRECTORY_FULL     -64
#define FATERR_FILE_SPACE_FULL         -65
#define FATERR_END_OF_FILE             -66
#define FATERR_END_OF_DIRECTORY        -67
#define FATERR_DIRECTORY_NOT_FOUND     -68
#define FATERR_CANNOT_DELETE           -69

#define FATERR_TOO_MANY_OPEN           -70
#define FATERR_OUT_OF_CONTIGUOUS_SPACE -71
#define FATERR_DISK_NOT_MOUNTED        -72

#define FATERR_FILE_SEEK_ERROR         -80
#define FATERR_FILE_LOCKED             -81
#define FATERR_FILE_DELETE_PROTECTED   -82
#define FATERR_FILE_WRITE_PROTECTED    -83
#define FATERR_READ_ONLY_FILE          -84
#define FATERR_ILLEGAL_ACCESS          -85

#define FATERR_SUCCESS       0
#define FATERR_UNDEFINED    -1

// Kept with the original numbers so I can map to the online docs more easily
#define ERR50_MSG "Invalid File Name"
#define ERR51_MSG "Invalid File Type"
#define ERR52_MSG "Invalid File Descriptor"
#define ERR53_MSG "Invalid Sector Number"
#define ERR54_MSG "Invalid FAT Chain"
#define ERR55_MSG "Invalid Directory"
#define ERR56_MSG "Invalid Open Mode"

#define ERR60_MSG "File Already Defined"
#define ERR61_MSG "File Not Defined"
#define ERR62_MSG "File Already Open"
#define ERR63_MSG "File Not Open"
#define ERR64_MSG "File Directory Full"
#define ERR65_MSG "File Space Full"
#define ERR66_MSG "End-Of-File"
#define ERR67_MSG "End-Of-Directory"
#define ERR68_MSG "Directory Not Found"
#define ERR69_MSG "Cannot Delete"

#define ERR70_MSG "Too Many Files Open"
#define ERR71_MSG "Not Enough Contiguous Space"
#define ERR72_MSG "Disk Not Mounted"

#define ERR80_MSG "File Seek Error"
#define ERR81_MSG "File Locked"
#define ERR82_MSG "File Delete Protected"
// wtf: what is the difference between "write protected" and "read only"?
#define ERR83_MSG "File Write Protected"
#define ERR84_MSG "Read Only File"
#define ERR85_MSG "Illegal Access"

#define SUCCESS_MSG   "Success"
#define UNDEFINED_MSG "Undefined Error"

#endif // __os345fat_h__
