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

using System.Windows.Forms;

namespace Statoscope
{
	public class ZRDICheckboxTreeView : EditableRDICheckboxTreeView<ZoneHighlighterRDI>
	{
		public ZRDICheckboxTreeView(LogControl logControl, ZoneHighlighterRDI zrdiTree)
			: base(logControl, zrdiTree)
		{
		}

		public override void AddRDIToTree(ZoneHighlighterRDI zrdi)
		{
			base.AddRDIToTree(zrdi);

			if (!zrdi.CanHaveChildren)
				LogControl.AddZoneHighlighter(zrdi);
		}

		public override void RemoveRDIFromTree(ZoneHighlighterRDI zrdi)
		{
			base.RemoveRDIFromTree(zrdi);

			if (!zrdi.CanHaveChildren)
				LogControl.RemoveZoneHighlighter(zrdi);
		}

		public override void RenameNode(TreeNode node, string newName)
		{
			ZoneHighlighterRDI zrdi = (ZoneHighlighterRDI)node.Tag;
			zrdi.NameFromPath = false;

			base.RenameNode(node, newName);
		}
	}
}
