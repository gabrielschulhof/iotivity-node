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

#include <string>
#include <vector>

#include "../common.h"
#include "../structures/handles.h"
#include "../structures/oc-client-response.h"
#include "../structures/oc-dev-addr.h"
#include "../structures/oc-header-option-array.h"
#include "../structures/oc-payload.h"

extern "C" {
#include <ocpayload.h>
#include <ocstack.h>
#include <rd_client.h>
#include <stdint.h>
#include <stdlib.h>
}

using namespace v8;

static void deleteNanCallback(CallbackInfo<OCDoHandle> *handle) {
  Nan::HandleScope scope;

  delete handle;
}

// Create an object containing the information from an OCClientResponse
// structure
static OCStackApplicationResult defaultOCClientResponseHandler(
    void *context, OCDoHandle handle, OCClientResponse *clientResponse) {
  CallbackInfo<OCDoHandle> *callbackInfo = (CallbackInfo<OCDoHandle> *)context;

  CALL_JS(
      &(callbackInfo->callback), Nan::GetCurrentContext()->Global(), 2,
      OC_STACK_KEEP_TRANSACTION, IsUint32,
      "OCClientResponseHandler return value",
      return ((OCStackApplicationResult)Nan::To<uint32_t>(jsReturn).FromJust()),
      Nan::New(callbackInfo->jsHandle), js_OCClientResponse(clientResponse));
}

#define OC_DO_CALL(prefix, jsCallback, doCall)                             \
  do {                                                                     \
    OCCallbackData callbackData;                                           \
    CallbackInfo<OCDoHandle> *callbackInfo = new CallbackInfo<OCDoHandle>; \
    if (!callbackInfo) {                                                   \
      Nan::ThrowError(prefix ": Failed to allocate callback info");        \
      return;                                                              \
    }                                                                      \
                                                                           \
    callbackData.context = (void *)callbackInfo;                           \
    callbackData.cb = defaultOCClientResponseHandler;                      \
    callbackData.cd = (OCClientContextDeleter)deleteNanCallback;           \
                                                                           \
    OCStackResult returnValue = doCall;                                    \
                                                                           \
    if (returnValue == OC_STACK_OK) {                                      \
      Nan::Set(Nan::To<Object>(info[0]).ToLocalChecked(),                  \
               Nan::New("handle").ToLocalChecked(),                        \
               callbackInfo->Init(JSOCDoHandle::New(callbackInfo),         \
                                  Local<Function>::Cast((jsCallback))));   \
    }                                                                      \
                                                                           \
    info.GetReturnValue().Set(Nan::New(returnValue));                      \
  } while (0)

NAN_METHOD(bind_OCDoResource) {
  VALIDATE_ARGUMENT_COUNT(info, 8);
  VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
  VALIDATE_ARGUMENT_TYPE(info, 1, IsUint32);
  VALIDATE_ARGUMENT_TYPE(info, 2, IsString);
  VALIDATE_ARGUMENT_TYPE_OR_NULL(info, 3, IsObject);
  VALIDATE_ARGUMENT_TYPE_OR_NULL(info, 4, IsObject);
  VALIDATE_ARGUMENT_TYPE(info, 5, IsUint32);
  VALIDATE_ARGUMENT_TYPE(info, 6, IsUint32);
  VALIDATE_ARGUMENT_TYPE(info, 7, IsFunction);
  VALIDATE_ARGUMENT_TYPE_OR_NULL(info, 8, IsArray);

  OCDevAddr *destination = 0, destinationToFillIn;
  OCPayload *payload = 0;
  std::vector<OCHeaderOption> options;
  uint8_t optionCount = 0;

  if (info[8]->IsArray()) {
    Local<Array> optionArray = Local<Array>::Cast(info[8]);
    size_t length = optionArray->Length();

    if (length > 0) {
      options.resize(length);
      if (!c_OCHeaderOption(optionArray, options.data(), &optionCount)) {
        return;
      }
    }
  }

  // If a destination is given, we only use it if it can be converted to a
  // OCDevAddr structure
  if (info[3]->IsObject()) {
    if (c_OCDevAddr(Nan::To<Object>(info[3]).ToLocalChecked(),
                    &destinationToFillIn)) {
      destination = &destinationToFillIn;
    } else {
      return;
    }
  }

  // If a payload is given, we only use it if it can be converted to a
  // OCPayload*
  if (info[4]->IsObject()) {
    if (!c_OCPayload(Nan::To<Object>(info[4]).ToLocalChecked(), &payload)) {
      return;
    }
  }

  // We need not free the payload after the call because it seems iotivity takes
  // ownership. Similarly, if OCDoResource() fails, iotivity calls the callback
  // that frees the data on our behalf.

  OC_DO_CALL(
      "OCDoResource", info[7],
      OCDoResource(&(callbackInfo->handle),
                   (OCMethod)Nan::To<uint32_t>(info[1]).FromJust(),
                   (const char *)*String::Utf8Value(info[2]), destination,
                   payload,
                   (OCConnectivityType)Nan::To<uint32_t>(info[5]).FromJust(),
                   (OCQualityOfService)Nan::To<uint32_t>(info[6]).FromJust(),
                   &callbackData, options.data(),
                   (uint8_t)Nan::To<uint32_t>(info[9]).FromJust()));
}

NAN_METHOD(bind_OCRDDiscover) {
  VALIDATE_ARGUMENT_COUNT(info, 4);
  VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
  VALIDATE_ARGUMENT_TYPE(info, 1, IsUint32);
  VALIDATE_ARGUMENT_TYPE(info, 2, IsFunction);
  VALIDATE_ARGUMENT_TYPE(info, 3, IsUint32);

  OC_DO_CALL(
      "OCRDDiscover", info[2],
      OCRDDiscover(&(callbackInfo->handle),
                   (OCConnectivityType)Nan::To<uint32_t>(info[1]).FromJust(),
                   &callbackData,
                   (OCQualityOfService)Nan::To<uint32_t>(info[6]).FromJust()));
}

static bool c_OCResourceArray(Local<Value> jsArrayValue,
                              std::vector<OCResourceHandle> &vector,
                              size_t limit) {
  Local<Array> jsArray = Local<Array>::Cast(jsArrayValue);
  size_t length = jsArray->Length();

  if (length > limit) {
    Nan::ThrowError("Resource array length exceeds limit");
    return false;
  }

  vector.resize(length);
  for (size_t index = 0; index < length; index++) {
    Nan::HandleScope scope;
    Local<Value> value = Nan::Get(jsArray, index).ToLocalChecked();
    VALIDATE_VALUE_TYPE(value, IsObject, "Resource array item", return false);
    vector[index] = JSOCResourceHandle::Resolve(Local<Object>::Cast(value));
    if (vector[index] == nullptr) {
      vector.resize(0);
      return false;
    }
  }

  return true;
}

NAN_METHOD(bind_OCRDPublish) {
  VALIDATE_ARGUMENT_COUNT(info, 7);
  VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
  VALIDATE_ARGUMENT_TYPE(info, 1, IsString);
  VALIDATE_ARGUMENT_TYPE(info, 2, IsUint32);
  VALIDATE_ARGUMENT_TYPE(info, 3, IsArray);
  VALIDATE_ARGUMENT_TYPE(info, 4, IsUint32);
  VALIDATE_ARGUMENT_TYPE(info, 5, IsFunction);
  VALIDATE_ARGUMENT_TYPE(info, 6, IsUint32);

  std::vector<OCResourceHandle> resources;
  if (!c_OCResourceArray(info[3], resources, UINT8_MAX)) {
    return;
  }

  OC_DO_CALL(
      "OCRDPublish", info[5],
      OCRDPublish(
          &(callbackInfo->handle), (const char *)*String::Utf8Value(info[1]),
          (OCConnectivityType)Nan::To<uint32_t>(info[2]).FromJust(),
          resources.data(), (uint8_t)resources.size(),
          (uint32_t)Nan::To<uint32_t>(info[4]).FromJust(), &callbackData,
          (OCQualityOfService)Nan::To<uint32_t>(info[6]).FromJust()));
}

NAN_METHOD(bind_OCRDPublishWithDeviceId) {
  VALIDATE_ARGUMENT_COUNT(info, 8);
  VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
  VALIDATE_ARGUMENT_TYPE(info, 1, IsString);
  VALIDATE_ARGUMENT_TYPE(info, 2, IsString);
  VALIDATE_ARGUMENT_TYPE(info, 3, IsUint32);
  VALIDATE_ARGUMENT_TYPE(info, 4, IsArray);
  VALIDATE_ARGUMENT_TYPE(info, 5, IsUint32);
  VALIDATE_ARGUMENT_TYPE(info, 6, IsFunction);
  VALIDATE_ARGUMENT_TYPE(info, 7, IsUint32);

  std::vector<OCResourceHandle> resources;
  if (!c_OCResourceArray(info[4], resources, UINT8_MAX)) {
    return;
  }

  OC_DO_CALL(
      "OCRDPublishWithDeviceId", info[6],
      OCRDPublishWithDeviceId(
          &(callbackInfo->handle), (const char *)*String::Utf8Value(info[1]),
          (const unsigned char *)*String::Utf8Value(info[2]),
          (OCConnectivityType)Nan::To<uint32_t>(info[3]).FromJust(),
          resources.data(), (uint8_t)resources.size(),
          (uint32_t)Nan::To<uint32_t>(info[5]).FromJust(), &callbackData,
          (OCQualityOfService)Nan::To<uint32_t>(info[7]).FromJust()));
}

NAN_METHOD(bind_OCRDDelete) {
  VALIDATE_ARGUMENT_COUNT(info, 6);
  VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
  VALIDATE_ARGUMENT_TYPE(info, 1, IsString);
  VALIDATE_ARGUMENT_TYPE(info, 2, IsUint32);
  VALIDATE_ARGUMENT_TYPE(info, 3, IsArray);
  VALIDATE_ARGUMENT_TYPE(info, 4, IsFunction);
  VALIDATE_ARGUMENT_TYPE(info, 5, IsUint32);

  std::vector<OCResourceHandle> resources;
  if (!c_OCResourceArray(info[3], resources, UINT8_MAX)) {
    return;
  }

  OC_DO_CALL(
      "OCRDDelete", info[4],
      OCRDDelete(&(callbackInfo->handle),
                 (const char *)*String::Utf8Value(info[1]),
                 (OCConnectivityType)Nan::To<uint32_t>(info[2]).FromJust(),
                 resources.data(), (uint8_t)resources.size(), &callbackData,
                 (OCQualityOfService)Nan::To<uint32_t>(info[5]).FromJust()));
}

NAN_METHOD(bind_OCCancel) {
  VALIDATE_ARGUMENT_COUNT(info, 3);
  VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
  VALIDATE_ARGUMENT_TYPE(info, 1, IsUint32);
  VALIDATE_ARGUMENT_TYPE_OR_NULL(info, 2, IsArray);

  OCHeaderOption headerOptions[MAX_HEADER_OPTIONS] = {{OC_INVALID_ID, 0, 0, 0}};
  uint8_t numberOfOptions = 0;

  if (info[2]->IsArray()) {
    if (!c_OCHeaderOption(Local<Array>::Cast(info[2]), headerOptions,
                          &numberOfOptions)) {
      return;
    }
  }

  CallbackInfo<OCDoHandle> *callbackInfo;
  JSCALLBACKHANDLE_RESOLVE(JSOCDoHandle, callbackInfo,
                           Nan::To<Object>(info[0]).ToLocalChecked());
  info.GetReturnValue().Set(Nan::New(OCCancel(

      callbackInfo->handle,
      (OCQualityOfService)Nan::To<uint32_t>(info[1]).FromJust(), headerOptions,
      numberOfOptions)));
}
