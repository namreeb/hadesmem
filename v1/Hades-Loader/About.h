/*
This file is part of HadesMem.
Copyright © 2010 Cypherjb (aka Chazwazza, aka Cypher). 
<http://www.cypherjb.com/> <cypher.jb@gmail.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

// Windows
#include <Windows.h>
#include <atlbase.h>
#include <atlwin.h>

// WTL
#include <atlapp.h>
#include <atluser.h>
#include <atlframe.h>

// Hades
#include "Resource.h"

namespace Hades
{
  namespace Loader
  {
    class AboutDialog : public CDialogImpl<AboutDialog>
    {
    public:
      enum { IDD = IDD_ABOUT };

      LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, 
        BOOL& bHandled);

      LRESULT OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, 
        BOOL& bHandled);

      BEGIN_MSG_MAP(AboutDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(ID_BUTTON_OK, OnCloseCmd)
      END_MSG_MAP()
    };
  }
}
