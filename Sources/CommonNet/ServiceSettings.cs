using System.Collections.Generic;
using System.ServiceModel.Channels;

namespace CeBackupNetCommon
{
    public class ServiceSettings
    {
        private static readonly string DefaultfBaseUrl = "localhost"; // + :port
        private static readonly string DefaultBindingName = "pipe"; //"WSDualHttp";

        private static ServiceSettings _instance = null;

        private string _baseUrl;
        private string _bindingName;

        private Dictionary<string, IBindingFactory> _bindingFactories = new Dictionary<string, IBindingFactory>();

        private ServiceSettings()
        {
            _baseUrl = DefaultfBaseUrl;
            _bindingName = DefaultBindingName;
           
            _bindingFactories["pipe"] = new PipeBindingFactory();
            _bindingFactories["netTcp"] = new NetTcpBindingFactory();
            // test if this is the reason
            _bindingFactories["WSDualHttp"] = new WSDualHttpBindingFactory();

            Logger.Info( string.Format( "ServiceSettings.Constructor: Using url {0} as a base address", BaseUrl ) );
        }

        public static ServiceSettings Instance
        {
            get
            {
                return _instance ?? (_instance = new ServiceSettings());
            }
        }

        public string BaseUrl
        {
            get
            {
                return _bindingFactories[_bindingName].GetUrlHead() + _baseUrl;
            }
        }

        public string LibrayUrl
        {
            get
            {
                return BaseUrl + "/" + "CeBackupService";
            }
        }

        public Binding CreateBinding()
        {
            try
            {
                return _bindingFactories[_bindingName].CreateBinding();
            }
            catch
            {
                return _bindingFactories[DefaultBindingName].CreateBinding();
            }
        }
    }
}
