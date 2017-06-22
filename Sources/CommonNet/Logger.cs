using System;

namespace CeBackupNetCommon
{
    public static class Logger
    {
        static string _filePath = "C:\\Temp\\CeUnknown.log";

        public static void SetPath( string Path )
        {
            _filePath = Path;
        }

        private static void _Log( string Prefix, string log )
        {
            System.IO.StreamWriter file = new System.IO.StreamWriter( _filePath, true );
            file.WriteLine( "[" + Prefix + "] " + log );
            file.Close();
        }

        public static void Info( string log )
        {
            _Log( "I", log );
        }

        public static void Info( string log, Exception ex )
        {
            _Log( "I", log + ", Exception: " + ex.ToString() );
        }

        public static void Warn( string log )
        {
            _Log( "W", log );
        }

        public static void Error( string log )
        {
            _Log( "E", log );
        }

        public static void Error( string log, Exception ex )
        {
            _Log( "E", log + ", Exception: " + ex.ToString() );
        }
    }
}
