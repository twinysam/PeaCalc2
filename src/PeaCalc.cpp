//
//  This file is part of PeaCalc++ project
//  Copyright (C)2018 Jens Daniel Schlachter <osw.schlachter@mailbox.org>
//  Modified/Forked by twinysam (2026) under GPL v3.0 (https://github.com/twinysam/PeaCalc2)
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
#include <string>
#include "resource.h"
#include "ConfigHandler.h"
#include "Term.h"
#include "CommandHandler.h"
#include <Richedit.h>
#include <shellapi.h>

/** Compiler Settings: ****************************************************************/

#ifdef _MSC_VER
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"User32.lib"  )
#pragma comment(lib,"gdi32.lib"   )
#pragma comment(lib,"version.lib" )
#pragma warning(disable : 4100)
#pragma warning(disable : 4996)
#else
#pragma GCC diagnostic ignored "-Wconversion-null"
#endif

/** Local Defines: ********************************************************************/

#define C_MINWIDTH    220
#define C_LINECOUNT   3

/** Global variables: *****************************************************************/

HWND            hWndMain;
HWND            hWndEdit;
WCHAR           szAppName[]    = TEXT("PeaCalc2 Portable");
const WCHAR     cszwHelpText[] = TEXT("  * This program comes with ABSOLUTELY NO WARRANTY.\r\n  * It is free software; you can redistribute it and/or modify it\r\n  * under the terms of the GNU General Public License version 3,\r\n  * or (at your option) any later version; type 'license' for details.\r\n  * Type 'help' to open the user-manual.\r\n\r\n  Project page and updates: https://github.com/twinysam/PeaCalc2");
WCHAR           pszwInfoText[C_TEXTBUFSIZE];
WNDPROC         lpfnEditBoxLowProc;
CConfigHandler  Config;
CCommandHandler Command(&Config);

/** Forward Declarations: *************************************************************/

LRESULT CALLBACK WndProc        (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditBoxProc    (HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK InfoBoxProc    (HWND, UINT, WPARAM, LPARAM);
void vDoTabScan(bool bDir, bool bReScan);
void vCreateInfoText (WCHAR* pszwOutput);
bool vAddVersionInfo(WCHAR* pszwOutput, const WCHAR* pszwEntry);
void ShowInfoDialog  (HWND hOwner);

/** Helper Functions for Colors: ******************************************************/

COLORREF cBgColor, cTxtColor, cResultColor;
HBRUSH   hBgBrush = NULL;

void UpdateColorSettings() {
    Config.vGetColors(cBgColor, cTxtColor, cResultColor);

    if (hBgBrush) DeleteObject(hBgBrush);
    hBgBrush = CreateSolidBrush(cBgColor);
}

typedef HRESULT(WINAPI* fnDwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);

void SetWindowDarkTheme(HWND hwnd, BOOL bDark) {
    /** Resolve dwmapi once and keep it for the process lifetime. */
    static HMODULE hDwm = LoadLibraryW(L"dwmapi.dll");
    static fnDwmSetWindowAttribute pfn = hDwm
        ? (fnDwmSetWindowAttribute)GetProcAddress(hDwm, "DwmSetWindowAttribute")
        : NULL;
    if (pfn) {
        BOOL bUseDark = bDark;
        pfn(hwnd, 20, &bUseDark, sizeof(bUseDark));
        pfn(hwnd, 19, &bUseDark, sizeof(bUseDark));
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
}

/** Application entry function: *******************************************************/

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) {
    /** Variables:                                                                    */
    MSG msg;
    WNDCLASSEX wndclass;
    /** Change the application-title if portable:                                     */
    if (!Config.bIsPortable()) szAppName[8] = 0;
    /** Create the info-text shown by the info pop-up:                                */
    vCreateInfoText(pszwInfoText);
    UpdateColorSettings(); // Initialize colors
    /** Prepare Window-Class:                                                         */
	wndclass.cbSize        = sizeof(WNDCLASSEX);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPICON));
    wndclass.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 16, 16, 0);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    /** The main window erases itself with the current theme brush (WM_ERASEBKGND),
        so no fixed stock brush is used here.                                         */
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szAppName;
    /** Try to register it:                                                           */
    if (!RegisterClassEx(&wndclass)) {
        MessageBox(NULL, TEXT("This program requires at least Windows 2K!"), szAppName, MB_ICONERROR);
        return 0;
    }
    DWORD dwExStyle = WS_EX_TOPMOST | WS_EX_APPWINDOW;
    DWORD dwStyle = WS_CAPTION | WS_BORDER | WS_SYSMENU | WS_SIZEBOX | WS_MINIMIZEBOX;
    if (Config.iDefaultUI == 0) {
        dwExStyle |= WS_EX_TOOLWINDOW;
    } else {
        dwStyle |= WS_MAXIMIZEBOX;
    }
    hWndMain = CreateWindowEx(
        dwExStyle,
        szAppName,
        szAppName,
        dwStyle,
        Config.iLeft, Config.iTop, Config.iWidth, Config.iHeight,
        NULL, NULL, hInstance, NULL);
    /** Set dark/light title bar theme:                                               */
    if (Config.iDefaultUI) {
        SetWindowDarkTheme(hWndMain, Config.bIsDarkTheme());
    }
    /** And show it:                                                                  */
    ShowWindow(hWndMain, iCmdShow);
    UpdateWindow(hWndMain);
    /** On the very first start (no stored text) greet with the info pop-up:          */
    if (Config.sText.empty()) {
        ShowInfoDialog(hWndMain);
    }
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int) msg.wParam;
}

/** Encapsulated creator of the edit-box: *********************************************/

HWND CreateEditBox(HWND hOwner, WPARAM wParam, LPARAM lParam) {
    /** Variables:                                                                    */
    HWND   hWndEdit;
    HFONT  hFont;
    /** Make sure the RichEdit library is loaded before creating the control:         */
    LoadLibrary(L"Msftedit.dll");
    /** Create the edit-box-control:                                                  */
    hWndEdit = CreateWindow(L"RICHEDIT50W", NULL,
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
        0, 0, 0, 0, hOwner, (HMENU)ID_EDIT,
        ((LPCREATESTRUCT)lParam)->hInstance, NULL);
    /** If RichEdit 4.1+ failed, try older class or error handling? */
    if (!hWndEdit) {
         // Fallback or error? Assuming Msftedit.dll is present on standard Windows.
         MessageBox(hOwner, L"Could not create RichEdit control.", L"Error", MB_OK);
         return NULL;
    }
    /** Overwrite its message-procedure and conserve the low-level one:               */
    lpfnEditBoxLowProc = (WNDPROC)SetWindowLongPtr(hWndEdit,GWLP_WNDPROC,(LONG_PTR)EditBoxProc );
    /** Set the font of the edit-box:                                                 */
    hFont = CreateFont(Config.iFontSize, 0, 0, 0,
        FW_DONTCARE,                  // nWeight
        FALSE,                        // bItalic
        FALSE,                        // bUnderline
        0,                            // cStrikeOut
        ANSI_CHARSET,                 // nCharSet
        OUT_DEFAULT_PRECIS,           // nOutPrecision
        CLIP_DEFAULT_PRECIS,          // nClipPrecision
        PROOF_QUALITY,                // nQuality
        VARIABLE_PITCH, TEXT("Consolas"));
    SendMessage(hWndEdit,             // Handle of edit control
        WM_SETFONT,                   // Message to change the font
        (WPARAM)hFont,                // handle of the font
        MAKELPARAM(TRUE, 0));

    /** Set the initial text:                                                         */
    Command.vSetText(hWndEdit, Config.sText.c_str());
    SendMessage(hWndEdit, EM_SETBKGNDCOLOR, 0, (LPARAM)cBgColor);
    /** And be done:                                                                  */
    return hWndEdit;
}

/** Encapsulated close-command for the window: ****************************************/

void CloseMain(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    /** Variables:                                                                    */
    RECT         rcWind;
    TCHAR        buffer[C_TEXTBUFSIZE];
    /** Get the windo-dimensions and store them:                                      */
    GetWindowRect(hwnd    , &rcWind);
    Config.iTop    = rcWind.top;
    Config.iLeft   = rcWind.left;
    Config.iHeight = (rcWind.bottom - rcWind.top);
    Config.iWidth  = (rcWind.right - rcWind.left);
    /** Get the edit-text and store it:                                               */
    GetWindowText(hWndEdit, buffer, C_TEXTBUFSIZE);
    Config.sText   = std::wstring(buffer);
    /** And send a quit message to the application:                                   */
    PostQuitMessage(0);
}

/** Main-Window Message-Handler: ******************************************************/

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    /** Variables:                                                                    */
    LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
    RECT         rcClient, rcWind;
    /** Parse the window-message:                                                     */
    switch (message) {
    case WM_CREATE:
        /** Prepare window for opacity:                                               */
        SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
        /** Create Edit-Box:                                                          */
        hWndEdit = CreateEditBox(hwnd, wParam, lParam);
        break;
    case WM_GETMINMAXINFO:
        /** Get the rectangles of window and client to calc the border-width:         */
        GetClientRect(hwnd, &rcClient);
        GetWindowRect(hwnd, &rcWind);
        /** And set the minimum size based as reply via lpMMI in lParam:              */
        lpMMI->ptMinTrackSize.x = C_MINWIDTH;
        lpMMI->ptMinTrackSize.y = ((rcWind.bottom - rcWind.top ) - rcClient.bottom) + (C_LINECOUNT * Config.iFontSize) + 2;
        break;
    case WM_SETFOCUS:
        /** When the window received the focus, pass it on to the edit-control:       */
        SetFocus(hWndEdit);
        return 0;
    case WM_SIZE:
        /** When the window is resized, do so with the edit-control:                  */
        MoveWindow(hWndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        SendMessage(hWndEdit, EM_SCROLLCARET, 0, 0);
        return 0;
    case WM_NCRBUTTONUP:
        /** Right-click on the title bar opens the info pop-up (instead of the
            system menu).                                                             */
        if (wParam == HTCAPTION) {
            ShowInfoDialog(hwnd);
            return 0;
        }
        break;
    case WM_COMMAND:
        /** If it is from the text-box, parse the text-box message:                   */
        if (LOWORD(wParam) == ID_EDIT) {
            switch (HIWORD(wParam)) {
            case EN_ERRSPACE:
            case EN_MAXTEXT:
                MessageBox(hwnd, TEXT("Edit control out of space."), szAppName, MB_OK | MB_ICONSTOP);
                return 0;
            case EN_SETFOCUS:
                /** When the text-box gets the focus, set the main-window strong:     */
                SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
                break;
            case EN_KILLFOCUS:
                /** When the text-box looses the focus, set the main-window opaque:   */
                SetLayeredWindowAttributes(hwnd, 0, (unsigned char) Config.iOpacity, LWA_ALPHA);
                break;
            }
        }
        break;
    case WM_SETTINGCHANGE:
        UpdateColorSettings();
        if (Config.iDefaultUI) {
            SetWindowDarkTheme(hwnd, Config.bIsDarkTheme());
        }
        SendMessage(hWndEdit, EM_SETBKGNDCOLOR, 0, (LPARAM)cBgColor);
        Command.vColorizeText(hWndEdit);
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    case WM_ERASEBKGND: {
        /** Paint the window background with the active theme colour to avoid the
            default white flash (especially in dark mode). */
        RECT rc;
        GetClientRect(hwnd, &rc);
        if (hBgBrush) FillRect((HDC)wParam, &rc, hBgBrush);
        return 1;
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
        SetTextColor((HDC)wParam, cTxtColor);
        SetBkColor((HDC)wParam, cBgColor);
        return (LRESULT)hBgBrush;
    case WM_DESTROY:
        /** Before the main-window is destroyed, store the window-properties:         */
        CloseMain(hwnd, wParam, lParam);
        return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

/** Edit-Box Message-Handler: *********************************************************/

LRESULT CALLBACK EditBoxProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    /** Variables:                                                                    */
    DWORD        dwIndex;
    static bool  bScanActive;
    /** Do the message-processing:                                                    */
    switch ( message){
    case WM_CREATE:
        bScanActive = false;
        break;
    case WM_KEYDOWN:
        /** Check, if it was a tab for scanning:                                      */
        if (wParam == VK_TAB) {
            /** It is, so check if there has been scanning before:                    */
            if (!bScanActive) {
                bScanActive = true;
                vDoTabScan((GetKeyState(VK_SHIFT) < 0), true);
            } else {
                vDoTabScan((GetKeyState(VK_SHIFT) < 0), false);
            }
            return 0;
        }
        /** Check if it was Enter:                                                    */
        if (wParam == VK_RETURN) {
            Command.vProcEnter(hWndMain, hwnd);
            return 0;
        }
        if (wParam == VK_BACK) {
            DWORD dwEnd;
            SendMessage(hwnd, EM_GETSEL, (WPARAM)&dwIndex, (LPARAM)&dwEnd);
            /** Block backspace if it would delete the prompt characters '> ':        */
            if (dwIndex == dwEnd) {
                /** No selection: block if cursor is at or before first user position:*/
                if (dwIndex <= (Command.m_dwEditLastLF + 2)) return 0;
            } else {
                /** Selection: block if selection starts at or before prompt:         */
                if (dwIndex <= (Command.m_dwEditLastLF + 1)) return 0;
            }
        }
        /** Check, if it was a delete:                                                */
        if (wParam == VK_DELETE) {
            DWORD dwEnd;
            SendMessage(hwnd, EM_GETSEL, (WPARAM)&dwIndex, (LPARAM)&dwEnd);
            /** Block DELETE if it would affect the prompt characters:                */
            if (dwIndex <= (Command.m_dwEditLastLF + 1)) return 0;
            /** It is not, thus make sure that scanning is inactive:                  */
        }
        /** Check, if it was a home-key:                                              */
        if (wParam == VK_HOME) {
            /** Check, if it comes from within the last line:                         */
            SendMessage(hwnd, EM_GETSEL, (WPARAM)&dwIndex, NULL);
            if (dwIndex > (Command.m_dwEditLastLF)) {
                /** If so, set the selection the starting entry-position:             */
                dwIndex = Command.m_dwEditLastLF + 2;
                SendMessage(hwnd, EM_SETSEL, dwIndex, dwIndex);
                return 0;
            }
        }
        break;
    case WM_CHAR:
        /** If it is a tab, ignore it:                                                */
        if (wParam == VK_TAB) {
            return 0;
        } else {
            /** If It is not, scanning should not be active:                          */
            bScanActive = false;
        }
        /** Make sure, that CTRL+C is always passed on:                               */
        if ((GetKeyState(VK_CONTROL) & 0x8000) && (wParam == 3)) break;
        /** Fetch the input-position:                                                 */
        SendMessage(hwnd, EM_GETSEL, (WPARAM)&dwIndex, NULL);
        /** Check, if something was entered before the allowed start-position:        */
        if (dwIndex < (Command.m_dwEditLastLF + 3)) {
            if (dwIndex > Command.m_dwEditLastLF) {
                dwIndex = Command.m_dwEditLastLF + 3;
            }else{
                dwIndex = GetWindowTextLength(hwnd);
            }
            /** Set the selection after the last character:                           */
            SendMessage(hwnd, EM_SETSEL, dwIndex, dwIndex);
        }
        /** Check, if it is a colon, which will be replaced by point:                 */
        if (wParam == 0x002C) {
            /** Mask out the keyboard-specifics:                                      */
            lParam = (lParam & 0xE000007FFF);
            /** And replace with the new unicode:                                     */
            wParam = 0x002E;
        }
        /** Check, if it is a slash, which will be replaced by a division-sign:       */
        if (wParam == 0x002F) {
            /** Mask out the keyboard-specifics:                                      */
            lParam = (lParam & 0xE000007FFF);
            /** And replace with the new unicode:                                     */
            wParam = 0x00F7;
        }
        /** Check, if it is a back-slash '\', which will be replaced by a root-sign:  */
        if (wParam == 0x005C) {
            /** Mask out the keyboard-specifics:                                      */
            lParam = (lParam & 0xE000007FFF);
            /** And replace with the new unicode:                                     */
            wParam = 0x221A;
        }
        break;
    }
    return CallWindowProc(lpfnEditBoxLowProc, hwnd, message, wParam, lParam);
}

/** Support-function to handle the scanning with tab: *********************************/

void vDoTabScan(bool bDir, bool bReScan) {
    /** Variables: */
    static WCHAR szwBoxText [C_TEXTBUFSIZE];
    static WCHAR szwScanText[C_TEXTBUFSIZE];
    static DWORD dwScanLen;
    static DWORD dwSafeLine;
    DWORD        dwScanLine;
    DWORD        dwScanPos;
    WCHAR        buffer[C_TEXTBUFSIZE];
    DWORD        dwIndex;
    /** Check, if rescan has to be done:                                              */
    if (bReScan) {
        /** It has to, so fetch the text and prepare the scan:                        */
        GetWindowText(hWndEdit, szwBoxText, C_TEXTBUFSIZE);
        /** Create the scan-string from the ACTUAL terminal text:                      */
        SendMessage(hWndEdit, EM_SETSEL, -1, -1);
        dwIndex = SendMessage(hWndEdit, EM_LINEINDEX, -1, 0);
        wcscpy(szwScanText, L"");
        // We get the line text directly from the RichEdit if possible, 
        // but for now we'll stick to szwBoxText but use the CORRECT index.
        // Get line text:
        int iLineLen = (int)SendMessage(hWndEdit, EM_LINELENGTH, dwIndex, 0);
        if (iLineLen > 2) {
            // Buffer line text
            WCHAR* pLine = new WCHAR[iLineLen + 1];
            *( (WORD*) pLine ) = iLineLen;
            SendMessage(hWndEdit, EM_GETLINE, (WPARAM) (SendMessage(hWndEdit, EM_LINEFROMCHAR, dwIndex, 0)), (LPARAM) pLine);
            pLine[iLineLen] = 0;
            // Copy the whole line including '>'
            wcscpy(szwScanText, pLine);
            delete[] pLine;
        }
        /** ... and init the numbers:                                                 */
        dwScanLen = wcslen(szwScanText);
        dwSafeLine = 1;
    }
    /** Scan according to parsing direction:                                          */
    if (!bDir) {
        /**                                                                           */
        /** Scan Backwards:                                                           */
        dwScanLine = dwSafeLine;
        dwScanPos  = Command.dwFindNthLastCR(szwBoxText, dwScanLine);
        /** Move to the character after the CR:                                       */
        if (dwScanPos > 0) dwScanPos++;
        /** Scan as long as not at the start yet:                                     */
        do {
            /** Search for the next CR up:                                            */
            if (dwScanPos > 1) dwScanLine++;
            dwScanPos = Command.dwFindNthLastCR(szwBoxText, dwScanLine);
            /** Move to the character after the CR:                                   */
            if (dwScanPos > 0) dwScanPos++;
            /** ... and check:                                                        */
        } while ((dwScanPos > 0) &&
                 ((wcsncmp(&szwBoxText[dwScanPos], szwScanText, dwScanLen) != 0) ||
                  (wcsncmp(&szwBoxText[dwScanPos], L"  =", 3) == 0) ||
                  (wcsncmp(&szwBoxText[dwScanPos], L"  *", 3) == 0)));
    }else {
        /**                                                                           */
        /** Scan Forwards:                                                            */
        dwScanLine = dwSafeLine;
        /** Scan as long as not at the last CR yet:                                   */
        do {
            /** Search for the next CR down:                                          */
            if (dwScanLine > 1) dwScanLine--;
            dwScanPos = Command.dwFindNthLastCR(szwBoxText, dwScanLine);
            /** Move to the character after the CR:                                   */
            if (dwScanPos > 0) dwScanPos++;
            /** ... and check:                                                        */
        } while ((dwScanLine > 1) &&
                 ((wcsncmp(&szwBoxText[dwScanPos], szwScanText, dwScanLen) != 0) ||
                  (wcsncmp(&szwBoxText[dwScanPos], L"  =", 3) == 0) ||
                  (wcsncmp(&szwBoxText[dwScanPos], L"  *", 3) == 0)));
    }
    /**                                                                               */
    /** Check, if something was found:                                                */
    if (wcsncmp(&szwBoxText[dwScanPos], szwScanText, dwScanLen) == 0) {
        /** Fetch the text from the found position onward:                            */
        wcscpy(buffer, &szwBoxText[dwScanPos]);
        /** Terminate it at the CR:                                                   */
        dwIndex = 0;
        while (buffer[dwIndex] != L'\r') dwIndex++;
        buffer[dwIndex] = 0;
        /** Copy it INTO the box-text by REPLACING THE LAST LINE:                     */
        SendMessage(hWndEdit, EM_SETSEL, -1, -1); // End
        int iLastLine = (int)SendMessage(hWndEdit, EM_LINEFROMCHAR, -1, 0);
        int iStart = (int)SendMessage(hWndEdit, EM_LINEINDEX, iLastLine, 0);
        SendMessage(hWndEdit, EM_SETSEL, iStart, -1);
        
        // Rebuild line: "> " + buffer[1+]
        std::wstring sNewPrompt = L"> ";
        sNewPrompt += &buffer[1];
        SendMessage(hWndEdit, EM_REPLACESEL, 0, (LPARAM)sNewPrompt.c_str());
        
        /** Update prompt-start location:                                             */
        SendMessage(hWndEdit, EM_SETSEL, -1, -1);
        Command.m_dwEditLastLF = SendMessage(hWndEdit, EM_LINEINDEX, -1, 0);
        /** Trigger a scroll:                                                         */
        SendMessage(hWndEdit, EM_SCROLLCARET, 0, 0);
        /** And store the newly found position for the next run:                      */
        dwSafeLine = dwScanLine;
    }
}

/** Support-function to build the info-text: ******************************************/

void vCreateInfoText(WCHAR* pszwOutput) {
    WCHAR buffer[256];
    pszwOutput[0] = 0;
    
    // Attempt to load info, but adding prefixes only if successful
    if (vAddVersionInfo(buffer, L"InternalName")) {
        wcscat(pszwOutput, L"  * ");
        wcscat(pszwOutput, buffer);
    }
    if (vAddVersionInfo(buffer, L"FileVersion")) {
        if (pszwOutput[0] != 0) wcscat(pszwOutput, L" ");
        else wcscat(pszwOutput, L"  * ");
        wcscat(pszwOutput, buffer);
    }
    if (vAddVersionInfo(buffer, L"LegalCopyright")) {
        if (pszwOutput[0] != 0) wcscat(pszwOutput, L", ");
        else wcscat(pszwOutput, L"  * ");
        wcscat(pszwOutput, buffer);
    }
    
    if (pszwOutput[0] != 0) wcscat(pszwOutput, L"\r\n");
    wcscat(pszwOutput, cszwHelpText);
}

/** Support-function to fetch info from the version-resource: *************************/

bool vAddVersionInfo(WCHAR* pszwOutput, const WCHAR* pszwEntry) {
    /** Variables:                                                                    */
    DWORD   vLen, langD;
    BOOL    retVal;
    LPVOID  retbuf = NULL;
    static  WCHAR fileEntry[256];
    /** Fetch-Code:                                                                   */
    HRSRC hVersion = FindResource(NULL, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
    if (hVersion != NULL) {
        HGLOBAL hGlobal = LoadResource(NULL, hVersion);
        if (hGlobal != NULL) {
            LPVOID versionInfo = LockResource(hGlobal);
            if (versionInfo != NULL) {
                swprintf(fileEntry, L"\\VarFileInfo\\Translation");
                retVal = VerQueryValue(versionInfo, fileEntry, &retbuf, (UINT *)&vLen);
                if (retVal && vLen == 4) {
                    memcpy(&langD, retbuf, 4);
                    #ifdef _MSC_VER
                    swprintf(fileEntry, L"\\StringFileInfo\\%02X%02X%02X%02X\\%s",
                        (langD & 0xff00) >> 8, langD & 0xff, (langD & 0xff000000) >> 24,
                        (langD & 0xff0000) >> 16, pszwEntry);
                    #else
                    swprintf(fileEntry, L"\\StringFileInfo\\%02X%02X%02X%02X\\%S",
                        (langD & 0xff00) >> 8, langD & 0xff, (langD & 0xff000000) >> 24,
                        (langD & 0xff0000) >> 16, pszwEntry);
                    #endif
                    if (VerQueryValue(versionInfo, fileEntry, &retbuf, (UINT *)&vLen)) {
                        wcscpy(pszwOutput, (WCHAR*)retbuf);
                        return true;
                    }
                }
                
                // Fallback 1: System Language
                swprintf(fileEntry, L"\\StringFileInfo\\%04X04B0\\%s", GetUserDefaultLangID(), pszwEntry);
                if (VerQueryValue(versionInfo, fileEntry, &retbuf, (UINT *)&vLen)) {
                    wcscpy(pszwOutput, (WCHAR*)retbuf);
                    return true;
                }

                // Fallback 2: Hardcoded US English (matches our .rc file)
                swprintf(fileEntry, L"\\StringFileInfo\\040904B0\\%s", pszwEntry);
                if (VerQueryValue(versionInfo, fileEntry, &retbuf, (UINT *)&vLen)) {
                    wcscpy(pszwOutput, (WCHAR*)retbuf);
                    return true;
                }
            }
        }
    }
    return false;
}

/** Info pop-up window procedure: *****************************************************/

INT_PTR CALLBACK InfoBoxProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG: {
        /** Use the dynamic application name (portable or not) as the title. */
        SetWindowText(hDlg, szAppName);
        HWND hInfo = GetDlgItem(hDlg, IDC_INFO_EDIT);
        if (hInfo) {
            SendMessage(hInfo, WM_SETTEXT, 0, (LPARAM)pszwInfoText);
            /** Let the RichEdit turn URLs into clickable links. */
            SendMessage(hInfo, EM_AUTOURLDETECT, (WPARAM)TRUE, 0);
            SendMessage(hInfo, EM_SETEVENTMASK, 0,
                        SendMessage(hInfo, EM_GETEVENTMASK, 0, 0) | ENM_LINK);
        }
        return TRUE;
    }
    case WM_NOTIFY: {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if ((pnmh->code == EN_LINK) && (pnmh->hwndFrom == GetDlgItem(hDlg, IDC_INFO_EDIT))) {
            ENLINK* pLink = (ENLINK*)lParam;
            if (pLink->msg == WM_LBUTTONUP) {
                int iLen = (int)(pLink->chrg.cpMax - pLink->chrg.cpMin);
                if (iLen > 0) {
                    std::wstring sUrl((size_t)iLen + 1, L'\0');
                    TEXTRANGEW tr;
                    tr.chrg      = pLink->chrg;
                    tr.lpstrText = &sUrl[0];
                    SendMessage(pnmh->hwndFrom, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
                    ShellExecute(NULL, L"open", sUrl.c_str(), NULL, NULL, SW_SHOW);
                }
            }
            return TRUE;
        }
        break;
    }
    case WM_COMMAND:
        if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL)) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }
    return FALSE;
}

/** Opens the modal info pop-up: ******************************************************/

void ShowInfoDialog(HWND hOwner) {
    /** The dialog hosts a RichEdit control, so make sure the library is loaded. */
    LoadLibrary(L"Msftedit.dll");
    INT_PTR result = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_INFOBOX), hOwner, InfoBoxProc);
    if (result == -1) {
        /** RichEdit (or the dialog) was unavailable: fall back to a plain message. */
        MessageBox(hOwner, pszwInfoText, szAppName, MB_OK | MB_ICONINFORMATION);
    }
}
