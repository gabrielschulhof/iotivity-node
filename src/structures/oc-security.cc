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

#define JS_SET_SID_FROM_OICUUID_T(destination, source, member) \
	Nan::Set((destination), Nan::New(#member).ToLocalChecked(), \
		js_SID((char *)((source)->member.id)));

#define C_SET_OICUUID_T_FROM_SID(destination, source, member, message, failLabel) \
	do { \
		Local<Value> jsMember = Nan::Get(source, \
			Nan::New(#member).ToLocalChecked()).ToLocalChecked(); \
		VALIDATE_STRING_AND_LENGTH(jsMember, UUID_LENGTH, message, failLabel); \
		strcpy((char *)((destination).member.id), \
			(const char *)*String::Utf8Value(jsMember)); \
	} while(0)

Local<Object> js_OicSecPstat(OicSecPstat_t *source,
		Local<Object> destination) {

	ADD_VALUE(destination, source, isOp, Boolean);
	ADD_VALUE(destination, source, cm, Int32);
	ADD_VALUE(destination, source, tm, Int32);
	JS_SET_SID_FROM_OICUUID_T(destination, source, deviceID);
	ADD_VALUE(destination, source, om, Int32);

	size_t index;
	Local<Array> jsSm = Nan::New<Array>(source->smLen);
	for (index = 0; index < source->smLen; index++) {
		Nan::Set(jsSm, index, Nan::New(source->sm[index]));
	}
	Nan::Set(destination, Nan::New("sm").ToLocalChecked(), jsSm);

	ADD_VALUE(destination, source, commitHash, Uint32);
	JS_SET_SID_FROM_OICUUID_T(destination, source, rownerID);

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

	ADD_VALUE(destination, source, oxmSel, Int32);
	ADD_VALUE(destination, source, sct, Int32);
	ADD_VALUE(destination, source, owned, Boolean);

	JS_SET_SID_FROM_OICUUID_T(destination, source, deviceID);

	ADD_VALUE(destination, source, dpc, Boolean);

	JS_SET_SID_FROM_OICUUID_T(destination, source, owner);	
	JS_SET_SID_FROM_OICUUID_T(destination, source, rownerID);	

	return destination;
}

Local<Object> js_OCProvisionResult(OCProvisionResult_t *source,
		Local<Object> destination) {

	JS_SET_SID_FROM_OICUUID_T(destination, source, deviceId);
	ADD_VALUE(destination, source, res, Int32);

	return destination;
}

bool c_OicSecPstat(Local<Object> jsSource, OicSecPstat_t *destination) {
	OicSecPstat_t local;

	VALIDATE_AND_ASSIGN(local, jsSource, isOp, "OicSecPstat",
		bool, IsBoolean, BooleanValue, fail);
	VALIDATE_AND_ASSIGN(local, jsSource, cm, "OicSecPstat",
		OicSecDpm_t, IsInt32, Int32Value, fail);
	VALIDATE_AND_ASSIGN(local, jsSource, tm, "OicSecPstat",
		OicSecDpm_t, IsInt32, Int32Value, fail);

	C_SET_OICUUID_T_FROM_SID(local, jsSource, deviceID,
			"OicSecPstat.deviceID", fail);

	VALIDATE_AND_ASSIGN(local, jsSource, om, "OicSecPstat",
		OicSecDpom_t, IsInt32, Int32Value, fail);
	VALIDATE_AND_ASSIGN(local, jsSource, commitHash, "OicSecPstat",
		uint16_t, IsUint32, Uint32Value, fail);

	C_SET_OICUUID_T_FROM_SID(local, jsSource, rownerID,
			"OicSecPstat.rownerID", fail);

	*destination = local;
	return true;
fail:
	return false;
}
