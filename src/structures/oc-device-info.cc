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

#include "oc-device-info.h"
#include "../common.h"

extern "C" {
#include <string.h>
}

using namespace v8;

napi_value js_OCDeviceInfo(napi_env env, OCDeviceInfo *info) {
  napi_value returnValue = napi_create_object(env);

  SET_STRING_IF_NOT_NULL(env, returnValue, info, deviceName);
  SET_STRING_IF_NOT_NULL(env, returnValue, info, specVersion);
  ADD_STRING_ARRAY(env, returnValue, info, types);
  ADD_STRING_ARRAY(env, returnValue, info, dataModelVersions);

  return returnValue;
}

void c_freeLinkedList(OCStringLL *list) {
  OCStringLL *item, *nextItem;
  for (item = list; item;) {
    nextItem = item->next;
    free(item->value);
    free(item);
    item = nextItem;
  }
}

void c_OCDeviceInfoFreeMembers(OCDeviceInfo *info) {
  free(info->deviceName);
  free(info->specVersion);
  c_freeLinkedList(info->types);
  c_freeLinkedList(info->dataModelVersions);
}

bool c_StringArrayFromProperty(napi_env env, napi_value source,
                               const char *propertyName,
                               OCStringLL **destination) {
  napi_value sourceValue =
      napi_get_property(env, source, napi_property_name(env, propertyName));
  if (!napi_is_array(env, sourceValue)) {
    napi_throw_type_error(
        env, (std::string("device info ") + propertyName + " must be an array")
                 .c_str());
    return false;
  }
  uint32_t index, length = napi_get_array_length(env, sourceValue);
  OCStringLL *local = 0, **previous = &local;

  for (index = 0; index < length; index++, previous = &((*previous)->next)) {
    napi_value itemValue = napi_get_element(env, sourceValue, index);
    VALIDATE_VALUE_TYPE(
        env, itemValue, napi_string,
        (std::string("device info ") + propertyName + " list item").c_str(),
        goto free);

    (*previous) = new OCStringLL;
    if (!(*previous)) {
      goto free;
    }
    (*previous)->next = 0;
    (*previous)->value = strdup((const char *)*(String::Utf8Value(itemValue)));
    if (!(*previous)->value) {
      goto free;
    }
  }

  *destination = local;
  return true;
free:
  c_freeLinkedList(local);
  return false;
}

bool c_OCDeviceInfo(Local<Object> deviceInfo, OCDeviceInfo *info) {
  OCDeviceInfo local = {0, 0, 0, 0};

  VALIDATE_AND_ASSIGN_STRING(&local, deviceInfo, deviceName, goto free);
  VALIDATE_AND_ASSIGN_STRING(&local, deviceInfo, specVersion, goto free);

  // Make sure the "types" property is an array and copy it to the C structure
  if (!c_StringArrayFromProperty(deviceInfo, "types", &(local.types))) {
    goto free;
  }

  // Make sure the "dataModelVersion" property is an array and copy it to the C
  // structure
  if (!c_StringArrayFromProperty(deviceInfo, "dataModelVersions",
                                 &(local.dataModelVersions))) {
    goto free;
  }

  // If we've successfully created the structure, we transfer it to the
  // passed-in structure
  *info = local;
  return true;
free:
  c_OCDeviceInfoFreeMembers(&local);
  return false;
}
