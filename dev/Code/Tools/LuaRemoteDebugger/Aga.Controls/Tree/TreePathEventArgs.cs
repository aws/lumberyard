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

namespace Aga.Controls.Tree
{
	public class TreePathEventArgs : EventArgs
	{
		private TreePath _path;
		public TreePath Path
		{
			get { return _path; }
		}

		public TreePathEventArgs()
		{
			_path = new TreePath();
		}

		public TreePathEventArgs(TreePath path)
		{
			if (path == null)
				throw new ArgumentNullException();

			_path = path;
		}
	}
}
