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
	partial class ZoneHighlighterCTVControlPanel : UserControl
	{
		ZRDICheckboxTreeView ZCTV { get { return (ZRDICheckboxTreeView)ctvControlPanel.CTV; } }

		public ZoneHighlighterCTVControlPanel(ZRDICheckboxTreeView ctv)
		{
			InitializeComponent();

			ctvControlPanel.CTV = ctv;
			ZCTV.AfterSelect += new TreeViewEventHandler(TCTV_AfterSelect);
			addZoneHighlighterButton.Enabled = false;
			addNodeButton.Enabled = false;
		}

		void TCTV_AfterSelect(object sender, TreeViewEventArgs e)
		{
			ZoneHighlighterRDI selectedZRDI = ZCTV.SelectedRDINode;
			addZoneHighlighterButton.Enabled = selectedZRDI.CanHaveChildren;
			addNodeButton.Enabled = selectedZRDI.CanHaveChildren;
		}

		private void addZoneHighlighterButton_Click(object sender, EventArgs e)
		{
			ZoneHighlighterRDI selectedZRDI = ZCTV.SelectedRDINode;
			string basePath = selectedZRDI.Path;
			string name = selectedZRDI.GetNewChildName("newZoneHighlighter");
			ZoneHighlighterRDI zrdi = new StdThresholdZRDI(basePath, name);
			ZCTV.AddRDIToTree(zrdi);
			ZCTV.SelectedRDINode = zrdi;
		}

		private void addNodeButton_Click(object sender, EventArgs e)
		{
			ZCTV.AddChildNodeToSelectedNode();
		}

		private void deleteButton_Click(object sender, EventArgs e)
		{
			ZCTV.RemoveSelectedNode();
		}
	}
}
