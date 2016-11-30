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

#include "handles.h"
#include "../common.h"

std::map<OCResourceHandle, napi_persistent> JSOCResourceHandle::handles;

napi_value jsArrayFromBytes(napi_env env, unsigned char *bytes, uint32_t length) {
  uint32_t index;
  napi_value returnValue = napi_create_array_with_length(env, length);

  for (index = 0; index < length; index++) {
    napi_set_element(env, returnValue, index,
                     napi_create_number(env, bytes[index]));
  }
  return returnValue;
}

bool fillCArrayFromJSArray(napi_env env, unsigned char *bytes, uint32_t length,
                           napi_value array) {
  uint32_t index, arrayLength;

  if (!napi_is_array(env, array)) {
    napi_throw_type_error(env, (char *)"not an array");
    return false;
  }

  arrayLength = napi_get_array_length(env, array);
  if (arrayLength != length) {
    napi_throw_error(env, (char *)"byte array has the wrong length");
    return false;
  }

  for (index = 0; index < length; index++) {
    napi_value byte = napi_get_element(env, array, index);
    VALIDATE_VALUE_TYPE(env, byte, napi_number, "byte array item",
                        return false);
    bytes[index] = (unsigned char)(napi_get_value_uint32(env, byte));
  }

  return true;
}
