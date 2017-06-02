using System;

namespace CeBackupLibNet
{
    public delegate void BackupEventHandler(string SrcPath, string DstPath, int Deleted, System.IntPtr Pid);
    public delegate void CleanupEventHandler(string SrcPath, string DstPath, int Deleted);

    public class CeBackupManager : IDisposable
    {
        public event BackupEventHandler BackupEvent;
        public event CleanupEventHandler CleanupEvent;

        private InternalCAPI.CallBackBackupCallback ptrBackupEvent = null;
        private InternalCAPI.CallBackCleanupEvent ptrCleanupEvent = null;

        private bool disposed = false;

        public CeBackupManager()
        {
            Init();
        }

        private void Init()
        {
            CeBackupException.RaiseIfNotSucceeded( InternalCAPI.Init() );
        }

        public void Start( string IniPath )
        {
            SubscribeForEvents();
            CeBackupException.RaiseIfNotSucceeded( InternalCAPI.Start( IniPath ) );
        }

        public void Stop()
        {
            UnsubscribeFromEvents();
            CeBackupException.RaiseIfNotSucceeded( InternalCAPI.Stop() );
        }

        private void SubscribeForEvents()
        {
            ptrBackupEvent = new InternalCAPI.CallBackBackupCallback(BackupFunction);
            ptrCleanupEvent = new InternalCAPI.CallBackCleanupEvent(CleanupFunction);

            CeBackupException.RaiseIfNotSucceeded( InternalCAPI.SubscribeForBackupEvents( ptrBackupEvent ) );
            CeBackupException.RaiseIfNotSucceeded( InternalCAPI.SubscribeForCleanupEvents( ptrCleanupEvent ) );
        }

        private void UnsubscribeFromEvents()
        {
            CeBackupException.RaiseIfNotSucceeded( InternalCAPI.UnsubscribeFromBackupEvents() );
            CeBackupException.RaiseIfNotSucceeded( InternalCAPI.UnsubscribeFromCleanupEvents() );
        }

        private void BackupFunction( string SrcPath, string DstPath, int Deleted, System.IntPtr Pid )
        {
            try
            {
                BackupEventHandler threadSafeTempEvent = BackupEvent;
                if (threadSafeTempEvent != null)
                {
                    threadSafeTempEvent( SrcPath, DstPath, Deleted, Pid );
                }
            }
            catch
            {
            }
        }

        private void CleanupFunction( string SrcPath, string DstPath, int Deleted )
        {
            try
            {
                CleanupEventHandler threadSafeTempEvent = CleanupEvent;
                if (threadSafeTempEvent != null)
                {
                    threadSafeTempEvent( SrcPath, DstPath, Deleted );
                }
            }
            catch
            {
            }
        }
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if( disposed )
                return; 

            CeBackupException.RaiseIfNotSucceeded( InternalCAPI.Uninit() );

            disposed = true;
        }

        ~CeBackupManager()
        {
            Dispose(false);
        }
    }
}
