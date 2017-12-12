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
using System.Text;
using System.Drawing;
using Aga.Controls.Properties;

namespace Aga.Controls.Tree.NodeControls
{
	public class NodeStateIcon: NodeIcon
	{
		private Image _leaf;
		private Image _opened;
		private Image _closed;

		public NodeStateIcon()
		{
			_leaf = MakeTransparent(Resources.Leaf);
			_opened = MakeTransparent(Resources.Folder);
			_closed = MakeTransparent(Resources.FolderClosed);
		}

		private static Image MakeTransparent(Bitmap bitmap)
		{
			bitmap.MakeTransparent(bitmap.GetPixel(0,0));
			return bitmap;
		}

		protected override Image GetIcon(TreeNodeAdv node)
		{
			Image icon = base.GetIcon(node);
			if (icon != null)
				return icon;
			else if (node.IsLeaf)
				return _leaf;
			else if (node.CanExpand && node.IsExpanded)
				return _opened;
			else
				return _closed;
		}
	}
}
