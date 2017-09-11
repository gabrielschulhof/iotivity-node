#include <string.h>
#include <stdlib.h>
#include <ocstack.h>
#include "boilerplate.h"

#define DESIRED_RESOURCE "/a/high-level-example"

static char *desiredUrl = NULL;

static OCStackApplicationResult
requestCallback(void *NOTHING, OCDoHandle handle, OCClientResponse *response) {
	printf("response->result: %d\n", response ? response->result : -1);
	return OC_STACK_KEEP_TRANSACTION;
}

static void
performRequest() {
	OCDoHandle handle = NULL;
	OCCallbackData callback = {
		.context = NULL,
		.cb = requestCallback,
		.cd = NULL,
	};
	printf("OCDoResource(request) to %s: %d\n", desiredUrl,
		OCDoResource(&handle, OC_REST_GET, desiredUrl, NULL,
			NULL, CT_DEFAULT, OC_HIGH_QOS, &callback, NULL, 0));
	free(desiredUrl);
}

static OCStackApplicationResult
discoverCallback(void *NOTHING, OCDoHandle handle, OCClientResponse *response) {
	OCResourcePayload *resource = NULL;
	if (response && response->result == OC_STACK_OK && response->payload &&
			response->payload->type == PAYLOAD_TYPE_DISCOVERY) {
		for (resource = ((OCDiscoveryPayload *)(response->payload))->resources;
				resource; resource = resource->next) {
			if (resource->uri && !strcmp(resource->uri, DESIRED_RESOURCE)) {
				for (OCEndpointPayload *ep = resource->eps; ep; ep = ep->next) {
					if (!desiredUrl && !strcmp(ep->tps, "coaps")) {
						char *percent = strstr(ep->addr, "%");
						if (percent) {
							*percent = 0;
						}
						desiredUrl = strdup_printf("%s://%s%s%s:%d%s",
							ep->tps,
							(ep->family & OC_IP_USE_V6 ? "[" : ""),
							ep->addr,
							(ep->family & OC_IP_USE_V6 ? "]" : ""),
							ep->port,
							DESIRED_RESOURCE);
					}
					printf("Resource endpoint: %s://%s%s%s:%d\n",
						ep->tps,
						(ep->family & OC_IP_USE_V6 ? "[" : ""),
						ep->addr,
						(ep->family & OC_IP_USE_V6 ? "]" : ""),
						ep->port);
				}
				printf("Resource found\n");
				performRequest();
				return OC_STACK_DELETE_TRANSACTION;
			}
		}
	}
	return OC_STACK_KEEP_TRANSACTION;
}

void doIoT() {
	OCDoHandle handle = NULL;
	OCCallbackData callback = {
		.context = NULL,
		.cb = discoverCallback,
		.cd = NULL
	};
	printf("OCDoResource(discovery): %d\n",
		OCDoResource(&handle, OC_REST_DISCOVER, OC_MULTICAST_DISCOVERY_URI,
			NULL, NULL, CT_DEFAULT, OC_HIGH_QOS, &callback, NULL, 0));
}

char *processFileName(const char *filename, bool *needFree) {
	*needFree = true;
	return strdup_printf("client.%s", filename);
}
