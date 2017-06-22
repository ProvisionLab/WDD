using System;
using System.ServiceModel;
using System.ServiceModel.Channels;
using System.ServiceModel.Description;
using CeBackupNetCommon;
using CeBackupServerLibNet;

namespace CeServiceLibNet
{
    public static class CeService
    {
        public static ServiceHost Start()
        {
            var dir = AppDomain.CurrentDomain.BaseDirectory;

            string iniPath = dir + "\\cebackup.ini";
            Logger.Info( string.Format("Starting CeBackupServerManager( '{0}')", iniPath) );
            CeBackupServerManager.Instance.Start( iniPath );
            Logger.Info( string.Format("Starting CeRestoreServerManager('{0}')", iniPath) );
            CeRestoreServerManager.Instance.Init( iniPath );

            Uri httpUrl = new Uri( ServiceSettings.Instance.LibrayUrl );

            //Create ServiceHost
            Logger.Info( string.Format("CeService.Start: Creating service host...") );
            ServiceHost host = new ServiceHost( typeof(WCFCeBackupService), httpUrl );

            //Uncomment if you WSDL is needed
            //ServiceMetadataBehavior smb = new ServiceMetadataBehavior();
            //smb.HttpGetEnabled = true;
            //host.Description.Behaviors.Add(smb);
            
            //Add a service endpoint
            Logger.Info( string.Format("CeService.Start: Creating binding...") );
            Binding binding = ServiceSettings.Instance.CreateBinding();

            Logger.Info( string.Format("CeService.Start: adding service point...") );
            host.AddServiceEndpoint( typeof(IWCFCeBackupService), binding, "" );
            host.AddServiceEndpoint( typeof(IWCFCeRestoreService), binding, "" );

            ServiceThrottlingBehavior throttle = new ServiceThrottlingBehavior();
            host.Description.Behaviors.Add(throttle);
            throttle.MaxConcurrentInstances = 1;
            throttle.MaxConcurrentCalls = 100;
            throttle.MaxConcurrentSessions = 500;

            //Start the Service
            Logger.Info( string.Format("CeService.Start: About to start a service...") );
            host.Open();
            host.Faulted += CeHost_Faulted;
            host.Closed += CeHost_Closed;

            Logger.Info( string.Format("CeService.Start: Backup service up and running!") );

            return host;
        }

        public static void Stop()
        {
            try
            {
                Logger.Info( string.Format("Dispose CeBackupServerManager") );
                CeBackupServerManager.Instance.Dispose();
            }
            catch { }

            try
            {
                Logger.Info( string.Format("Dispose CeRestoreServerManager") );
                CeRestoreServerManager.Instance.Dispose();
            }
            catch { }
        }

        private static void CeHost_Faulted( object sender, EventArgs e )
        {
            ServiceHost host = sender as ServiceHost;

            host.Abort();

            Start();
        }

        private static void CeHost_Closed( object sender, EventArgs e )
        {
            Logger.Warn("Main Backup service closed");
        }        
    }
}
