using System;
using System.ComponentModel;
using System.Reflection;
using System.Runtime.Serialization;
using System.ServiceModel;

namespace CeBackupNetCommon
{
    [DataContract]
    public class ServiceException
    {
        [DataMember]
        public string Message;
        [DataMember]
        public string StackTrace;

        internal ServiceException( string message, string stackTrace )
        {
            Message = message;
            StackTrace = stackTrace;
        }
    }

    public static class ErrorUtil
    {
        public static string GetDescription( Enum en )
        {
            Type type = en.GetType();
            MemberInfo[] memInfo = type.GetMember(en.ToString());
            if( memInfo != null && memInfo.Length > 0 )
            {
                object[] attrs = memInfo[0].GetCustomAttributes( typeof(DescriptionAttribute), false );
                if( attrs != null && attrs.Length > 0 )
                    return ((DescriptionAttribute)attrs[0]).Description;
            }

            return en.ToString();
        }
    }

    [DataContract]
    public class ServiceFault
    {
        [DataMember]
        public int ErrorCode;
        [DataMember]
        public ServiceException InternalException = null;
 
        public ServiceFault( int errorCode )
        {
            ErrorCode = (int)errorCode;
        }

        public ServiceFault( Exception e )
        {
            ErrorCode = 0;
            if( e != null )
                InternalException = new ServiceException( e.Message, e.StackTrace );
        }
    }

    [ServiceContract(CallbackContract = typeof(IBackupNotifications))]
    public interface IWCFCeBackupService
    {
        [OperationContract]
        uint GetVersion();
        [OperationContract]
        bool IsCompatible(uint ClientVersion);
        [OperationContract]
        [FaultContract(typeof(ServiceFault))]
        void SubscribeForEvents();
        [OperationContract]
        [FaultContract(typeof(ServiceFault))]
        void UnsubscribeFromEvents();
        [OperationContract]
        [FaultContract(typeof(ServiceFault))]
        void ReloadConfig( string IniPath );
        [OperationContract]
        [FaultContract(typeof(ServiceFault))]
        bool IsDriverStarted();
        [OperationContract]
        [FaultContract(typeof(ServiceFault))]
        bool IsBackupStarted();
    }

    [ServiceContract]
    public interface IWCFCeRestoreService
    {
        [OperationContract]
        [FaultContract(typeof(ServiceFault))]
        string[] Restore_ListAll();
        [OperationContract]
        [FaultContract(typeof(ServiceFault))]
        void Restore_Restore( string BackupPath );
        [OperationContract]
        [FaultContract(typeof(ServiceFault))]
        void Restore_RestoreTo( string BackupPath, string ToDirectory );
    }

    public interface IBackupNotifications
    {
        [OperationContract(IsOneWay = true)]
        void OnBackup( string SrcPath, string DstPath, int Deleted, System.IntPtr Pid );
        [OperationContract(IsOneWay = true)]
        void OnCleanup( string SrcPath, string DstPath, int Deleted );
    }
}
