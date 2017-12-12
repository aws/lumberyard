/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace Statoscope
{
	// checkbox treeview control
	partial class CTVControl : UserControl
	{
		public CTVControl(CheckboxTreeView ctv)
			: this(ctv, new CTVControlPanel(ctv))
		{
		}

		public CTVControl(CheckboxTreeView ctv, Control controlPanel)
		{
			InitializeComponent();
			Dock = DockStyle.Fill;
			tableLayoutPanel.Controls.Add(controlPanel);
			tableLayoutPanel.Controls.Add(ctv, 0, 1);
		}
	}
}
