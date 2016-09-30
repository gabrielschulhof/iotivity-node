#include <glib.h>
#include <ocstack.h>
#include <unistd.h>
#include "boilerplate.h"

static gboolean
do_OCProcess(gpointer NOTHING) {
	OCStackResult result = OCProcess();
	if (result != OC_STACK_OK) {
		printf("OCProcess: %d\n", result);
	}
	return true;
}

static FILE *my_open(const char *filename, const char *mode) {
	gboolean needFree = FALSE;
	char *processedFileName = processFileName(filename, &needFree);
	FILE *returnValue = fopen(processedFileName, mode);
	if (needFree) {
		g_free(processedFileName);
	}
	return returnValue;
}

static int my_unlink(const char *filename) {
	gboolean needFree = FALSE;
	char *processedFileName = processFileName(filename, &needFree);
	int returnValue = unlink(processedFileName);
	if (needFree) {
		g_free(processedFileName);
	}
	return returnValue;
}

int
main(int argc, char **argv) {
	GMainLoop *loop = g_main_loop_new(NULL, FALSE);

	OCPersistentStorage storage = {
		.open = my_open,
		.read = fread,
		.write = fwrite,
		.close = fclose,
		.unlink = my_unlink
	};

	printf("OCRegisterPersistentStorageHandler: %d\n",
		OCRegisterPersistentStorageHandler(&storage));

	printf("OCInit: %d\n",
		OCInit(NULL, 0, OC_CLIENT_SERVER));

	g_timeout_add(100, do_OCProcess, NULL);

	doIoT();

	g_main_loop_run(loop);
	return 0;
}
