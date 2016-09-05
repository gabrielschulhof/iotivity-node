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

var _ = require( "lodash" );
var csdk = require( "./csdk" );
var Resource = require( "./Resource" );
var querystring = require( "querystring" );
var payload = require( "./payload" );

var querySeparatorRegex = new RegExp(
	"[" + csdk.OC_QUERY_SEPARATOR.replace( "&", "" ) + "]", "g" );

var Server = function Server() {};
require( "util" ).inherits( Server, require( "events" ).EventEmitter );

// api: The API to call.
// error: The message of the Error with which to reject
// The rest of the arguments are passed to the API requested.
function doAPI( api, error ) {
	var apiArguments = Array.prototype.slice.call( arguments, 2 );
	return new Promise( function( fulfill, reject ) {
		var result = csdk[ api ].apply( this, apiArguments );
		if ( result === csdk.OCStackResult.OC_STACK_OK ) {
			fulfill();
		} else {
			reject( _.extend( new Error( error ), {
				result: result
			} ) );
		}
	} );
}

function isValidResource( resource ) {
	return new Promise( function( fulfill, reject ) {
		if ( ( resource instanceof Resource &&
				resource._private &&
				resource._private.handle ) ) {
			fulfill( resource );
		} else {
			reject( _.extend( new Error( "Invalid resource" ), { resource: resource } ) );
		}
	} );
}

function isNonEmptyStringArray( theArray ) {
	var index;

	if ( Array.isArray( theArray ) && theArray.length > 0 ) {
		for ( index in theArray ) {
			if ( typeof theArray[ index ] !== "string" ) {
				return false;
			}
		}
		return true;
	}

	return false;
}

function bindStringsToResource( resource, strings, binder, reject ) {
	var index, result, errorMessage;
	var extension = {};

	for ( index = 1; index < strings.length; index++ ) {
		result = csdk[ binder ](
			resource._private.handle, strings[ index ] );
		if ( result !== csdk.OCStackResult.OC_STACK_OK ) {
			errorMessage = "Failed to perform " + binder;
			extension[ binder ] = { value: strings[ index ], result: result };
			result = csdk.OCDeleteResource( resource._private.handle );
			if ( result !== csdk.OCStackResult.OC_STACK_OK ) {
				extension.OCDeleteResource = result;
			}
			reject( _.extend( new Error( errorMessage ), extension ) );
			return false;
		}
	}

	return true;
}

_.extend( Server.prototype, {
	_queryInfo: function( resource, queryString ) {
		var query, existingQueryString;

		queryString = queryString.replace( querySeparatorRegex, "&" );
		query = querystring.parse( queryString );
		existingQueryString = _.findKey( resource._private.observers, function( value, key ) {
			return _.isEqual( querystring.parse( key ), query );
		} );

		return {
			query: query,
			queryString: existingQueryString ? existingQueryString : queryString
		};
	},
	_createEntityHandler: function( resource ) {
		return _.bind( function( flag, request ) {
			var payload, queryInfo, eventName, observeFlag;

			if ( request.resource && request.resource !== resource._private.handle ) {
				this.emit( "error", _.extend( new Error( "Request received for wrong resource" ), {
					resource: resource,
					request: request
				} ) );
				return csdk.OCEntityHandlerResult.OC_EH_ERROR;
			}

			payload = request.payload ? payload.payloadToObject( request.payload ) : null;
			if ( payload instanceof Error ) {
				this.emit( "error", _.extend( payload, {
					resource: resource,
					request: request
				} ) );
				return csdk.OCEntityHandlerResult.OC_EH_ERROR;
			}

			eventName =
				( request.method === csdk.OCMethod.OC_REST_GET ||
					request.method === csdk.OCMethod.OC_REST_OBSERVE ) ? "retrieve" :
				request.method === csdk.OCMethod.OC_REST_PUT ? "update" :
				request.method === csdk.OCMethod.OC_REST_POST ? "create" :
				request.method === csdk.OCMethod.OC_REST_DELETE ? "delete" :
				_.extend( new Error( "Unknown event" ), {
					resource: resource,
					request: request
				} );
			if ( eventName instanceof Error ) {
				this.emit( "error", eventName );
				return csdk.OCEntityHandlerResult.OC_EH_ERROR;
			}

			queryInfo = this._queryInfo( resource, request.query );

			if ( flag & csdk.OCEntityHandlerFlag.OC_OBSERVE_FLAG ) {
				if ( request.obsInfo.action === csdk.OCObserveAction.OC_OBSERVE_REGISTER ) {
					observeFlag = true;
					resource._private.observers[ queryInfo.queryString ] =
						( resource._private.observers[ queryInfo.queryString ] ?
							resource._private.observers[ queryInfo.queryString ] : [] )
								.concat( [ request.obsInfo.obsId ] );
				} else if ( request.obsInfo.action ===
						csdk.OCObserveAction.OC_OBSERVE_DEREGISTER ) {
					observeFlag = false;
					_.remove( resource._private.observers[ queryInfo.queryString ],
						function( value ) {
							return ( value === request.obsInfo.obsId );
						} );
				}
			}

			this.emit.apply( this, [ eventName, _.extend( {
				id: request.requestHandle,
				target: resource,
				query: queryInfo.query,
			}, payload !== null ? { data: payload } : {} ) ]
				.concat( observeFlag !== undefined ? [ observeFlag ] : [] ) );

			return csdk.OCEntityHandlerResult.OC_EH_OK;
		}, this );
	},
	register: function( init ) {
		return new Promise( _.bind( function( fulfill, reject ) {
			var result;
			var resource;
			var handleReceptacle = {};

			if ( !( init.resourcePath &&
					isNonEmptyStringArray( init.resourceTypes ) &&
					isNonEmptyStringArray( init.interfaces ) ) ) {
				return reject( _.extend( new Error( "Invalid ResourceInit" ), {
					resourceInit: init
				} ) );
			}
			resource = _.extend( Resource(), init );
			result = csdk.OCCreateResource( handleReceptacle,
				init.resourceTypes[ 0 ],
				init.interfaces[ 0 ],
				init.resourcePath,
				this._createEntityHandler( resource ),
				0 |
				( init.discoverable ? csdk.OCResourceProperty.OC_DISCOVERABLE : 0 ) |
				( init.observable ? csdk.OCResourceProperty.OC_OBSERVABLE : 0 ) |
				( init.secure ? csdk.OCResourceProperty.OC_SECURE : 0 ) |
				( init.slow ? csdk.OCResourceProperty.OC_SLOW : 0 ) |
				( init.active ? csdk.OCResourceProperty.OC_ACTIVE : 0 ) );

			if ( result !== csdk.OCStackResult.OC_STACK_OK ) {
				return reject( _.extend( new Error( "register: OCCreateResource() failed" ), {
					result: result
				} ) );
			}

			// observers: {
			//   representation(JSON.stringified object): [ obsId ]
			// }
			resource._private.observers = {};
			resource._private.handle = handleReceptacle.handle;

			if ( !( bindStringsToResource( resource, init.resourceTypes.slice( 1 ),
						"OCBindResourceTypeToResource", reject ) &&
					bindStringsToResource( resource, init.interfaces.slice( 1 ),
						"OCBindInterfaceToResource", reject ) ) ) {
				return;
			}

			fulfill( resource );
		}, this ) );
	},
	unregister: function( resource ) {
		return isValidResource( resource )
			.then( function( resource ) {
				return doAPI( "OCDeleteResource", "Failed to delete resource",
					resource._private.handle );
			} );
	},
	notify: function( resource, transformRepresentation ) {
		return isValidResource( resource )
			.then( function( resource ) {
				return new Promise( function( fulfill, reject ) {
					var query, result, thePayload;
					var errors = [];

					for ( query in resource._private.observers ) {
						thePayload = payload.objectToRepPayload(
							transformRepresentation ?
								transformRepresentation( resource, querystring.parse( query ) ) :
								resource.properties );
						if ( thePayload instanceof Error ) {
							errors.push( thePayload );
						} else {
							result = csdk.OCNotifyListOfObservers( resource._private.handle,
								resource._private.observers[ query ], thePayload,
								csdk.OCQualityOfService.OC_HIGH_QOS );
							if ( result !== csdk.OCStackResult.OC_STACK_OK ) {
								errors.push( { query: query, result: result } );
							}
						}
					}

					if ( errors.length > 0 ) {
						reject( errors );
					} else {
						fulfill();
					}
				} );
			} );
	},
	enablePresence: function( timeToLive ) {
		return doAPI( "OCStartPresence", "Failed to enable presence", timeToLive );
	},
	disablePresence: function() {
		return doAPI( "OCStopPresence", "Failed to disable presence" );
	},
	respond: function( request, error, data ) {
		var responsePayload = data ? payload.objectToRepPayload( data ) : null;
		var resourceHandle = request.target._private.handle;

		resourceHandle = ( ( resourceHandle && resourceHandle.stale ) ? null : resourceHandle );

		if ( responsePayload instanceof Error ) {
			error = responsePayload;
			responsePayload = null;
		}

		return doAPI( "OCDoResponse", "Failed to send response", {
			requestHandle: request.id,
			resourceHandle: resourceHandle,
			ehResult: csdk.OCEntityHandlerResult[ error ? "OC_EH_ERROR" : "OC_EH_OK" ],
			payload: responsePayload,
			sendVendorSpecificHeaderOptions: [],
			resourceUri: request.target.resourcePath
		} );
	}
} );

module.exports = new Server();
