#include <glib.h>
#include <ocstack.h>

#define DESIRED_RESOURCE "/a/client-test"

static OCDevAddr desiredLocation;

static OCStackApplicationResult
requestCallback(void *NOTHING, OCDoHandle handle, OCClientResponse *response) {
	printf("response->result: %d\n", response ? response->result : -1);
	return OC_STACK_KEEP_TRANSACTION;
}

static gboolean
performRequest(gpointer NOTHING) {
	OCDoHandle handle = NULL;
	OCCallbackData callback = {
		.context = NULL,
		.cb = requestCallback,
		.cd = NULL,
	};
	printf("OCDoResource(request): %d\n",
		OCDoResource(&handle, OC_REST_OBSERVE, DESIRED_RESOURCE, &desiredLocation,
			NULL, CT_DEFAULT, OC_HIGH_QOS, &callback, NULL, 0));
	return false;
}

static OCStackApplicationResult
discoverCallback(void *NOTHING, OCDoHandle handle, OCClientResponse *response) {
	OCResourcePayload *resource = NULL;
	if (response && response->result == OC_STACK_OK && response->payload &&
			response->payload->type == PAYLOAD_TYPE_DISCOVERY) {
		for (resource = ((OCDiscoveryPayload *)(response->payload))->resources;
				resource; resource = resource->next) {
			if (resource->uri && !g_strcmp0(resource->uri, DESIRED_RESOURCE)) {
				desiredLocation = response->devAddr;
				printf("Resource found\n");
				g_idle_add(performRequest, NULL);
				return OC_STACK_DELETE_TRANSACTION;
			}
		}
	}
	return OC_STACK_KEEP_TRANSACTION;
}

static gboolean
discoverResource(gpointer NOTHING) {
	OCDoHandle handle = NULL;
	OCCallbackData callback = {
		.context = NULL,
		.cb = discoverCallback,
		.cd = NULL
	};
	printf("OCDoResource(discovery): %d\n",
		OCDoResource(&handle, OC_REST_DISCOVER, OC_MULTICAST_DISCOVERY_URI,
			NULL, NULL, CT_DEFAULT, OC_HIGH_QOS, &callback, NULL, 0));
	return false;
}

void doIoT() {
	g_idle_add(discoverResource, NULL);
}

char *processFileName(const char *filename, gboolean *needFree) {
	*needFree = true;
	return g_strdup_printf("client.%s", filename);
}
