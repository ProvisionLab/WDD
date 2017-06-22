using System;
using System.Collections.Generic;
using System.Linq;
using System.ServiceModel;
using System.ServiceModel.Channels;
using System.Text;

namespace CeBackupNetCommon
{
    public interface IBindingFactory
    {
       Binding CreateBinding();

       string GetUrlHead();
    }

    internal class PipeBindingFactory : IBindingFactory
    {
        public Binding CreateBinding()
        {
            NetNamedPipeBinding pipeBinding = new NetNamedPipeBinding();

            pipeBinding.MaxBufferSize = 0x7FFFFFFF;
            pipeBinding.MaxBufferPoolSize = 0x80000; //default, avoid big memory reservations
            pipeBinding.MaxReceivedMessageSize = 0x7FFFFFFF;
            pipeBinding.ReaderQuotas.MaxArrayLength = 0x7FFFFFFF;
            pipeBinding.ReaderQuotas.MaxBytesPerRead = 0x7FFFFFFF;
            pipeBinding.ReaderQuotas.MaxNameTableCharCount = 0x7FFFFFFF;
            pipeBinding.ReaderQuotas.MaxStringContentLength = 0x7FFFFFFF;
            pipeBinding.ReaderQuotas.MaxDepth = 0x7FFFFFFF;
            pipeBinding.TransferMode = TransferMode.Buffered;

            pipeBinding.ReceiveTimeout = TimeSpan.MaxValue;

            //pipeBinding.CloseTimeout = TimeSpan.Zero;

            return pipeBinding;
        }

        public string GetUrlHead()
        {
            return "net.pipe://";
        }
    }

    internal class NetTcpBindingFactory : IBindingFactory
    {
        public Binding CreateBinding()
        {
            NetTcpBinding pipeBinding = new NetTcpBinding();

            pipeBinding.MaxBufferSize = 0x7FFFFFFF;
            pipeBinding.MaxBufferPoolSize = 0x80000; //default, avoid big memory reservations
            pipeBinding.MaxReceivedMessageSize = 0x7FFFFFFF;
            pipeBinding.ReaderQuotas.MaxArrayLength = 0x7FFFFFFF;
            pipeBinding.ReaderQuotas.MaxBytesPerRead = 0x7FFFFFFF;
            pipeBinding.ReaderQuotas.MaxNameTableCharCount = 0x7FFFFFFF;
            pipeBinding.ReaderQuotas.MaxStringContentLength = 0x7FFFFFFF;
            pipeBinding.ReaderQuotas.MaxDepth = 0x7FFFFFFF;
            pipeBinding.TransferMode = TransferMode.Buffered;

            pipeBinding.ReceiveTimeout = TimeSpan.MaxValue;
            // pipeBinding.CloseTimeout = TimeSpan.Zero;

            pipeBinding.Security.Mode = SecurityMode.None;

            return pipeBinding;
        }

        public string GetUrlHead()
        {
            return "net.tcp://";
        }
    }

    internal class WSDualHttpBindingFactory : IBindingFactory
    {
        public Binding CreateBinding()
        {
            WSDualHttpBinding pipeBinding = new WSDualHttpBinding();

            pipeBinding.MaxBufferPoolSize = 0x80000; //default, avoid big memory reservations
            pipeBinding.MaxReceivedMessageSize = 0x7FFFFFFF;
            pipeBinding.ReaderQuotas.MaxArrayLength = 0x7FFFFFFF;
            pipeBinding.ReaderQuotas.MaxBytesPerRead = 0x7FFFFFFF;
            pipeBinding.ReaderQuotas.MaxNameTableCharCount = 0x7FFFFFFF;
            pipeBinding.ReaderQuotas.MaxStringContentLength = 0x7FFFFFFF;
            pipeBinding.ReaderQuotas.MaxDepth = 0x7FFFFFFF;

            pipeBinding.ReceiveTimeout = TimeSpan.MaxValue;
            // pipeBinding.CloseTimeout = TimeSpan.Zero;

            pipeBinding.Security.Mode = WSDualHttpSecurityMode.None;

            pipeBinding.ClientBaseAddress = new Uri( "http://localhost:8081/Ceedo.client" ) ;
            return pipeBinding;
        }

        public string GetUrlHead()
        {
            return "http://";
        }
    } 
}
