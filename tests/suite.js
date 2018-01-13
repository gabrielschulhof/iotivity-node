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

var QUnit,
	async = require( "async" ),
	glob = require( "glob" ),
	_ = {
		map: require( "lodash.map" ),
		each: require( "lodash.foreach" ),
		bind: require( "lodash.bind" ),
		without: require( "lodash.without" ),
		extend: require( "lodash.assignin" )
	},
	childProcess = require( "child_process" ),
	fs = require( "fs" ),
	path = require( "path" ),
	uuid = require( "uuid" ),
	provision = require( "./provision" ),
	runningProcesses = [],
	getQUnit = function() {
		if ( !QUnit ) {
			QUnit = require( "./setup" );
		}
		return QUnit;
	};

// Spawn a single child and process its stdout.
function spawnOne( assert, options ) {
	var theChild;
	var commandLine = [ options.path ]
		.concat( options.uuid ? [ options.uuid ] : [ "" ] )

		// What to put into require() to load iotivity-node
		.concat( [ process.argv[ 2 ] ] );

	theChild = childProcess.spawn(
		"node", commandLine, {
			stdio: [ process.stdin, "pipe", process.stderr, "ipc" ],
			env: _.extend( {}, process.env, { LOG_PREFIX: options.name } )
		} );
	runningProcesses.push( theChild );

	theChild
		.on( "exit", function( code, signal ) {
			var exitCodeOK = ( code === 0 || code === null ),
				signalOK = ( signal !== "SIGSEGV" );

			assert.ok( exitCodeOK, options.name + " exited successfully (" + code + ")" );
			assert.ok( signalOK, options.name + " did not segfault" );
		} )
		.on( "close", function() {
			var childIndex = runningProcesses.indexOf( theChild );
			if ( childIndex >= 0 ) {
				runningProcesses.splice( childIndex, 1 );
			}
			options.maybeQuit( theChild );
		} );

	// The stdout of the child is a sequence of \n-separated stringified JSON objects.
	theChild.stdout.on( "data", function serverStdoutData( data ) {
		_.each( data.toString().split( "\n" ), function( value ) {
			var jsonObject;

			if ( !value ) {
				return;
			}

			// Attempt to retrieve a JSON object from stdout.
			try {
				jsonObject = JSON.parse( value );
			} catch ( e ) {
				options.teardown( "Error parsing " + options.name + " JSON: '" + value + "'" +
					( e.message ? e.message : e ), true );
				return;
			}

			// The child is reporting the number of assertions it will be making. We add our own
			// two assertions ( 1.) successful exit and 2.) no segfault) to that count.
			if ( jsonObject.assertionCount ) {
				options.reportAssertions( jsonObject.assertionCount + 2 );

			} else if ( jsonObject.info ) {
				console.log( "\x1b[46;30mi\x1b[0m " + jsonObject.message );

			// The child has requested a teardown.
			} else if ( jsonObject.teardown ) {
				options.teardown(
					options.name + " requested teardown: " + jsonObject.message );

			// The child has requested that its peer be killed.
			} else if ( jsonObject.killPeer ) {
				options.teardown( null, theChild );

			// The child is reporting that it is ready. Only servers do this.
			} else if ( jsonObject.ready ) {
				if ( options.onReady ) {
					options.onReady();
				}

			// The child is making an assertion.
			} else if ( jsonObject.assertion ) {
				assert[ jsonObject.assertion ].apply( assert, jsonObject.arguments );

			// Otherwise, we have received unknown JSON from the child - bail.
			} else {
				options.teardown( "Unkown JSON from " + options.name + ": " + value, true );
			}
		} );
	} );

	return theChild;
}

function resolvePath( aPath ) {
	return path.resolve( aPath );
}

function runTestSuites( files ) {
	_.each( files, function( item ) {
		var clientPathIndex,
			clientPaths = glob.sync( path.join( item, "client*.js" ) ).map( resolvePath ),
			serverPaths = glob.sync( path.join( item, "server*.js" ) ).map( resolvePath );

		if ( fs.lstatSync( item ).isFile() ) {
			getQUnit().test( path.basename( item ).replace( /\.js$/, "" ), function( assert ) {
				var theChild,
					spawnOptions = {
						uuid: uuid.v4(),
						name: "Test",
						path: item,
						teardown: function() {
							if ( theChild ) {
								theChild.kill( "SIGTERM" );
							}
						},
						maybeQuit: assert.async(),
						reportAssertions: _.bind( assert.expect, assert )
					};
				theChild = spawnOne( assert, spawnOptions );
			} );
			return;
		}

		if ( !fs.lstatSync( item ).isDirectory() ) {
			return;
		}

		for ( clientPathIndex in clientPaths ) {
			if ( !( fs.lstatSync( clientPaths[ clientPathIndex ] ).isFile() ) ) {
				throw new Error( "Cannot find client at " + clientPaths[ clientPathIndex ] );
			}
		}

		serverPaths.forEach( function( serverPath ) {
			if ( !( fs.lstatSync( serverPath ).isFile() ) ) {
				throw new Error( "Cannot find server at " + serverPath );
			}
		} );

		getQUnit().test( path.basename( item ), function( assert ) {
			var totalChildren = clientPaths.length + serverPaths.length,

				// Track the child processes involved in this test in this array
				children = [],

				// Turn this test async
				done = assert.async(),

				// Count assertions made by the children. Report them to assert.expect() when both
				// children have reported their number of assertions.
				totalAssertions = 0,
				childrenAssertionsReported = 0,

				spawnOptions = {
					uuid: uuid.v4(),
					teardown: function( error, sourceProcess ) {
						var index,
							signal = error ? "SIGTERM" : "SIGINT",

							// When killing child processes in a loop we have to copy the array
							// because it may become modified by the incoming notifications that a
							// process has exited.
							copyOfChildren = children.slice();

						for ( index in copyOfChildren ) {
							if ( sourceProcess && sourceProcess === copyOfChildren[ index ] ) {
								continue;
							}
							if ( signal === "SIGINT" ) {
								copyOfChildren[ index ].send( "die" );
							} else {
								copyOfChildren[ index ].kill( signal );
							}
						}

						if ( error ) {
							throw new Error( error );
						}
					},
					maybeQuit: function( theChild ) {
						var childIndex = children.indexOf( theChild );
						if ( childIndex >= 0 ) {
							children.splice( childIndex, 1 );
						}
						if ( children.length === 0 ) {
							done();
						}
					},
					reportAssertions: function( assertionCount ) {
						childrenAssertionsReported++;
						totalAssertions += assertionCount;
						if ( childrenAssertionsReported === totalChildren ) {
							assert.expect( totalAssertions );
						}
					}
				};

			provision( {
				clientPaths: clientPaths,
				serverPaths: serverPaths,
				uuid: spawnOptions.uuid,
				location: process.argv[ 2 ]
			} );

			// We run the servers first, because the servers have to be there before the clients
			// can run. OTOH, the clients may initiate the termination of the test via a non-error
			// teardown request.
			async.each( serverPaths, function( serverPath, callback ) {
				var serverIndex = 0;
				children.push( spawnOne( assert, _.extend( {}, spawnOptions, {
					name: "Server" + ( serverPaths.length > 1 ? " " + ( ++serverIndex ) : "" ),
					path: serverPath,
					onReady: callback
				} ) ) );
			}, function() {
				var clientIndex = 0;
				async.eachSeries( clientPaths, function startOneChild( item, callback ) {
					children.push( spawnOne( assert, _.extend( {}, spawnOptions, {
						name: "Client" +
							( clientPaths.length > 1 ? " " + ( ++clientIndex ) : "" ),
					path: item } ) ) );

					setTimeout( callback, 0 );
				} );
			} );
		} );
	} );
}

// Run tests. If no tests were specified on the command line, we scan the tests directory and run
// all the tests we find therein. If the command line starts with a "!", we subtract from the
// contents of the tests/ the list specified on the command line.
runTestSuites( ( function assembleTestList( inSubdirectory ) {
		return ( process.argv.length > 3 ) ?
			( process.argv[ 3 ].substr( 0, 1 ) === "!" ) ?

				// Test list was given and starts with a "!" - subtract tests from list.
				_.without.apply( _,
					[ inSubdirectory ].concat( process.argv[ 3 ].substr( 1 ).split( "," ) ) ) :

				// Test list was given and does not start with a "!" - use.
				process.argv[ 3 ].split( "," ) :

			// Test list was not given - use subdirectory contents.
			inSubdirectory;
	} )( fs.readdirSync( path.join( __dirname, "tests" ) ) )

	// Convert test list to list of absolute paths to the test directories.
	.map( function( item ) {
		return path.join( __dirname, "tests", item );
	} ) );

process.on( "exit", function() {
	var childIndex;

	for ( childIndex in runningProcesses ) {
		runningProcesses[ childIndex ].kill( "SIGTERM" );
	}
} );
