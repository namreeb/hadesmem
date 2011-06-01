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

namespace Hades_DotNetTest
{
  partial class Main
  {
    /// <summary>
    /// Required designer variable.
    /// </summary>
    private System.ComponentModel.IContainer components = null;

    /// <summary>
    /// Clean up any resources being used.
    /// </summary>
    /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
    protected override void Dispose(bool disposing)
    {
      if (disposing && (components != null))
      {
        components.Dispose();
      }
      base.Dispose(disposing);
    }

    #region Windows Form Designer generated code

    /// <summary>
    /// Required method for Designer support - do not modify
    /// the contents of this method with the code editor.
    /// </summary>
    private void InitializeComponent()
    {
      this.LstOutput = new System.Windows.Forms.ListBox();
      this.SuspendLayout();
      // 
      // LstOutput
      // 
      this.LstOutput.FormattingEnabled = true;
      this.LstOutput.Location = new System.Drawing.Point(12, 12);
      this.LstOutput.Name = "LstOutput";
      this.LstOutput.Size = new System.Drawing.Size(268, 251);
      this.LstOutput.TabIndex = 1;
      // 
      // Main
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(292, 273);
      this.Controls.Add(this.LstOutput);
      this.Name = "Main";
      this.Text = "Hades .NET Test";
      this.Load += new System.EventHandler(this.Main_Load);
      this.ResumeLayout(false);

    }

    #endregion

    private System.Windows.Forms.ListBox LstOutput;

  }
}

