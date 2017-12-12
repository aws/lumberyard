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
using System.Windows.Forms;
using AssetTaggingImport;
using System.Runtime.InteropServices;

namespace AssetTaggingAdminTool
{
	public partial class AssetTaggingAdminTool : Form
	{
		private bool m_initialised = false;
		public AssetTaggingAdminTool()
		{
			InitializeComponent();
			AssetTagging.LoadAssetTaggingLibraries();

			if (AssetTagging.AssetTagging_Initialize(".\\"))
			{
				m_initialised = true;
				UpdateProjectsList();
			}
			updateTimer.Enabled = true;
		}

		~AssetTaggingAdminTool()
		{
			AssetTagging.FreeAssetTaggingLibraries();
		}

		private void AddNewCategory(String itemname)
		{
			if (m_initialised)
				AssetTagging.AssetTagging_CreateCategory(itemname, (String)projectSelectionComboBox.SelectedItem);
		}

		private void AddNewTag(String tag, String category)
		{
			if (m_initialised)
				AssetTagging.AssetTagging_CreateTag(tag, category, (String)projectSelectionComboBox.SelectedItem);
		}

		private void RemoveCategory(String category)
		{
			if (m_initialised)
				AssetTagging.AssetTagging_DestroyCategory(category, (String)projectSelectionComboBox.SelectedItem);
		}

		private void RemoveTag(String tag)
		{
			if (m_initialised)
				AssetTagging.AssetTagging_DestroyTag(tag, (String)projectSelectionComboBox.SelectedItem);
		}

		private void UpdateProjectsList()
		{
			projectSelectionComboBox.Items.Clear();
			int stringLen = AssetTagging.AssetTagging_MaxStringLen();

			int projectsCount = AssetTagging.AssetTagging_GetNumProjects();
			IntPtr[] buffer = AssetTagging.Marshal2DToArray(projectsCount, stringLen);
			int catRet = AssetTagging.AssetTagging_GetProjects(buffer, projectsCount);
			String[] output = AssetTagging.MarshalBufferToStringArray(buffer, catRet);

			foreach (String str in output)
			{
				projectSelectionComboBox.Items.Add(str);
			}

			projectSelectionComboBox.SelectedIndex = 0;
		}

		private void UpdateCategoriesList()
		{
			categoryListBox.Items.Clear();

			int stringLen = AssetTagging.AssetTagging_MaxStringLen();
			int categoriesCount = AssetTagging.AssetTagging_GetNumCategories((String)projectSelectionComboBox.SelectedItem);
			IntPtr[] buffer = AssetTagging.Marshal2DToArray(categoriesCount, stringLen);
			int catRet = AssetTagging.AssetTagging_GetAllCategories((String)projectSelectionComboBox.SelectedItem, buffer, categoriesCount);
			String[] output = AssetTagging.MarshalBufferToStringArray(buffer, catRet);

			foreach (String str in output)
			{
				categoryListBox.Items.Add(str);
			}
		}

		private void UpdateTagList()
		{
			tagListBox.Items.Clear();

			String categoryText = (String)categoryListBox.SelectedItem;
			int stringLen = AssetTagging.AssetTagging_MaxStringLen();
			int tagCount = AssetTagging.AssetTagging_GetNumTagsForCategory(categoryText, (String)projectSelectionComboBox.SelectedItem);
			IntPtr[] buffer = AssetTagging.Marshal2DToArray(tagCount, stringLen);
			int tagRet = AssetTagging.AssetTagging_GetTagsForCategory(categoryText, (String)projectSelectionComboBox.SelectedItem, buffer, tagCount);
			String[] output = AssetTagging.MarshalBufferToStringArray(buffer, tagRet);

			foreach (String str in output)
			{
				tagListBox.Items.Add(str);
			}
		}

		private void categoryListBox_SelectedIndexChanged(object sender, System.EventArgs e)
		{
			if (!m_initialised)
				return;

			UpdateTagList();
		}

		private void addCategoryButton_Click(object sender, System.EventArgs e)
		{
			AddItem newItemDialog = new AddItem("Category");
			if (newItemDialog.ShowDialog(this) == DialogResult.OK)
			{
				String newCategoryName = newItemDialog.ItemName;

				if (m_initialised && AssetTagging.AssetTagging_CategoryExists(newCategoryName, (String)projectSelectionComboBox.SelectedItem) > 0)
				{
					MessageBox.Show("Category with that name already exists!", "Add category error", MessageBoxButtons.OK, MessageBoxIcon.Error);
					return;
				}

				AddNewCategory(newCategoryName);
				UpdateCategoriesList();
				categoryListBox.SelectedItem = newCategoryName;
			}
		}

		private void removeCategoryButton_Click(object sender, System.EventArgs e)
		{
			foreach( String category in categoryListBox.SelectedItems )
			{
				int stringLen = AssetTagging.AssetTagging_MaxStringLen();
				int tagCount = AssetTagging.AssetTagging_GetNumTagsForCategory(category, (String)projectSelectionComboBox.SelectedItem);
				IntPtr[] buffer = AssetTagging.Marshal2DToArray(tagCount, stringLen);
				int tagRet = AssetTagging.AssetTagging_GetTagsForCategory(category, (String)projectSelectionComboBox.SelectedItem, buffer, tagCount);
				String[] output = AssetTagging.MarshalBufferToStringArray(buffer, tagRet);

				foreach (String str in output)
				{
					RemoveTag(str);
				}

				RemoveCategory(category);
			}
			UpdateCategoriesList();
			UpdateTagList();
		}

		private void removeTagButton_Click(object sender, EventArgs e)
		{
			foreach (String tag in tagListBox.SelectedItems)
			{
				RemoveTag(tag);
			}
			UpdateTagList();
		}

		private void addTagButton_Click(object sender, EventArgs e)
		{
			if (categoryListBox.SelectedItem == null)
				return; 

			String categoryString = (String)categoryListBox.SelectedItem;

			AddItem newItemDialog = new AddItem("Tag");
			if (newItemDialog.ShowDialog(this) == DialogResult.OK)
			{
				String newTagName = newItemDialog.ItemName;

				if (m_initialised && AssetTagging.AssetTagging_TagExists(newTagName, categoryString, (String)projectSelectionComboBox.SelectedItem) > 0)
				{
					MessageBox.Show("Tag with that name already exists!", "Add tag error", MessageBoxButtons.OK, MessageBoxIcon.Error);
					return;
				}
				
				AddNewTag(newTagName, categoryString);
				UpdateTagList();
				tagListBox.SelectedItem = newTagName;
			}
		}

		private void moveUpButton_Click(object sender, EventArgs e)
		{
			if (categoryListBox.SelectedItem == null)
				return;
			
			if (categoryListBox.Items.Count < 2)
				return;

			int selectedIdx = categoryListBox.SelectedIndex;
			if (selectedIdx == 0)
				return;

			int newIdx = Math.Max(0, selectedIdx - 1);
			String selectedItem = (string)categoryListBox.Items[selectedIdx];
			categoryListBox.Items.RemoveAt(selectedIdx);
			categoryListBox.Items.Insert(newIdx,selectedItem);

			for (int idx = 0; idx < categoryListBox.Items.Count; ++idx )
			{
				String category = (String)categoryListBox.Items[idx];
				AssetTagging.AssetTagging_UpdateCategoryOrderId(category, idx, (String)projectSelectionComboBox.SelectedItem);
			}

			categoryListBox.SelectedIndex = newIdx;
		}

		private void moveDownButton_Click(object sender, EventArgs e)
		{
			if (categoryListBox.SelectedItem == null)
				return;

			if (categoryListBox.Items.Count < 2)
				return;

			int selectedIdx = categoryListBox.SelectedIndex;
			if (selectedIdx == categoryListBox.Items.Count - 1)
				return;

			int newIdx = Math.Min(selectedIdx+1,categoryListBox.Items.Count);
			String selectedItem = (string)categoryListBox.Items[selectedIdx];
			categoryListBox.Items.RemoveAt(selectedIdx);
			categoryListBox.Items.Insert(newIdx,selectedItem);

			for (int idx = 0; idx < categoryListBox.Items.Count; ++idx)
			{
				String category = (String)categoryListBox.Items[idx];
				AssetTagging.AssetTagging_UpdateCategoryOrderId(category, idx, (String)projectSelectionComboBox.SelectedItem);
			}

			categoryListBox.SelectedIndex = newIdx;
		}

		private void projectSelectionComboBox_SelectedIndexChanged(object sender, EventArgs e)
		{
			UpdateCategoriesList();
			tagListBox.Items.Clear();
		}

		private void addProjectButton_Click(object sender, EventArgs e)
		{
			AddItem newItemDialog = new AddItem("Project");
			if (newItemDialog.ShowDialog(this) == DialogResult.OK)
			{
				String newProjectName = newItemDialog.ItemName;

				if (m_initialised)
				{
					if (AssetTagging.AssetTagging_ProjectExists(newProjectName) > 0)
					{
						MessageBox.Show("Project with that name already exists!", "Add tag error", MessageBoxButtons.OK, MessageBoxIcon.Error);
						return;
					}

					AssetTagging.AssetTagging_CreateProject(newProjectName);
					projectSelectionComboBox.Items.Add(newProjectName);
					projectSelectionComboBox.SelectedIndex = projectSelectionComboBox.Items.IndexOf(newProjectName);
				}
			}
		}

		private void updateTimer_Tick(object sender, EventArgs e)
		{
			String item = (String)categoryListBox.SelectedItem;
			String tagitem = (String)tagListBox.SelectedItem;

			UpdateCategoriesList();

			if (item != null && categoryListBox.Items.Contains(item))
				categoryListBox.SelectedItem = item;

			if (tagitem != null && tagListBox.Items.Contains(tagitem))
				tagListBox.SelectedItem = tagitem;
		}
	}
}
