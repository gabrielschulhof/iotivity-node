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
#include "../common.h"
#include "../structures/handles.h"

extern "C" {
#include <ocstack.h>
}

#define GET_STRING_COUNT(api)                                                 \
  do {                                                                        \
    VALIDATE_ARGUMENT_COUNT(env, info, 2);                                    \
    napi_value arguments[2];                                                  \
    napi_get_cb_args(env, info, arguments, 2);                                \
    VALIDATE_ARGUMENT_TYPE(env, arguments, 0, napi_object);                   \
    VALIDATE_ARGUMENT_TYPE(env, arguments, 1, napi_object);                   \
                                                                              \
    uint8_t interfaceCount = 0;                                               \
    OCStackResult result;                                                     \
                                                                              \
    CallbackInfo<OCResourceHandle> *callbackInfo;                             \
    JSCALLBACKHANDLE_RESOLVE(JSOCResourceHandle, callbackInfo, env,           \
                             arguments[0]);                                   \
    result = api(callbackInfo->handle, &interfaceCount);                      \
                                                                              \
    if (result == OC_STACK_OK) {                                              \
      napi_set_property(env, arguments[1], napi_property_name(env, "count"),  \
                        napi_create_number(env, interfaceCount));             \
    }                                                                         \
                                                                              \
    napi_set_return_value(env, info, napi_create_number(env, result));        \
  } while (0)

NAPI_METHOD(bind_OCGetNumberOfResourceInterfaces) {
  GET_STRING_COUNT(OCGetNumberOfResourceInterfaces);
}

NAPI_METHOD(bind_OCGetNumberOfResourceTypes) {
  GET_STRING_COUNT(OCGetNumberOfResourceTypes);
}

#define RETURN_RESOURCE_HANDLE(handle)                                          \
  do {                                                                          \
    OCResourceHandle localHandle = (handle);                                    \
    if (localHandle) {                                                          \
      if (!JSOCResourceHandle::handles[localHandle]) {                          \
        napi_throw_error(env, (char *)"JS handle not found for native handle"); \
        return;                                                                 \
      }                                                                         \
      napi_set_return_value(env, info,                                          \
        napi_get_persistent_value(env,                                          \
                                  JSOCResourceHandle::handles[localHandle]));   \
    } else {                                                                    \
      napi_set_return_value(env, info, napi_get_null(env));                     \
    }                                                                           \
  } while (0)

NAPI_METHOD(bind_OCGetResourceHandle) {
  VALIDATE_ARGUMENT_COUNT(env, info, 1);

  napi_value arguments[1];
  napi_get_cb_args(env, info, arguments, 1);

  VALIDATE_ARGUMENT_TYPE(env, arguments, 0, napi_number);

  RETURN_RESOURCE_HANDLE(
      OCGetResourceHandle((uint8_t)(napi_get_value_uint32(env, arguments[0]))));
}

#define RESOURCE_BY_INDEX_ACCESSOR_BOILERPLATE()             \
  VALIDATE_ARGUMENT_COUNT(env, info, 2);                     \
  napi_value arguments[2];                                   \
  napi_get_cb_args(env, info, arguments, 2);                 \
  VALIDATE_ARGUMENT_TYPE(env, arguments, 0, napi_object);    \
  VALIDATE_ARGUMENT_TYPE(env, arguments, 1, napi_number);    \
  CallbackInfo<OCResourceHandle> *callbackInfo;              \
  JSCALLBACKHANDLE_RESOLVE(JSOCResourceHandle, callbackInfo, \
                           env, arguments[0]);

NAPI_METHOD(bind_OCGetResourceHandleFromCollection) {
  RESOURCE_BY_INDEX_ACCESSOR_BOILERPLATE();

  RETURN_RESOURCE_HANDLE(OCGetResourceHandleFromCollection(
      callbackInfo->handle, napi_get_value_uint32(env, arguments[1])));
}

#define GET_STRING_FROM_RESOURCE_BY_INDEX_BOILERPLATE(apiFunction)     \
  RESOURCE_BY_INDEX_ACCESSOR_BOILERPLATE();                            \
  const char *resultOf##apiFunction = apiFunction(                     \
      callbackInfo->handle, napi_get_value_uint32(env, arguments[1])); \
  napi_set_return_value(env, info, (resultOf##apiFunction ?            \
    napi_create_string(env, resultOf##apiFunction) :                   \
    napi_get_null(env)));

NAPI_METHOD(bind_OCGetResourceInterfaceName) {
  GET_STRING_FROM_RESOURCE_BY_INDEX_BOILERPLATE(OCGetResourceInterfaceName);
}

NAPI_METHOD(bind_OCGetResourceTypeName) {
  GET_STRING_FROM_RESOURCE_BY_INDEX_BOILERPLATE(OCGetResourceTypeName);
}

#define LONE_ARGUMENT_IS_RESOURCE_HANDLE_BOILERPLATE()       \
  VALIDATE_ARGUMENT_COUNT(env, info, 1);                     \
  napi_value argument;                                       \
  napi_get_cb_args(env, info, &argument, 1);                 \
  VALIDATE_ARGUMENT_TYPE(env, &argument, 0, napi_object);    \
  CallbackInfo<OCResourceHandle> *callbackInfo;              \
  JSCALLBACKHANDLE_RESOLVE(JSOCResourceHandle, callbackInfo, \
                           env, argument);

NAPI_METHOD(bind_OCGetResourceProperties) {
  LONE_ARGUMENT_IS_RESOURCE_HANDLE_BOILERPLATE();

  napi_set_return_value(env, info,
    napi_create_number(env, OCGetResourceProperties(callbackInfo->handle)));
}

NAPI_METHOD(bind_OCGetResourceUri) {
  LONE_ARGUMENT_IS_RESOURCE_HANDLE_BOILERPLATE();

  const char *uri = OCGetResourceUri(callbackInfo->handle);

  napi_set_return_value(env, info,
                        uri ? napi_create_string(env, uri) : napi_get_null(env));
}
