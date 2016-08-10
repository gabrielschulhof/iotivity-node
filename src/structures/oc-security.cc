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

#include <nan.h>

extern "C" {
#include <ocstack.h>
}

#include "oc-security.h"
#include "../common.h"
#include "oc-sid.h"

using namespace v8;

#define SET_SID_FROM_OICUUID_T(destination, source, member) \
	Nan::Set((destination), Nan::New(#member).ToLocalChecked(), \
		js_SID((char *)((source)->member.id)));

Local<Object> js_OicSecPstat(OicSecPstat_t *source,
		Local<Object> destination) {

	SET_VALUE_ON_OBJECT(destination, Boolean, source, isOp);
	SET_VALUE_ON_OBJECT(destination, Int32, source, cm);
	SET_VALUE_ON_OBJECT(destination, Int32, source, tm);
	SET_SID_FROM_OICUUID_T(destination, source, deviceID);
	SET_VALUE_ON_OBJECT(destination, Int32, source, om);

	size_t index;
	Local<Array> jsSm = Nan::New<Array>(source->smLen);
	for (index = 0; index < source->smLen; index++) {
		Nan::Set(jsSm, index, Nan::New(source->sm[index]));
	}
	Nan::Set(destination, Nan::New("sm").ToLocalChecked(), jsSm);

	SET_VALUE_ON_OBJECT(destination, Uint32, source, commitHash);
	SET_SID_FROM_OICUUID_T(destination, source, rownerID);

	return destination;
}

Local<Object> js_OicSecDoxm(OicSecDoxm_t *source,
		Local<Object> destination) {
	size_t index;

	Local<Array> jsOxmType = Nan::New<Array>(source->oxmTypeLen);
	for (index = 0; index < source->oxmTypeLen; index++) {
		Nan::Set(jsOxmType, index,
			Nan::New(source->oxmType[index]).ToLocalChecked());
	}
	Nan::Set(destination, Nan::New("oxmType").ToLocalChecked(), jsOxmType);

	Local<Array> jsOxm = Nan::New<Array>(source->oxmLen);
	for (index = 0; index < source->oxmLen; index++) {
		Nan::Set(jsOxm, index, Nan::New(source->oxm[index]));
	}
	Nan::Set(destination, Nan::New("oxm").ToLocalChecked(), jsOxm);

	SET_VALUE_ON_OBJECT(destination, Int32, source, oxmSel);
	SET_VALUE_ON_OBJECT(destination, Int32, source, sct);
	SET_VALUE_ON_OBJECT(destination, Boolean, source, owned);

	SET_SID_FROM_OICUUID_T(destination, source, deviceID);

	SET_VALUE_ON_OBJECT(destination, Boolean, source, dpc);

	SET_SID_FROM_OICUUID_T(destination, source, owner);	
	SET_SID_FROM_OICUUID_T(destination, source, rownerID);	

	return destination;
}
