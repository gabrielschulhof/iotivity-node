#include <glib.h>
#include <ocstack.h>
#include "boilerplate.h"

static OCEntityHandlerResult
entityHandler(OCEntityHandlerFlag flag, OCEntityHandlerRequest *request,
		void *NOTHING) {
	printf("flag: %d, request->method: %d\n", flag, request->method);
	return OC_EH_OK;
}

static gboolean
setupResource(gpointer NOTHING) {
	OCResourceHandle handle = 0;

	printf("OCCreateResource: %d\n",
		OCCreateResource(&handle, "core.light", "oic.if.baseline",
			"/a/client-test", entityHandler, NULL,
			OC_DISCOVERABLE | OC_OBSERVABLE | OC_SECURE));

	return false;
}

char *processFileName(const char *filename, gboolean *needFree) {
	*needFree = TRUE;
	return g_strdup_printf("server.%s", filename);
}

void doIoT() {
	g_idle_add(setupResource, NULL);
}
