//
//  This file is part of PeaCalc++ project
//  Copyright (C)2018 Jens Daniel Schlachter <osw.schlachter@mailbox.org>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

/** Global Includes: ******************************************************************/

#include "stdafx.h"
#include <winuser.h>
#include <stdio.h>
#include <Shlobj.h>
#include <string>
#include "ConfigHandler.h"

/** Compiler Settings: ****************************************************************/

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

/** Public Functions: *****************************************************************/

/** Constructor: **********************************************************************
 *    Triggers the check for portability and tries to load the settings               *
 *    from an ini-file either from the current path when running portable,            *
 *    or from the windows user's path:                                                */
 
CConfigHandler::CConfigHandler() {
    WCHAR szFileName[MAX_PATH];
    /** Check portability:                                                            */
    vSetDefaultData(); 
    vCheckPortable();
    /** Setup the file-name:                                                          */
    if (bPortable) {
        /** If it is portable, it should be right there:                              */
        wcscpy(szFileName, L"PeaCalc.ini");
    }else{
        /** If it is not, try to build the path from the user-data-path:              */
        if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, szFileName) != S_OK) {
            vSetDefaultData();
            return;
        }
        wcscat(szFileName, L"\\PeaCalc\\PeaCalc.ini");
    }
    if (!bReadFromFile(szFileName)) vSetDefaultData();
}

/** Destructor: ***********************************************************************
 *    Tries to store the settings in an ini-file either in the current path           *
 *    when running portable, or in the windows user's path:                           */

CConfigHandler::~CConfigHandler() {
    WCHAR szFileName[MAX_PATH];
    /** Setup the file-name:                                                          */
    if (bPortable) {
        /** If it is portable, it should be right there:                              */
        if (! bWriteToFile(L"PeaCalc.ini")) {
            MessageBox(NULL, TEXT("Failure in attempt to store\nconfiguration in portable ini-file!"), TEXT("PeaCalc Portable"), MB_OK | MB_ICONEXCLAMATION);
            return;
        }
    }else{
        /** If it is local, so fetch the user-directory:                              */
        if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, szFileName) != S_OK) return;
        /** Create the directory in case it does not exist yet:                       */
        wcscat(szFileName, L"\\PeaCalc");
        if (!CreateDirectory(szFileName, NULL) && ERROR_ALREADY_EXISTS != GetLastError()) {
            /** It was not successful, and not because it already existed:            */
            MessageBox(NULL, TEXT("Failure in attempt to create\nsettings folder in user-directory!"), TEXT("PeaCalc Portable"), MB_OK | MB_ICONEXCLAMATION);
            return;
        }
        /** Add the actual file-name and try to save:                                 */
        wcscat(szFileName, L"\\PeaCalc.ini");
        if (! bWriteToFile(szFileName)) {
            MessageBox(NULL, TEXT("Failure in attempt to store\nconfiguration in user ini-file!"), TEXT("PeaCalc Portable"), MB_OK | MB_ICONEXCLAMATION);
        }
    }
}

/** Get-Function of the portability-flag: *********************************************
 *    This allows other classes to check, if the application runs                     *
 *    as portable:                                                                    */

bool CConfigHandler::bIsPortable(void) {
    return (bPortable);
}

/** Private Functions: ****************************************************************/

/** Check-function for portability: ***************************************************
 *    Sets the according flag to portable, when                                       *
 *     - an ini-file was found in the current directory or                            *
 *     - the drive, from which the application was started is a removable one.        */

void CConfigHandler::vCheckPortable(void) {
    /** Variables:                                                                    */
    WCHAR szFileName[MAX_PATH];
    DWORD dwDriveType;
    DWORD dwAttrib;
    HINSTANCE hInstance;
    /** Fetch the name and file-path, by which this application was started:          */
    hInstance  = GetModuleHandle(NULL);
    GetModuleFileName(hInstance, szFileName, MAX_PATH);
    /** Reduce it to the drive-letter and fetch the drive-type via the API:           */
    szFileName[2] = 0;
    dwDriveType = GetDriveType(szFileName);
    /** Try to fetch the file-attributes of the ini-file.                             *
      * Getting a valid result means, that the file exists.                           */
    dwAttrib = GetFileAttributes(TEXT("PeaCalc.ini"));
    /** When a removable drive or an existing ini-file was detected, then             *
      * this is running portable:                                                     */
    bPortable = (dwDriveType == DRIVE_REMOVABLE        ) ||
                (dwAttrib != INVALID_FILE_ATTRIBUTES &&
               !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) );
}

/** File-Writer: **********************************************************************
 *    Writes the configuration into a plain text-file:                                */

bool CConfigHandler::bWriteToFile(const WCHAR* pszwFName) {
    /** Variables:                                                                    */
    FILE *fp;
    std::size_t i;
    /** Try to open the file:                                                         */ 
    fp = _wfopen(pszwFName, L"w, ccs=UTF-8");
    if (fp == NULL) return false;
    /** Write the items:                                                              */
    fwprintf(fp, L"[PeaCalc]\n");
    fwprintf(fp, L"Top=%d\n"      , iTop      );
    fwprintf(fp, L"Left=%d\n"     , iLeft     );
    fwprintf(fp, L"Height=%d\n"   , iHeight   );
    fwprintf(fp, L"Width=%d\n"    , iWidth    );
    fwprintf(fp, L"Opacity=%d\n"  , iOpacity  );
    fwprintf(fp, L"FontSize=%d\n" , iFontSize );
    fwprintf(fp, L"Precision=%d\n", iPrecision);
    fwprintf(fp, L"Lines=%d\n"            , iLines           );
    fwprintf(fp, L"ColorMode=%d\n"        , iColorMode       );
    fwprintf(fp, L"LightBg=%ls\n"          , sLightBg.c_str() );
    fwprintf(fp, L"LightTxt=%ls\n"         , sLightTxt.c_str());
    fwprintf(fp, L"DarkBg=%ls\n"           , sDarkBg.c_str()  );
    fwprintf(fp, L"DarkTxt=%ls\n"          , sDarkTxt.c_str() );
    fwprintf(fp, L"ResultLightColor=%ls\n" , sResultLightColor.c_str());
    fwprintf(fp, L"ResultDarkColor=%ls\n"  , sResultDarkColor.c_str());
    fwprintf(fp, L"[Text]\n"              );
    /** Prepare the text bei removing \r:                                             */
    i = sText.find(L"\r");
    while (i != std::string::npos) {
        sText.erase(i, 1);
        i = sText.find(L"\r");
    }
    #ifdef _MSC_VER
        fwprintf(fp, L"%ls"           , sText.c_str());
    #else
        fwprintf(fp, L"%ls", sText.c_str()); // Use %ls for wide string on MinGW/Standard
    #endif
    /** And close:                                                                    */
    fclose(fp); 
    return true;
}

/** Entry-Parser: *********************************************************************
 *    Tries to read and parse a value with a given token from the ini-file:           */

INT32 CConfigHandler::iParseFileEntry(FILE *fp, const WCHAR* pszwToken, DWORD dwLim, INT32 ulDefault) {
    WCHAR buf[1000];
    DWORD dwRdVal;
    WCHAR *epr;
    /** Try to fetch a line from the file:                                            */
    if (fgetws(buf, 1000, fp) == NULL) return ulDefault;
    /** Check, if it contains the expected token:                                     */
    if (wcsncmp(buf, pszwToken, wcslen(pszwToken)) != 0) return ulDefault;
    /** Try to parse the numeric argument behind it:                                  */
    dwRdVal = wcstol(&buf[wcslen(pszwToken)], &epr, 10);
    /** When the result is not in bounds, it was not successful:                      */
    if ((dwRdVal < 1) || (dwRdVal > dwLim)) return ulDefault;
    /** If it was, the value can be written out:                                      */
    return (INT32)dwRdVal;
}

/** Entry-Parser for strings: *********************************************************
 *    Tries to read and parse a string value with a given token from the ini-file:    */

void CConfigHandler::vParseStringEntry(FILE *fp, const WCHAR* pszwToken, std::wstring &sTarget, const WCHAR* pszwDefault) {
    // This function is kept for structural consistency but checking code shows it is unused now.
}

/** File-Reader: **********************************************************************
 *    Open the source-file and uses the parser above to fetch the configuration-      *
 *    values one by one:                                                              */

bool CConfigHandler::bReadFromFile(const WCHAR* pszwFName) {
    FILE *fp;
    WCHAR buf[1000];
    WCHAR *endptr;
    
    fp = _wfopen(pszwFName, L"r, ccs=UTF-8");
    if (fp == NULL) return false;
    
    // Header
    if (fgetws(buf, 1000, fp) == NULL) { fclose(fp); return false; }
    if (wcsncmp(buf, L"[PeaCalc]", 9) != 0) { fclose(fp); return false; }

    // Read configuration lines
    bool bTextSection = false;
    while (fgetws(buf, 1000, fp) != NULL) {
        // Strip newline
        size_t len = wcslen(buf);
        while(len > 0 && (buf[len-1] == L'\r' || buf[len-1] == L'\n')) buf[--len] = 0;
        
        if (wcsncmp(buf, L"[Text]", 6) == 0) {
            bTextSection = true;
            break;
        }

        if (wcsncmp(buf, L"Top=", 4) == 0) iTop = wcstol(buf + 4, NULL, 10);
        else if (wcsncmp(buf, L"Left=", 5) == 0) iLeft = wcstol(buf + 5, NULL, 10);
        else if (wcsncmp(buf, L"Height=", 7) == 0) iHeight = wcstol(buf + 7, NULL, 10);
        else if (wcsncmp(buf, L"Width=", 6) == 0) iWidth = wcstol(buf + 6, NULL, 10);
        else if (wcsncmp(buf, L"Opacity=", 8) == 0) iOpacity = wcstol(buf + 8, NULL, 10);
        else if (wcsncmp(buf, L"FontSize=", 9) == 0) iFontSize = wcstol(buf + 9, NULL, 10);
        else if (wcsncmp(buf, L"Precision=", 10) == 0) iPrecision = wcstol(buf + 10, NULL, 10);
        else if (wcsncmp(buf, L"Lines=", 6) == 0) iLines = wcstol(buf + 6, NULL, 10);
        else if (wcsncmp(buf, L"ColorMode=", 10) == 0) iColorMode = wcstol(buf + 10, NULL, 10);
        else if (wcsncmp(buf, L"LightBg=", 8) == 0) sLightBg = buf + 8;
        else if (wcsncmp(buf, L"LightTxt=", 9) == 0) sLightTxt = buf + 9;
        else if (wcsncmp(buf, L"DarkBg=", 7) == 0) sDarkBg = buf + 7;
        else if (wcsncmp(buf, L"DarkTxt=", 8) == 0) sDarkTxt = buf + 8;
        else if (wcsncmp(buf, L"ResultLightColor=", 17) == 0) sResultLightColor = buf + 17;
        else if (wcsncmp(buf, L"ResultDarkColor=", 16) == 0) sResultDarkColor = buf + 16;
    }

    if ((iLines & 1)==0) iLines++;

    if (bTextSection) {
        sText = L"";
        while (fgetws(buf, 1000, fp) != NULL) {
            sText += buf;
            if (sText.back() == L'\n' && (sText.length() < 2 || sText[sText.length()-2] != L'\r')) {
                 sText.back() = L'\r';
                 sText += L"\n";
            }
        }
    }

    fclose(fp);
    return true;
}
/** Default-Configurator: *************************************************************
 *    This sets the configuration values to the default values, when anything         *
 *    failed while trying to read them from somewhere:                                */

void CConfigHandler::vSetDefaultData(void) {
    iTop       = CNF_DEF_TOP;
    iLeft      = CNF_DEF_LEFT;
    iHeight    = CNF_DEF_HEIGHT;
    iWidth     = CNF_DEF_WIDTH;
    iOpacity   = CNF_DEF_OPACITY;
    iPrecision = CNF_DEF_PRECISION;
    iFontSize  = CNF_DEF_FONTSIZE;
    iLines     = CNF_DEF_LINES;
    sText      = L"";
    
    iColorMode = CNF_DEF_COLORMODE;
    sLightBg   = L"FFFFFF";
    sLightTxt  = L"000000";
    sDarkBg    = L"000000";
    sDarkTxt   = L"FFFFFF";
    sResultLightColor = L"00008B"; // DarkBlue
    sResultDarkColor  = L"00BFFF"; // DeepSkyBlue
}

/** Color Getter: *********************************************************************/

void CConfigHandler::vGetColors(DWORD &cBg, DWORD &cTxt, DWORD &cRes) {
    bool bDark = false;
    
    if (iColorMode == 2) {
        bDark = true;
    } else if (iColorMode == 0) {
        bDark = bIsSystemDarkMode();
    }
    
    // Default Colors
    DWORD defBg = bDark ? RGB(0, 0, 0) : RGB(255, 255, 255);
    DWORD defTxt = bDark ? RGB(255, 255, 255) : RGB(0, 0, 0);
    DWORD defRes = bDark ? RGB(0, 191, 255) : RGB(0, 0, 139); // DeepSkyBlue vs DarkBlue

    if (bDark) {
        cBg = dwHexToRGB(sDarkBg, defBg);
        cTxt = dwHexToRGB(sDarkTxt, defTxt);
        cRes = dwHexToRGB(sResultDarkColor, defRes);
    } else {
        cBg = dwHexToRGB(sLightBg, defBg);
        cTxt = dwHexToRGB(sLightTxt, defTxt);
        cRes = dwHexToRGB(sResultLightColor, defRes);
    }
}

/** Helper to parse Hex-String to RGB: ************************************************/

DWORD CConfigHandler::dwHexToRGB(std::wstring sHex, DWORD cDefault) {
    if (sHex.empty()) return cDefault;
    try {
        unsigned long ulVal = std::stoul(sHex, nullptr, 16);
        return RGB((ulVal >> 16) & 0xFF, (ulVal >> 8) & 0xFF, ulVal & 0xFF);
    } catch (...) {
        return cDefault;
    }
}

/** Helper to check system dark mode: *************************************************/

bool CConfigHandler::bIsSystemDarkMode(void) {
    HKEY hKey;
    DWORD dwValue = 0;
    DWORD dwSize = sizeof(dwValue);
    bool bAppsLight = true;
    bool bSystemLight = true;
    bool bFoundApps = false;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            bAppsLight = (dwValue != 0);
            bFoundApps = true;
        }
        dwSize = sizeof(dwValue);
        if (RegQueryValueExW(hKey, L"SystemUsesLightTheme", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            bSystemLight = (dwValue != 0);
        }
        RegCloseKey(hKey);
    }
    
    // If we found the app setting, strictly use that.
    // However, if the user complains, maybe we should respect "System" if "Apps" is Light but System is Dark?
    // Usually AppsUseLightTheme covers apps.
    // But let's check: if AppsUseLightTheme is missing, use System.
    if (bFoundApps) return !bAppsLight;
    
    // Fallback validity check: if System is dark, assume dark.
    return !bSystemLight;
}
