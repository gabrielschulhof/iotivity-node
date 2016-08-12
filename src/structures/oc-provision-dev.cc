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

extern "C" {
#include <ocstack.h>
}
#include "../common.h"
#include "oc-dev-addr.h"
#include "oc-provision-dev.h"
#include "oc-security.h"

using namespace v8;

Local<Object> js_OCProvisionDev(OCProvisionDev_t *source,
		Local<Object> destination) {
	Nan::Set(destination,
		Nan::New("endpoint").ToLocalChecked(),
		js_OCDevAddr(&(source->endpoint)));
	Nan::Set(destination,
		Nan::New("pstat").ToLocalChecked(),
		js_OicSecPstat(source->pstat));
	Nan::Set(destination,
		Nan::New("doxm").ToLocalChecked(),
		js_OicSecDoxm(source->doxm));
	ADD_VALUE(destination, source, connType, Int32);
	ADD_VALUE(destination, source, securePort, Uint32);
	Nan::Set(destination, Nan::New("secVer").ToLocalChecked(),
		Nan::New(source->secVer).ToLocalChecked());
	ADD_VALUE(destination, source, devStatus, Int32);

	return destination;
}

bool c_OCProvisionDev(v8::Local<v8::Object> jsSource,
		OCProvisionDev_t *destination) {
	OCProvisionDev_t local = {
		.endpoint = {
			.adapter = OC_DEFAULT_ADAPTER,
			.flags = OC_DEFAULT_FLAGS,
			.port = 0,
			.addr = {0},
			.interface = 0,
			.routeData = {0}
		},
		.pstat = 0,
		.doxm = 0,
		.connType = CT_DEFAULT,
		.securePort = 0,
		.secVer = {0},
		.devStatus = (DeviceStatus)0,
		.next = 0
	};

	VALIDATE_AND_CONVERT_STRUCTURE(local, jsSource, endpoint,
		"OCProvisionDev", c_OCDevAddr, false, free_and_fail);
	ALLOCATE_AND_CONVERT_STRUCTURE(local, jsSource, pstat,
			"OCProvisionDev", OicSecPstat_t, c_OicSecPstat, free_and_fail);
	ALLOCATE_AND_CONVERT_STRUCTURE(local, jsSource, doxm,
			"OCProvisionDev", OicSecDoxm_t, c_OicSecDoxm, free_and_fail);
	VALIDATE_AND_ASSIGN(local, jsSource, connType, "OCProvisionDev",
			OCConnectivityType, IsInt32, Int32Value, free_and_fail);
	VALIDATE_AND_ASSIGN(local, jsSource, securePort, "OCProvisionDev",
			uint16_t, IsUint32, Uint32Value, free_and_fail);
	VALIDATE_AND_COPY_STRING(local, jsSource, secVer,
			"OCProvisionDev", MAX_VERSION_LEN, free_and_fail);
	VALIDATE_AND_ASSIGN(local, jsSource, devStatus, "OCProvisionDev",
			DeviceStatus, IsUint32, Uint32Value, free_and_fail);

	*destination = local;
	return true;

free_and_fail:
	c_OCProvisionDev_freeMembers(&local);
	return false;
}

void c_OCProvisionDev_freeMembers(OCProvisionDev_t *device) {
	if (device->pstat) {
		delete device->pstat;
	}
	if (device->doxm) {
		delete device->doxm;
	}
}
