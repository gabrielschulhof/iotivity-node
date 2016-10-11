var iotivity = require( "bindings" )( "iotivity" );
var DeviceInfo = iotivity.DeclareInterceptableClass( "DeviceInfo",
	function( initialValue ) {
		if ( !( this instanceof DeviceInfo ) ) {
			return new DeviceInfo( initialvalue );
		}
		this._private = {
			value: initialValue,
			validKeys: [ 
	}, {
	} );
