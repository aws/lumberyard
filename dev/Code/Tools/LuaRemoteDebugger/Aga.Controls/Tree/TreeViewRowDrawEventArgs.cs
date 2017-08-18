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
	public class TreeViewRowDrawEventArgs: PaintEventArgs
	{
		TreeNodeAdv _node;
		DrawContext _context;
		int _row;
		Rectangle _rowRect;

		public TreeViewRowDrawEventArgs(Graphics graphics, Rectangle clipRectangle, TreeNodeAdv node, DrawContext context, int row, Rectangle rowRect)
			: base(graphics, clipRectangle)
		{
			_node = node;
			_context = context;
			_row = row;
			_rowRect = rowRect;
		}

		public TreeNodeAdv Node
		{
			get { return _node; }
		}

		public DrawContext Context
		{
			get { return _context; }
		}

		public int Row
		{
			get { return _row; }
		}

		public Rectangle RowRect
		{
			get { return _rowRect; }
		}
	}

}
