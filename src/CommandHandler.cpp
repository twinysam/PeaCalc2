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
#include <shellapi.h>
#include <string>
#include <algorithm>
#include <math.h>
#include <Richedit.h>
#include "ConfigHandler.h"
#include "Term.h"
#include "CommandHandler.h"

/** Compiler Settings: ****************************************************************/

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

/** Public Functions: *****************************************************************/

/** Constructor: **********************************************************************/

CCommandHandler::CCommandHandler(CConfigHandler* Config) {
    m_pConfig = Config;
}

/** Destructor: ***********************************************************************/

CCommandHandler::~CCommandHandler() {
}

/** Tiny function to store a pointer to the info-text: ********************************/

void CCommandHandler::vSetInfoText(WCHAR* pszwTextPtr) {
    this->m_pszwInfoText = pszwTextPtr;
}

/** Set-function, which adds the info-text if there's empty input: ********************/

void CCommandHandler::vSetText(HWND hEditBox, const WCHAR* pszwNewText) {
    TCHAR  buffer[C_TEXTBUFSIZE];
    DWORD  dwIndex;
    if (pszwNewText[0] != L'\0') {
        wcscpy(buffer, pszwNewText);
    } else {
        wcscpy(buffer, m_pszwInfoText);
    }
    SetWindowText(hEditBox, buffer);
    /** Set the selection at its end:                                                 */
    dwIndex = GetWindowTextLength(hEditBox);
    SendMessage(hEditBox, EM_SETSEL, dwIndex, dwIndex);
    /** Store the new starting-location:                                              */
    m_dwEditLastLF = SendMessage(hEditBox, EM_LINEINDEX, -1, 0);
    /** Apply colors:                                                                 */
    vColorizeText(hEditBox);
}



/** Handler for a command to be executed: *********************************************/


void CCommandHandler::vProcEnter(HWND hMain, HWND hEditBox) {
    // Get current line index (line with caret)
    DWORD dwIndex;
    CHARRANGE cr;
    CHARFORMAT2W cfDef = { sizeof(CHARFORMAT2W) };
    std::wstring sInput;
    std::wstring sFullOutput;
    
    DWORD dwLineIndex = SendMessage(hEditBox, EM_LINEINDEX, -1, 0);
    // Get length of text
    DWORD dwTextLen = GetWindowTextLength(hEditBox);
    
    // Get text from this line only?
    // Get everything from dwLineIndex to end
    dwIndex = dwTextLen - dwLineIndex;
    if (dwIndex <= 2) return; // Only prompt or empty
    
    WCHAR* pszBuff = new WCHAR[dwIndex + 1];
    TEXTRANGEW tr;
    tr.chrg.cpMin = dwLineIndex;
    tr.chrg.cpMax = dwTextLen;
    tr.lpstrText = pszBuff;
    SendMessage(hEditBox, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
    // Null terminate manually just in case
    // Note: cpMax - cpMin is length.
    // EM_GETTEXTRANGE needs buffer of size (cpMax - cpMin + 1)
    
    sInput = pszBuff;
    delete[] pszBuff;
    
    // Check prompt
    if (sInput.substr(0, 2) != L"> ") return;
    
    // Strip prompt
    sInput = sInput.substr(2);
    // Trim newlines from end if any (user pressed Enter, so there shouldn't be valid ones, but...)
    while (!sInput.empty() && (sInput.back() == L'\r' || sInput.back() == L'\n')) {
        sInput.pop_back();
    }

    // Commands
    if (sInput == L"exit") {
         SendMessage(hMain, WM_DESTROY, 0, 0);
         return;
    } else if (sInput == L"clear") {
         SetWindowText(hEditBox, L"> ");
         m_dwEditLastLF = 0;
         dwIndex = 2;
         SendMessage(hEditBox, EM_SETSEL, dwIndex, dwIndex);
         return;
    } else if (sInput == L"help") {
         ShellExecute(NULL, L"open", L"PeaCalc.html", NULL, NULL, SW_SHOW);
         // Just append new prompt
         SendMessage(hEditBox, EM_SETSEL, -1, -1);
         SendMessage(hEditBox, EM_REPLACESEL, 0, (LPARAM)L"\r\n> ");
         SendMessage(hEditBox, EM_SETSEL, -1, -1); // Move to absolute end
         m_dwEditLastLF = SendMessage(hEditBox, EM_LINEINDEX, -1, 0);
         return;
    } 

    // Math Processing
    sFullOutput = vProcMath(sInput);
    
    // Prepare Defaults for Color
    cfDef.dwMask = CFM_COLOR;
    DWORD cBg, cTxt, cRes;
    m_pConfig->vGetColors(cBg, cTxt, cRes);
    cfDef.crTextColor = cTxt;

    // Replace the LAST PARAGRAPH with the new output
    // Use EM_SETSEL for simplicity and reliability
    SendMessage(hEditBox, EM_SETSEL, dwLineIndex, -1);
    
    // Set default format for the new block
    SendMessage(hEditBox, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfDef); 
    // Replace
    SendMessage(hEditBox, EM_REPLACESEL, 0, (LPARAM)sFullOutput.c_str());
    
    // Scroll
    SendMessage(hEditBox, EM_SCROLLCARET, 0, 0);
    
    // Limit lines logic
    // Get new line count (Visual count is fine for limiting total output)
    DWORD dwLineCount = SendMessage(hEditBox, EM_GETLINECOUNT, 0, 0);
    if (dwLineCount > (DWORD)m_pConfig->iLines) {
        // Delete top lines
        DWORD dwLinesToRemove = dwLineCount - m_pConfig->iLines;
        DWORD dwEndChar = SendMessage(hEditBox, EM_LINEINDEX, dwLinesToRemove, 0);
        cr.cpMin = 0;
        cr.cpMax = dwEndChar;
        SendMessage(hEditBox, EM_EXSETSEL, 0, (LPARAM)&cr);
        SendMessage(hEditBox, EM_REPLACESEL, 0, (LPARAM)L"");
    }

    // Apply Colors Globaly
    vColorizeText(hEditBox);

    // Store cursor
    m_dwEditLastLF = SendMessage(hEditBox, EM_LINEINDEX, -1, 0);
}

/** Handler for a mathematical input: *************************************************/

std::wstring CCommandHandler::vProcMath(std::wstring sInput) {
    /** Variables:                                                                    */
    std::wstring sOutput;
    bool         bOutputHex = false;
    bool         bOutputBin = false;
    double       dOutput;
    INT32        s32Result;
    /** Save the input in the output-string:                                          */
    sOutput = L"  " + sInput + L"\r\n";
    /** Change the input to lower-case:                                               */
    std::transform(sInput.begin(), sInput.end(), sInput.begin(), ::tolower);
    /** Check for output-formatting:                                                  */
    if (sInput.substr(0,4) == L"hex(") {
        /** It shall be hexadecimal:                                                  */
        sInput = sInput.substr(3);
        bOutputHex = true;
    }else if (sInput.substr(0, 4) == L"bin(") {
        sInput = sInput.substr(3);
        bOutputBin = true;
    }
    /** Try to parse it:                                                              */
    s32Result = m_TermMain.s32Parse(sInput);
    if (s32Result == C_TERM_FuncOK      ) return (sOutput + L"  * Results in function!\r\n> ");
    if (s32Result != C_TERM_NumOK       ) return (sOutput + L"  * Parsing Error!\r\n> ");
    /** If we got here, the term can be calculated:                                   */
    s32Result = m_TermMain.s32Execute(0, &dOutput);
    if (s32Result == C_TERM_DivByZero   ) return (sOutput + L"  * Division by zero!\r\n> ");
    if (s32Result == C_TERM_BoolTooLarge) return (sOutput + L"  * Boolean operator too large!\r\n> ");
    /**                                                                               */
    /** Build up the output:                                                          */
    if (bOutputHex) {
        /** Build as hex:                                                             */
        if ((!isInteger(dOutput)) || (dOutput >= C_TERM_MAXINT)) {
            sOutput += L"  = " + sOutputHexFloat(dOutput) + L"\r\n> ";
        } else {
            sOutput += L"  = " + sOutputHexInt(dOutput) + L"\r\n> ";
        }
        return sOutput;
    }else if (bOutputBin) {
        /** Build as binary:                                                        */
        if (!isInteger(dOutput)) return (sOutput + L"  * Binary output only supported for integers!\r\n> ");
        if (dOutput >= C_TERM_MAXINT) return (sOutput + L"  * Result too large for binary output!\r\n> ");
        sOutput += L"  = " + sOutputBin(dOutput) + L"\r\n> ";
        return sOutput;
    }else if (isInteger(dOutput)) {
        /** Build as usual integer:                                                   */
        sOutput += L"  = " + sOutputInt(dOutput) + L"\r\n> ";
        return sOutput;
    }
    /** Build as some kind of float:                                                  */
    sOutput += L"  = " + sOutputFloat(dOutput) + L"\r\n> ";
    return sOutput;
}

/** Small support-function to scan for CRs: *******************************************/

DWORD CCommandHandler::dwFindNthLastCR(const WCHAR* pszwInput, int iCount) {
    DWORD dwPos = wcslen(pszwInput);
    while ((dwPos > 0) && (iCount>0)) {
        dwPos--;
        if (pszwInput[dwPos] == L'\n') iCount--;
    }
    return dwPos;
}

/** Formats an output as hex-int: *****************************************************/

std::wstring CCommandHandler::sOutputHexInt(double dInput) {
    WCHAR szwNumBuf[40];
    INT64 s64Temp = (INT64) dInput;
    swprintf(szwNumBuf, L"0x%I64X", s64Temp);
    return std::wstring(szwNumBuf);
}

/** Formats an output as hex-float: ***************************************************/

std::wstring CCommandHandler::sOutputHexFloat(double dInput) {
    tUnifNum num;
    WCHAR    szwNumBuf[40];
    num.f = (float) dInput;
    swprintf(szwNumBuf, L"0x%02X%02X%02X%02X f", num.u[3], num.u[2], num.u[1], num.u[0]);
    return std::wstring(szwNumBuf);
}

/** Formats an output as binary: ******************************************************/

std::wstring CCommandHandler::sOutputBin(double dInput) {
    /** Variables:                                                                    */
    WCHAR   szwNumBuf[80];
    INT64   s64Temp = (INT64)dInput;
    uint8_t u8Pos = 4;
    /** Find the right length:                                                        */
    while (((INT64)1 << u8Pos) <= abs(s64Temp)) u8Pos += 4;
    /** Build the according number of digits:                                         */
    wcscpy(szwNumBuf, L"0b");
    while (u8Pos>0) {
        /** Put spaces before each 4th digit:                                         */
        if (((u8Pos % 4) == 0) && (u8Pos>0)) wcscat(szwNumBuf, L" ");
        /** Go one bit further:                                                       */
        u8Pos--;
        /** And add the digit:                                                        */
        if (s64Temp & ((INT64)1 << u8Pos)) {
            wcscat(szwNumBuf, L"1");
        }else{
            wcscat(szwNumBuf, L"0");
        }
    }
    return std::wstring(szwNumBuf);
}

/** Formats an output as decimal integer: *********************************************/

std::wstring CCommandHandler::sOutputInt(double dInput) {
    WCHAR szwNumBuf[40];
    INT64 s64Temp = (INT64)dInput;
    swprintf(szwNumBuf, L"%I64i", s64Temp);
    return std::wstring(szwNumBuf);
}

/** Formats an output as floating-point value: ****************************************/

std::wstring CCommandHandler::sOutputFloat(double dInput) {
    WCHAR  szwNumBuf[40];
    WCHAR  szwFormat[40];
    double dTemp = dInput/10;
    int    iIntDigits = 1;
    int    iDecimals  = m_pConfig->iPrecision;
    /** Check, if the input is in the range for fixed-point:                          */
    if ((abs(dInput) < 1000000) && (abs(dInput) > 0.09)) {
        /** It is, so get the number of integer-digits:                               */
        iIntDigits = 1;
        while (abs(dTemp) > 1) {
            dTemp = dTemp / 10;
            iIntDigits++;
        }
        /** Make sure, that there's enough space for the precision:                   */
        if ((iIntDigits + iDecimals) > CNF_MAX_PRECISION) iDecimals = CNF_MAX_PRECISION - iIntDigits + 1;
        /** And build the output:                                                     */
        swprintf(szwFormat, L"%%1.%df", iDecimals);
        swprintf(szwNumBuf, szwFormat , dInput);
        return std::wstring(szwNumBuf);
    }
    /** It is not fixed-point, thus write it exponential style:                       */
    swprintf(szwFormat, L"%%1.%dE", m_pConfig->iPrecision);
    swprintf(szwNumBuf, szwFormat, dInput);
    return std::wstring(szwNumBuf);
}

/** Small support-functions: **********************************************************/

bool CCommandHandler::isInteger(double dInput) {
  double fractpart, intpart;
  fractpart = modf (dInput , &intpart);
  if (fractpart>0) return false;
  return true;
}

void CCommandHandler::vRollback(WCHAR* pszwInput, WCHAR* pszwNewStart) {
    while (*pszwNewStart != L'\0') {
        *pszwInput = *pszwNewStart;
        pszwInput++;
        pszwNewStart++;
    }
    *pszwInput = L'\0';
}

/** Colorizes the text in the editor: *************************************************/

void CCommandHandler::vColorizeText(HWND hEditBox) {
    /** Determine Colors:                                                             */
    DWORD cBg, cTxt, cRes;
    m_pConfig->vGetColors(cBg, cTxt, cRes);

    /** Setup Formats:                                                                */
    CHARFORMAT2W cfDef = { sizeof(CHARFORMAT2W) };
    cfDef.cbSize = sizeof(CHARFORMAT2W);
    cfDef.dwMask = CFM_COLOR;
    cfDef.crTextColor = cTxt;

    CHARFORMAT2W cfRes = { sizeof(CHARFORMAT2W) };
    cfRes.cbSize = sizeof(CHARFORMAT2W);
    cfRes.dwMask = CFM_COLOR;
    cfRes.crTextColor = cRes;

    /** Apply Default Color to ALL text first:                                        */
    SendMessage(hEditBox, EM_SETSEL, 0, -1);
    SendMessage(hEditBox, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfDef);

    /** Scan text and colorize results:                                               */
    DWORD dwLen = GetWindowTextLength(hEditBox);
    if (dwLen == 0) return;

    // Use EM_GETTEXTRANGE to get exactly what RichEdit sees (no CRLF conversion issues)
    WCHAR* pszText = new WCHAR[dwLen + 1];
    TEXTRANGEW tr;
    tr.chrg.cpMin = 0;
    tr.chrg.cpMax = dwLen;
    tr.lpstrText = pszText;
    SendMessage(hEditBox, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
    
    // Safety null terminate (though standard says it copies up to...)
    // Wait, EM_GETTEXTRANGE doesn't promise NULL termination if full buffer used?
    // It copies null usually. But let's be safe.
    // Actually we can't be sure where it ended if it didn't copy NULL.
    // But dwLen is length. So pszText[dwLen] = 0;
    pszText[dwLen] = L'\0'; // Basic safety

    DWORD dwPos = 0;
    while (dwPos < dwLen) {
        // Check if line starts with "  = "
        // Logic: Start of buffer OR Previous char was \r or \n
        bool bLineStart = (dwPos == 0) || (pszText[dwPos-1] == L'\r') || (pszText[dwPos-1] == L'\n');
        
        if (bLineStart) {
             // Check for "  = "
             // We need to look ahead
             if ((dwPos + 4 <= dwLen)) {
                  if (wcsncmp(&pszText[dwPos], L"  = ", 4) == 0) {
                      // Found a result line!
                      // Find end of this line
                      DWORD dwEnd = dwPos;
                      while (dwEnd < dwLen && pszText[dwEnd] != L'\r' && pszText[dwEnd] != L'\n') {
                          dwEnd++;
                      }
                      
                      // Apply Color
                      SendMessage(hEditBox, EM_SETSEL, dwPos, dwEnd);
                      SendMessage(hEditBox, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfRes);
                      
                      dwPos = dwEnd;
                  }
             }
        }
        dwPos++;
    }
    
    delete[] pszText;
    
    /** Restore selection to end:                                                     */
    SendMessage(hEditBox, EM_SETSEL, dwLen, dwLen);
}

