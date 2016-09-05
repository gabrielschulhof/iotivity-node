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
var EventEmitter = require( "events" ).EventEmitter;
var handles = require( "./ClientHandles" );
var listenerCount = require( "./listenerCount" );
var payload = require( "./payload" );

var Resource = function Resource( resource ) {
	var result;

	if ( resource instanceof Resource ) {
		return resource;
	} else if ( !this._isResource ) {
		return new Resource( resource );
	}

	this.properties = {};

	Object.defineProperty( this, "_private", { writable: true, value: {} } );

	this.on( "newListener", _.bind( function( eventName, listener ) {
		if ( eventName === "update" && !listener._fromObserve ) {
			Resource.observe( this, listener );
		}
	}, this ) );

	this.on( "removeListener", _.bind( function() {
		if ( listenerCount( this, "update" ) === 0 && this._private.observer ) {
			result = handles.replace( {
				method: "OC_REST_OBSERVE",
				requestUri: this.resourcePath,
				query: this._private.query,
				deviceId: this.deviceId
			}, this._private.observer, "remove" );
			if ( result instanceof Error ) {
				this.emit( "error", result );
			}
		}
	}, this ) );
};
require( "util" ).inherits( Resource, EventEmitter );
Resource.prototype._isResource = true;

Resource.observe = function( resource, listener, fulfill, reject ) {
	var result;

	// Influences the behaviour of updateResource. If set to true, it causes
	// updateResource to call fulfill/reject when it is first called.
	var needsResolve = ( fulfill && reject );

	// Function called for both observation and retrieval. needsResolve tells us
	// whether there is an outstanding promise.
	var updateResource = function( response ) {
		var properties = response.payload ?
			payload.repPayloadToObject( response.payload ) : {};
		if ( properties instanceof Error ) {
			resource.emit( "error", properties );
			if ( needsResolve ) {
				reject( properties );
			}
		} else {
			_.extend( resource.properties, properties );
			resource.emit( "update", resource );
			if ( needsResolve ) {
				fulfill( resource );
			}
		}

		// Once we've resolved the promise we set this to false to avoid resolving
		// it multiple times.
		needsResolve = false;
	};

	// Avoid an infinite recursion by decorating the listener before attaching it
	listener._fromObserve = true;
	resource.on( "update", listener );
	delete listener._fromObserve;

	if ( !resource._private.observer ) {
		result = handles.replace( {
			method: "OC_REST_OBSERVE",
			requestUri: resource.resourcePath,
			query: resource._private.query,
			deviceId: resource.deviceId
		}, updateResource, "accumulate" );

		// If there was an error opening the handle, we reject the promise.
		if ( result instanceof Error ) {
			if ( needsResolve ) {
				reject( result );
			}
			resource.emit( "error", result );
		} else {

			resource._private.observer = updateResource;

			// If there was already a handle attached, then this handler was
			// added to the existing ones, but no new information will arrive
			// from the resource, so we cannot call fulfill() or reject() in time.
			if ( result === true ) {
				needsResolve = false;
				return true;
			}
		}
	}
};

module.exports = Resource;
