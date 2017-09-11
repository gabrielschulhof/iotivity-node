#include <ocstack.h>
#include "boilerplate.h"

static OCEntityHandlerResult
entityHandler(OCEntityHandlerFlag flag, OCEntityHandlerRequest *request,
		void *NOTHING) {
	OCEntityHandlerResponse response = {
		request->requestHandle,
		request->resource,
		OC_EH_OK,
		NULL,
		0,
		{{0, 0}},
		"/a/high-level-example",
		1
	};
	printf("flag: %d, request->method: %d\n", flag, request->method);
	printf("OCDoResponse: %d\n", OCDoResponse(&response));
	return OC_EH_OK;
}

char *processFileName(const char *filename, bool *needFree) {
	*needFree = true;
	return strdup_printf("server.%s", filename);
}

void doIoT() {
	OCResourceHandle handle = 0;

	printf("OCCreateResource: %d\n",
		OCCreateResource(&handle, "core.light", "oic.if.baseline",
			"/a/high-level-example", entityHandler, NULL,
			OC_DISCOVERABLE | OC_OBSERVABLE | OC_SECURE));
}
