using System;
using System.Runtime.InteropServices;
using UnmarshaledString = System.IntPtr;

namespace CeBackupServerLibNet
{
    public class CeRestoreServerManager : IDisposable
    {
        private string _IniPath = null;
        private bool disposed = false;

        private static CeRestoreServerManager _instance = null;

        public static CeRestoreServerManager Instance
        {
            get
            {
                if( _instance == null )
                    _instance = new CeRestoreServerManager();

                return _instance;
            }
        }

        private CeRestoreServerManager()
        {
        }

        public void Init( string IniPath )
        {
            _IniPath = IniPath;
            CeBackupServerException.RaiseIfNotSucceeded( InternalCAPI.Restore_Init( _IniPath ) );
        }

        public string[] Restore_ListAll()
        {
            uint count = 0;
            UnmarshaledString UnmanagedStringArray = IntPtr.Zero;
            CeBackupServerException.RaiseIfNotSucceeded( InternalCAPI.Restore_ListAll( out UnmanagedStringArray, out count ) );

            string[] ManagedStringArray = null;
            MarshalUnmananagedStrArray2ManagedStrArray( UnmanagedStringArray, (int)count, out ManagedStringArray );
            return ManagedStringArray;
        }

        public void Restore( string BackupPath )
        {
            CeBackupServerException.RaiseIfNotSucceeded( InternalCAPI.Restore_Restore( BackupPath ) );
        }

        public void RestoreTo( string BackupPath, string DirTo )
        {
            CeBackupServerException.RaiseIfNotSucceeded( InternalCAPI.Restore_RestoreTo( BackupPath, DirTo ) );
        }

        static void MarshalUnmananagedStrArray2ManagedStrArray( UnmarshaledString pUnmanagedStringArray, int StringCount, out string[] ManagedStringArray )
        {
            IntPtr[] pIntPtrArray = new IntPtr[StringCount];
            ManagedStringArray = new string[StringCount];

            Marshal.Copy( pUnmanagedStringArray, pIntPtrArray, 0, StringCount );

            for( int i = 0; i < StringCount; i++ )
            {
                ManagedStringArray[i] = Marshal.PtrToStringAuto( pIntPtrArray[i] );
                Marshal.FreeCoTaskMem( pIntPtrArray[i] );
            }

            Marshal.FreeCoTaskMem( pUnmanagedStringArray );
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

            CeBackupServerException.RaiseIfNotSucceeded( InternalCAPI.Restore_Uninit() );

            disposed = true;
        }

        ~CeRestoreServerManager()
        {
            Dispose( false );
        }
    }
}
