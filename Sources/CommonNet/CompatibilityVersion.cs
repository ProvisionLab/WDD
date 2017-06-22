using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CeBackupNetCommon
{
    public static class CompatibilityVersion
    {
        // this member should be increased every time service contract is changed
        private static uint _version = 1;

        public static uint Version
        {
            get
            {
                return _version;
            }
        }

        /*static public bool IsCompatible(uint serviceVersion)
        {
            return (serviceVersion >= _version); 
        }*/
    }
}
