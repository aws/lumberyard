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
	partial class TargetLineCTVControlPanel : UserControl
	{
		TRDICheckboxTreeView TCTV { get { return (TRDICheckboxTreeView)ctvControlPanel.CTV; } }

		public TargetLineCTVControlPanel(TRDICheckboxTreeView ctv)
		{
			InitializeComponent();
			ctvControlPanel.CTV = ctv;
			TCTV.AfterSelect += new TreeViewEventHandler(TCTV_AfterSelect);
			addTargetLineButton.Enabled = false;
			addNodeButton.Enabled = false;
		}

		void TCTV_AfterSelect(object sender, TreeViewEventArgs e)
		{
			TargetLineRDI selectedTRDI = TCTV.SelectedRDINode;
			addTargetLineButton.Enabled = selectedTRDI.CanHaveChildren;
			addNodeButton.Enabled = selectedTRDI.CanHaveChildren;
		}

		private void addTargetLineButton_Click(object sender, EventArgs e)
		{
			TargetLineRDI selectedTRDI = TCTV.SelectedRDINode;
			string basePath = selectedTRDI.Path;
			string name = selectedTRDI.GetNewChildName("newTargetLine");
			TargetLineRDI trdi = new TargetLineRDI(basePath, name, 0, RGB.RandomHueRGB(), TargetLineRDI.ELabelLocation.ELL_Right);
			TCTV.AddRDIToTree(trdi);
			TCTV.SelectedRDINode = trdi;
		}

		private void addNodeButton_Click(object sender, EventArgs e)
		{
			TCTV.AddChildNodeToSelectedNode();
		}

		private void deleteButton_Click(object sender, EventArgs e)
		{
			TCTV.RemoveSelectedNode();
		}
	}
}
