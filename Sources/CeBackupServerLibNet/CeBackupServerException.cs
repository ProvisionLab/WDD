using CeBackupNetCommon;
using System;
using System.ComponentModel;
using System.Reflection;
using System.Runtime.InteropServices;
using UnmarshaledString = System.IntPtr;

namespace CeBackupServerLibNet
{
    public enum CeUserLib_Retval
    {
		CEUSERLIB_OK,
        CEUSERLIB_Failed,
        CEUSERLIB_InvalidParameter,
		CEUSERLIB_NotInitialized,
        CEUSERLIB_AlreadyInitialized,
        CEUSERLIB_OneInstanceOnly,
		CEUSERLIB_ConfigurationError,
        CEUSERLIB_DriverIsNotStarted,
        CEUSERLIB_BackupIsNotStarted,
        CEUSERLIB_BackupIsAlreadyStarted,
        CEUSERLIB_BufferOverflow,
        CEUSERLIB_OutOfMemory,
        CEUSERLIB_AlreadySubscribed
    }

    public class CeBackupServerException : Exception
    {
        new string StackTrace;

        public CeBackupServerException()
            : base()
        {
        }

        public CeBackupServerException( CeUserLib_Retval errorCode )
            : base("An exception " + errorCode + " occurred during execution.", new Exception( GetDescription( errorCode ) ))
        {
            this.HResult = (int)errorCode;
        }

        public static void RaiseIfNotSucceeded( int result )
        {
            if( result != 0 )
            {
                CeBackupServerException ex = new CeBackupServerException( GetError(result) );
                ex.StackTrace = Environment.StackTrace;
                Logger.Error( ex.ToString() + "\nStackTrace: " + ex.StackTrace );
                throw ex;
            }
        }

        internal static CeUserLib_Retval GetError( int errorCode )
        {
            CeUserLib_Retval error = CeUserLib_Retval.CEUSERLIB_Failed;

            try
            {
                error = (CeUserLib_Retval)Enum.ToObject( typeof(CeUserLib_Retval), errorCode);
            }
            catch( Exception )
            {
            }

            return error;
        }

        internal static string GetDescription( Enum en )
        {
            Type type = en.GetType();
            MemberInfo[] memInfo = type.GetMember( en.ToString() );
            if( memInfo != null && memInfo.Length > 0 )
            {
                object[] attrs = memInfo[0].GetCustomAttributes( typeof(DescriptionAttribute), false );
                if( attrs != null && attrs.Length > 0 )
                    return ((System.ComponentModel.DescriptionAttribute) attrs[0]).Description;
            }

            UnmarshaledString ptr = (UnmarshaledString)InternalCAPI.GetLastErrorString();
            string strLastError = Marshal.PtrToStringAuto( ptr );
            Marshal.FreeCoTaskMem( ptr );

            return en.ToString() + " - \r\n" + strLastError;
        }
    }
}
