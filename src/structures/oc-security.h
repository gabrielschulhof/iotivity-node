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

#ifndef __IOTIVITY_NODE_OCSECURITY_H__
#define __IOTIVITY_NODE_OCSECURITY_H__

#include <v8.h>
#include <nan.h>
extern "C" {
#include <pmtypes.h>
}

v8::Local<v8::Object> js_OicSecPstat(OicSecPstat_t *source,
	v8::Local<v8::Object> destination = Nan::New<v8::Object>());

v8::Local<v8::Object> js_OicSecDoxm(OicSecDoxm_t *source,
	v8::Local<v8::Object> destination = Nan::New<v8::Object>());

v8::Local<v8::Object> js_OCProvisionResult(OCProvisionResult_t *source,
	v8::Local<v8::Object> destination = Nan::New<v8::Object>());

bool c_OicSecPstat(v8::Local<v8::Object> jsSource, OicSecPstat_t *destination);

bool c_OicSecDoxm(v8::Local<v8::Object> jsSource, OicSecDoxm_t *destination);
#endif /* __IOTIVITY_NODE_OCSECURITY_H__ */
