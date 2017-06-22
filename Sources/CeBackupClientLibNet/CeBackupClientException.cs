using CeBackupNetCommon;
using System;
using System.ComponentModel;
using System.ServiceModel;

namespace CeBackupClientLibNet
{
    public enum CLIENT_ERROR : short
    {
        [Description("Compatibility error. Service version is not supported.")]
        ERROR_VERSIONMISMATCH,
        [Description("Communication error. WCF connection is down.")]
        ERROR_COMMUNICATIONERROR,
        [Description("Invalid parameter")]
        ERROR_INVALIDPARAMETER,
        [Description("WCF channel error")]
        ERROR_CHANNELERROR,
        [Description("WCF server timeout")]
        ERROR_SERVICETIMEOUT
    }

    internal static class ErrorHandling
    {
        internal static Exception Proceed( Exception ex )
        {
            try
            {
                throw ex;
            }
            catch( FaultException<ServiceFault> error )
            {
                LogServiceException( error.Detail.InternalException );
           
                return new CeBackupClientException( error );
            }
            catch( FaultException error )
            {
                return new CeBackupClientException( error );
            }
            catch( CeBackupClientException )
            {
                throw;
            }
            catch( CommunicationException error )
            {
                Logger.Error( "Communication error has occurred.", error );
                return new CeBackupClientException( error );
            }
            catch( TimeoutException error )
            {
                // Whenever a send or receive operation on the channel takes too long
                Logger.Error( "Operation on the channel took too long time.", error );
                return new CeBackupClientException( error );
            }
            catch( Exception error )
            {
                Logger.Error( "Proxy/Channel error has occurred.", error );

                return new CeBackupClientException( error );
            }
        }

        internal static void CheckObjectForNull( object obj  )
        {
            if( obj == null )
                CheckObjectForNull( obj, false );
        }

        internal static void CheckObjectForNull( object obj, bool backupException )
        {
            if( obj == null )
            {
                throw new CeBackupClientException( CLIENT_ERROR.ERROR_INVALIDPARAMETER );
            }
        }

        private static void LogServiceException( ServiceException ex )
        {
            if( ex != null )
            {
                Logger.Error(string.Format("Exception from service side: Message: {0}, StackTrace {1}", ex.Message, ex.StackTrace));
            }
        }
    }

    public class CeBackupClientException : Exception
    {
        protected CLIENT_ERROR _errorCode;

        internal CeBackupClientException( CLIENT_ERROR errorCode )
            : base( ErrorUtil.GetDescription( errorCode ) )
        {
            _errorCode = errorCode;
        }

        internal CeBackupClientException( FaultException<ServiceFault> error )
            : base( ErrorUtil.GetDescription( (CLIENT_ERROR)error.Detail.ErrorCode ) )
        {
            _errorCode = (CLIENT_ERROR)error.Detail.ErrorCode;
        }

        internal CeBackupClientException( CLIENT_ERROR errorCode, Exception innerException )
            : base( ErrorUtil.GetDescription(errorCode), innerException )
        {
            _errorCode = errorCode;
        }

        internal CeBackupClientException( CommunicationException error )
            : base( ErrorUtil.GetDescription( CLIENT_ERROR.ERROR_COMMUNICATIONERROR ), error )
        {
            _errorCode = CLIENT_ERROR.ERROR_COMMUNICATIONERROR;
        }

        internal CeBackupClientException( TimeoutException error )
            : base( ErrorUtil.GetDescription( CLIENT_ERROR.ERROR_SERVICETIMEOUT ), error )
        {
            _errorCode = CLIENT_ERROR.ERROR_SERVICETIMEOUT;
        }

        internal CeBackupClientException( Exception error )
            : base( ErrorUtil.GetDescription( CLIENT_ERROR.ERROR_CHANNELERROR ), error )
        {
            _errorCode = CLIENT_ERROR.ERROR_CHANNELERROR;
        }

        public CLIENT_ERROR ErrorCode
        {
            get { return _errorCode; }
            internal set { _errorCode = value; }
        }
    }
}
