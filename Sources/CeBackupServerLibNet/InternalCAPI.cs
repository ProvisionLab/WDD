using System.Runtime.InteropServices;

using HANDLE = System.IntPtr;
using UnmarshaledString = System.IntPtr;
using UnmarshaledStringArray = System.IntPtr;

namespace CeBackupServerLibNet
{
    internal class InternalCAPI
    {
        internal const string CeBackupDllName = "ceuserlib";

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
        internal delegate void ServerBackupCallback(string SrcPath, string DstPath, int Deleted, HANDLE Pid);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
        internal delegate void ServerCleanupCallback(string SrcPath, string DstPath, int Deleted);

        [DllImport(CeBackupDllName, EntryPoint = "Init", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
        internal static extern int Init();

        [DllImport(CeBackupDllName, EntryPoint = "Uninit", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
        internal static extern int Uninit();

        [DllImport(CeBackupDllName, EntryPoint = "GetLastErrorString", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
        internal static extern UnmarshaledString GetLastErrorString();

        [DllImport(CeBackupDllName, EntryPoint = "SubscribeForBackupEvents", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
        internal static extern int SubscribeForBackupEvents(ServerBackupCallback BackupEventFunction);

        [DllImport(CeBackupDllName, EntryPoint = "UnsubscribeFromBackupEvents", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
        internal static extern int UnsubscribeFromBackupEvents();

        [DllImport(CeBackupDllName, EntryPoint = "SubscribeForCleanupEvents", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
        internal static extern int SubscribeForCleanupEvents(ServerCleanupCallback CleanupEventFunction);

        [DllImport(CeBackupDllName, EntryPoint = "UnsubscribeFromCleanupEvents", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
        internal static extern int UnsubscribeFromCleanupEvents();

        [DllImport(CeBackupDllName, EntryPoint = "ReloadConfig", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
        internal static extern int ReloadConfig( string IniPath );

        [DllImport(CeBackupDllName, EntryPoint = "IsDriverStarted", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
        internal static extern int IsDriverStarted();

        [DllImport(CeBackupDllName, EntryPoint = "IsBackupStarted", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
        internal static extern int IsBackupStarted();

        [DllImport(CeBackupDllName, EntryPoint = "Start", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
        internal static extern int Start( string IniPath );

        [DllImport(CeBackupDllName, EntryPoint = "Stop", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
        internal static extern int Stop();

        [DllImport(CeBackupDllName, EntryPoint = "Restore_Init", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
        internal static extern int Restore_Init( string IniPath );

        [DllImport(CeBackupDllName, EntryPoint = "Restore_Uninit", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
        internal static extern int Restore_Uninit();

        [DllImport(CeBackupDllName, EntryPoint = "Restore_ListAll", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
        internal static extern int Restore_ListAll( out UnmarshaledStringArray RestorePathArray, out uint PathCount );

        [DllImport(CeBackupDllName, EntryPoint = "Restore_Restore", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
        internal static extern int Restore_Restore( string BackupPath );

        [DllImport(CeBackupDllName, EntryPoint = "Restore_RestoreTo", ExactSpelling = true, CallingConvention = CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
        internal static extern int Restore_RestoreTo( string BackupPath, string ToDirectory );
    }
}
