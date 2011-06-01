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

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using HadesAD;

namespace Hades_DotNetTest
{
  public partial class Main : Form
  {
    private string[] MainArgs;

    public Main(string[] args)
    {
      MainArgs = args;
      InitializeComponent();
    }

    private void Main_Load(object sender, EventArgs e)
    {
      foreach (string arg in MainArgs)
        LstOutput.Items.Add(arg);
      HadesVM.AddFrameHandler(OnFrame);
      LstOutput.Items.Add(HadesVM.Scripting.GetScriptResult(
          "return Hades.GetSessionName()", 0));
    }

    public void OnFrame()
    {
      LstOutput.Items.Add("OnFrame!");
//       MessageBox.Show("OnFrame!");
    }
  }
}
