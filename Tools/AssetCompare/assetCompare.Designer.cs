namespace AssetCompare
{
	partial class assetCompare
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
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(assetCompare));
			this.assetFolder = new System.Windows.Forms.FolderBrowserDialog();
			this.assetPath = new System.Windows.Forms.TextBox();
			this.chooseDirectory = new System.Windows.Forms.Button();
			this.label1 = new System.Windows.Forms.Label();
			this.progressBar = new System.Windows.Forms.ProgressBar();
			this.startstopBtn = new System.Windows.Forms.Button();
			this.scanWorker = new System.ComponentModel.BackgroundWorker();
			this.matchList = new System.Windows.Forms.RichTextBox();
			this.matchContent = new System.Windows.Forms.CheckBox();
			this.progressLabel = new System.Windows.Forms.Label();
			this.matchNames = new System.Windows.Forms.CheckBox();
			this.blameGranik = new System.Windows.Forms.CheckBox();
			this.toolTip1 = new System.Windows.Forms.ToolTip();
			this.label2 = new System.Windows.Forms.Label();
			this.fileMask = new System.Windows.Forms.ComboBox();
			this.SuspendLayout();
			// 
			// assetFolder
			// 
			this.assetFolder.Description = "Locate the asset folder you wish to scan.";
			this.assetFolder.RootFolder = System.Environment.SpecialFolder.MyComputer;
			this.assetFolder.ShowNewFolderButton = false;
			// 
			// assetPath
			// 
			this.assetPath.Enabled = false;
			this.assetPath.Location = new System.Drawing.Point(86, 6);
			this.assetPath.MaxLength = 256;
			this.assetPath.Name = "assetPath";
			this.assetPath.Size = new System.Drawing.Size(510, 20);
			this.assetPath.TabIndex = 0;
			this.assetPath.TabStop = false;
			this.toolTip1.SetToolTip(this.assetPath, "Current folder to be scanned. Click the \"...\" button to the left to change.");
			// 
			// chooseDirectory
			// 
			this.chooseDirectory.Location = new System.Drawing.Point(602, 4);
			this.chooseDirectory.Name = "chooseDirectory";
			this.chooseDirectory.Size = new System.Drawing.Size(32, 23);
			this.chooseDirectory.TabIndex = 1;
			this.chooseDirectory.Text = "...";
			this.toolTip1.SetToolTip(this.chooseDirectory, "Click to select the folder to be scanned");
			this.chooseDirectory.UseVisualStyleBackColor = true;
			this.chooseDirectory.Click += new System.EventHandler(this.chooseDirectory_Click);
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Location = new System.Drawing.Point(12, 9);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(68, 13);
			this.label1.TabIndex = 0;
			this.label1.Text = "Asset Folder:";
			// 
			// progressBar
			// 
			this.progressBar.Location = new System.Drawing.Point(11, 84);
			this.progressBar.Name = "progressBar";
			this.progressBar.Size = new System.Drawing.Size(621, 17);
			this.progressBar.Step = 1;
			this.progressBar.TabIndex = 0;
			this.progressBar.Visible = false;
			// 
			// startstopBtn
			// 
			this.startstopBtn.Enabled = false;
			this.startstopBtn.Location = new System.Drawing.Point(557, 32);
			this.startstopBtn.Name = "startstopBtn";
			this.startstopBtn.Size = new System.Drawing.Size(75, 23);
			this.startstopBtn.TabIndex = 3;
			this.startstopBtn.Text = "Start";
			this.toolTip1.SetToolTip(this.startstopBtn, "Click to start or stop the scanning progress.");
			this.startstopBtn.UseVisualStyleBackColor = true;
			this.startstopBtn.Click += new System.EventHandler(this.startstopBtn_Click);
			// 
			// scanWorker
			// 
			this.scanWorker.WorkerReportsProgress = true;
			this.scanWorker.WorkerSupportsCancellation = true;
			this.scanWorker.DoWork += new System.ComponentModel.DoWorkEventHandler(this.scanWorker_DoWork);
			this.scanWorker.ProgressChanged += new System.ComponentModel.ProgressChangedEventHandler(this.scanWorker_ProgressChanged);
			this.scanWorker.RunWorkerCompleted += new System.ComponentModel.RunWorkerCompletedEventHandler(this.scanWorker_RunWorkerCompleted);
			// 
			// matchList
			// 
			this.matchList.AutoWordSelection = true;
			this.matchList.DetectUrls = false;
			this.matchList.Font = new System.Drawing.Font("Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.matchList.HideSelection = false;
			this.matchList.Location = new System.Drawing.Point(12, 107);
			this.matchList.Name = "matchList";
			this.matchList.ReadOnly = true;
			this.matchList.Size = new System.Drawing.Size(621, 302);
			this.matchList.TabIndex = 5;
			this.matchList.TabStop = false;
			this.matchList.Text = "";
			this.matchList.WordWrap = false;
			// 
			// matchContent
			// 
			this.matchContent.AutoSize = true;
			this.matchContent.Checked = true;
			this.matchContent.CheckState = System.Windows.Forms.CheckState.Checked;
			this.matchContent.Location = new System.Drawing.Point(447, 36);
			this.matchContent.Name = "matchContent";
			this.matchContent.Size = new System.Drawing.Size(96, 17);
			this.matchContent.TabIndex = 2;
			this.matchContent.Text = "Match Content";
			this.toolTip1.SetToolTip(this.matchContent, "Check to enable matching of files by content.");
			this.matchContent.UseVisualStyleBackColor = true;
			// 
			// progressLabel
			// 
			this.progressLabel.AutoEllipsis = true;
			this.progressLabel.Font = new System.Drawing.Font("Courier New", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.progressLabel.Location = new System.Drawing.Point(11, 68);
			this.progressLabel.Name = "progressLabel";
			this.progressLabel.Size = new System.Drawing.Size(621, 13);
			this.progressLabel.TabIndex = 0;
			this.progressLabel.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
			// 
			// matchNames
			// 
			this.matchNames.AutoSize = true;
			this.matchNames.Location = new System.Drawing.Point(349, 36);
			this.matchNames.Name = "matchNames";
			this.matchNames.Size = new System.Drawing.Size(92, 17);
			this.matchNames.TabIndex = 6;
			this.matchNames.Text = "Match Names";
			this.toolTip1.SetToolTip(this.matchNames, "Check to enable matching of file names across folders.");
			this.matchNames.UseVisualStyleBackColor = true;
			// 
			// blameGranik
			// 
			this.blameGranik.AutoSize = true;
			this.blameGranik.Checked = true;
			this.blameGranik.CheckState = System.Windows.Forms.CheckState.Checked;
			this.blameGranik.Location = new System.Drawing.Point(254, 36);
			this.blameGranik.Name = "blameGranik";
			this.blameGranik.Size = new System.Drawing.Size(89, 17);
			this.blameGranik.TabIndex = 7;
			this.blameGranik.Text = "Blame Granik";
			this.toolTip1.SetToolTip(this.blameGranik, "It\'s his fault!");
			this.blameGranik.UseVisualStyleBackColor = true;
			this.blameGranik.CheckedChanged += new System.EventHandler(this.blameGranik_CheckedChanged);
			// 
			// label2
			// 
			this.label2.AutoSize = true;
			this.label2.Location = new System.Drawing.Point(12, 37);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(55, 13);
			this.label2.TabIndex = 8;
			this.label2.Text = "File Mask:";
			// 
			// fileMask
			// 
			this.fileMask.FormattingEnabled = true;
			this.fileMask.Items.AddRange(new object[] {
			"*",
			"*.bmp",
			"*.dat",
			"*.dds",
			"*.dll",
			"*.flt",
			"*.gr2",
			"*.js",
			"*.lua",
			"*.pcx",
			"*.ttf",
			"*.txt",
			"*.uv",
			"*.wav",
			"*.xml"});
			this.fileMask.Location = new System.Drawing.Point(86, 34);
			this.fileMask.Name = "fileMask";
			this.fileMask.Size = new System.Drawing.Size(121, 21);
			this.fileMask.Sorted = true;
			this.fileMask.TabIndex = 2;
			this.fileMask.Text = "*";
			this.toolTip1.SetToolTip(this.fileMask, "Limit files to those matching the mask");
			this.fileMask.SelectedValueChanged += new System.EventHandler(this.fileMask_TextChanged);
			// 
			// assetCompare
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
			this.ClientSize = new System.Drawing.Size(645, 421);
			this.Controls.Add(this.fileMask);
			this.Controls.Add(this.label2);
			this.Controls.Add(this.blameGranik);
			this.Controls.Add(this.matchNames);
			this.Controls.Add(this.progressLabel);
			this.Controls.Add(this.matchContent);
			this.Controls.Add(this.matchList);
			this.Controls.Add(this.startstopBtn);
			this.Controls.Add(this.progressBar);
			this.Controls.Add(this.label1);
			this.Controls.Add(this.chooseDirectory);
			this.Controls.Add(this.assetPath);
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.MaximizeBox = false;
			this.Name = "assetCompare";
			this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
			this.Text = "AssetCompare";
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.FolderBrowserDialog assetFolder;
		private System.Windows.Forms.TextBox assetPath;
		private System.Windows.Forms.Button chooseDirectory;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.ProgressBar progressBar;
		private System.Windows.Forms.Button startstopBtn;
		private System.ComponentModel.BackgroundWorker scanWorker;
		private System.Windows.Forms.RichTextBox matchList;
		private System.Windows.Forms.CheckBox matchContent;
		private System.Windows.Forms.Label progressLabel;
		private System.Windows.Forms.CheckBox matchNames;
		private System.Windows.Forms.CheckBox blameGranik;
		private System.Windows.Forms.ToolTip toolTip1;
		private System.Windows.Forms.Label label2;
		private System.Windows.Forms.ComboBox fileMask;
	}
}

