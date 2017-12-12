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
using System.Windows.Forms;
using System.Drawing;

namespace Aga.Controls.Tree
{
	internal class ClickColumnState : ColumnState
	{
		private Point _location;

		public ClickColumnState(TreeViewAdv tree, TreeColumn column, Point location)
			: base(tree, column)
		{
			_location = location;
		}

		public override void KeyDown(KeyEventArgs args)
		{
		}

		public override void MouseDown(TreeNodeAdvMouseEventArgs args)
		{
		}

		public override bool MouseMove(MouseEventArgs args)
		{
			if (TreeViewAdv.Dist(_location, args.Location) > TreeViewAdv.ItemDragSensivity
				&& Tree.AllowColumnReorder)
			{
				Tree.Input = new ReorderColumnState(Tree, Column, args.Location);
				Tree.UpdateView();
			}
			return true;
		}

		public override void MouseUp(TreeNodeAdvMouseEventArgs args)
		{
			Tree.ChangeInput();
			Tree.UpdateView();
			Tree.OnColumnClicked(Column);
		}
	}
}
