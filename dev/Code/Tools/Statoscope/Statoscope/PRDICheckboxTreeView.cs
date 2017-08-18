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
using System.Windows.Forms;

namespace Statoscope
{
	public class PRDICheckboxTreeView : RDICheckboxTreeView<ProfilerRDI>
	{
		public PRDICheckboxTreeView(LogControl logControl, ProfilerRDI prdiTree)
			: base(logControl, prdiTree)
		{
			m_toolTip.SetToolTip(this,
				"Right-click: toggle subtree" + Environment.NewLine +
				"Ctrl-left-click: isolate subtree" + Environment.NewLine +
				"Shift-left-click: collapse subtree");
		}

		public override void AddRDIs(IEnumerable<ProfilerRDI> newToAdd)
		{
			base.AddRDIs(newToAdd);
			LogControl.AddPRDIs(newToAdd);
		}

		public override void SerializeInRDIs(ProfilerRDI[] prdis)
		{
			if (prdis == null)
				return;

			base.SerializeInRDIs(prdis);

			using (CheckStateBlock())
			{
				foreach (TreeNode node in TreeNodeHierarchy)
				{
					ProfilerRDI prdi = (ProfilerRDI)node.Tag;
					node.Checked = prdi.Displayed;
					node.ForeColor = prdi.IsCollapsed ? System.Drawing.Color.White : System.Drawing.Color.Empty;
					node.BackColor = prdi.IsCollapsed ? System.Drawing.Color.DarkGray : System.Drawing.Color.Empty;
				}
			}

			LogControl.PRDIsInvalidated();
		}

		public override void Undo()
		{
			base.Undo();
			LogControl.PRDIsInvalidated();
		}

		public override void Redo()
		{
			base.Redo();
			LogControl.PRDIsInvalidated();
		}

		public override void FilterTextChanged(string filterText)
		{
			base.FilterTextChanged(filterText);
			LogControl.InvalidateGraphControl();
		}

		protected override void OnAfterCheck(TreeViewEventArgs e)
		{
			base.OnAfterCheck(e);

			LogControl.m_spikeFinder.UpdateCheckState(e.Node.FullPath);

			if (m_refreshOnTreeNodeChecked)
				LogControl.PRDIsInvalidated();
    }

		protected override void OnNodeMouseClick(TreeNodeMouseClickEventArgs e)
		{
			base.OnNodeMouseClick(e);

			if (e.Button == MouseButtons.Left)
			{
				if (StatoscopeForm.m_shiftIsBeingPressed)
				{
					ProfilerRDI prdi = (ProfilerRDI)e.Node.Tag;
					prdi.IsCollapsed = !prdi.IsCollapsed;
					e.Node.ForeColor = prdi.IsCollapsed ? System.Drawing.Color.White : System.Drawing.Color.Empty;
					e.Node.BackColor = prdi.IsCollapsed ? System.Drawing.Color.DarkGray : System.Drawing.Color.Empty;

					LogControl.InvalidateGraphControl();
				}

				if (StatoscopeForm.m_ctrlIsBeingPressed)
				{
					LogControl.PRDIsInvalidated();
				}
			}
			else if (e.Button == MouseButtons.Right)
			{
				LogControl.PRDIsInvalidated();
			}
		}
	}
}
