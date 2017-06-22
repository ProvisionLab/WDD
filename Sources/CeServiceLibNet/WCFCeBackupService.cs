using System.ServiceModel;
using CeBackupNetCommon;
using CeBackupServerLibNet;
using System;

namespace CeServiceLibNet
{
    [ServiceBehavior(InstanceContextMode = InstanceContextMode.Single, ConcurrencyMode = ConcurrencyMode.Multiple, UseSynchronizationContext = false)]
    [ErrorBehavior(typeof(ServiceErrorHandler))]
    public class WCFCeBackupService : IWCFCeBackupService, IWCFCeRestoreService
    {
        static uint ServerVersion = 1;
        private event BackupEventDelegate OnBackup;
        private event CleanupEventDelegate OnCleanup;

        public WCFCeBackupService()
        {
            Logger.Info( string.Format("WCFCeBackupService()") );
        }

        ~WCFCeBackupService()
        {
        }

        public uint GetVersion()
        {
            return ServerVersion;
        }

        public bool IsCompatible( uint ClientVersion )
        {
            return ServerVersion >= ClientVersion;
        }
        
        public void SubscribeForEvents()
        {
            Logger.Info( string.Format("WCFCeBackupService::SubscribeForEvents()") );
            CeBackupServerManager.Instance.BackupEvent += CeBackupServerManager_OnBackup;
            CeBackupServerManager.Instance.CleanupEvent += CeBackupServerManager_OnCleanup;

            IBackupNotifications callbacks = OperationContext.Current.GetCallbackChannel<IBackupNotifications>();
            OnBackup += callbacks.OnBackup;
            OnCleanup += callbacks.OnCleanup;

            ICommunicationObject obj = (ICommunicationObject)callbacks;

            obj.Closed += ObjClosed;
        }
        
        public void UnsubscribeFromEvents()
        {
            Logger.Info( string.Format("WCFCeBackupService::UnsubscribeFromEvents()") );
            CeBackupServerManager.Instance.BackupEvent -= CeBackupServerManager_OnBackup;
            CeBackupServerManager.Instance.CleanupEvent -= CeBackupServerManager_OnCleanup;

            IBackupNotifications callbacks = OperationContext.Current.GetCallbackChannel<IBackupNotifications>();
            if( callbacks != null )
            {
                OnBackup -= callbacks.OnBackup;
                OnCleanup -= callbacks.OnCleanup;
            }
        }
        
        public void ReloadConfig( string IniPath )
        {
            Logger.Info( string.Format("WCFCeBackupService::ReloadConfig()") );
            CeBackupServerManager.Instance.ReloadConfig( IniPath );
        }
        
        public bool IsDriverStarted()
        {
            Logger.Info( string.Format("WCFCeBackupService::IsDriverStarted()") );
            return CeBackupServerManager.Instance.IsDriverStarted();
        }
        
        public bool IsBackupStarted()
        {
            Logger.Info( string.Format("WCFCeBackupService::IsBackupStarted()") );
            return CeBackupServerManager.Instance.IsBackupStarted();
        }

        void CeBackupServerManager_OnBackup( string SrcPath, string DstPath, int Deleted, System.IntPtr Pid )
        {
            Logger.Info( string.Format("WCFCeBackupService::OnBackup() {0} - {1} Deleted={2} Pid={3}", SrcPath, DstPath, Deleted, Pid) );

            OnBackup?.Invoke( SrcPath, DstPath, Deleted, Pid );
        }
        
        void CeBackupServerManager_OnCleanup( string SrcPath, string DstPath, int Deleted )
        {
            Logger.Info( string.Format("WCFCeBackupService::OnCleanup() {0} - {1} Deleted={2}", SrcPath, DstPath, Deleted) );

            OnCleanup?.Invoke( SrcPath, DstPath, Deleted );
        }

        private void ObjClosed(object sender, EventArgs e)
        {
            if( sender != null )
            {
                OnBackup -= ((IBackupNotifications)sender).OnBackup;
                OnCleanup -= ((IBackupNotifications)sender).OnCleanup;
            }
        }

        public string[] Restore_ListAll()
        {
            Logger.Info( string.Format("WCFCeRestoreService::Restore_ListAll()") );

            return CeRestoreServerManager.Instance.Restore_ListAll();
        }

        public void Restore_Restore( string BackupPath )
        {
            Logger.Info( string.Format("WCFCeRestoreService::Restore_Restore()") );

            CeRestoreServerManager.Instance.Restore( BackupPath );
        }

        public void Restore_RestoreTo( string BackupPath, string ToDirectory )
        {
            Logger.Info( string.Format("WCFCeRestoreService::Restore_Restore()") );

            CeRestoreServerManager.Instance.RestoreTo( BackupPath, ToDirectory );
        }
    }
}
