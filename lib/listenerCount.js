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
		"events"
	], factory );
} )( this,
function( events ) {

return function listenerCount( emitter, event ) {
	return ( typeof emitter.listenerCount === "function" ) ?
		emitter.listenerCount( event ) :
		events.EventEmitter.listenerCount( emitter, event );
};

} );
