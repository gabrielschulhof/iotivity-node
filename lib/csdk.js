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

( function( root, factory ) {

	// Not AMD. Assume Node.
	if ( !( typeof define === "function" && define.amd ) ) {
		define = function( dependencies, factory ) {
			module.exports = factory.apply( root, dependencies.map( require ) );
		};
	}
	define( [
		"bindings",
		"./StorageHandler"
	], factory );
} )( this,
function( bindings, StorageHandler ) {

var iotivity = bindings( "iotivity" );

function stackOKOrThrow( result, message ) {
	var theError;

	if ( result !== iotivity.OCStackResult.OC_STACK_OK ) {
		theError = new Error( message );
		theError.result = result;
		throw theError;
	}
}

stackOKOrThrow( iotivity.OCRegisterPersistentStorageHandler( new StorageHandler() ),
	"OCRegisterPersistentStorageHandler() failed" );
stackOKOrThrow( iotivity.OCInit( null, 0, iotivity.OCMode.OC_CLIENT_SERVER ), "OCInit() failed" );
stackOKOrThrow( iotivity.OCStartPresence( 0 ), "OCStartPresence() failed" );
setInterval( function() {
	var result;

	// This catches errors thrown from native callbacks - for example, if the JS callback return
	// value fails validation
	try {
		result = iotivity.OCProcess();
	} catch ( theError ) {
		throw theError;
	}
	stackOKOrThrow( result, "OCProcess() failed" );
}, 100 );

return iotivity;

} );
