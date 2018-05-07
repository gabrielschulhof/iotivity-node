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

console.log( "Acquiring OCF device" );

var client = require( "iotivity-node" ).client;

function errorHandler( error ) {
  console.log( error.stack + ": " + JSON.stringify( error, null, 4 ) );
  process.exit( 1 );
}

function doDiscovery() {
  return client
    .on( "resourcefound", onResourceFound )
    .findResources( { resourcePath: "/a/light" } )
    .catch( errorHandler );
}

function onResourceFound( resource ) {
  console.log( "Found resource: " + JSON.stringify( resource, null, 4 ) );

  client
    .removeListener( "resourcefound", onResourceFound )
    .update( resource )
    .then( doDiscovery, errorHandler );
}

doDiscovery();

console.log( "Started looking for resources" );
