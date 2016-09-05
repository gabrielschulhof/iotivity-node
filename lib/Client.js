// Copyright 2016 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

var client;

var _ = require( "lodash" );
var util = require( "util" );
var csdk = require( "./csdk" );
var handles = require( "./ClientHandles" );
var payload = require( "./payload" );
var Resource = require( "./Resource" );
var resolver = require( "./Resolver" );
var querystring = require( "querystring" );

var Client = function Client() {};
require( "util" ).inherits( Client, require( "events" ).EventEmitter );

function makeDevice( thePayload, address ) {
	var result = payload.makeDeviceInfo( thePayload, address );

	// Side-effect: Update resolver using the device found
	if ( !( result instanceof Error ) ) {
		resolver.add( result.uuid, address );
	}

	return result;
}

function makeResources( resources, address, resolve, filterPath, emit ) {
	var resource, index;
	var filteredResources = [];

	if ( resources instanceof Error ) {
		return resources;
	}

	// Side-effect: Update the resolver with the address of the device from which
	// the new resources have arrived
	if ( resources.length && resources[ 0 ].deviceId ) {
		resolver.add( resources[ 0 ].deviceId, address );
	}

	for ( index in resources ) {
		if ( !filterPath || filterPath === resources[ index ].resourcePath ) {
			resource = _.extend( Resource(), resources[ index ] );
			filteredResources.push( resource );
			if ( emit ) {
				client.emit( "resourcefound", resource );
			}
		}
	}

	return resolve ?
		( resource ? resource : new Error( "Resource not found" ) ) :
		( emit ? undefined : filteredResources );
}

function plainFinder( prefix, eventName, listener, uri, query, itemMaker ) {
	return new Promise( function( fulfill, reject ) {
		var anError;

		if ( listener ) {
			client.on( eventName, listener );
		}

		anError = handles.replace( _.extend( {
			method: "OC_REST_DISCOVER",
			requestUri: uri
		}, query ? { query: query } : {} ), function( response ) {
			var index, newItem;

			if ( !( response && response.result === csdk.OCStackResult.OC_STACK_OK ) ) {
				newItem = _.extend( new Error( prefix + ": Unexpected response" ), {
						response: response
					} );
			} else {
				newItem = itemMaker( response.payload, response.addr );
			}

			if ( newItem instanceof Error ) {
				client.emit( "error", newItem );
			} else if ( util.isArray( newItem ) ) {
				for ( index in newItem ) {
					client.emit( eventName, newItem[ index ] );
				}
			} else {
				client.emit( eventName, newItem );
			}

			return csdk.OCStackApplicationResult.OC_STACK_KEEP_TRANSACTION;
		} );

		if ( anError ) {
			reject( anError );
		} else {
			fulfill();
		}
	} );
}

function oneShotRequest( options ) {
	return new Promise( function( fulfill, reject ) {
		var result;
		var handleReceptacle = {};
		var destination = null;

		if ( options.deviceId ) {
			destination = resolver.get( options.deviceId );
			if ( destination instanceof Error ) {
				reject( destination );
				return;
			}
		}

		result = csdk.OCDoResource( handleReceptacle,
			csdk.OCMethod[ options.method ],
			options.requestUri +
				( options.query ? ( "?" + querystring.stringify( options.query ) ) : "" ),
			destination,
			options.payload || null,
			csdk.OCConnectivityType.CT_DEFAULT,
			csdk.OCQualityOfService.OC_HIGH_QOS,
			function( handle, response ) {
				var answer;

				if ( response && ( response.result === csdk.OCStackResult.OC_STACK_OK ||
						response.result === options.expected ) ) {
					if ( options.createAnswer ) {
						answer = options.createAnswer( response );
					}
				} else {
					answer = _.extend( new Error( options.prefix + ": unexpected response" ), {
						response: response
					} );
				}

				( ( answer instanceof Error ) ? reject : fulfill )
					.apply( this, answer ? [ answer ] : [] );

				return csdk.OCStackApplicationResult.OC_STACK_DELETE_TRANSACTION;
			},
			null, 0 );

		if ( result !== csdk.OCStackResult.OC_STACK_OK ) {
			reject( _.extend( new Error( options.prefix + ": request failed" ), {
				result: result
			} ) );
		}
	} );
}

client = _.extend( new Client(), {
	getDeviceInfo: function( deviceId ) {
		return oneShotRequest( {
			method: "OC_REST_GET",
			requestUri: csdk.OC_RSRVD_DEVICE_URI,
			deviceId: deviceId,
			createAnswer: function( response ) {
				return makeDevice( response.payload, response.address );
			}
		} );
	},
	getPlatformInfo: function( deviceId ) {
		return oneShotRequest( {
			method: "OC_REST_GET",
			requestUri: csdk.OC_RSRVD_PLATFORM_URI,
			deviceId: deviceId,
			createAnswer: function( response ) {
				return payload.makePlatformInfo( response.payload );
			}
		} );
	},
	create: function( target, resourceInit ) {
		var properties = payload.objectToPayload( resourceInit.properties );
		if ( properties instanceof Error ) {
			return Promise.reject( properties );
		}

		return oneShotRequest( {
			method: "OC_REST_POST",
			requestUri: target.resourcePath,
			deviceId: target.deviceId,
			payload: _.extend( properties, {
				uri: resourceInit.resourcePath,
				types: resourceInit.resourceTypes,
				interfaces: resourceInit.interfaces
			} ),
			createAnswer: function( response ) {
				var init = payload.resourceFromRepresentation( response.payload, target.deviceId );
				if ( init instanceof Error ) {
					return init;
				}
				return _.extend( Resource(), init );
			}
		} );
	},
	retrieve: function( resourceId, query, listener ) {

		// Argument juggling
		// If @query is a function, then the query is not specified, but a listener is, so we
		// shift the arguments around and initialize @query
		if ( arguments.length === 2 && typeof query === "function" ) {
			listener = query;
			query = undefined;
		}

		// We must first ensure we have a valid Resource object
		return ( ( resourceId instanceof Resource ) ?

			// If resourceId is a Resource object we can use it as such, but only if the options
			// given are the same as the options assigned to it. Otherwise we must create and
			// return a new Resource object.
			Promise.resolve( _.isEqual( resourceId._private.query, query ) ?
				resourceId : _.extend( Resource(), resourceId ) ) :

			// Otherwise, we must discover the resource first, because we need to establish its
			// types and interfaces before we can perform the retrieve().
			client.findResources( {
				deviceId: resourceId.deviceId,
				resourcePath: resourceId.resourcePath,
				_resolve: true
			} ).then( function( resource ) {

				// Mark the retrieved resource with the given query.
				resource._private.query = query;
				return resource;
			} ) ).then( function( resource ) {
				var get = function() {
					return oneShotRequest( {
						method: "OC_REST_GET",
						requestUri: resource.resourcePath,
						query: query,
						deviceId: resource.deviceId,
						createAnswer: function( response ) {
							var properties = response.payload ?
								payload.repPayloadToObject( response.payload ) : {};
							if ( properties instanceof Error ) {
								return properties;
							} else {
								_.extend( resource.properties, properties );
								return resource;
							}
						}
					} );
				};

				return listener ?
					new Promise( function( fulfill, reject ) {

						// When Resource.observe() returns true it means it is unable to resolve
						// the promise, which in turn means that we must perform a get(). So, let's
						// resolve this promise with a special value (true), so that the chained
						// promise can call get().
						if ( Resource.observe( resource, listener, fulfill, reject ) ) {
							fulfill( true );
						}
					} ).then( function( result ) {
						return ( result === true ? get() : result );
					} ) :
					get();
			} );
	},
	update: function( resource ) {
		var properties = payload.objectToPayload( resource.properties );
		if ( properties instanceof Error ) {
			return Promise.reject( properties );
		}

		return oneShotRequest( {
			method: "OC_REST_PUT",
			requestUri: resource.resourcePath,
			deviceId: resource.deviceId,
			query: resource._private.query,
			payload: properties,
			createAnswer: function( response ) {
				var newProperties = payload.payloadToObject( response.payload );
				if ( newProperties instanceof Error ) {
					return newProperties;
				}
				_.extend( resource.properties, newProperties );
				return resource;
			}
		} );
	},
	delete: function( resourceId ) {
		return oneShotRequest( {
			method: "OC_REST_DELETE",
			requestUri: resourceId.resourcePath,
			deviceId: resourceId.deviceId
		} );
	},

	findDevices: function( listener ) {
		return plainFinder( "findDevices", "devicefound", listener, csdk.OC_RSRVD_DEVICE_URI,
			null, makeDevice );
	},
	findPlatforms: function( listener ) {
		return plainFinder( "findPlatforms", "platformfound", listener, csdk.OC_RSRVD_PLATFORM_URI,
			null, payload.makePlatformInfo );
	},
	findResources: function( options, listener ) {
		var query = options.resourceType ? { rt: options.resourceType } : null;


		// If only one argument is passed, it's the listener
		if ( arguments.length === 1 && typeof options === "function" ) {
			listener = options;
			options = {};
		}

		if ( options.deviceId ) {
			if ( listener ) {
				client.on( "resourcefound", listener );
			}
			return oneShotRequest( _.extend( {
				method: "OC_REST_GET",
				requestUri: csdk.OC_MULTICAST_DISCOVERY_URI,
				deviceId: options.deviceId,
				query: query,
				createAnswer: function( response ) {
					return makeResources( payload.processGetOicRes( response.payload ),
						response.addr, options._resolve, options.resourcePath, true );
				}
			}, query ? { query: query } : {} ) );
		} else {
			return plainFinder( "findResources", "resourcefound", listener,
				csdk.OC_MULTICAST_DISCOVERY_URI, query, function( discoveryPayload, address ) {
					return makeResources( payload.processDiscoveryPayload( discoveryPayload ),
						address, false, options.resourcePath );
				} );
		}
	}
} );

module.exports = client;
