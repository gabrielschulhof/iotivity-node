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

var intervalId,
	handleReceptacle = {},

	// This is the same value as server.get.js
	sampleUri = "/a/high-level-example",
	iotivity = require( "iotivity-node/lowlevel" );

console.log( "Starting OCF stack in client mode" );

var path = require( "path" );
require( "../tests/preamble" )( __filename, [ {
	href: sampleUri,
	rel: "",
	rt: [ "core.fan" ],
	"if": [ iotivity.OC_RSRVD_INTERFACE_DEFAULT ]
} ], path.resolve( path.join( __dirname, ".." ) ) );

iotivity.OCRegisterPersistentStorageHandler( require( "../lib/StorageHandler" )() );

// Start iotivity and set up the processing loop
iotivity.OCInit( null, 0, iotivity.OCMode.OC_CLIENT_SERVER );

intervalId = setInterval( function() {
	iotivity.OCProcess();
}, 1000 );

console.log( "Issuing discovery request" );

iotivity.OCDoResource(
	{},
	iotivity.OCMethod.OC_REST_GET,
	"coaps://192.168.1.34:52183/" + sampleUri,
	null,
	{
		type: iotivity.OCPayloadType.PAYLOAD_TYPE_REPRESENTATION,
		values: {
			question: "How many angels can dance on the head of a pin?"
		}
	},
	iotivity.OCConnectivityType.CT_DEFAULT,
	iotivity.OCQualityOfService.OC_HIGH_QOS,
	function() { console.log( JSON.stringify( arguments, null, 4 ) ); },
	null );

// Exit gracefully when interrupted
process.on( "SIGINT", function() {
	console.log( "SIGINT: Quitting..." );

	// Tear down the processing loop and stop iotivity
	clearInterval( intervalId );
	iotivity.OCStop();

	// Exit
	process.exit( 0 );
} );
