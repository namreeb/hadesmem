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

// Hades
#include "About.h"

namespace Hades
{
  namespace Loader
  {
    LRESULT AboutDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, 
      LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
      CenterWindow(GetParent());
      return 0;
    }

    LRESULT AboutDialog::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, 
      HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
      EndDialog(wID);
      return 0;
    }
  }
}
