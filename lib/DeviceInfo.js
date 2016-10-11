var _ = require( "lodash" );
var csdk = require( "./csdk" );
var defaultDeviceInfo = {
	uuid: csdk.OCGetServerInstanceIDString(),
	name: "default",
	coreSpecVersion: "1.0.0",
	dataModels: [ "res.1.0.0" ]
};

var DeviceInfo = csdk.DeclareInterceptableClass( "DeviceInfo",
	function( initialValue ) {
		if ( !( this instanceof DeviceInfo ) ) {
			return new DeviceInfo( initialValue );
		}
		initialValue = initialValue || defaultDeviceInfo;
		this._private = {
			value: initialValue,
			validKeys: [ "uuid", "url", "name", "dataModels", "coreSpecVersion" ]
		};
	}, {
		getter: function( key ) {
			console.log( "getter: " + key );
			if ( key === "_private" || !( key in this._private.validKeys ) ) {
				console.log( "getter: " + key + ": throwing null" );
				throw null;
			} else if ( key === "uuid" ) {
				console.log( "getter: " + key + ": retrieving ID" );
				var value = csdk.OCGetServerInstanceIdString();
				this._private.value.uuid = value;
				return value;
			}
			console.log( "getter: " + key + ": retrieving value: " + this._private.value[ key ] );
			return this._private.value[ key ];
		},
		setter: function( key, value ) {
			console.log( "setter: " + key + ", " + value );
			if ( key === "_private" || !( key in this._private.validKeys ) ) {
				console.log( "setter: " + key + ": throwing null" );
				throw null;
			} else if ( key === "uuid" ) {
				console.log( "getter: " + key + ": throwing error" );
				throw new Error( "Cannot set UUID at runtime" );
			}

			var result;
			var toCommit = _.extend( {}, this._private.value );
			toCommit[ key ] = value;
			DeviceInfo.commit( toCommit );
			this._private.value = toCommit;
		},
		namedEnumerator: function() {
			var index;
			var result = [];
			for ( index in this._private.validKeys ) {
				if ( this._private.value[ this._private.validKeys[ index ] ] ) {
					result.push( this._private.validKeys );
				}
			}
		}
	} );

module.exports = _.extend( DeviceInfo, {
	commit: function( deviceInfo ) {
		result = csdk.OCSetDeviceInfo( deviceInfo );
		if ( result !== csdk.OCStackResult.OC_STACK_OK ) {
			throw new Error( "Failed to save device info" );
		}
	}
} );
