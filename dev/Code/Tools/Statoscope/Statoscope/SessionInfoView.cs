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

using System.Collections.Generic;
using System.Windows.Forms;

namespace Statoscope
{
	class SessionInfoListView : ListView
	{
		SessionInfo Session;
		List<ListViewItem> LviCache;

		public SessionInfoListView(SessionInfo sessionInfo)
		{
			Session = sessionInfo;

			Columns.Add(LogControl.CreateColumnHeader("", 70));
			Columns.Add(LogControl.CreateColumnHeader("", 90));

			Columns[0].TextAlign = HorizontalAlignment.Left;

			View = View.Details;
			Dock = DockStyle.Fill;
			ShowItemToolTips = true;
			VirtualMode = true;
			VirtualListSize = Session.Items.GetUpperBound(0) + 1;
			RetrieveVirtualItem += new RetrieveVirtualItemEventHandler(SessionInfoListView_RetrieveVirtualItem);
			CacheVirtualItems += new CacheVirtualItemsEventHandler(SessionInfoListView_CacheVirtualItems);
			StatoscopeForm.SetDoubleBuffered(this);

			LviCache = new List<ListViewItem>(VirtualListSize);
			UpdateLviCache();
		}

		public void UpdateLviCache()
		{
			LviCache.Clear();
			string[,] Items = Session.Items;

			for (int i = 0; i < VirtualListSize; i++)
			{
				ListViewItem lvi = new ListViewItem(Items[i, 0]);
				lvi.ToolTipText = Items[i, 1];
				lvi.SubItems.Add(Items[i, 1]);
				LviCache.Add(lvi);
			}
		}

		private void SessionInfoListView_RetrieveVirtualItem(object sender, RetrieveVirtualItemEventArgs e)
		{
			e.Item = LviCache[e.ItemIndex];
		}

		private void SessionInfoListView_CacheVirtualItems(object sender, CacheVirtualItemsEventArgs e)
		{
			UpdateLviCache();
		}
	}
}