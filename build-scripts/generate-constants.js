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

var _ = {
	map: require( "lodash.map" ),
	uniq: require( "lodash.uniq" ),
	without: require( "lodash.without" )
};
var path = require( "path" );
var fs = require( "fs" );
var includePaths = require( "./helpers/header-paths" );
var repoPaths = require( "./helpers/repo-paths" );

function parseFileForConstants( fileName ) {
	return _.map(
		_.without(
			_.uniq(
				_.map(
					fs.readFileSync( fileName, { encoding: "utf8" } )
						.replace( /\r/g, "" )
						.split( "\n" ),
					function( line ) {
						var fields;

						if ( !line.match( /^#define/ ) ) {
							return;
						}

						// Do what awk does - split into tokens by whitespace
						fields = line.match( /\S+/g );
						if ( fields.length > 2 && !fields[ 1 ].match( /[()]/ ) &&

								// Constants we don't want
								fields[ 1 ] !== "OC_RSRVD_DEVICE_TTL" ) {
							return ( fields[ 2 ][ 0 ] === "\"" ? "string_utf8" : "double" ) +
								" " + fields[ 1 ];
						}
					} )
				),
			undefined ),
		function( line ) {
			var fields = line.split( " " );
			return [
				"#ifdef " + fields[ 1 ],
				"if (napi_create_" + fields[ 0 ] +
					"(" +
					[
						"env",
						fields[ 1 ]
					]
					.concat( fields[ 0 ] === "string_utf8" ? [
						"strlen(" + fields[ 1 ] + ")"
					] : [] )
					.concat( [ "&value" ] )
					.join( ", " ) +
					") != napi_ok) goto failure;",
				"if (napi_set_named_property(" +
					[
						"env",
						"exports",
						"\"" + fields[ 1 ] + "\"",
						"value"
					].join( ", " ) +
				") != napi_ok) goto failure;",
				"#endif /* def " + fields[ 1 ] + " */"
			].join( "\n" ) + "\n";
		} ).join( "\n" );
}

var constantsCC = path.join( repoPaths.generated, "constants.cc" );

fs.writeFileSync( constantsCC, [
	fs.readFileSync( path.join( repoPaths.src, "constants.cc.in" ), { encoding: "utf8" } ),
	"std::string InitConstants(napi_env env, napi_value exports) {",
	"  napi_value value;",
	"",
	"  // ocstackconfig.h: Stack configuration",
	parseFileForConstants( includePaths[ "ocstackconfig.h" ] ),
	"  // octypes.h: Type definitions",
	parseFileForConstants( includePaths[ "octypes.h" ] ),
	"  // octypes.h: Type definitions",
	parseFileForConstants( includePaths[ "rd_client.h" ] ),
	"  return std::string();",
	"failure:",
	"  return napi_status_to_string(env) + \"\\n\" + SOURCE_LOCATION;",
	"}"
].join( "\n" ) );
