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
using Aga.Controls.Tree.NodeControls;

namespace Aga.Controls.Tree
{
	public struct DrawContext
	{
		private Graphics _graphics;
		public Graphics Graphics
		{
			get { return _graphics; }
			set { _graphics = value; }
		}

		private Rectangle _bounds;
		public Rectangle Bounds
		{
			get { return _bounds; }
			set { _bounds = value; }
		}

		private Font _font;
		public Font Font
		{
			get { return _font; }
			set { _font = value; }
		}

		private DrawSelectionMode _drawSelection;
		public DrawSelectionMode DrawSelection
		{
			get { return _drawSelection; }
			set { _drawSelection = value; }
		}

		private bool _drawFocus;
		public bool DrawFocus
		{
			get { return _drawFocus; }
			set { _drawFocus = value; }
		}

		private NodeControl _currentEditorOwner;
		public NodeControl CurrentEditorOwner
		{
			get { return _currentEditorOwner; }
			set { _currentEditorOwner = value; }
		}

		private bool _enabled;
		public bool Enabled
		{
			get { return _enabled; }
			set { _enabled = value; }
		}
	}
}
