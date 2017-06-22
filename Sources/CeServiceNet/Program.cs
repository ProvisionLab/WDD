using CeBackupNetCommon;
using System.IO;
using System.ServiceProcess;

namespace CeServiceNet
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        static void Main()
        {
            Logger.SetPath( Path.GetTempPath() + "CeServiceNet.log" );
            Logger.Info( string.Format("CeHostNet.Starting ...") );

            ServiceBase[] ServicesToRun;
            ServicesToRun = new ServiceBase[]
            {
                new CeBackupWindowsService()
            };
            ServiceBase.Run( ServicesToRun );
        }
    }
}
