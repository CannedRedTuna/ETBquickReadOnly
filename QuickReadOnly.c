#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>

enum HotkeyID{
    SAVE_HOTKEY = 1,
    RESTORE_HOTKEY = 2,
};

void BuildPath(char* out, const char* dir, const char* fileName) {
    snprintf(out, MAX_PATH, "%s\\%s", dir, fileName);
}

BOOL SaveSnapshot(const char *Target, const char* basePath) {
    char SaveCopy[MAX_PATH];
    BuildPath(SaveCopy, basePath, "ReadOnlySaveCopy.sav");

    if (!CopyFileA(Target, SaveCopy, FALSE)) {
        printf("Copy failed. Error: %lu\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}

BOOL RestoreSnapshot(const char *Target, const char* basePath) {
    char SaveCopy[MAX_PATH];
    BuildPath(SaveCopy, basePath, "ReadOnlySaveCopy.sav");

    if (!CopyFileA(SaveCopy, Target, FALSE)){
        printf("CopyFailed\n");
        return FALSE;
    }
    else{
        SetFileAttributesA(Target, FILE_ATTRIBUTE_READONLY);
        printf("Read-Only applied.\n");
        Sleep(2000);
        SetFileAttributesA(Target, FILE_ATTRIBUTE_NORMAL);
        printf("Read-Only removed.\n");
    }

    return TRUE;
}

BOOL IsExcluded(const char* fileName, const char** excludeList, size_t excludeCount) {
    for (size_t i = 0; i < excludeCount; i++) {
        if (strcmp(fileName, excludeList[i]) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL GetRecent(const char* SaveDir, const char** ExcludedFiles, size_t ExcludeCount, char* outPath){
    char SearchPath[MAX_PATH];
    snprintf(SearchPath, sizeof(SearchPath), "%s\\*", SaveDir);


    WIN32_FIND_DATA FindData;
    HANDLE hFind = FindFirstFileA(SearchPath, &FindData);
    if  (hFind == INVALID_HANDLE_VALUE){
        return FALSE;
    }

    FILETIME MostRecentTime = {0};
    char MostRecentName[MAX_PATH] = {0};
    BOOL Found = FALSE;

    do {
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
            continue;
        }

        if(IsExcluded(FindData.cFileName, ExcludedFiles, ExcludeCount)){
            continue;
        }

        if(!Found || CompareFileTime(&FindData.ftLastWriteTime, &MostRecentTime) > 0){
            MostRecentTime = FindData.ftLastWriteTime;
            strncpy(MostRecentName, FindData.cFileName, MAX_PATH - 1);
            Found = TRUE;
        }

    } while (FindNextFileA(hFind, &FindData));

    FindClose(hFind);

    if (!Found){
        return FALSE;
    }

    BuildPath(outPath, SaveDir, MostRecentName);
    return TRUE;

}

void GetUserInput(const char* basePath){
    const char* ExcludeList[] = { "MAINSAVE.sav", "GameSettings_SaveSlot.sav", "steam_autocloud.vdf", "SpeedrunUtilsSave.sav", "Mission.sav", "CostumeSettings.sav", "ReadOnlySaveCopy.sav", "BlockedPlayers.sav"};
    const size_t ExcludeCount = 8;
    char TargetPath[MAX_PATH];
    char TestPath[MAX_PATH];
    BOOL SnapshotExists = FALSE;

    // If you plan to manually compile it to change the hotkeys, you only need to change these two.
    RegisterHotKey(NULL, SAVE_HOTKEY, MOD_CONTROL | MOD_NOREPEAT, VK_F7);
    RegisterHotKey(NULL, RESTORE_HOTKEY, MOD_NOREPEAT, VK_F7);

    MSG Msg;
    while(GetMessage(&Msg, NULL, 0, 0)){
        if(Msg.message == WM_HOTKEY){
            switch(Msg.wParam){
                case SAVE_HOTKEY:
                    if (GetRecent(basePath, ExcludeList, ExcludeCount, TargetPath)) {
                        if (SaveSnapshot(TargetPath, basePath)) {
                            SnapshotExists = TRUE;
							printf("Snapshot  Taken.\n");
                        }
                    }
                    break;

                case RESTORE_HOTKEY :
                    if (SnapshotExists) {
                        RestoreSnapshot(TargetPath, basePath);
                        printf("Snapshot Restored.\n");
                    } else {
                        printf("No snapshot taken yet.\n");
                    }
                    break;
                default: break;
            }
        }
    }
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
    printf("Waitng for user input.\n");
    GetUserInput(basePath);
    
    return 0;
}
