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

var result,
	processCallCount = 0,
	processLoop = null,
	iotivity = require( process.argv[ 3 ] + "/lowlevel" ),
	testUtils = require( "../../utils" )( iotivity );

function cleanup() {
	var cleanupResult;

	if ( processLoop ) {
		clearInterval( processLoop );
		processLoop = null;
	}

	testUtils.assert( "ok", true, "RD Server: OCProcess succeeded " + processCallCount +
		" times" );

	cleanupResult = iotivity.OCRDStop();
	if ( testUtils.stackOKOrDie( "RD Server", "OCRDStop", cleanupResult ) ) {
		process.exit( 0 );
	}

	cleanupResult = iotivity.OCStop();
	if ( testUtils.stackOKOrDie( "RD Server", "OCStop", cleanupResult ) ) {
		process.exit( 0 );
	}
}

console.log( JSON.stringify( { assertionCount: 5 } ) );

// Initialize
result = iotivity.OCRegisterPersistentStorageHandler( require( "../../../lib/StorageHandler" )() );
testUtils.stackOKOrDie( "RD Server", "OCRegisterPersistentStorageHandler", result );

result = iotivity.OCInit( null, 0, iotivity.OCMode.OC_SERVER );
testUtils.stackOKOrDie( "RD Server", "OCInit", result );

result = iotivity.OCRDStart();
testUtils.stackOKOrDie( "RD Server", "OCRDStart", result );

// Set up process loop
processLoop = setInterval( function() {
	var processResult = iotivity.OCProcess();

	if ( processResult === iotivity.OCStackResult.OC_STACK_OK ) {
		processCallCount++;
	} else {
		testUtils.stackOKOrDie(
			"RD Server",
			"OCProcess(after " + processCallCount + " successful calls)",
			processResult );
	}
}, 100 );

// Report that the server has successfully created its resource(s).
console.log( JSON.stringify( { ready: true } ) );

// Exit gracefully when interrupted
process.on( "message", cleanup );
