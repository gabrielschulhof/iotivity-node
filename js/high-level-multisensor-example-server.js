var _ = require( "lodash" );
var oic = require( "iotivity-node" )( "server" );
var util = require( "util" );

oic.device = _.extend( oic.device, { name: "multisensor-example-server" } );

function Sensor( sensorType, init ) {
	if ( !this._isSensor ) {
		return oic.register( init ).then( function( resource ) {
			var theSensor = new sensorType();
			theSensor._resource = resource;
			Object.keys( theSensor ).forEach( function( theMethod ) {
				var theEvent = ( theMethod.match( /_([a-z]+)Handler/ ) || [ "", "" ] )[ 1 ];
				if ( theEvent ) {
					oic.addEventListener( theEvent, function( request ) {
						if ( event.target.id.deviceId === resource.id.deviceId &&
							event.target.id.path === resource.id.path ) {
							theSensor[ "_" + theEvent + "Handler" ]( request );
						}
					} );
				}
			} );
			return theSensor;
		} );
	}
}

Sensor.prototype._isSensor = true;

function HumiditySensor( init ) {
	return Sensor( HumiditySensor, init )
		.then( function( theSensor ) {
			setInterval( function() {
				theSensor._resource.properties.relativeHumidity = Math.random();
				theSensor._hardwareChanged();
			}, 2000 );
		} );
}

util.inherits( HumiditySensor, Sensor );

_.extend( HumiditySensor.prototype, {
	_retrieverequestHandler: function( request ) {
		request.sendResponse( this._resource ).catch( function( error ) {
			console.error( "Error sending response to HumiditySensor retrieve: " +
				error.message + error.stack );
		} );
	}
} );
