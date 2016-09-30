#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <ocstack.h>
#include <signal.h>

static GMainLoop *loop;
static guint timeout_id;
static char *sampleUri = "/a/iotivity-node-presence-sample";

static void cleanupAndExit( int whatSignal ) {
	int result;

	g_message( "Cleaning up and exiting" );
	g_source_remove( timeout_id );

	result = OCStopPresence();
	g_message( "OCStopPresence: %d", result );

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

static OCEntityHandlerResult entityHandler( OCEntityHandlerFlag flag, OCEntityHandlerRequest *request, void *nothingHere ) {
	g_message( "entity handler: entering and returning OC_EH_OK" );
	return OC_EH_OK;
}

int main( int argc, char **argv ) {
	OCStackResult result;
	OCResourceHandle handle;

	loop = g_main_loop_new( NULL, false );
	timeout_id = g_timeout_add( 1000, run_OCProcess, NULL );
	signal( SIGINT, cleanupAndExit );

	result = OCInit( NULL, 0, OC_SERVER );
	g_message( "OCInit: %d", result );

	result = OCCreateResource(
		&handle,
		"core.fan",
		OC_RSRVD_INTERFACE_DEFAULT,
		sampleUri,
		entityHandler,
		NULL,
		OC_DISCOVERABLE );
	g_message( "OCCreateResource: %d", result );

	result = OCStartPresence( 0 );
	g_message( "OCStartPresence: %d", result );

	g_main_loop_run( loop );
	return 0;
}
