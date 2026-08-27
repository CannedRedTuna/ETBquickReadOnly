#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>

#define Int32Size (sizeof(uint32_t))
#define Int64Size (sizeof(uint64_t))
#define SAFE_FREE(ptr) do { free(ptr); (ptr) = NULL; } while (0)

void BuildPath(char* out, size_t outSize, const char* dir, const char* fileName) {
    snprintf(out, outSize, "%s\\%s", dir, fileName);
}

int BestowFolder(const char* basePath){
    char filePath[MAX_PATH];
    BuildPath(filePath, sizeof(filePath), basePath, "ReplacementFiles");
    if (CreateDirectoryA(filePath, NULL)){
        return 0;
    }
    else{
        DWORD error = GetLastError();
        if(error == ERROR_ALREADY_EXISTS){
            return 0;
        } else{
            fprintf(stderr, "Failed to create directory (error %lu)\n", error);
            return -1;
        }
    }
}

void WatchDeletion(const char* basePath){
    HANDLE hDir = CreateFileA(
        basePath, 
        FILE_LIST_DIRECTORY, 
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 
        NULL, 
        OPEN_EXISTING, 
        FILE_FLAG_BACKUP_SEMANTICS, 
        NULL
    );

    if (hDir == INVALID_HANDLE_VALUE){
        printf("Failed to open directory. Error: %lu\n", GetLastError());
        return;
    }

    char backupDir[MAX_PATH];
    BuildPath(backupDir, sizeof(backupDir), basePath, "ReplacementFiles");

    BYTE buffer[1024];
    DWORD bytesReturned;

    printf("Waiting for file deletions in %s...\n", basePath);

    while (ReadDirectoryChangesW(hDir, buffer, sizeof(buffer), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME, &bytesReturned, NULL, NULL)){
        FILE_NOTIFY_INFORMATION* pNotify = (FILE_NOTIFY_INFORMATION*)buffer;

        do {
            if (pNotify->Action == FILE_ACTION_REMOVED) {
                int nameLen = pNotify->FileNameLength / sizeof(WCHAR);
                wchar_t wFileName[MAX_PATH] = {0};
                wcsncpy_s(wFileName, MAX_PATH, pNotify->FileName, nameLen);
                char fileNameA[MAX_PATH] = {0};
                WideCharToMultiByte(CP_ACP, 0, wFileName, -1, fileNameA, MAX_PATH, NULL, NULL);

                printf("File removed: %s\n", fileNameA);

                if (_stricmp(fileNameA, "ReplacementFiles") == 0) {
                    goto NEXT_ENTRY;
                }

                char backupFilePath[MAX_PATH];
                char targetFilePath[MAX_PATH];

                BuildPath(backupFilePath, sizeof(backupFilePath), backupDir, fileNameA);
                BuildPath(targetFilePath, sizeof(targetFilePath), basePath, fileNameA);

                if (!CopyFileA(backupFilePath, targetFilePath, FALSE)) {
                    printf("Copy failed. Error: %lu\n", GetLastError());
                } else {
                    printf("Replaced %s\n", fileNameA);
                    SetFileAttributesA(targetFilePath, FILE_ATTRIBUTE_READONLY);
                    printf("Read-Only applied to %s\n", fileNameA);
                    Sleep(5000);
                    SetFileAttributesA(targetFilePath, FILE_ATTRIBUTE_NORMAL);
                    printf("Read-Only removed from %s\n", fileNameA);
                }
            }

        NEXT_ENTRY:
            if (pNotify->NextEntryOffset == 0) break;
            pNotify = (FILE_NOTIFY_INFORMATION*)((BYTE*)pNotify + pNotify->NextEntryOffset);

        } while (1);
    }

    CloseHandle(hDir);
}

int main(){
    printf("Starting...\n");

    char basePath[MAX_PATH];
    const char *localappdata = getenv("LOCALAPPDATA");
    if (!localappdata) {
        fprintf(stderr, "Failed to get LOCALAPPDATA path.\n");
        return -1;
    }
    
    snprintf(basePath, sizeof(basePath), "%s\\EscapeTheBackrooms\\Saved\\SaveGames", localappdata);

    if (BestowFolder(basePath) == -1){
        return -1;
    }
    
    WatchDeletion(basePath);

    return 0;
}