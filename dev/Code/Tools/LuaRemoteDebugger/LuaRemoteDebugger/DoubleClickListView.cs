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
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.ComponentModel;

namespace LuaRemoteDebuggerControls
{
	class DoubleClickListView : ListView
	{
		[Category("Action")]
		public event EventHandler<ItemDoubleClickEventArgs> ItemDoubleClicked;

		protected override void OnSelectedIndexChanged(EventArgs e)
		{
			base.OnSelectedIndexChanged(e);
			if (this.SelectedItems.Count == 1)
			{
				this.SelectedItems[0].EnsureVisible();
			}
		}

		protected override void WndProc(ref Message m)
		{
			// Filter WM_LBUTTONDBLCLK
			if (m.Msg != 0x203)
				base.WndProc(ref m);
			else
				OnDoubleClick();
		}

		private void OnDoubleClick()
		{
			if (this.SelectedItems.Count == 1)
			{
				ListViewItem item = this.SelectedItems[0];
				if (ItemDoubleClicked != null)
				{
					ItemDoubleClicked(this, new ItemDoubleClickEventArgs(item));
				}
			}
		}
	}

	public class ItemDoubleClickEventArgs : EventArgs
	{
		ListViewItem item;

		public ItemDoubleClickEventArgs(ListViewItem item) { this.item = item; }

		public ListViewItem Item { get { return item; } }
	}
}
