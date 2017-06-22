using System;
using System.ServiceModel;
using System.ServiceModel.Channels;

namespace CeBackupNetCommon
{
    public class CeChannelFactory
    {
        private static CeChannelFactory _instance = null;

        private CeChannelFactory()
        {
        }

        public static CeChannelFactory Instance
        {
            get
            {
                if( _instance == null )
                    _instance = new CeChannelFactory();

                return _instance;
            }
        }

        public DuplexChannelFactory<IWCFCeBackupService> CreateBackupFactory( string url, IBackupNotifications eventTarget )
        {
            DuplexChannelFactory<IWCFCeBackupService> backupFactory = null;
            try
            {
                Binding binding = ServiceSettings.Instance.CreateBinding();
                EndpointAddress ep = new EndpointAddress( url );
                backupFactory = new DuplexChannelFactory<IWCFCeBackupService>( eventTarget, binding, ep );
            }
            catch( Exception e )
            {
                Logger.Error( "ChannelFactory.CreateBackupFactory: Failed to create backup factory", e );
            }

            return backupFactory;
        }

        public ChannelFactory<IWCFCeRestoreService> CreateRestoreFactory( string url )
        {
            ChannelFactory<IWCFCeRestoreService> restoreFactory = null;
            try
            {
                Binding binding = ServiceSettings.Instance.CreateBinding();
                EndpointAddress ep = new EndpointAddress( url );
                restoreFactory = new ChannelFactory<IWCFCeRestoreService>( binding, ep );
            }
            catch( Exception e )
            {
                Logger.Error( "ChannelFactory.CreateRestoreFactory: Failed to create backup factory", e );
            }

            return restoreFactory;
        }
    }
}
