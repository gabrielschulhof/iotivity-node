#include <glib.h>
#include <ocstack.h>
#include "boilerplate.h"

static OCResourceHandle resource = NULL;

static OCEntityHandlerResult
entityHandler(OCEntityHandlerFlag flag, OCEntityHandlerRequest *request,
		void *NOTHING) {
	OCEntityHandlerResponse response = {
		.requestHandle = request->requestHandle,
		.resourceHandle = request->resource,
		.ehResult = OC_EH_OK,
		.payload = NULL,
		.numSendVendorSpecificHeaderOptions = 0,
		.sendVendorSpecificHeaderOptions = { { 0, 0, 0, "" } },
		.resourceUri = "/direct",
		.persistentBufferFlag = true	
	};

	OCStackResult result;

	result = OCNotifyListOfObservers(request->resource, &(request->obsInfo.obsId), 1, NULL, OC_HIGH_QOS);

	printf("entityHandler: notify success status for request handle %p: %d\n", request->requestHandle, result);

	result = OCDoResponse(&response);

	printf("entityHandler: response success status for request handle %p: %d\n", request->requestHandle, result);

	return OC_EH_OK;
}

void doIoT() {
	OCStackResult result = OCCreateResource(&resource, "core.light", "oic.if.baseline",
	  "/direct", entityHandler, NULL, OC_DISCOVERABLE | OC_OBSERVABLE );

	printf("doIoT: create resource success status: %d\n", result);
}

char *processFileName(const char *filename, gboolean *needFree) {
	*needFree = true;
	return g_strdup_printf("server.observe.resource.%s", filename);
}
