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

function errorHandler( error ) {
  console.log( error.stack + ": " + JSON.stringify( error, null, 4 ) );
  process.exit( 1 );
}

console.log( "Acquiring OCF device" );

var ocf = require( "iotivity-node" );

ocf.device.coreSpecVersion = "ocf.1.1.0";

ocf.server
  .register( {
    resourcePath: "/a/light",
    resourceTypes: [ "core.light" ],
    interfaces: [ "oic.if.baseline" ],
    discoverable: true,
    observable: true,
    secure: true
  } )
  .then( function( resource ) {
    console.log( "Resource registered: " + JSON.stringify( resource, null, 4 ) );
    resource
      .onupdate( function( request ) {
        console.log( "Update request: " + JSON.stringify( request, null, 4 ) );

        // Toggle resource type between core.fan and core.light
        resource.resourceTypes =
          ( resource.resourceTypes[ 0 ] === "core.light" ? [ "core.fan" ] : [ "core.light" ] );
        console.log( "Updated resource types. Resource now looks like this:" +
            JSON.stringify( resource, null, 4 ) );
        request
          .respond()
          .catch( errorHandler );
      } );
  } )
  .catch( errorHandler );
