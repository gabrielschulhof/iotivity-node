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
#include <string>
#include "../common.h"
#include "../structures/handles.h"

extern "C" {
#include <ocprovisioningmanager.h>
#include <ocstack.h>
}

using namespace v8;

// From
// http://stackoverflow.com/questions/34158318/are-there-some-v8-functions-to-create-a-c-v8date-object-from-a-string-like#answer-36200373
Local<Date> dateFromString(const char *str) {
  static Nan::Callback dateConstructor;
  if (dateConstructor.IsEmpty()) {
    Local<Date> date = Nan::New<Date>(0).ToLocalChecked();
    dateConstructor.Reset(Local<Function>::Cast(
        Nan::Get(date, Nan::New("constructor").ToLocalChecked())
            .ToLocalChecked()));
  }
  Local<Value> jsString = Nan::New(str).ToLocalChecked();
  return Local<Date>::Cast(
      Nan::NewInstance(*dateConstructor, 1, &jsString).ToLocalChecked());
}

NAN_METHOD(bind_OCInit) {
  VALIDATE_ARGUMENT_COUNT(info, 3);
  VALIDATE_ARGUMENT_TYPE_OR_NULL(info, 0, IsString);
  VALIDATE_ARGUMENT_TYPE(info, 1, IsUint32);
  VALIDATE_ARGUMENT_TYPE(info, 2, IsUint32);

  info.GetReturnValue().Set(Nan::New(OCInit(
      (const char *)(info[0]->IsString() ? (*String::Utf8Value(info[0])) : 0),
      (uint16_t)Nan::To<uint32_t>(info[1]).FromJust(),
      (OCMode)Nan::To<uint32_t>(info[2]).FromJust())));
}

NAN_METHOD(bind_OCStop) { info.GetReturnValue().Set(Nan::New(OCStop())); }

NAN_METHOD(bind_OCProcess) { info.GetReturnValue().Set(Nan::New(OCProcess())); }

NAN_METHOD(bind_OCStartPresence) {
  VALIDATE_ARGUMENT_COUNT(info, 1);
  VALIDATE_ARGUMENT_TYPE(info, 0, IsUint32);

  info.GetReturnValue().Set(Nan::New(
      OCStartPresence((uint32_t)Nan::To<uint32_t>(info[0]).FromJust())));
}

NAN_METHOD(bind_OCStopPresence) {
  info.GetReturnValue().Set(Nan::New(OCStopPresence()));
}

NAN_METHOD(bind_OCGetNumberOfResources) {
  VALIDATE_ARGUMENT_COUNT(info, 1);
  VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);

  OCStackResult result;
  uint8_t resourceCount = 0;

  result = OCGetNumberOfResources(&resourceCount);

  if (result == OC_STACK_OK) {
    Nan::Set(Nan::To<Object>(info[0]).ToLocalChecked(),
             Nan::New("count").ToLocalChecked(), Nan::New(resourceCount));
  }

  info.GetReturnValue().Set(Nan::New(result));
}

NAN_METHOD(bind_OCGetServerInstanceIDString) {
  VALIDATE_ARGUMENT_COUNT(info, 0);

  const char *idString = OCGetServerInstanceIDString();

  info.GetReturnValue().Set(idString ? (Nan::New(idString).ToLocalChecked())
                                     : Nan::EmptyString());
}

NAN_METHOD(bind_OCGetPropertyValue) {
  VALIDATE_ARGUMENT_COUNT(info, 3);
  VALIDATE_ARGUMENT_TYPE(info, 0, IsUint32);
  VALIDATE_ARGUMENT_TYPE(info, 1, IsString);
  VALIDATE_ARGUMENT_TYPE(info, 2, IsObject);

  String::Utf8Value propName(info[1]);
  OCPayloadType payloadType =
      (OCPayloadType)Nan::To<uint32_t>(info[0]).FromJust();
  OCStackResult returnValue;
  void *result = nullptr;

  Local<Value> jsResult;

  returnValue =
      OCGetPropertyValue(payloadType, (const char *)*propName, &result);

  if (returnValue == OC_STACK_OK) {
    // string conditions
    if ((payloadType == PAYLOAD_TYPE_DEVICE &&
         !strcmp(*propName, OC_RSRVD_SPEC_VERSION)) ||
        (payloadType == PAYLOAD_TYPE_DEVICE &&
         !strcmp(*propName, OC_RSRVD_DEVICE_NAME)) ||
        (payloadType == PAYLOAD_TYPE_PLATFORM &&
         !strcmp(*propName, OC_RSRVD_MFG_URL)) ||
        (payloadType == PAYLOAD_TYPE_PLATFORM &&
         !strcmp(*propName, OC_RSRVD_MFG_NAME))) {
      jsResult = Nan::New((char *)result).ToLocalChecked();

      // string list conditions
    } else if (payloadType == PAYLOAD_TYPE_DEVICE &&
               !strcmp(*propName, OC_RSRVD_DATA_MODEL_VERSION)) {
      jsResult = js_StringArray((OCStringLL *)result);

      // date conditions
    } else if ((payloadType == PAYLOAD_TYPE_PLATFORM &&
                !strcmp(*propName, OC_RSRVD_MFG_DATE)) ||
               (payloadType == PAYLOAD_TYPE_PLATFORM &&
                !strcmp(*propName, OC_RSRVD_SYSTEM_TIME))) {
      jsResult = dateFromString((const char *)result);
    }

    Nan::Set(Local<Object>::Cast(info[2]), Nan::New("value").ToLocalChecked(),
             jsResult);
  }
  info.GetReturnValue().Set(Nan::New(returnValue));
}

NAN_METHOD(bind_OCSetPropertyValue) {
  VALIDATE_ARGUMENT_COUNT(info, 3);
  VALIDATE_ARGUMENT_TYPE(info, 0, IsUint32);
  VALIDATE_ARGUMENT_TYPE(info, 1, IsString);
  if (!(info[2]->IsString() || info[2]->IsArray() || info[2]->IsDate())) {
    return Nan::ThrowTypeError(
        (std::string("Property value ") +
         std::string((const char *)*String::Utf8Value(info[1])) +
         std::string(" must be a string, an array, or a date"))
            .c_str());
  }

  OCStackResult returnValue;

  if (info[2]->IsString()) {
    returnValue =
        OCSetPropertyValue((OCPayloadType)Nan::To<uint32_t>(info[0]).FromJust(),
                           (const char *)*String::Utf8Value(info[1]),
                           (const void *)*String::Utf8Value(info[2]));
  } else if (info[2]->IsArray()) {
    OCStringLL *start = 0, *current = 0, *previous = 0;
    Local<Array> jsList = Local<Array>::Cast(info[2]);
    size_t index, length = jsList->Length();
    for (index = 0; index < length; index++) {
      current = new OCStringLL;
      current->value = strdup((const char *)*String::Utf8Value(
          Nan::Get(jsList, index).ToLocalChecked()));
      current->next = 0;
      if (previous) {
        previous->next = current;
      }
      previous = current;
      if (!start) {
        start = previous;
      }
    }
    returnValue = OCSetPropertyValue(
        (OCPayloadType)Nan::To<uint32_t>(info[0]).FromJust(),
        (const char *)*String::Utf8Value(info[1]), (const void *)start);
    if (returnValue != OC_STACK_OK) {
      for (current = start; start; current = start) {
        free(current->value);
        start = current->next;
        delete current;
      }
    }
  } else {
    returnValue =
        OCSetPropertyValue((OCPayloadType)Nan::To<uint32_t>(info[0]).FromJust(),
                           (const char *)*String::Utf8Value(info[1]),
                           (const char *)*String::Utf8Value(
                               Nan::To<String>(info[2]).ToLocalChecked()));
  }
  info.GetReturnValue().Set(Nan::New(returnValue));
}

NAN_METHOD(bind_OCDiscoverUnownedDevices) {
  VALIDATE_ARGUMENT_COUNT(info, 2);
  VALIDATE_ARGUMENT_TYPE(info, 0, IsUint32);
  VALIDATE_ARGUMENT_TYPE(info, 1, IsArray);

  Local<Array> jsArray = Local<Array>::Cast(info[1]);
  uint32_t index;
  OCProvisionDev_t *devices = 0, *device;
  OCStackResult result =
      OCDiscoverUnownedDevices(Nan::To<uint32_t>(info[0]).FromJust(), &devices);
  if (result == OC_STACK_OK) {
    for (device = devices, index = jsArray->Length(); device != 0;
         device = device->next, index++) {
      Nan::Set(jsArray, index, JSOCProvisionDev::New(device));
    }
  }
  info.GetReturnValue().Set(Nan::New(result));
}

void default_OCProvisionResultCB(void *context, size_t resultCount,
                                 OCProvisionResult_t *results, bool hasError) {
  Nan::HandleScope scope;
  Nan::Callback *jsCallback = (Nan::Callback *)context;
  size_t index;
  char uuid_string[37] = "";
  Local<Array> jsResults = Nan::New<Array>(resultCount);
  for (index = 0; index < resultCount; index++) {
    Nan::HandleScope loopScope;
    Local<Object> jsResult = Nan::New<Object>();
    Nan::Set(jsResult, Nan::New("res").ToLocalChecked(),
             Nan::New(results[index].res));
    snprintf(
        uuid_string, 37,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        results[index].deviceId.id[0], results[index].deviceId.id[1],
        results[index].deviceId.id[2], results[index].deviceId.id[3],
        results[index].deviceId.id[4], results[index].deviceId.id[5],
        results[index].deviceId.id[6], results[index].deviceId.id[7],
        results[index].deviceId.id[8], results[index].deviceId.id[9],
        results[index].deviceId.id[10], results[index].deviceId.id[11],
        results[index].deviceId.id[12], results[index].deviceId.id[13],
        results[index].deviceId.id[14], results[index].deviceId.id[15]);
    Nan::Set(jsResult, Nan::New("deviceId").ToLocalChecked(),
             Nan::New(uuid_string).ToLocalChecked());
    Nan::Set(jsResults, index, jsResult);
  }
  Local<Value> jsArgv[2] = {jsResults, Nan::New(hasError)};
  jsCallback->Call(2, jsArgv);
  delete jsCallback;
}

NAN_METHOD(bind_OCDoOwnershipTransfer) {
  VALIDATE_ARGUMENT_COUNT(info, 2);
  VALIDATE_ARGUMENT_TYPE(info, 0, IsArray);
  VALIDATE_ARGUMENT_TYPE(info, 1, IsFunction);
  OCProvisionDev_t *devices, **destination = &devices;

  Local<Array> jsArray = Local<Array>::Cast(info[0]);
  uint32_t length = jsArray->Length(), index;
  for (index = 0; index < length;
       index++, destination = &((*destination)->next)) {
    (*destination) = JSOCProvisionDev::Resolve(
        Local<Object>::Cast(Nan::Get(jsArray, index).ToLocalChecked()));
    if (!(*destination)) {
      return;
    }
  }

  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[1]));
  OCStackResult result =
      OCDoOwnershipTransfer(callback, devices, default_OCProvisionResultCB);
  if (result != OC_STACK_OK) {
    delete callback;
  }
  info.GetReturnValue().Set(Nan::New(result));
}
