using CeBackupNetCommon;
using System;
using System.ServiceModel;
using System.ServiceModel.Channels;
using System.ServiceModel.Dispatcher;
using System.ServiceModel.Description;

namespace CeServiceLibNet
{
    internal class ServiceErrorHandler : IErrorHandler
    {
        public void ProvideFault( Exception error, MessageVersion version, ref Message fault )
        {
            Logger.Info( "Host error has occurred.", error );

            if( error is FaultException )
            {
                // Let WCF do normal processing
                return;
            }

            //Convert to FaultException
            FaultException faultException = new FaultException<ServiceFault>( new ServiceFault( error.HResult ) );
            MessageFault messageFault = faultException.CreateMessageFault();

            fault = Message.CreateMessage( version, messageFault, faultException.Action );
        }

        public bool HandleError( Exception error )
        {
            if( error is FaultException )
            {
                return false; // Let WCF do normal processing
            }
            else
            {
                return true; // Fault message is already generated
            }
        }
    }

    public sealed class ErrorBehaviorAttribute : Attribute, IServiceBehavior
    {
        private Type errorHandlerType;

        public ErrorBehaviorAttribute( Type errorHandlerType )
        {
            this.errorHandlerType = errorHandlerType;
        }

        public Type ErrorHandlerType
        {
            get { return this.errorHandlerType; }
        }

        void IServiceBehavior.Validate( ServiceDescription description, ServiceHostBase serviceHostBase )
        {
        }

        void IServiceBehavior.AddBindingParameters( ServiceDescription description, ServiceHostBase serviceHostBase, System.Collections.ObjectModel.Collection<ServiceEndpoint> endpoints, BindingParameterCollection parameters )
        {
        }

        void IServiceBehavior.ApplyDispatchBehavior( ServiceDescription description, ServiceHostBase serviceHostBase )
        {
            IErrorHandler errorHandler;

            try
            {
                errorHandler = (IErrorHandler)Activator.CreateInstance( errorHandlerType );
            }
            catch( MissingMethodException e )
            {
                throw new ArgumentException( "The errorHandlerType specified in the ErrorBehaviorAttribute constructor must have a public empty constructor.", e );
            }
            catch( InvalidCastException e )
            {
                throw new ArgumentException( "The errorHandlerType specified in the ErrorBehaviorAttribute constructor must implement System.ServiceModel.Dispatcher.IErrorHandler.", e  );
            }

            foreach( ChannelDispatcherBase channelDispatcherBase in serviceHostBase.ChannelDispatchers )
            {
                ChannelDispatcher channelDispatcher = channelDispatcherBase as ChannelDispatcher;
                channelDispatcher.ErrorHandlers.Add( errorHandler );
            }
        }
    }
}
