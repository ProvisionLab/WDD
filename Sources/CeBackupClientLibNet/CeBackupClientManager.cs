using CeBackupNetCommon;
using System;
using System.IO;
using System.ServiceModel;

namespace CeBackupClientLibNet
{
    public delegate void BackupEventDelegate( string SrcPath, string DstPath, int Deleted, System.IntPtr Pid );
    public delegate void CleanupEventDelegate( string SrcPath, string DstPath, int Deleted );

    [CallbackBehavior(ConcurrencyMode = ConcurrencyMode.Multiple, UseSynchronizationContext = false)]
    public class CeBackupClientManager : IBackupNotifications
    {
        private DuplexChannelFactory<IWCFCeBackupService> _backupChannelFactory;
        private IWCFCeBackupService _service;

        public event BackupEventDelegate BackupEvent;
        public event CleanupEventDelegate CleanupEvent;

        public CeBackupClientManager()
        {
            Logger.SetPath( Path.GetTempPath() + "CeBackupClientLibNet.log" );
            Logger.Info( "Staring ..." );
            _backupChannelFactory = CeChannelFactory.Instance.CreateBackupFactory( ServiceSettings.Instance.LibrayUrl, this );
            InitializeChannel();
        }

        private void InitializeChannel()
        {
            try
            {
                Logger.Info( "ChannelFactory.InitializeChannel: Initializing service channel..." );

                _service = _backupChannelFactory.CreateChannel();

                if( _service == null )
                    throw new Exception( "Failed to connect to backup service" );

                if( ! _service.IsCompatible( CompatibilityVersion.Version ) )
                    throw new CeBackupClientException( CLIENT_ERROR.ERROR_VERSIONMISMATCH );

                Logger.Info("ChannelFactory.InitializeChannel: Initializing succeeded!");
            }
            catch( Exception ex )
            {
                Logger.Error("ServiceHandler.InitializeChannel: Unexpected exception caught.", ex);
            }
        }

        public void OnBackup( string SrcPath, string DstPath, int Deleted, System.IntPtr Pid )
        {
            try
            {
                BackupEvent?.Invoke( SrcPath, DstPath, Deleted, Pid );
            }
            catch( Exception ex )
            {
                throw ErrorHandling.Proceed( ex );
            }
        }

        public void OnCleanup( string SrcPath, string DstPath, int Deleted )
        {
            try
            {
                CleanupEvent?.Invoke( SrcPath, DstPath, Deleted );
            }
            catch( Exception ex )
            {
                throw ErrorHandling.Proceed( ex );
            }
        }

        public void SubscribeForEvents()
        {
            try
            {
                _service.SubscribeForEvents();
            }
            catch( Exception ex )
            {
                throw ErrorHandling.Proceed( ex );
            }
        }

        public void UnsubscribeFromEvents()
        {
            try
            {
                _service.UnsubscribeFromEvents();
            }
            catch( Exception ex )
            {
                throw ErrorHandling.Proceed( ex );
            }
        }
    }
}
