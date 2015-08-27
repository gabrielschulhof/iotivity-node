#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <ocstack.h>
#include <signal.h>

static GMainLoop *loop;
static guint timeout_id;
static char *sampleUri = "/a/iotivity-node-presence-sample";

static void deleteContextNoop( void *context ) {}

static void cleanupAndExit( int whatSignal ) {
	int result;

	g_message( "Cleaning up and exiting" );
	g_source_remove( timeout_id );

	result = OCStop();
	g_message( "OCStop: %d", result );

	g_main_loop_quit( loop );
}

static gboolean run_OCProcess( void *nothingHere ) {
	int result = OCProcess();

	if ( result != OC_STACK_OK ) {
		g_warning( "OCProcess: %d, exiting\n", result );
		cleanupAndExit( 0 );
		return FALSE;
	}

	return TRUE;
}

static void dumpIdentity( const char *prefix, OCIdentity *id ) {
	int index;

	printf( "%s: identity:\n", prefix );
	printf( "%s: identity.id_length: %d (vs. max %d)\n", prefix, id->id_length, MAX_IDENTITY_SIZE );
	for ( index = 0 ; index < MIN( id->id_length, MAX_IDENTITY_SIZE ) ; index++ ) {
		printf( "%s: identity.id[%d]: %d\n", prefix, index, id->id[index] );
	}
}

static void dumpResponse( const char *prefix, OCClientResponse *response ) {
	char *new_prefix;

	printf( "%s: response:\n", prefix );
	printf( "%s: ->devAddr:\n", prefix );
	printf( "%s: ->devAddr.adapter: %d\n", prefix, response->devAddr.adapter );
	printf( "%s: ->devAddr.flags: %d\n", prefix, response->devAddr.flags );
	printf( "%s: ->devAddr.interface: %d\n", prefix, response->devAddr.interface );
	printf( "%s: ->devAddr.port: %d\n", prefix, response->devAddr.port );
	printf( "%s: ->devAddr.addr: %s\n", prefix, response->devAddr.addr );

	new_prefix = g_strdup_printf( "%s: ->devAddr", prefix );
	dumpIdentity( new_prefix, &( response->devAddr.identity ) );
	g_free( new_prefix );

	printf( "%s: response->payload: %s\n", prefix, response->payload ? "present": "absent" );
}

static OCStackApplicationResult presenceCallback( void *nothingHere, OCDoHandle handle, OCClientResponse *response ) {
	dumpResponse( "presence", response );

	if ( response->payload ) {
		printf( "presence: response->payload->type: %d\n", response->payload->type );
		if ( response->payload->type == PAYLOAD_TYPE_PRESENCE ) {
			OCPresencePayload *payload = ( OCPresencePayload * )( response->payload );
			printf( "presence: sequenceNumber: %d\n", payload->sequenceNumber );
			printf( "presence: maxAge: %d\n", payload->maxAge );
			printf( "presence: trigger: %d\n", payload->trigger );
			printf( "presence: resourceType: %s\n", payload->resourceType );
		} else {
			g_warning( "What?! payload type is not PAYLOAD_TYPE_PRESENCE?!" );
		}
	}

	return OC_STACK_KEEP_TRANSACTION;
}

static OCStackApplicationResult discoverCallback( void *nothingHere, OCDoHandle handle, OCClientResponse *response ) {
	OCStackApplicationResult returnValue = OC_STACK_KEEP_TRANSACTION;

	dumpResponse( "discovery", response );

	if ( response->payload ) {
		printf( "discovery: response->payload->type: %d\n", response->payload->type );
		if ( response->payload->type == PAYLOAD_TYPE_DISCOVERY ) {
			OCDiscoveryPayload *payload = ( OCDiscoveryPayload * )( response->payload );
			OCResourcePayload *resource;

			for ( resource = payload->resources; resource; resource = resource->next ) {
				if ( resource->uri ) {
					if ( !strcmp( resource->uri, sampleUri ) ) {
						OCStackResult result;
						OCDoHandle presenceHandle;
						OCCallbackData presenceData = { NULL, presenceCallback, deleteContextNoop };

						printf( "discovery: Found resource '%s'\n", sampleUri );

						result = OCDoResource(
							&presenceHandle,
							OC_REST_PRESENCE,
							OC_RSRVD_PRESENCE_URI,
							response->addr,
							NULL,
							CT_DEFAULT,
							OC_HIGH_QOS,
							&presenceData,
							NULL,
							0 );

						g_message( "discovery: OCDoResource(presence): %d", result );

						returnValue = OC_STACK_DELETE_TRANSACTION;
					}
				}
			}
		} else {
			g_warning( "What?! payload type is not PAYLOAD_TYPE_DISCOVERY?!" );
		}
	}
	return returnValue;
}

int main( int argc, char **argv ) {
	OCStackResult result;
	OCDoHandle handle;
	OCCallbackData discoverData = { NULL, discoverCallback, deleteContextNoop };

	loop = g_main_loop_new( NULL, false );
	timeout_id = g_timeout_add( 1000, run_OCProcess, NULL );
	signal( SIGINT, cleanupAndExit );

	result = OCInit( NULL, 0, OC_CLIENT );
	g_message( "OCInit: %d", result );

	result = OCDoResource(
		&handle,
		OC_REST_DISCOVER,
		OC_RSRVD_WELL_KNOWN_URI,
		NULL,
		NULL,
		CT_DEFAULT,
		OC_HIGH_QOS,
		&discoverData,
		NULL,
		0 );
	g_message( "OCDoResource(discovery): %d", result );

	g_main_loop_run( loop );
	return 0;
}
