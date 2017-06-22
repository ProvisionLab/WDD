using System;
using System.ServiceModel;
using CeServiceLibNet;
using CeBackupNetCommon;
using System.IO;

namespace CeHostNet
{
    class Program
    {
        private static ServiceHost _serviceHost;

        static void Main( string[] args )
        {
            Logger.SetPath( Path.GetTempPath() + "CeHostNet.log" );
            Logger.Info( string.Format("CeHostNet.Starting ...") );

            try
            {
                Logger.Info( string.Format("Starting CeService...") );
                _serviceHost = CeService.Start();

                Console.WriteLine("CeService is started");

                Console.WriteLine("Press enter to exit.");
                Console.ReadLine();
            }
            catch
            {
                Console.WriteLine("Service failed to start");
                return;
            }

            try
            {
                Logger.Info( string.Format("Stopping CeService...") );
                CeService.Stop();
            }
            catch
            {
                Console.WriteLine("Service failed to stop");
            }

            if( _serviceHost != null )
                _serviceHost.Close();
        }
    }
}
