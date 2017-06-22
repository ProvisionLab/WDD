using CeBackupNetCommon;
using CeServiceLibNet;
using System.IO;
using System.ServiceModel;
using System.ServiceProcess;

namespace CeServiceNet
{
    public partial class CeBackupWindowsService : ServiceBase
    {
        private static ServiceHost _serviceHost;
        private bool _Started = false;

        public CeBackupWindowsService()
        {
            InitializeComponent();
        }

        protected override void OnStart( string[] args )
        {
            Logger.SetPath( Path.GetTempPath() + "CeServiceNet.log" );
            Logger.Info( string.Format("CeServiceNet.Starting ...") );

            try
            {
                Logger.Info( string.Format("Starting CeService...") );
                _serviceHost = CeService.Start();
                _Started = true;
            }
            catch
            {
                Stop();
            }
        }

        protected override void OnStop()
        {
            if( _serviceHost != null )
            {
                Logger.Info( string.Format("Stopping ServiceHost...") );
                _serviceHost.Close();
                _serviceHost = null;
            }

            try
            {
                Logger.Info( string.Format("Stopping CeService...") );
                if( _Started )
                {
                    CeService.Stop();
                    _Started = false;
                }
            }
            catch { }
        }
    }
}
