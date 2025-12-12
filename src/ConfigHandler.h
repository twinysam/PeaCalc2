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

/** Used Defines: *********************************************************************/

#pragma once

#define CNF_MAX_TOP       1000
#define CNF_MAX_LEFT      5000
#define CNF_MAX_HEIGHT    1000
#define CNF_MAX_WIDTH     2000
#define CNF_MAX_OPACITY   255
#define CNF_MAX_PRECISION 15
#define CNF_MAX_LINES     255
#define CNF_MAX_FONTSIZE  30

#define CNF_DEF_TOP       CW_USEDEFAULT
#define CNF_DEF_LEFT      CW_USEDEFAULT
#define CNF_DEF_HEIGHT    190
#define CNF_DEF_WIDTH     640
#define CNF_DEF_OPACITY   90
#define CNF_DEF_PRECISION 5
#define CNF_DEF_LINES     44
#define CNF_DEF_FONTSIZE  25
#define CNF_DEF_COLORMODE 0

#define CNF_MAX_COLORMODE 2

#define C_TEXTBUFSIZE 10000

/** Class Definition: *****************************************************************/

class CConfigHandler {
public:
    // Properties:
    INT32        iTop, iLeft, iHeight, iWidth, iOpacity, iPrecision, iLines, iFontSize, iColorMode;
    std::wstring sText, sLightBg, sLightTxt, sDarkBg, sDarkTxt, sResultLightColor, sResultDarkColor;
    // Methods:
    CConfigHandler();
    ~CConfigHandler();
    bool   bIsPortable(void);
    void   vGetColors(DWORD &cBg, DWORD &cTxt, DWORD &cRes);
private:
    bool  bPortable;
    void  vCheckPortable(void);
    bool  bWriteToFile(const WCHAR* pszwFName);
    INT32 iParseFileEntry(FILE *fp, const WCHAR* pszwToken, DWORD dwLim, INT32 ulDefault);
    void  vParseStringEntry(FILE *fp, const WCHAR* pszwToken, std::wstring &sTarget, const WCHAR* pszwDefault);
    bool  bReadFromFile(const WCHAR* pszwFName);
    void  vSetDefaultData(void);
    DWORD dwHexToRGB(std::wstring sHex, DWORD cDefault);
    bool  bIsSystemDarkMode(void);
};
