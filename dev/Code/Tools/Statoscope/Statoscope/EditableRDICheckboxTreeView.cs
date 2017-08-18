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
using System.Collections;
using System.Windows.Forms;

namespace Statoscope
{
	public class EditableRDICheckboxTreeView<T> : RDICheckboxTreeView<T> where T : RecordDisplayInfo<T>, new()
	{
		public EditableRDICheckboxTreeView(LogControl logControl, T rdiTree)
			: base(logControl, rdiTree)
		{
			LabelEdit = true;
			TreeViewNodeSorter = new NaturalNodeComparer();
			Sorted = true;
		}

		public virtual void RenameNode(TreeNode node, string newName)
		{
			node.Name = newName;
			node.Text = newName;
			BeginInvoke((Action)(() => Sort()));

			T rdi = (T)node.Tag;
			rdi.Name = newName;
			rdi.DisplayName = newName;

			LogControl.RefreshItemInfoPanel();
		}

		protected override void OnKeyDown(System.Windows.Forms.KeyEventArgs e)
		{
			base.OnKeyDown(e);

			if (SelectedNode == null)
				return;

			if (e.KeyData == Keys.F2)
				SelectedNode.BeginEdit();
			else if (e.KeyData == Keys.Delete)
				RemoveSelectedNode();
		}

		protected override void OnAfterLabelEdit(NodeLabelEditEventArgs e)
		{
			base.OnAfterLabelEdit(e);

			if (e.CancelEdit)
				return;

			e.CancelEdit = true;

			if ((e.Label != null) && (e.Label.Length > 0) && !e.Label.Contains("/"))
			{
				T rdi = (T)e.Node.Tag;
				string newPath = rdi.BasePath + "/" + e.Label;

				if (!RDITree.ContainsPath(newPath))
				{
					e.CancelEdit = false;
					RenameNode(e.Node, e.Label);
				}
			}
		}

		public override void SerializeInRDIs(T[] rdis)
		{
			if (rdis == null)
				return;

			foreach (T rdi in RDITree.GetEnumerator(ERDIEnumeration.Full))
				rdi.Displayed = false;

			foreach (T rdi in rdis)
			{
				T treeRDI = RDITree[rdi.Path];

				if ((treeRDI != null) && (treeRDI.CanHaveChildren != rdi.CanHaveChildren))
				{
					TreeNode treeNode = RDITreeNodeHierarchy[treeRDI];
					RenameNode(treeNode, treeRDI.GetNewName());
					treeRDI = null;
				}

				if (treeRDI == null)
					AddRDIToTree(rdi);
				else
					CopySerializedMembers(treeRDI, rdi);
			}

			foreach (TreeNode node in TreeNodeHierarchy)
			{
				T rdi = (T)node.Tag;
				node.Checked = rdi.Displayed;
			}
		}

		class NaturalNodeComparer : IComparer
		{
			public int Compare(object x, object y)
			{
				TreeNode nodeX = x as TreeNode;
				TreeNode nodeY = y as TreeNode;
				return SafeNativeMethods.StrCmpLogicalW(nodeX.Text, nodeY.Text);
			}
		}
	}
}
