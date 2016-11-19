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

#include <node_jsvmapi.h>
#include <node_api_helpers.h>
#include "../common.h"
#include "../structures/oc-device-info.h"
#include "../structures/oc-platform-info.h"

extern "C" {
#include <ocstack.h>
}

using namespace v8;

NAPI_METHOD(bind_OCInit) {
  VALIDATE_ARGUMENT_COUNT(env, info, 3);

  napi_value arguments[3];
  napi_get_cb_args(env, info, arguments, 3);

  VALIDATE_ARGUMENT_TYPE_OR_NULL(env, arguments, 0, napi_string);
  VALIDATE_ARGUMENT_TYPE(env, arguments, 1, napi_number);
  VALIDATE_ARGUMENT_TYPE(env, arguments, 2, napi_number);

  int ipAddr_length = 0;
  bool ipAddr_isString =
  	( napi_get_type_of_value(env, arguments[0]) == napi_string );
  if (ipAddr_isString) {
  	ipAddr_length = napi_get_string_length(env, arguments[0]);
  }
  char ipAddr[ipAddr_length + 1] = {0};
  if ( ipAddr_isString) {
  	napi_get_string_from_value(env, arguments[0], ipAddr, ipAddr_length);
  }

  napi_set_return_value(env, info, napi_create_number(env, (double)OCInit(
      (const char *)(ipAddr_isString ? ipAddr : 0),
      napi_get_value_uint32(env, arguments[1]),
      (OCMode)napi_get_value_uint32(env, arguments[2]))));
}

#define SIMPLE_METHOD(env, info, api) \
	napi_set_return_value((env), (info), \
		napi_create_number((env), (double)api()));

NAPI_METHOD(bind_OCStop) {
	SIMPLE_METHOD(env, info, OCStop);
}

NAPI_METHOD(bind_OCProcess) {
	SIMPLE_METHOD(env, info, OCProcess);
}

NAPI_METHOD(bind_OCStopPresence) {
  SIMPLE_METHOD(env, info, OCStopPresence);
}

NAPI_METHOD(bind_OCStartPresence) {
	VALIDATE_ARGUMENT_COUNT(env, info, 1);

  napi_value arguments[1];
  napi_get_cb_args(env, info, arguments, 1);

	VALIDATE_ARGUMENT_TYPE(env, arguments, 0, napi_number);
	
  napi_set_return_value(env, info,
  	napi_create_number(env, (double)OCStartPresence(
		napi_get_value_uint32(env, arguments[0]))));
}

NAPI_METHOD(bind_OCGetServerInstanceIDString) {
	VALIDATE_ARGUMENT_COUNT(env, info, 0);
	const char *idString = OCGetServerInstanceIDString();
	napi_set_return_value(env, info, idString ?
		napi_create_string(env, idString) :
		napi_get_null(env));
}


NAPI_METHOD(bind_OCSetDeviceInfo) {
  VALIDATE_ARGUMENT_COUNT(env, info, 1);

  napi_value arguments[1];
  napi_get_cb_args(env, info, arguments, 1);

  VALIDATE_ARGUMENT_TYPE(env, arguments, 0, napi_object);

  OCDeviceInfo deviceInfo;

  if (!c_OCDeviceInfo(env, arguments[ 0 ], &deviceInfo)) {
    return;
  }

  OCStackResult result = OCSetDeviceInfo(deviceInfo);

  c_OCDeviceInfoFreeMembers(&deviceInfo);

	napi_set_return_value(env, info, napi_create_number(env, result));
}

NAPI_METHOD(bind_OCSetPlatformInfo) {
  VALIDATE_ARGUMENT_COUNT(env, info, 1);

  napi_value arguments[1];
  napi_get_cb_args(env, info, arguments, 1);

  VALIDATE_ARGUMENT_TYPE(env, arguments, 0, napi_object);

  OCPlatformInfo platformInfo;

  if (!c_OCPlatformInfo(env, arguments[0], &platformInfo)) {
    return;
  }

  OCStackResult result = OCSetPlatformInfo(platformInfo);

  c_OCPlatformInfoFreeMembers(&platformInfo);

  napi_set_return_value(env, info, napi_create_number(env, result));
}

NAPI_METHOD(bind_OCGetNumberOfResources) {
  VALIDATE_ARGUMENT_COUNT(env, info, 1);

  napi_value arguments[1];
  napi_get_cb_args(env, info, arguments, 1);

  VALIDATE_ARGUMENT_TYPE(env, arguments, 0, napi_object);

  OCStackResult result;
  uint8_t resourceCount = 0;

  result = OCGetNumberOfResources(&resourceCount);

  if (result == OC_STACK_OK) {

    napi_set_property(env, arguments[0], napi_property_name(env, "count"),
      napi_create_number(env, resourceCount));    
  }

  napi_set_return_value(env, info, napi_create_number(env, result));
}
