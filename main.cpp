#include <stdio.h>
#include <windows.h>
#include <tchar.h>

struct ICONDIRENTRY
{
    BYTE bWidth;               // Width of the image
    BYTE bHeight;              // Height of the image (times 2)
    BYTE bColorCount;          // Number of colors in image (0 if >=8bpp)
    BYTE bReserved;            // Reserved
    WORD wPlanes;              // Color Planes
    WORD wBitCount;            // Bits per pixel
    DWORD dwBytesInRes;         // how many bytes in this resource?
    DWORD dwImageOffset;        // where in the file is this image
};

struct ICONDIR
{
    WORD idReserved;   // Reserved
    WORD idType;       // resource type (1 for icons)
    WORD idCount;      // how many images?
    ICONDIRENTRY *idEntries; // the entries for each image
};

struct GRPICONDIRENTRY
{
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwBytesInRes;
    WORD nID;
};

struct GRPICONDIR
{
    WORD idReserved;
    WORD idType;
    WORD idCount;
    GRPICONDIRENTRY *idEntries;
};


bool ChangeExeIcon(wchar_t *exeFile, wchar_t *iconFile)
{
    // Define additional variables
    BOOL ret;
    DWORD dwIconFileSize, dwBytesRead;
    HANDLE hIcon = CreateFile(iconFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    dwIconFileSize = GetFileSize(hIcon, NULL);

    ICONDIR iconDir;
    // read header
    ret = ReadFile(hIcon, (LPVOID)&(iconDir.idReserved), sizeof(WORD), &dwBytesRead, NULL);
    if (!ret) { printf("error line: %d\n", __LINE__); return false; }
    ret = ReadFile(hIcon, (LPVOID)&(iconDir.idType), sizeof(WORD), &dwBytesRead, NULL);
    if (!ret) { printf("error line: %d\n", __LINE__); return false; }
    ret = ReadFile(hIcon, (LPVOID)&(iconDir.idCount), sizeof(WORD), &dwBytesRead, NULL);
    if (!ret) { printf("error line: %d\n", __LINE__); return false; }

    // read entry
    iconDir.idEntries = (ICONDIRENTRY*)malloc(sizeof(ICONDIRENTRY) * iconDir.idCount);
    for(int i = 0; i < iconDir.idCount; i++) {
        ret = ReadFile(hIcon, (LPVOID)&iconDir.idEntries[i], sizeof(ICONDIRENTRY), &dwBytesRead, NULL);
        if (!ret) { printf("error line: %d\n", __LINE__); return false; }
    }

    // read data
    BYTE **iconImages = (BYTE**)malloc(sizeof(BYTE*) * iconDir.idCount);
    for(int i = 0; i < iconDir.idCount; i++) {
      iconImages[i] = (BYTE*)malloc(iconDir.idEntries[i].dwBytesInRes);
      SetFilePointer(hIcon, iconDir.idEntries[i].dwImageOffset, NULL, FILE_BEGIN);
      ReadFile(hIcon, iconImages[i], iconDir.idEntries[i].dwBytesInRes, &dwBytesRead, NULL);
    }

    GRPICONDIR graIconDir;
    graIconDir.idReserved = iconDir.idReserved;
    graIconDir.idType = iconDir.idType;
    graIconDir.idCount = iconDir.idCount;
    WORD cbRes = 3 * sizeof(WORD) + graIconDir.idCount * sizeof(GRPICONDIRENTRY);
    BYTE *test = (BYTE*)malloc(cbRes);
    BYTE *temp = test;
    CopyMemory(test, &graIconDir.idReserved, sizeof(WORD));
    test = test + sizeof(WORD);
    CopyMemory(test, &graIconDir.idType, sizeof(WORD));
    test = test + sizeof(WORD);
    CopyMemory(test, &graIconDir.idCount, sizeof(WORD));
    test = test + sizeof(WORD);

    graIconDir.idEntries = (GRPICONDIRENTRY*)malloc(graIconDir.idCount * sizeof(GRPICONDIRENTRY));
    for(int i = 0; i < graIconDir.idCount; i++) {
        graIconDir.idEntries[i].bWidth = iconDir.idEntries[i].bWidth;
        CopyMemory(test, &graIconDir.idEntries[i].bWidth, sizeof(BYTE));
        test = test + sizeof(BYTE);
        graIconDir.idEntries[i].bHeight = iconDir.idEntries[i].bHeight;
        CopyMemory(test, &graIconDir.idEntries[i].bHeight, sizeof(BYTE));
        test = test + sizeof(BYTE);
        graIconDir.idEntries[i].bColorCount = iconDir.idEntries[i].bColorCount;
        CopyMemory(test, &graIconDir.idEntries[i].bColorCount, sizeof(BYTE));
        test = test + sizeof(BYTE);
        graIconDir.idEntries[i].bReserved = iconDir.idEntries[i].bReserved;
        CopyMemory(test, &graIconDir.idEntries[i].bReserved, sizeof(BYTE));
        test = test + sizeof(BYTE);
        graIconDir.idEntries[i].wPlanes = iconDir.idEntries[i].wPlanes;
        CopyMemory(test, &graIconDir.idEntries[i].wPlanes, sizeof(WORD));
        test = test + sizeof(WORD);
        graIconDir.idEntries[i].wBitCount = iconDir.idEntries[i].wBitCount;
        CopyMemory(test, &graIconDir.idEntries[i].wBitCount, sizeof(WORD));
        test = test + sizeof(WORD);
        graIconDir.idEntries[i].dwBytesInRes = iconDir.idEntries[i].dwBytesInRes;
        CopyMemory(test, &graIconDir.idEntries[i].dwBytesInRes, sizeof(DWORD));
        test = test + sizeof(DWORD);
        graIconDir.idEntries[i].nID= i + 1;
        CopyMemory(test, &graIconDir.idEntries[i].nID,sizeof(WORD));
        test = test + sizeof(WORD);
    }

    HANDLE hExe = BeginUpdateResource(exeFile, FALSE);
    ret = UpdateResource(hExe, RT_GROUP_ICON, L"MAINICON", MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), temp, cbRes);
    if (!ret) { printf("error line: %d\n", __LINE__); return false; }
    for(int i = 0; i < graIconDir.idCount; i++) {
        ret = UpdateResource(hExe, RT_ICON,
                             MAKEINTRESOURCE(graIconDir.idEntries[i].nID),
                             MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
                             iconImages[i],
                             graIconDir.idEntries[i].dwBytesInRes);
        if (!ret) { printf("error line: %d\n", __LINE__); return false; }
    }
    EndUpdateResource(hExe,FALSE);
    // todo: free memory

    return true;
}

int wmain(int argc, wchar_t *argv[])
{
    if (argc < 3) {
        printf("parameter error (1: exe, 2: icon)\n");
        return -1;
    }

    if (!ChangeExeIcon(argv[1], argv[2])) {
        printf("change icon failed\n");
        return -1;
    }
    printf("change icon successed\n");
    return 0;
}
