#include "stdafx.h"
#include <sal.h>
#include <windows.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Version.lib")
#include <Strsafe.h>

#ifndef __ATLBASE_H__
#define __ATLBASE_H__
extern "C" const IMAGE_DOS_HEADER __ImageBase;//from atlbase.h
#endif

#ifndef _MAX_NTFS_PATH
#define _MAX_NTFS_PATH   0x8000
#endif

UINT DateDif(int d1, int m1, int y1, int d2, int m2, int y2)
{
	int m, temp=d1, sum=0, month[]={31,28,31,30,31,30,31,31,30,31,30,31};
	for(m=m1; m<m2+(y2-y1)*12; m++)
	{
		if(m>12)
		{
			m=1;
			y1++;
		}
		if(m==2)
		{
			if(y1%4==0 && (y1%100!=0 || y1%400==0))
			month[m-1]=29;
			else
			month[m-1]=28;
		}
		sum=sum+(month[m-1]-temp);
		temp=0;
	}
	sum=sum+d2-temp;
return sum;
}

// these macros help to align on r-byte boundaries (by Ted Peck)
#define roundoffs(a,b,r) (((BYTE *) (b) - (BYTE *) (a) + ((r) - 1)) & ~((r) - 1))
#define roundpos(a,b,r) (((BYTE *) (a)) + roundoffs(a,b,r))

struct VS_VERSIONINFO
{
    WORD                wLength;
    WORD                wValueLength;
    WORD                wType;
    WCHAR               szKey[1];
    WORD                wPadding1[1];
    VS_FIXEDFILEINFO    Value;
    WORD                wPadding2[1];
    WORD                wChildren[1];
};


BOOL UpdateISSFileVersion(__in LPCTSTR pszFile, WORD wBuild, WORD wRevision, __out_opt VS_FIXEDFILEINFO* pFixedFileInfo)
{
	BOOL bRet=FALSE;

	//GetPrivateProfileString needs full path!
	HANDLE hProcHeap=GetProcessHeap();
	LPTSTR pszIniFile=(LPTSTR)HeapAlloc(hProcHeap, HEAP_ZERO_MEMORY, sizeof(TCHAR)*_MAX_NTFS_PATH);
	if (pszIniFile==NULL) return FALSE;
	if (!GetFullPathName(pszFile, _MAX_NTFS_PATH, pszIniFile, NULL)) return FALSE;

	TCHAR strVal[32];
	SecureZeroMemory(strVal, 32*sizeof(TCHAR));
	GetPrivateProfileString(_T("Setup"), _T("AppVersion"), _T("1.0.0.0"), strVal, 32, pszIniFile);
	//HRESULT htrace=HRESULT_FROM_WIN32(GetLastError());

	WORD wMajor=1;
	WORD wMinor=0;
	if (*strVal)
	{
		wMajor=(WORD)_tcstoul(strVal, NULL, 10);
		LPWSTR pstrMinor=StrChr(strVal, _T('.'))+1;
		if (pstrMinor && *pstrMinor)
			wMinor=(WORD)_tcstoul(pstrMinor, NULL, 10);
	}

	if (pFixedFileInfo)
	{
		pFixedFileInfo->dwFileVersionMS=MAKELONG(wMinor,wMajor);
		pFixedFileInfo->dwFileVersionLS=MAKELONG(wRevision,wBuild);
	}

	if (S_OK==StringCchPrintf(strVal, 32, _T("%d.%d.%d.%d"), wMajor, wMinor, wBuild, wRevision))
	{
		bRet=WritePrivateProfileString(_T("Setup"), _T("VersionInfoVersion"), strVal, pszIniFile);
	}
	HeapFree(hProcHeap, 0, pszIniFile);

return bRet;
}

BOOL UpdatePEFileVersion(__in LPCTSTR pszFile, const WORD wBuild, const WORD wRevision,
					   const BOOL bUpdateProdVer, __out_opt VS_FIXEDFILEINFO* pFixedFileInfo)
{
	BOOL bRet=FALSE;
	DWORD dwHandle;
    DWORD dwSize = GetFileVersionInfoSize(pszFile, &dwHandle);
    if (dwSize)
    {
		HANDLE hProcHeap=GetProcessHeap();
        VS_VERSIONINFO* pVerInfo = (VS_VERSIONINFO*)HeapAlloc(hProcHeap, HEAP_ZERO_MEMORY, dwSize);
        if (pVerInfo && GetFileVersionInfo(pszFile, 0, dwSize, pVerInfo))
        {
            LPBYTE pOffsetBytes = (BYTE*)&pVerInfo->szKey[16];
            VS_FIXEDFILEINFO* pFixedInfo = (VS_FIXEDFILEINFO *) roundpos(pVerInfo, pOffsetBytes, 4);
			pFixedInfo->dwFileVersionLS=MAKELONG(wRevision,wBuild);
			if (bUpdateProdVer) pFixedInfo->dwProductVersionLS = pFixedInfo->dwFileVersionLS;
			HANDLE hResource = BeginUpdateResource(pszFile, FALSE);
			if (hResource)
			{
				if (UpdateResource(hResource, RT_VERSION, MAKEINTRESOURCE(VS_VERSION_INFO), 
									MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), pVerInfo, dwSize))
				{
					bRet=EndUpdateResource(hResource, FALSE);
					if (pFixedFileInfo) CopyMemory(pFixedFileInfo, pFixedInfo, sizeof(VS_FIXEDFILEINFO));
				}
			}
			HeapFree(hProcHeap, 0, pVerInfo);
		}
    }
return bRet;
}


BOOL GetModuleVersionInfo(__in HMODULE hMod, __inout VS_FIXEDFILEINFO* pFixedFileInfo)
{
	if (HRSRC hRes=FindResource(hMod, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION))
	{
		if (HGLOBAL hGlob=LoadResource(hMod, hRes))
		{
			VS_VERSIONINFO* pVerInfo=(VS_VERSIONINFO*)LockResource(hGlob);
		    LPBYTE pOffsetBytes = (BYTE*)&pVerInfo->szKey[16];
            VS_FIXEDFILEINFO* pFixedInfo = (VS_FIXEDFILEINFO *) roundpos(pVerInfo, pOffsetBytes, 4);
			if (pFixedInfo->dwSignature==0xFEEF04BD)
			{
				CopyMemory(pFixedFileInfo, pFixedInfo, sizeof(VS_FIXEDFILEINFO));
				return TRUE;
			}
		}
	}
return FALSE;
}



int _tmain(int argc, _TCHAR* argv[])
{
//DebugBreak();
	VS_FIXEDFILEINFO ffi;
	if (argc<2)
	{
		SecureZeroMemory(&ffi, sizeof(VS_FIXEDFILEINFO));
		GetModuleVersionInfo(NULL, &ffi);//use (HMODULE)&__ImageBase inside dll
		printf("Automatic File Version Tool %u.%u.%u.%u\nT800 Productions (C) 2014\n",
						HIWORD(ffi.dwFileVersionMS), LOWORD(ffi.dwFileVersionMS),
						HIWORD(ffi.dwFileVersionLS), LOWORD(ffi.dwFileVersionLS));
		printf("\n  Usage:    AutoVer FILE [OPTIONS] [FILE] [OPTIONS]...\n");
		printf("\n  Options:  /p - set Product Version build and revision number\n");
		return 0;
	}

	SYSTEMTIME st;
	GetLocalTime(&st);
	WORD wBuild=(WORD)DateDif(1,1,2000, st.wDay, st.wMonth, st.wYear);//C4244 workaround
	WORD wRevision=(st.wHour*3600 + st.wMinute*60 + st.wSecond)/2;

	int i;
	for (i=1; i<argc; i++)
	{
		LPCTSTR pszFile=argv[i];
		BOOL bUpdateProdVer=FALSE;	
		if ((i+1)<argc && (((argv[i+1][0]==_T('/')) && ((argv[i+1][1]==_T('p')) || (argv[i+1][1]==_T('P'))))))
			bUpdateProdVer=++i;

		//VS_FIXEDFILEINFO ffi;
		SecureZeroMemory(&ffi, sizeof(VS_FIXEDFILEINFO));

		//comma operator!!!
		if ((StrCmpI(PathFindExtension(pszFile), _T(".iss"))==0) ?
				(bUpdateProdVer=FALSE, UpdateISSFileVersion(pszFile, wBuild, wRevision, &ffi)) :
				UpdatePEFileVersion(pszFile, wBuild, wRevision, bUpdateProdVer, &ffi))
		{
			wprintf(bUpdateProdVer ? L"Setting build and revision number:  %ws   File Version=%u.%u.%u.%u   Product Version=%u.%u.%u.%u\n" :
								L"Setting build and revision number:  %ws   File Version=%u.%u.%u.%u\n", 
								PathFindFileName(pszFile),
								HIWORD(ffi.dwFileVersionMS),LOWORD(ffi.dwFileVersionMS),
								HIWORD(ffi.dwFileVersionLS),LOWORD(ffi.dwFileVersionLS),
								HIWORD(ffi.dwProductVersionMS),LOWORD(ffi.dwProductVersionMS),
								HIWORD(ffi.dwProductVersionLS),LOWORD(ffi.dwProductVersionLS));
		}
		else
		{
			printf("Error:\n");
			DWORD dwResult = GetLastError();
			LPWSTR lpMsgBuf=NULL;
			DWORD bufLen = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
											NULL, HRESULT_FROM_WIN32(dwResult), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
											(LPWSTR)&lpMsgBuf, 0, NULL);
			if (bufLen) wprintf(lpMsgBuf);
			LocalFree(lpMsgBuf);
			printf("\n");
		}
	}

return 0;
}
