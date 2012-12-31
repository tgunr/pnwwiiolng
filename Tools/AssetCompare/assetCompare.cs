using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace AssetCompare
{
    public partial class assetCompare : Form
    {
        public class ProgressState : Object
        {
            public string filename;
            public int maximum = 0;
            public int current = -1;
            public int pctg = -1;
            public int maxLen = 0;
            public int maxCount = 0;
        } ;
        public class LengthsDict : Dictionary<int, List<string>> { }
        public class NamesDict : Dictionary<string, List<string>> { }

        int fileCount = 0;
        int granikFaults = 0;
        bool scanning = false;
        string blameUser = "Granik";
        string scanPath;
        List<FileInfo> fileInfo;
        LengthsDict lengths;
        NamesDict namelists;
        string usingMask;

        public assetCompare()
        {
            InitializeComponent();
        }

        private void chooseDirectory_Click(object sender, EventArgs e)
        {
            progressLabel.Text = "";

            // Display directory select dialog.
            if (assetFolder.ShowDialog() != System.Windows.Forms.DialogResult.OK)
                return;

            // Copy the path to a visible field the user can see.
            string path = assetFolder.SelectedPath;
            assetPath.Text = path;

			updateFileCount();
        }

        private void _recurseDirectory(DirectoryInfo dir)
        {
            if ( dir.Name == ".svn" )
                return ;

            DirectoryInfo[] dirInfo = dir.GetDirectories();

            foreach (var subdir in dirInfo)
            {
                _recurseDirectory(subdir);
            }

            FileInfo[] files = dir.GetFiles(usingMask);
            foreach (var file in files)
            {
                if (file.Name.EndsWith(".sdm") == false)
                    fileInfo.Add(file);
            }
        }

		private void updateFileCount()
		{
			// Verify its something we can access.
			DirectoryInfo dir = new DirectoryInfo(assetPath.Text);
			if (!dir.Exists)
			{
				MessageBox.Show("Directory access error.", "Directory error", MessageBoxButtons.OK, MessageBoxIcon.Error);
				return;
			}

            fileInfo = new List<FileInfo>();

			usingMask = (fileMask.Text.Length > 0 ? fileMask.Text : "*.*");
            _recurseDirectory(dir);

			fileCount = fileInfo.Count;

			progressLabel.Text = "Found " + fileCount + " files.";

			progressBar.Maximum = 100;

			startstopBtn.Text = "Start";
			startstopBtn.Enabled = (fileCount > 0);
			scanning = false;
			matchList.Clear();
		}

        private void startstopBtn_Click(object sender, EventArgs e)
        {
            if (scanning == false)
            {
                progressBar.Visible = true;
                scanPath = assetPath.Text;
                matchList.Clear();
                scanning = true;
                scanWorker.RunWorkerAsync();
                startstopBtn.Text = "Stop";
            }
            else
            {
				scanning = false;
				startstopBtn.Text = "Start";
				progressBar.Visible = false;
				startstopBtn.Enabled = false;
				scanWorker.CancelAsync();
            }
        }

        // Iain M. Banks reference. If you don't get it, you're laaaame.
        // Considers a file name in terms of the various scans available.
        private void _considerFilebas(ProgressState status, FileInfo info)
        {
            if (matchContent.Checked == true)
            {
                _considerFileForContentMatching(status, info);
            }

            if (matchNames.Checked == true)
            {
                _considerFileForNameMatching(status, info);
            }
        }

        private void _considerFileForContentMatching(ProgressState status, FileInfo info)
        {
            int length = (int)info.Length;
            if (length < 16)
                return;
            if (length > status.maxLen)
                status.maxLen = length;
            if (lengths.ContainsKey(length) == false)
                lengths[length] = new List<string>();
            lengths[length].Add(status.filename);
            if (lengths[length].Count > status.maxCount)
                status.maxCount = lengths[length].Count;
        }

        private void _considerFileForNameMatching(ProgressState status, FileInfo info)
        {
            string name = info.Name;
            if (namelists.ContainsKey(name) == false)
                namelists[name] = new List<string>();
            namelists[name].Add(info.DirectoryName);
        }

        // Mechanism for scanWorker to add a string.
        private void swAddString(string text)
        {
            scanWorker.ReportProgress(1, text);
        }

        private void scanWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            swAddString(String.Format("Results for folder \"{0}\":\n", scanPath));

            lengths = new LengthsDict();
            namelists = new NamesDict();

            ProgressState status = new ProgressState();
            status.maximum = fileCount;

            try
            {
                while ( status.current < status.maximum - 1 )
                {
                    if (scanWorker.CancellationPending)
                        break;
                    workerStep(status, fileInfo[status.current + 1].FullName);
                    _considerFilebas(status, fileInfo[status.current]);
                }
            }
            catch (Exception)
            {
                throw;
            }

            if (scanWorker.CancellationPending)
                return;

            if (matchContent.Checked == true)
            {
                swAddString("\n========== Content Matches ==========\n");
                _reportContentMatches(status.maxLen, status.maxCount);
            }

            if (matchNames.Checked == true)
            {
                swAddString("\n========== File name Matches ==========\n");
                _reportFileNameMatches();
            }
        }

        private void _reportFileNameMatches()
        {
            ProgressState status = new ProgressState();
            status.filename = "Comparing Filenames";
            status.maximum = namelists.Count;

            foreach (KeyValuePair<string, List<string>> pair in namelists)
            {
                if (scanWorker.CancellationPending)
                    return;

                workerStep(status, pair.Value[0]);

                if (pair.Value.Count <= 1)
                    continue;
                swAddString(String.Format("'{0}' occurs in:\n", pair.Key));
                for (int i = 0; i < pair.Value.Count; ++i)
                {
                    swAddString(String.Format("  {0}\n", pair.Value[i]));
                }
            }
        }

        private void _reportContentMatches(int maxLen, int maxCount)
        {
            FileStream f;
            ProgressState status = new ProgressState();
            status.maximum = lengths.Count;

            byte[] buffer1 = new byte[maxLen];
            byte[] buffer2 = new byte[maxLen];
			bool matches = false;

			var sorted = from item in lengths
						 orderby item.Key descending
						 select item.Key;
            foreach (int dataLen in sorted)
            {
                if (scanWorker.CancellationPending)
                    return;

				var list = lengths[dataLen];
				
                workerStep(status, list[0]);

                int pathNameLen = assetPath.Text.Length + 1;
                int numPairs = list.Count;

                if (numPairs <= 1)
                {
                    continue;
                }

				int lastSizeInKb = -1;

                for (int b = 0; b < numPairs - 1; ++b)
                {
					if (scanWorker.CancellationPending)
						return;

					status.filename = list[b];
                    scanWorker.ReportProgress(0, status);

                    bool haveRead = false;

                    for (int i = b + 1; i < numPairs; ++i)
                    {
                        if (haveRead == false)
                        {
							f = File.OpenRead(list[b]);
                            f.Read(buffer1, 0, dataLen);
                            f.Close();
                            haveRead = true;
                        }

						f = File.OpenRead(list[i]);
                        f.Read(buffer2, 0, dataLen);
                        f.Close();

                        bool match = true;

                        for (int off = 0; off < dataLen; ++off)
                        {
                            if (buffer1[off] != buffer2[off])
                            {
                                match = false;
                                break;
                            }
                        }
                        if (match == true)
                        {
							int sizeInKb = (dataLen + 1023) / 1024;
							if (sizeInKb != lastSizeInKb)
							{
								swAddString(String.Format("Files of size {0}Kb:\n", sizeInKb)) ;
								lastSizeInKb = sizeInKb;
							}
							swAddString(String.Format("\"{0}\" matches \"{1}\"\n", list[b].Substring(pathNameLen), list[i].Substring(pathNameLen)));
							matches = true;
                        }

                    }
                }
            }
            if (matches == false)
                swAddString("\nNo matches found.\n\n");
            else
                swAddString("\n");
        }

        private void scanWorker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
			if (scanning == false) return;
            if (e.ProgressPercentage == 0)
            {
                ProgressState status = (ProgressState)e.UserState;
                if ( progressBar.Maximum != 100 || progressBar.Value != status.pctg )
                {
                    progressBar.Maximum = 100;
                    progressBar.Value = (status.pctg >= 0) ? status.pctg : 0;
                    progressBar.Update();
                }
                progressLabel.Text = status.filename;
                progressLabel.Update();
            }
            else
            {
                matchList.AppendText((string)e.UserState);
            }
        }

        private void scanWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            startstopBtn.Text = "Start";
			startstopBtn.Enabled = true;
            progressBar.Value = 0;
            progressBar.Maximum = 100;
            progressBar.Update();
            progressBar.Visible = false;
            progressLabel.Text = "Finished";
            scanning = false;
        }

        private void blameGranik_CheckedChanged(object sender, EventArgs e)
        {
            if (blameGranik.Checked == false)
            {
                ++granikFaults;
                if (granikFaults < 8 || granikFaults > 12)
                    matchList.AppendText("ERROR: Fault determined to be "+blameUser+"'s. Check box re-checked.\n");
                else if ( granikFaults <= 9 ) 
                {
                    
                    matchList.Clear();
                    matchList.AppendText("STOP THAT.\n");
                }
                else if (granikFaults == 10)
                {
                    matchList.Clear();
                    matchList.AppendText("I'm warning you...\n");
                }
                else if (granikFaults == 11)
                {
                    matchList.Clear();
                    matchList.AppendText("Don't say I didn't warn you :(\n");
                }
                else if (granikFaults == 12)
                {
                    matchList.Clear();
                    blameUser = "Toto";
                    blameGranik.Text = "Blame " + blameUser;
                }

                blameGranik.Checked = true;
            }
        }

        public bool workerStep(ProgressState status, string filename)
        {
            if (status.maximum == 0) return false;
            ++status.current;
            status.pctg = status.current * 100 / status.maximum;
            status.filename = filename;
            scanWorker.ReportProgress(0, status);
            return (status.current < status.maximum);
        }

		private void fileMask_TextChanged(object sender, EventArgs e)
		{
			if (assetPath.Text.Length > 0)
				updateFileCount();
		}
    }
}
