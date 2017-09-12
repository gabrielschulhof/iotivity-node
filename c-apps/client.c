#include <string.h>
#include <stdlib.h>
#include <ocstack.h>
#include "boilerplate.h"

#define DESIRED_RESOURCE "/a/high-level-example"

static OCStackApplicationResult
requestCallback(void *url, OCDoHandle handle, OCClientResponse *response) {
	printf("%s: response->result: %d\n", (char *)url,
			response ? response->result : -1);
	return OC_STACK_KEEP_TRANSACTION;
}

static void
performRequest(const char *desiredUrl) {
	OCDoHandle handle = NULL;
	OCCallbackData callback = {
		.context = desiredUrl,
		.cb = requestCallback,
		.cd = free,
	};
	printf("OCDoResource(request) to %s: %d\n", desiredUrl,
		OCDoResource(&handle, OC_REST_GET, desiredUrl, NULL,
			NULL, CT_DEFAULT, OC_HIGH_QOS, &callback, NULL, 0));
}

static char *epToUrl(OCEndpointPayload *ep) {
	char *percent = strstr(ep->addr, "%");
	if (percent) {
		*percent = 0;
	}
	return strdup_printf("%s://%s%s%s:%d%s",
		ep->tps,
		(ep->family & OC_IP_USE_V6 ? "[" : ""),
		ep->addr,
		(ep->family & OC_IP_USE_V6 ? "]" : ""),
		ep->port,
		DESIRED_RESOURCE);
}

static OCStackApplicationResult
discoverCallback(void *NOTHING, OCDoHandle handle, OCClientResponse *response) {
	char *coapsUrl = NULL, *coapUrl = NULL;
	OCResourcePayload *resource = NULL;
	if (response && response->result == OC_STACK_OK && response->payload &&
			response->payload->type == PAYLOAD_TYPE_DISCOVERY) {
		for (resource = ((OCDiscoveryPayload *)(response->payload))->resources;
				resource; resource = resource->next) {
			if (resource->uri && !strcmp(resource->uri, DESIRED_RESOURCE)) {
				for (OCEndpointPayload *ep = resource->eps; ep; ep = ep->next) {
					if (!coapsUrl && !strcmp(ep->tps, "coaps")) {
						coapsUrl = epToUrl(ep);
					} else if (!coapUrl && !strcmp(ep->tps, "coap")) {
						coapUrl = epToUrl(ep);
					}
					if (coapsUrl && coapUrl) {
						break;
					}
				}
				if (coapsUrl) {
					performRequest(coapsUrl);
				}
				if (coapUrl) {
					performRequest(coapUrl);
				}
				printf("Resource found\n");
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
