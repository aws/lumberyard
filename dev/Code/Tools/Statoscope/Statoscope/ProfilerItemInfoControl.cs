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
	partial class ProfilerItemInfoControl : DesignableBasePIIC
	{
		public ProfilerItemInfoControl(LogControl logControl, ProfilerRDI prdiTree, RDICheckboxTreeView<ProfilerRDI> prdiCTV)
			: base(logControl, prdiTree, prdiCTV)
		{
			InitializeComponent();
			ItemPathLabel = itemPathLabel;
		}

		protected override ProfilerRDI RDI
		{
			set
			{
				base.RDI = value;

				itemPathLabel.Text = RDI.Path;
				ItemPathLabelToolTip.SetToolTip(itemPathLabel, RDI.Path);

				if (LogControl.m_logViews.Count == 1)
				{
					ItemMetaData itemMetaData = LogControl.m_logViews[0].m_baseLogData.ItemMetaData[RDI.LeafPath];

					itemColourGroupBox.Enabled = (RDI.IsLeaf || RDI.IsCollapsed);
					itemColourButton.Show();
					itemColourButton.BackColor = RDI.Colour.ConvertToSysDrawCol();
					itemColourRandomButton.Show();

					itemDescriptionGroupBox.Enabled = true;
					itemDescriptionLabel.Text = "Value: " + RDI.LeafName + Environment.NewLine;
					itemDescriptionLabel.Text += "Type: " + (itemMetaData.m_type == EItemType.Float ? "Float" : "Int");

					itemStatsGroupBox.Hide();
					itemStatsGroupBox.Controls.Clear();
					itemStatsGroupBox.Controls.Add(new StatsListView(LogControl.m_logViews, RDI.LeafPath));
					itemStatsGroupBox.Enabled = true;
					itemStatsGroupBox.Show();
				}
				else
				{
					itemColourGroupBox.Hide();
					itemDescriptionGroupBox.Hide();
					itemStatsGroupBox.Hide();
				}
			}
		}

		protected override void SetItemColour(RGB colour)
		{
			base.SetItemColour(colour);
			itemColourButton.BackColor = colour.ConvertToSysDrawCol();
		}

		public override void OnLogChanged()
		{
			itemStatsGroupBox.Invalidate(true);
		}

		private void itemColourButton_Click(object sender, EventArgs e)
		{
			ItemColourButton_Click();
		}

		private void itemColourRandomButton_Click(object sender, EventArgs e)
		{
			ItemColourRandomButton_Click();
		}
	}

	class DesignableBasePIIC : RDIInfoControl<ProfilerRDI>
	{
		public DesignableBasePIIC()
			: this(null, null, null)
		{
		}

		public DesignableBasePIIC(LogControl logControl, ProfilerRDI prdiTree, RDICheckboxTreeView<ProfilerRDI> prdiCTV)
			: base(logControl, prdiTree, prdiCTV)
		{
		}
	}
}
