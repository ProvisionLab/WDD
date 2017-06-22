using CeBackupNetCommon;
using System;
using System.IO;
using System.ServiceModel;

namespace CeBackupClientLibNet
{
    [CallbackBehavior(ConcurrencyMode = ConcurrencyMode.Multiple, UseSynchronizationContext = false)]
    public class CeRestoreClientManager
    {
        private ChannelFactory<IWCFCeRestoreService> _restoreChannelFactory;
        private IWCFCeRestoreService _service;

        public CeRestoreClientManager( string IniPath )
        {
            Logger.SetPath( Path.GetTempPath() + "CeBackupClientLibNet.log" );
            Logger.Info( "Staring ..." );
            _restoreChannelFactory = CeChannelFactory.Instance.CreateRestoreFactory(ServiceSettings.Instance.LibrayUrl);
            InitializeChannel();
        }

        private void InitializeChannel()
        {
            try
            {
                Logger.Info( "ChannelFactory.InitializeChannel: Initializing service channel..." );

                _service = _restoreChannelFactory.CreateChannel();

                if( _service == null )
                    throw new Exception( "Failed to connect to restore service" );

                Logger.Info("ChannelFactory.InitializeChannel: Initializing succeeded!");
            }
            catch( Exception ex )
            {
                Logger.Error("ServiceHandler.InitializeChannel: Unexpected exception caught.", ex);
                throw;
            }
        }

        public string[] Restore_ListAll()
        {
            return _service.Restore_ListAll();
        }

        public void Restore( string BackupPath )
        {
            _service.Restore_Restore( BackupPath );
        }

        public void RestoreTo( string BackupPath, string DirTo )
        {
            _service.Restore_RestoreTo( BackupPath, DirTo );
        }
    }
}
