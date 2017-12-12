namespace AssetTaggingAdminTool
{
	partial class AssetTaggingAdminTool
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing && (components != null))
			{
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.components = new System.ComponentModel.Container();
			this.categoryListBox = new System.Windows.Forms.ListBox();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.moveDownButton = new System.Windows.Forms.Button();
			this.moveUpButton = new System.Windows.Forms.Button();
			this.removeCategoryButton = new System.Windows.Forms.Button();
			this.addCategoryButton = new System.Windows.Forms.Button();
			this.groupBox2 = new System.Windows.Forms.GroupBox();
			this.tagListBox = new System.Windows.Forms.ListBox();
			this.removeTagButton = new System.Windows.Forms.Button();
			this.addTagButton = new System.Windows.Forms.Button();
			this.projectSelectionComboBox = new System.Windows.Forms.ComboBox();
			this.label1 = new System.Windows.Forms.Label();
			this.addProjectButton = new System.Windows.Forms.Button();
			this.updateTimer = new System.Windows.Forms.Timer(this.components);
			this.groupBox1.SuspendLayout();
			this.groupBox2.SuspendLayout();
			this.SuspendLayout();
			// 
			// categoryListBox
			// 
			this.categoryListBox.Dock = System.Windows.Forms.DockStyle.Top;
			this.categoryListBox.FormattingEnabled = true;
			this.categoryListBox.Location = new System.Drawing.Point(3, 16);
			this.categoryListBox.Name = "categoryListBox";
			this.categoryListBox.ScrollAlwaysVisible = true;
			this.categoryListBox.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended;
			this.categoryListBox.Size = new System.Drawing.Size(135, 251);
			this.categoryListBox.TabIndex = 0;
			this.categoryListBox.SelectedIndexChanged += new System.EventHandler(this.categoryListBox_SelectedIndexChanged);
			// 
			// groupBox1
			// 
			this.groupBox1.Controls.Add(this.moveDownButton);
			this.groupBox1.Controls.Add(this.moveUpButton);
			this.groupBox1.Controls.Add(this.removeCategoryButton);
			this.groupBox1.Controls.Add(this.addCategoryButton);
			this.groupBox1.Controls.Add(this.categoryListBox);
			this.groupBox1.Location = new System.Drawing.Point(12, 36);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(141, 301);
			this.groupBox1.TabIndex = 1;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Categories";
			// 
			// moveDownButton
			// 
			this.moveDownButton.Location = new System.Drawing.Point(81, 273);
			this.moveDownButton.Name = "moveDownButton";
			this.moveDownButton.Size = new System.Drawing.Size(20, 21);
			this.moveDownButton.TabIndex = 6;
			this.moveDownButton.Text = ">";
			this.moveDownButton.UseVisualStyleBackColor = true;
			this.moveDownButton.Click += new System.EventHandler(this.moveDownButton_Click);
			// 
			// moveUpButton
			// 
			this.moveUpButton.Location = new System.Drawing.Point(55, 273);
			this.moveUpButton.Name = "moveUpButton";
			this.moveUpButton.Size = new System.Drawing.Size(20, 21);
			this.moveUpButton.TabIndex = 5;
			this.moveUpButton.Text = "<";
			this.moveUpButton.UseVisualStyleBackColor = true;
			this.moveUpButton.Click += new System.EventHandler(this.moveUpButton_Click);
			// 
			// removeCategoryButton
			// 
			this.removeCategoryButton.Location = new System.Drawing.Point(29, 273);
			this.removeCategoryButton.Name = "removeCategoryButton";
			this.removeCategoryButton.Size = new System.Drawing.Size(20, 21);
			this.removeCategoryButton.TabIndex = 4;
			this.removeCategoryButton.Text = "-";
			this.removeCategoryButton.UseVisualStyleBackColor = true;
			this.removeCategoryButton.Click += new System.EventHandler(this.removeCategoryButton_Click);
			// 
			// addCategoryButton
			// 
			this.addCategoryButton.Location = new System.Drawing.Point(3, 273);
			this.addCategoryButton.Name = "addCategoryButton";
			this.addCategoryButton.Size = new System.Drawing.Size(20, 21);
			this.addCategoryButton.TabIndex = 3;
			this.addCategoryButton.Text = "+";
			this.addCategoryButton.UseVisualStyleBackColor = true;
			this.addCategoryButton.Click += new System.EventHandler(this.addCategoryButton_Click);
			// 
			// groupBox2
			// 
			this.groupBox2.Controls.Add(this.tagListBox);
			this.groupBox2.Controls.Add(this.removeTagButton);
			this.groupBox2.Controls.Add(this.addTagButton);
			this.groupBox2.Location = new System.Drawing.Point(159, 36);
			this.groupBox2.Name = "groupBox2";
			this.groupBox2.Size = new System.Drawing.Size(141, 301);
			this.groupBox2.TabIndex = 2;
			this.groupBox2.TabStop = false;
			this.groupBox2.Text = "Tags";
			// 
			// tagListBox
			// 
			this.tagListBox.Dock = System.Windows.Forms.DockStyle.Top;
			this.tagListBox.FormattingEnabled = true;
			this.tagListBox.Location = new System.Drawing.Point(3, 16);
			this.tagListBox.Name = "tagListBox";
			this.tagListBox.ScrollAlwaysVisible = true;
			this.tagListBox.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended;
			this.tagListBox.Size = new System.Drawing.Size(135, 251);
			this.tagListBox.TabIndex = 0;
			// 
			// removeTagButton
			// 
			this.removeTagButton.Location = new System.Drawing.Point(29, 273);
			this.removeTagButton.Name = "removeTagButton";
			this.removeTagButton.Size = new System.Drawing.Size(20, 21);
			this.removeTagButton.TabIndex = 7;
			this.removeTagButton.Text = "-";
			this.removeTagButton.UseVisualStyleBackColor = true;
			this.removeTagButton.Click += new System.EventHandler(this.removeTagButton_Click);
			// 
			// addTagButton
			// 
			this.addTagButton.Location = new System.Drawing.Point(3, 273);
			this.addTagButton.Name = "addTagButton";
			this.addTagButton.Size = new System.Drawing.Size(20, 21);
			this.addTagButton.TabIndex = 6;
			this.addTagButton.Text = "+";
			this.addTagButton.UseVisualStyleBackColor = true;
			this.addTagButton.Click += new System.EventHandler(this.addTagButton_Click);
			// 
			// projectSelectionComboBox
			// 
			this.projectSelectionComboBox.FormattingEnabled = true;
			this.projectSelectionComboBox.Location = new System.Drawing.Point(102, 9);
			this.projectSelectionComboBox.Name = "projectSelectionComboBox";
			this.projectSelectionComboBox.Size = new System.Drawing.Size(169, 21);
			this.projectSelectionComboBox.TabIndex = 3;
			this.projectSelectionComboBox.SelectedIndexChanged += new System.EventHandler(this.projectSelectionComboBox_SelectedIndexChanged);
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Location = new System.Drawing.Point(12, 12);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(84, 13);
			this.label1.TabIndex = 4;
			this.label1.Text = "Select a project:";
			// 
			// addProjectButton
			// 
			this.addProjectButton.Location = new System.Drawing.Point(277, 9);
			this.addProjectButton.Name = "addProjectButton";
			this.addProjectButton.Size = new System.Drawing.Size(20, 21);
			this.addProjectButton.TabIndex = 8;
			this.addProjectButton.Text = "+";
			this.addProjectButton.UseVisualStyleBackColor = true;
			this.addProjectButton.Click += new System.EventHandler(this.addProjectButton_Click);
			// 
			// updateTimer
			// 
			this.updateTimer.Interval = 500;
			this.updateTimer.Tick += new System.EventHandler(this.updateTimer_Tick);
			// 
			// AssetTaggingAdminTool
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(312, 342);
			this.Controls.Add(this.label1);
			this.Controls.Add(this.addProjectButton);
			this.Controls.Add(this.projectSelectionComboBox);
			this.Controls.Add(this.groupBox2);
			this.Controls.Add(this.groupBox1);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.Name = "AssetTaggingAdminTool";
			this.Text = "Asset Tagging Admin Tool";
			this.groupBox1.ResumeLayout(false);
			this.groupBox2.ResumeLayout(false);
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.ListBox categoryListBox;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.Button removeCategoryButton;
		private System.Windows.Forms.Button addCategoryButton;
		private System.Windows.Forms.GroupBox groupBox2;
		private System.Windows.Forms.ListBox tagListBox;
		private System.Windows.Forms.Button removeTagButton;
		private System.Windows.Forms.Button addTagButton;
		private System.Windows.Forms.Button moveDownButton;
		private System.Windows.Forms.Button moveUpButton;
		private System.Windows.Forms.ComboBox projectSelectionComboBox;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.Button addProjectButton;
		private System.Windows.Forms.Timer updateTimer;
	}
}

