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

var path = require( "path" );
var fs = require( "fs" );
var shelljs = require( "shelljs" );
var repoPaths = require( "./helpers/repo-paths" );

var isDependency;

try {
	isDependency = ( JSON.parse( process.env.npm_config_argv ).remain.length > 0 );
} catch ( anError ) {}
isDependency = isDependency ||
	( "NODE_ENV" in process.env && process.env.NODE_ENV === "production" );
if ( !isDependency ) {
	try {
		isDependency = ( "iotivity-node" in
			JSON.parse(
				fs.readFileSync(
					path.join( __dirname, "..", "..", "..", "package.json" ) ) ).dependencies );
	} catch ( anError ) {}
}

// If we're not installed as a dependency of another package then leave intermediate files,
// sources, and build scripts lying around
if ( !isDependency ) {
	return;
}

// Purge any and all files not needed after building
shelljs.rm( "-rf",
	path.join( repoPaths.root, "binding.gyp" ),
	path.join( repoPaths.root, "node_modules", "nan" ),
	path.join( repoPaths.root, "node_modules", ".bin" ),
	path.join( repoPaths.root, "build-scripts" ),
	path.join( repoPaths.installLibraries, "*.a" ),
	path.join( repoPaths.installLibraries, "*.lib" ),
	path.join( repoPaths.installLibraries, "*.dll" ),
	path.join( repoPaths.installLibraries, "extlibs" ),
	path.join( repoPaths.installLibraries, "resource" ),
	repoPaths.installHeaders,
	repoPaths.generated,
	repoPaths.iotivity,
	repoPaths.patchesPath,
	repoPaths.src );
