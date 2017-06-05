namespace csharptest
{
    partial class TestForm
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
            this.btnStart = new System.Windows.Forms.Button();
            this.lstBackup = new System.Windows.Forms.ListBox();
            this.btnStop = new System.Windows.Forms.Button();
            this.lstCleanup = new System.Windows.Forms.ListBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.btnRestoreGetAll = new System.Windows.Forms.Button();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.txtRestoreTo = new System.Windows.Forms.TextBox();
            this.btnRestoreTo = new System.Windows.Forms.Button();
            this.btnRestore = new System.Windows.Forms.Button();
            this.lstRestore = new System.Windows.Forms.ListBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.lblStatus = new System.Windows.Forms.Label();
            this.btnBrowse = new System.Windows.Forms.Button();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.SuspendLayout();
            // 
            // btnStart
            // 
            this.btnStart.Location = new System.Drawing.Point(4, 18);
            this.btnStart.Name = "btnStart";
            this.btnStart.Size = new System.Drawing.Size(75, 23);
            this.btnStart.TabIndex = 0;
            this.btnStart.Text = "Start";
            this.btnStart.UseVisualStyleBackColor = true;
            this.btnStart.Click += new System.EventHandler(this.btnStart_Click);
            // 
            // lstBackup
            // 
            this.lstBackup.FormattingEnabled = true;
            this.lstBackup.Location = new System.Drawing.Point(17, 81);
            this.lstBackup.Name = "lstBackup";
            this.lstBackup.Size = new System.Drawing.Size(816, 186);
            this.lstBackup.TabIndex = 1;
            // 
            // btnStop
            // 
            this.btnStop.Location = new System.Drawing.Point(102, 18);
            this.btnStop.Name = "btnStop";
            this.btnStop.Size = new System.Drawing.Size(75, 23);
            this.btnStop.TabIndex = 2;
            this.btnStop.Text = "Stop";
            this.btnStop.UseVisualStyleBackColor = true;
            this.btnStop.Click += new System.EventHandler(this.btnStop_Click);
            // 
            // lstCleanup
            // 
            this.lstCleanup.FormattingEnabled = true;
            this.lstCleanup.Location = new System.Drawing.Point(15, 290);
            this.lstCleanup.Name = "lstCleanup";
            this.lstCleanup.Size = new System.Drawing.Size(818, 147);
            this.lstCleanup.TabIndex = 3;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(1, 44);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(108, 13);
            this.label1.TabIndex = 5;
            this.label1.Text = "Backup Notifications:";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(1, 252);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(110, 13);
            this.label2.TabIndex = 6;
            this.label2.Text = "Cleanup Notifications:";
            // 
            // btnRestoreGetAll
            // 
            this.btnRestoreGetAll.Location = new System.Drawing.Point(6, 19);
            this.btnRestoreGetAll.Name = "btnRestoreGetAll";
            this.btnRestoreGetAll.Size = new System.Drawing.Size(106, 25);
            this.btnRestoreGetAll.TabIndex = 7;
            this.btnRestoreGetAll.Text = "GetAll";
            this.btnRestoreGetAll.UseVisualStyleBackColor = true;
            this.btnRestoreGetAll.Click += new System.EventHandler(this.btnRestoreGetAll_Click);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.btnBrowse);
            this.groupBox1.Controls.Add(this.txtRestoreTo);
            this.groupBox1.Controls.Add(this.btnRestoreTo);
            this.groupBox1.Controls.Add(this.btnRestore);
            this.groupBox1.Controls.Add(this.lstRestore);
            this.groupBox1.Controls.Add(this.btnRestoreGetAll);
            this.groupBox1.Location = new System.Drawing.Point(11, 459);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(829, 225);
            this.groupBox1.TabIndex = 8;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Restore:";
            // 
            // txtRestoreTo
            // 
            this.txtRestoreTo.Location = new System.Drawing.Point(390, 21);
            this.txtRestoreTo.Name = "txtRestoreTo";
            this.txtRestoreTo.Size = new System.Drawing.Size(335, 20);
            this.txtRestoreTo.TabIndex = 11;
            this.txtRestoreTo.Text = "c:\\Max\\RestoreTo\\";
            // 
            // btnRestoreTo
            // 
            this.btnRestoreTo.Location = new System.Drawing.Point(261, 19);
            this.btnRestoreTo.Name = "btnRestoreTo";
            this.btnRestoreTo.Size = new System.Drawing.Size(123, 24);
            this.btnRestoreTo.TabIndex = 10;
            this.btnRestoreTo.Text = "Restore To";
            this.btnRestoreTo.UseVisualStyleBackColor = true;
            this.btnRestoreTo.Click += new System.EventHandler(this.btnRestoreTo_Click);
            // 
            // btnRestore
            // 
            this.btnRestore.Location = new System.Drawing.Point(131, 19);
            this.btnRestore.Name = "btnRestore";
            this.btnRestore.Size = new System.Drawing.Size(111, 24);
            this.btnRestore.TabIndex = 9;
            this.btnRestore.Text = "Restore";
            this.btnRestore.UseVisualStyleBackColor = true;
            this.btnRestore.Click += new System.EventHandler(this.btnRestore_Click);
            // 
            // lstRestore
            // 
            this.lstRestore.FormattingEnabled = true;
            this.lstRestore.Location = new System.Drawing.Point(9, 54);
            this.lstRestore.Name = "lstRestore";
            this.lstRestore.Size = new System.Drawing.Size(813, 160);
            this.lstRestore.TabIndex = 8;
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.label2);
            this.groupBox2.Controls.Add(this.label1);
            this.groupBox2.Controls.Add(this.btnStop);
            this.groupBox2.Controls.Add(this.btnStart);
            this.groupBox2.Location = new System.Drawing.Point(11, 18);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(828, 431);
            this.groupBox2.TabIndex = 9;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "CeUser";
            // 
            // lblStatus
            // 
            this.lblStatus.AutoSize = true;
            this.lblStatus.Location = new System.Drawing.Point(20, 689);
            this.lblStatus.Name = "lblStatus";
            this.lblStatus.Size = new System.Drawing.Size(0, 13);
            this.lblStatus.TabIndex = 10;
            // 
            // btnBrowse
            // 
            this.btnBrowse.Location = new System.Drawing.Point(726, 20);
            this.btnBrowse.Name = "btnBrowse";
            this.btnBrowse.Size = new System.Drawing.Size(27, 22);
            this.btnBrowse.TabIndex = 12;
            this.btnBrowse.Text = "...";
            this.btnBrowse.UseVisualStyleBackColor = true;
            this.btnBrowse.Click += new System.EventHandler(this.btnBrowse_Click);
            // 
            // TestForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(857, 707);
            this.Controls.Add(this.lblStatus);
            this.Controls.Add(this.lstCleanup);
            this.Controls.Add(this.lstBackup);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox1);
            this.Name = "TestForm";
            this.Text = "CeBackup C# Test App";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnStart;
        private System.Windows.Forms.ListBox lstBackup;
        private System.Windows.Forms.Button btnStop;
        private System.Windows.Forms.ListBox lstCleanup;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Button btnRestoreGetAll;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.ListBox lstRestore;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Button btnRestoreTo;
        private System.Windows.Forms.Button btnRestore;
        private System.Windows.Forms.TextBox txtRestoreTo;
        private System.Windows.Forms.Label lblStatus;
        private System.Windows.Forms.Button btnBrowse;
    }
}

