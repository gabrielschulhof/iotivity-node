/*
 * Copyright 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../structures/oc-provision-dev.h"
#include "oc-provisioning-ownership-transfer.h"

extern "C" {
#include <ocstack.h>
#include <ocprovisioningmanager.h>
}

#include "../common.h"
#include "../structures/oc-security.h"

using namespace v8;
using namespace node;

struct CallbackInfo {
	Nan::Callback *jsCallback;
	OCProvisionDev_t **devices;
};

void defaultOwnershipTransferCallback(void *context, int numberOfResults,
		OCProvisionResult_t *arr, bool hasError) {
	Nan::HandleScope scope;

	CallbackInfo *info = (CallbackInfo *)context;

	Local<Array> results = Nan::New<Array>(numberOfResults);
	int index;
	for (index = 0; index < numberOfResults; index++) {
		Nan::Set(results, index, js_OCProvisionResult(&(arr[index])));
	}
	Local<Value> arguments[2] = {
		results, Nan::New(hasError)
	};

	info->jsCallback->Call(2, arguments);

	free(info->devices);
	delete info->jsCallback;
	delete info;
}

NAN_METHOD(bind_OCDoOwnershipTransfer) {
/*
	VALIDATE_ARGUMENT_COUNT(info, 2);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsArray);
	VALIDATE_ARGUMENT_TYPE(info, 1, IsFunction);

	CallbackInfo *callbackInfo = new CallbackInfo;
	if (!callbackInfo) {
		Nan::ThrowError("Failed to allocate memory for callback info");
		goto throw_error;
	}

	callbackInfo->jsCallback =
		new Nan::Callback(Local<Function>::Cast(info[1]));
	if (!callbackInfo->jsCallback) {
		Nan::ThrowError("Failed to allocate memory for Javascript callback");
		goto delete_info;
	}

	Local<Array> jsDevices = Local<Array>::Cast(info[0]);
	size_t index, length = jsDevices->Length();
	callbackInfo->devices =
		(OCProvisionDev_t **)malloc(length * sizeof(OCProvisionDev_t *));
	if (!(callbackInfo->devices)) {
		Nan::ThrowError("Failed to allocate memory for device list");
		goto delete_callback;
	}
	for (index = 0; index < length; index++) {
		Local<Value> jsDevice = Nan::Get(jsDevices, index).ToLocalChecked();
		if (!jsDevice->IsObject()) {
			Nan::ThrowError("Device list item must satisfy IsObject");
			goto free_devices;
		}
		OCProvisionDev_t *device = 0;
		if (!device) {
			goto free_devices;
		}
		callbackInfo->devices[index] = device;
	}

	OCStackResult result = OCDoOwnershipTransfer(callbackInfo,
		callbackInfo->devices, defaultOwnershipTransferCallback);

	info.GetReturnValue().Set(Nan::New(result));

	if (result == OC_STACK_OK) {
		return;
	}

free_devices:
	free(callbackInfo->devices);
delete_callback:
	delete callbackInfo->jsCallback;
delete_info:
	delete callbackInfo;
*/
}
