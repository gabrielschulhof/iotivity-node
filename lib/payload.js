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

function isRepPayload( payload ) {
	return ( payload && payload.type === csdk.OCPayloadType.PAYLOAD_TYPE_REPRESENTATION &&
				!util.isArray( payload ) && typeof payload.values === "object" );
}

var _ = require( "lodash" );
var util = require( "util" );
var csdk = require( "./csdk" );
var payloadUtils = {
	objectToRepPayload: function( theObject, visitedObjects ) {
		var index, theValue, childPayload,
			payload = { type: csdk.OCPayloadType.PAYLOAD_TYPE_REPRESENTATION, values: {} };

		if ( visitedObjects === undefined ) {
			visitedObjects = {};
		}

		for ( index in theObject ) {
			theValue = theObject[ index ];
			if ( typeof theValue === "object" && theValue !== null ) {
				if ( visitedObjects[ theValue ] ) {
					return new Error( "objectToPayload: Circular object reference" );
				}
				visitedObjects[ theValue ] = true;
				if ( !util.isArray( theValue ) ) {
					childPayload = payloadUtils.objectToRepPayload( theValue, visitedObjects );
					if ( childPayload instanceof Error ) {
						return childPayload;
					}
					payload.values[ index ] = childPayload;
					continue;
				}
			}
			payload.values[ index ] = theValue;
		}

		return payload;
	},
	repPayloadToObject: function( payload ) {
		var index, theValue, childValue,
			result = {};

		if ( !isRepPayload( payload ) ) {
			return _.extend( new Error( "Invalid representation payload" ), {
				payload: payload
			} );
		}

		for ( index in payload.values ) {
			theValue = payload.values[ index ];
			if ( isRepPayload( theValue ) ) {
				childValue = payloadUtils.repPayloadToObject( theValue );
				if ( childValue instanceof Error ) {
					return childValue;
				}
				result[ index ] = childValue;
			} else {
				result[ index ] = theValue;
			}
		}

		return result;
	},
	ocAddressToUrl: function( ocAddress ) {
		var theResult = ocAddress.addr;

		if ( ocAddress.flags & csdk.OCTransportFlags.OC_IP_USE_V6 ) {
			theResult = "[" + theResult + "]";
		}

		if ( theResult && ocAddress.port ) {
			theResult += ":" + ocAddress.port;
		}

		return theResult;
	},
	makePlatformInfo: function( payload ) {
		var ocPlatformInfo;

		if ( !( payload &&
				payload.type === csdk.OCPayloadType.PAYLOAD_TYPE_PLATFORM &&
				payload.info &&
				typeof payload.info === "object" ) ) {
			return new Error( "platformInfo: invalid payload" );
		}

		ocPlatformInfo = payload.info;

		return _.extend( {},
			( ( "platformID" in ocPlatformInfo ) ?
				{ id: ocPlatformInfo.platformID } : {} ),
			( ( "operatingSystemVersion" in ocPlatformInfo ) ?
				{ osVersion: ocPlatformInfo.operatingSystemVersion } : {} ),
			( ( "modelNumber" in ocPlatformInfo ) ?
				{ model: ocPlatformInfo.modelNumber } : {} ),
			( ( "manufacturerName" in ocPlatformInfo ) ?
				{ manufacturerName: ocPlatformInfo.manufacturerName } : {} ),
			( ( "manufacturerUrl" in ocPlatformInfo ) ?
				{ manufacturerUrl: ocPlatformInfo.manufacturerUrl } : {} ),
			( ( "dateOfManufacture" in ocPlatformInfo ) ?
				{ manufactureDate: new Date( ocPlatformInfo.dateOfManufacture ) } : {} ),
			( ( "platformVersion" in ocPlatformInfo ) ?
				{ platformVersion: ocPlatformInfo.platformVersion } : {} ),
			( ( "firmwareVersion" in ocPlatformInfo ) ?
				{ firmwareVersion: ocPlatformInfo.firmwareVersion } : {} ),
			( ( "supportUrl" in ocPlatformInfo ) ?
				{ supportUrl: ocPlatformInfo.supportUrl } : {} ) );
	},
	makeDeviceInfo: function( payload, address ) {
		if ( !( payload &&
				payload.type === csdk.OCPayloadType.PAYLOAD_TYPE_DEVICE ) ) {
			return _.extend( new Error( "deviceInfo: Invalid payload" ), {
				payload: payload
			} );
		}

		return {
			uuid: payload.sid,
			url: payloadUtils.ocAddressToUrl( address ),
			name: payload.deviceName,
			dataModels: payload.dataModelVersions,
			coreSpecVersion: payload.specVersion
		};
	},
	resourceFromRepresentation: function( payload, deviceId ) {
		var resource = payloadUtils.repPayloadToObject( payload );

		return ( resource instanceof Error ) ?
			_.extend( resource, { deviceId: deviceId } ) :
			( typeof resource.href === "string" && resource.if && resource.if.length > 0 &&
					resource.rt && resource.rt.length > 0 && typeof resource.p === "object" &&
					"sec" in resource.p && "bm" in resource.p ) ? {
				deviceId: deviceId,
				resourcePath: resource.href,
				resourceTypes: resource.rt,
				interfaces: resource.if,
				secure: resource.p.sec,
				discoverable: !!( resource.p.bm & csdk.OCResourceProperty.OC_DISCOVERABLE ),
				observable: !!( resource.p.bm & csdk.OCResourceProperty.OC_OBSERVABLE ),
				slow: !!( resource.p.bm & csdk.OCResourceProperty.OC_SLOW ),
				active: !!( resource.p.bm & csdk.OCResourceProperty.OC_ACTIVE )
			} :
			_.extend( new Error( "Invalid resource representation from device" ), {
				deviceId: deviceId,
				payload: payload
			} );
	},
	processGetOicRes: function( payload ) {
		var index, resource;
		var newResources = [];

		if ( isRepPayload( payload ) ) {
			if ( payload.values.links && payload.values.links.length ) {
				for ( index in payload.values.links ) {
					resource = payloadUtils.resourceFromRepresentation(
						payload.values.links[ index ], payload.values.di );
					if ( resource instanceof Error ) {
						return resource;
					} else {
						newResources.push( resource );
					}
				}
			}
		} else {
			return _.extend( new Error( "processGetOicRes: unexpected payload" ), {
				payload: payload
			} );
		}

		return newResources;
	},
	processDiscoveryPayload: function( payload ) {
		var index, resource;
		var newResources = [];

		if ( payload && payload.type === csdk.OCPayloadType.PAYLOAD_TYPE_DISCOVERY &&
				payload.resources && payload.resources.length ) {
			for ( index in payload.resources ) {
				resource = payload.resources[ index ];
				if ( resource.uri && resource.types && resource.types.length > 0 &&
						resource.interfaces && resource.interfaces.length > 0 &&
						"bitmap" in resource && "secure" in resource ) {
					newResources.push( {
						deviceId: payload.sid,
						resourcePath: resource.uri,
						resourceTypes: resource.types,
						interfaces: resource.interfaces,
						secure: resource.secure,
						discoverable: !!( resource.bitmap &
							csdk.OCResourceProperty.OC_DISCOVERABLE ),
						observable: !!( resource.bitmap &
							csdk.OCResourceProperty.OC_OBSERVABLE ),
						slow: !!( resource.bitmap &
							csdk.OCResourceProperty.OC_SLOW ),
						active: !!( resource.bitmap &
							csdk.OCResourceProperty.OC_ACTIVE )
					} );
				} else {
					return _.extend(
						new Error( "processDiscoveryPayload: invalid resource in payload" ), {
							payload: payload
						} );
				}
			}
		} else {
			return _.extend( new Error( "processDiscoveryPayload: invalid payload" ), {
				payload: payload
			} );
		}
		return newResources;
	}
};

module.exports = payloadUtils;
