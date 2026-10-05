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

/** Definitions: **********************************************************************/

#define IDI_APPICON   100
#define IDD_INFOBOX   101
#define ID_EDIT       110
#define IDC_INFO_EDIT 1001

/** Command for the "About" entry in the window (title-bar) menu. The low four bits
    of a WM_SYSCOMMAND wParam are reserved by the system, so the identifier has to be
    a multiple of 16 and must stay clear of the SC_* range (0xF000 and up).          */

#define IDM_ABOUT     0x0100
