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
	SET_VALUE_ON_OBJECT(destination, Int32, source, connType);
	SET_VALUE_ON_OBJECT(destination, Uint32, source, securePort);

	return destination;
}
