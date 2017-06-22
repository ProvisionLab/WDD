using System;

namespace CeBackupServerLibNet
{
    public delegate void BackupEventDelegate( string SrcPath, string DstPath, int Deleted, System.IntPtr Pid );
    public delegate void CleanupEventDelegate( string SrcPath, string DstPath, int Deleted );

    public class CeBackupServerManager : IDisposable
    {
        public event BackupEventDelegate BackupEvent;
        public event CleanupEventDelegate CleanupEvent;

        private InternalCAPI.ServerBackupCallback ptrBackupEvent = null;
        private InternalCAPI.ServerCleanupCallback ptrCleanupEvent = null;

        private bool disposed = false;

        private static CeBackupServerManager _instance = null;

        public static CeBackupServerManager Instance
        {
            get
            {
                if( _instance == null )
                    _instance = new CeBackupServerManager();

                return _instance;
            }
        }

        private CeBackupServerManager()
        {
            CeBackupServerException.RaiseIfNotSucceeded( InternalCAPI.Init() );
        }

        public void Start( string IniPath )
        {
            SubscribeForEvents();
            CeBackupServerException.RaiseIfNotSucceeded( InternalCAPI.Start( IniPath ) );
        }

        public void Stop()
        {
            UnsubscribeFromEvents();
            CeBackupServerException.RaiseIfNotSucceeded( InternalCAPI.Stop() );
        }

        private void SubscribeForEvents()
        {
            ptrBackupEvent = new InternalCAPI.ServerBackupCallback( BackupFunction);
            ptrCleanupEvent = new InternalCAPI.ServerCleanupCallback( CleanupFunction );

            CeBackupServerException.RaiseIfNotSucceeded( InternalCAPI.SubscribeForBackupEvents( ptrBackupEvent ) );
            CeBackupServerException.RaiseIfNotSucceeded( InternalCAPI.SubscribeForCleanupEvents( ptrCleanupEvent ) );
        }

        private void UnsubscribeFromEvents()
        {
            CeBackupServerException.RaiseIfNotSucceeded( InternalCAPI.UnsubscribeFromBackupEvents() );
            CeBackupServerException.RaiseIfNotSucceeded( InternalCAPI.UnsubscribeFromCleanupEvents() );
        }

        private void BackupFunction( string SrcPath, string DstPath, int Deleted, System.IntPtr Pid )
        {
            try
            {
                BackupEvent?.Invoke( SrcPath, DstPath, Deleted, Pid );
            }
            catch {}
        }

        private void CleanupFunction( string SrcPath, string DstPath, int Deleted )
        {
            try
            {
                CleanupEvent?.Invoke( SrcPath, DstPath, Deleted );
            }
            catch {}
        }

        public void ReloadConfig( string IniPath )
        {
            CeBackupServerException.RaiseIfNotSucceeded( InternalCAPI.ReloadConfig( IniPath ) );
        }

        public bool IsDriverStarted()
        {
            return InternalCAPI.IsDriverStarted() > 0;
        }

        public bool IsBackupStarted()
        {
            return InternalCAPI.IsBackupStarted() > 0;
        }

        public void Dispose()
        {
            Dispose( true );
            GC.SuppressFinalize( this );
        }

        protected virtual void Dispose( bool disposing )
        {
            if( disposed )
                return; 

            UnsubscribeFromEvents();
            CeBackupServerException.RaiseIfNotSucceeded( InternalCAPI.Uninit() );

            disposed = true;
            _instance = null;
        }

        ~CeBackupServerManager()
        {
            Dispose( false );
        }
    }
}
