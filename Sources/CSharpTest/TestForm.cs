using System;
using System.Windows.Forms;
using CeBackupLibNet;

namespace csharptest
{
    public partial class TestForm : Form
    {
        private CeBackupManager _BackupManager;
        private CeRestoreManager _RestoreManager;

        public TestForm()
        {
            InitializeComponent();
        }

        private void Form1_Load( object sender, EventArgs e )
        {
            CenterToScreen();
        }

        private void btnStart_Click( object sender, EventArgs e )
        {
            lstBackup.Items.Clear();
            try
            {
                DisposeBackupManager();

                _BackupManager = new CeBackupManager();

                _BackupManager.BackupEvent += BackupFunction;
                _BackupManager.CleanupEvent += CleanupFunction;

                _BackupManager.Start( "cetest.ini" );

                lblStatus.Text = "CeBackup Started OK";
            }
            catch( Exception ex )
            {
                MessageBox.Show( ex.ToString(), "Error" );
            }
        }

        private void btnStop_Click( object sender, EventArgs e )
        {
            try
            {
                DisposeBackupManager();

                lblStatus.Text = "CeBackup Stopped OK";
            }
            catch( Exception ex )
            {
                MessageBox.Show( ex.ToString(), "Error" );
            }
        }

        private void DisposeBackupManager()
        {
            if( _BackupManager != null )
            {
                _BackupManager.Stop();
                _BackupManager.Dispose();
                _BackupManager = null;
            }
        }

        private void DisposeRestoreManager()
        {
            if( _RestoreManager != null )
            {
                _RestoreManager.Dispose();
                _RestoreManager = null;
            }
        }

        private void BackupFunction( string SrcPath, string DstPath, int Deleted, System.IntPtr Pid )
        {
            this.Invoke( new MethodInvoker(delegate
            {
                lstBackup.Items.Add( SrcPath + " <-> " + DstPath + (Deleted > 0 ? " (Deleted)" : "") + " PID=" + Pid );
            }));
        }

        private void CleanupFunction( string SrcPath, string DstPath, int Deleted )
        {
            this.Invoke( new MethodInvoker(delegate
            {
                lstCleanup.Items.Add( SrcPath + " <-> " + DstPath + (Deleted > 0 ? " (Deleted)" : "") );
            }));
        }

        private void btnRestoreGetAll_Click( object sender, EventArgs e )
        {
            DisposeRestoreManager();

            _RestoreManager = new CeRestoreManager( "cetest.ini" );

            lblStatus.Text = "Restore Init OK";

            lstRestore.Items.Clear();
            string[] list = _RestoreManager.Restore_ListAll();
            foreach( string str in list )
            {
                lstRestore.Items.Add( str );
            }
            lblStatus.Text = "Restore ListAll OK";
        }

        private void btnRestore_Click( object sender, EventArgs e )
        {
            if( lstRestore.SelectedIndex == -1 )
            {
                MessageBox.Show( "Please select Path to restore" );
                return;
            }

            _RestoreManager.Restore( lstRestore.Items[lstRestore.SelectedIndex].ToString() );
            lblStatus.Text = "Restore OK";
        }

        private void btnRestoreTo_Click( object sender, EventArgs e )
        {
            if( lstRestore.SelectedIndex == -1 )
            {
                MessageBox.Show( "Please select Path to restore" );
                return;
            }

            if( txtRestoreTo.Text == "" )
            {
                MessageBox.Show( "Please set Directory to restore to" );
                return;
            }

            _RestoreManager.RestoreTo( lstRestore.Items[lstRestore.SelectedIndex].ToString(), txtRestoreTo.Text );
            lblStatus.Text = "RestoreTo OK";
        }

        private void btnBrowse_Click( object sender, EventArgs e )
        {
            FolderBrowserDialog folderBrowserDialog = new FolderBrowserDialog();
            if( folderBrowserDialog.ShowDialog() == DialogResult.OK )
            {
                txtRestoreTo.Text = folderBrowserDialog.SelectedPath ;
            }
        }
    }
}
