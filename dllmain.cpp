#include "pch.h"
#include "str_ing.h"
#define SHMEMSIZE 4096 
static LPVOID lpvMem = NULL;      // pointer to shared memory
static HANDLE hMapObject = NULL;  // handle to file mapping

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved) {
    BOOL fInit, fIgnore;

    switch (ul_reason_for_call) {
        // DLL load due to process initialization or LoadLibrary

    case DLL_PROCESS_ATTACH:

        // Create a named file mapping object
        hMapObject = CreateFileMapping(
            INVALID_HANDLE_VALUE,   // use paging file
            NULL,                   // default security attributes
            PAGE_READWRITE,         // read/write access
            0,                      // size: high 32-bits
            SHMEMSIZE,              // size: low 32-bits
            TEXT("sapphiredllmap")); // name of map object
        if (hMapObject == NULL)
            return FALSE;

        // The first process to attach initializes memory
        fInit = (GetLastError() != ERROR_ALREADY_EXISTS);

        // Get a pointer to the file-mapped shared memory
        lpvMem = MapViewOfFile(
            hMapObject,     // object to map view of
            FILE_MAP_WRITE, // read/write access
            0,              // high offset:  map from
            0,              // low offset:   beginning
            0);             // default: map entire file
        if (lpvMem == NULL)
            return FALSE;

        // Initialize memory if this is the first process
        if (fInit)
            memset(lpvMem, '\0', SHMEMSIZE);
        break;

    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;

        // DLL unload due to process termination or FreeLibrary
    case DLL_PROCESS_DETACH:

        // Unmap shared memory from the process's address space
        fIgnore = UnmapViewOfFile(lpvMem);
        // Close the process's handle to the file-mapping object
        fIgnore = CloseHandle(hMapObject);
        break;

    default:
        break;
    }
    return TRUE;
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(lpReserved);
}
// SetSharedMem sets the contents of the shared memory 
//    __declspec(dllexport) VOID __cdecl SetSharedMem(LPWSTR lpszBuf)
extern "C" __declspec(dllexport) VOID __cdecl SetSharedMem(const char* lpszBuf) {
    char* lpszTmp;
    DWORD dwCount = 1;

    // Get the address of the shared memory block
    lpszTmp = (char*)lpvMem;
    // Copy the null-terminated string into shared memory
    while (*lpszBuf && dwCount < SHMEMSIZE) {
        *lpszTmp++ = *lpszBuf++;
        dwCount++;
    }
    *lpszTmp = '\0';
}

// GetSharedMem gets the contents of the shared memory
extern "C" __declspec(dllexport) VOID __cdecl GetSharedMem(char* lpszBuf, DWORD cchSize) {
    char* lpszTmp;

    // Get the address of the shared memory block
    lpszTmp = (char*)lpvMem;
    // Copy from shared memory into the caller's buffer
    while (*lpszTmp && --cchSize)
        *lpszBuf++ = *lpszTmp++;
    *lpszBuf = '\0';
}
//========================================================================
// cloned from Ndbdefs.h
#define MAXNAME			32			// size of user, table, field, or index name
// local
#define MAXUSER         10
#define MAXLOCK         100
#define OFFSETUSER      0
#define OFFSETLOCK      MAXUSER*(MAXNAME+1)

typedef struct user_array {
    char    userName[MAXNAME + 1];
    } USER;
typedef struct record_lock_array {
    int     userno;
    int     fileno;
    int     tableno;
    int     recno;
    } LOCK;

//USER    usr[MAXUSER];
USER*   usrp;
//LOCK    lock[MAXLOCK];
LOCK*   lockp;
int lockscan(int  usr, int fd, int tab, int recno);

//========================================================================
// Add a char string username to shared userarray (if it doesn't already
// exist. Return subscript or -1
//extern "C" __declspec(dllexport) int userAdd(const char* username) {
int userAdd(const char* username) {
    usrp = (USER*)lpvMem;
    // scan array for existing usage of 'username'
    for (int i = 0; i < MAXUSER; i++, usrp++) {
        if (strcmp(usrp->userName, username) == 0)
            return (-i);
        }
    usrp = (USER*)lpvMem;
    // scan array for unused slot
    for (int i = 0; i < MAXUSER; i++, usrp++) {
        if (strcmp(usrp->userName, "\0") == 0) {
            str_cpy(usrp->userName, username);    // add new username
            return i;
            }
        }
    return -1;
    }
//========================================================================
// Retire a username from shared userarray
//extern "C" __declspec(dllexport) void userRetire(int userno) {
void userRetire(int userno) {
    usrp = (USER*)lpvMem;
    memset((void *)(usrp+userno), '\0', MAXNAME);
    }
//========================================================================
// Try to lock a record in lock_array. Return 0 if successful, -1 if not
//extern "C" __declspec(dllexport) int recordLock(int usr, int fd, int tab, int recno) {
int recordLock(int usr, int fd, int tab, int recno) {
    int     rc;
    LOCK*   arrayend;
    char* temp;

    temp = (char*)lpvMem + OFFSETLOCK;
    lockp = (LOCK*)(temp);

    arrayend = lockp + (MAXLOCK - 1);  // @last array item
    rc = lockscan(usr, fd, tab, recno); // N.B. alters global lockp
    if (rc > 0) {
        if (lockp->userno == usr)
            return 0;               // already locked by us
        return -1;      // locked by someone else
        }
    if (rc == 0)        // no vacancy, abort
        return 0;
    if (arrayend->userno != 0)  // lock array full, abort
        return 0;
    // move upper part of list UP (if any) to open space
    rc = -rc;
    if (rc < MAXLOCK)
        memmove((void*)(lockp+1), (void*)lockp, sizeof(LOCK)*(MAXLOCK - rc));
    // move in new lock specs
    lockp->userno = usr;
    lockp->fileno = fd;
    lockp->tableno = tab;
    lockp->recno = recno;
    return 0;
    }

//========================================================================
// Unlock a record in lock_array
//extern "C" __declspec(dllexport) int recordUnlock(int usr, int fd, int tab, int recno) {
int recordUnlock(int usr, int fd, int tab, int recno) {
    int     rc;
    char* temp;

    temp = (char*)lpvMem + OFFSETLOCK;
    lockp = (LOCK*)(temp);

    rc = lockscan(usr, fd, tab, recno); // N.B. alters global lockp
    if (rc <= 0)
        return -1; // no hit
    if (lockp->userno != usr)
        return -1;  // someone else's lock
     // move upper part of list DOWN
    if (rc < MAXLOCK)
        memmove((void*)(lockp), (void*)(lockp+1), sizeof(LOCK) * (MAXLOCK - rc));
    // clear out last lock
    memset((void*)(lockp+(MAXLOCK-1)), '\0', sizeof(int)*4);
    return 0;
    }
//========================================================================
// IsLocked?
//  0 = nohit (FALSE)
//  1 = self hit
// -1 = alien hit
//extern "C" __declspec(dllexport) int recordQuery(int usr, int fd, int tab, int recno) {
int recordQuery(int usr, int fd, int tab, int recno) {
        int     rc;

    lockp = (LOCK*)(lpvMem)+OFFSETLOCK;
    rc = lockscan(usr, fd, tab, recno); // N.B. alters lockp
    if (rc <= 0)
        return 0;       // no hit
    if (lockp->userno == usr)
        return 1;  // hit on our lock
    return -1; // hit on someone else
    }
//=========================================================================
// scan record lock list
//  hit returns list#
//  avail returns -list#
//  uses global 'lockp'
// ordinal subscript
// +-------+----+-------+-------+
// | usrno | fd | table | recno |
// +-------+----+-------+-------+
//       .
//       .
//       .
// Idea:
//  if recno == 0 -> table lock
//  if table == 0 -> dbf lock

int lockscan(int  usr, int fd, int tab, int recno) {
    for (int i = 1; i <= MAXLOCK; i++, lockp++) {
        if (lockp->userno == 0)        // end of list
            return -i;
        if (lockp->fileno == fd &&     // we found a lock
            lockp->tableno == tab &&
            lockp->recno == recno)
            return i;
        if (lockp->userno > usr ||      // were past possible lock
            lockp->fileno > fd ||
            lockp->tableno > tab ||
            lockp->recno > recno)
            return -i;
         }
    return (0);         // exhausted the array
    }