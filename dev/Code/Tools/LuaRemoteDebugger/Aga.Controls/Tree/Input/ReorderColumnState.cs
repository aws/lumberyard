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
using System.Windows.Forms;

namespace Aga.Controls.Tree
{
	internal class ReorderColumnState : ColumnState
	{
		#region Properties

		private Point _location;
		public Point Location
		{
			get { return _location; }
		}

		private Bitmap _ghostImage;
		public Bitmap GhostImage
		{
			get { return _ghostImage; }
		}

		private TreeColumn _dropColumn;
		public TreeColumn DropColumn
		{
			get { return _dropColumn; }
		}

		private int _dragOffset;
		public int DragOffset
		{
			get { return _dragOffset; }
		}

		#endregion

		public ReorderColumnState(TreeViewAdv tree, TreeColumn column, Point initialMouseLocation)
			: base(tree, column)
		{
			_location = new Point(initialMouseLocation.X + Tree.OffsetX, 0);
			_dragOffset = tree.GetColumnX(column) - initialMouseLocation.X;
			_ghostImage = column.CreateGhostImage(new Rectangle(0, 0, column.Width, tree.ColumnHeaderHeight), tree.Font);
		}

		public override void KeyDown(KeyEventArgs args)
		{
			args.Handled = true;
			if (args.KeyCode == Keys.Escape)
				FinishResize();
		}

		public override void MouseDown(TreeNodeAdvMouseEventArgs args)
		{
		}

		public override void MouseUp(TreeNodeAdvMouseEventArgs args)
		{
			FinishResize();
		}

		public override bool MouseMove(MouseEventArgs args)
		{
			_dropColumn = null;
			_location = new Point(args.X + Tree.OffsetX, 0);
			int x = 0;
			foreach (TreeColumn c in Tree.Columns)
			{
				if (c.IsVisible)
				{
					if (_location.X < x + c.Width / 2)
					{
						_dropColumn = c;
						break;
					}
					x += c.Width;
				}
			}
			Tree.UpdateHeaders();
			return true;
		}

		private void FinishResize()
		{
			Tree.ChangeInput();
			if (Column == DropColumn)
				Tree.UpdateView();
			else
			{
				Tree.Columns.Remove(Column);
				if (DropColumn == null)
					Tree.Columns.Add(Column);
				else
					Tree.Columns.Insert(Tree.Columns.IndexOf(DropColumn), Column);

				Tree.OnColumnReordered(Column);
			}
		}
	}
}