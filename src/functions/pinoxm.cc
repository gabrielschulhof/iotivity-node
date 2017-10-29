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
#include <pinoxmcommon.h>
}

#include "../common.h"
#include "../structures/handles.h"

using namespace v8;

static Nan::Callback DisplayPinWithContext_jsCallback;
static Nan::Callback InputPinWithContext_jsCallback;
static Nan::Callback ClosePinDisplay_jsCallback;

static void defaultDisplayPinWithContext_callback(char* pinData, size_t pinSize,
                                                  void* context) {
  CALL_JS(&DisplayPinWithContext_jsCallback, Nan::GetCurrentContext()->Global(),
          1, , IsUndefined, "DisplayPinCallbackWithContext return value",
          return, Nan::New(pinData, pinSize).ToLocalChecked());
}

static void defaultInputPinWithContext_callback(OicUuid_t deviceId,
                                                char* pinBuffer,
                                                size_t pinBufferSize,
                                                void* context) {
  CALL_JS(&InputPinWithContext_jsCallback, Nan::GetCurrentContext()->Global(),
          2, , IsString, "InputPinCallbackWithContext return value",
          do {
            Local<String> jsString = Nan::To<String>(jsReturn).ToLocalChecked();
            if (!jsString->IsOneByte()) {
              Nan::ThrowTypeError(
                  "InputPunCallbackWithContext return value must be a plain "
                  "ASCII string");
              return;
            }
            size_t length = (size_t)jsString->Length();
            if (length != pinBufferSize) {
              Nan::ThrowRangeError(
                  "InputPunCallbackWithContext return value length exceeds pin "
                  "buffer size");
              return;
            }
            memcpy(pinBuffer, *String::Value(jsReturn), pinBufferSize);
          } while (0),
          jsArrayFromBytes(deviceId.id, UUID_LENGTH),
          Nan::New((uint32_t)pinBufferSize));
}

static void defaultClosePinDisplay_callback(void) {
  CALL_JS(&ClosePinDisplay_jsCallback, Nan::GetCurrentContext()->Global(),
          0, , IsUndefined, "ClosePinDisplay return value", return,);
}

#define BIND_PIN_METHOD(apiKey)                                               \
  do {                                                                        \
    VALIDATE_ARGUMENT_COUNT(info, 1);                                         \
    VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction);                              \
    OCStackResult result = Set##apiKey##CB(default##apiKey##_callback, NULL); \
    if (result == OC_STACK_OK) {                                              \
      apiKey##_jsCallback.Reset(Local<Function>::Cast(info[0]));              \
    }                                                                         \
    info.GetReturnValue().Set(Nan::New(result));                              \
  } while (0)

NAN_METHOD(bind_SetDisplayPinWithContextCB) {
  BIND_PIN_METHOD(DisplayPinWithContext);
}

NAN_METHOD(bind_SetInputPinWithContextCB) {
  BIND_PIN_METHOD(InputPinWithContext);
}

NAN_METHOD(bind_SetClosePinDisplayCB) {
  VALIDATE_ARGUMENT_COUNT(info, 1);
  VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction);
  SetClosePinDisplayCB(defaultClosePinDisplay_callback);
  ClosePinDisplay_jsCallback.Reset(Local<Function>::Cast(info[0]));
}

NAN_METHOD(bind_UnsetInputPinWithContextCB) {
  UnsetInputPinWithContextCB();
}

NAN_METHOD(bind_UnsetDisplayPinWithContextCB) {
  UnsetDisplayPinWithContextCB();
}

NAN_METHOD(bind_UnsetClosePinCB) {
  UnsetClosePinCB();
}

NAN_METHOD(bind_SetRandomPinPolicy) {
  VALIDATE_ARGUMENT_COUNT(info, 2);
  VALIDATE_ARGUMENT_TYPE(info, 0, IsUint32);
  VALIDATE_ARGUMENT_TYPE(info, 1, IsUint32);

  size_t pinSize = (size_t)Nan::To<uint32_t>(info[0]).FromJust();
  OicSecPinType_t pinType = (OicSecPinType_t)Nan::To<uint32_t>(info[1]).FromJust();
  info.GetReturnValue().Set(SetRandomPinPolicy(pinSize, pinType));
}

NAN_METHOD(bind_SetUuidForPinBasedOxm) {
  VALIDATE_ARGUMENT_COUNT(info, 1);
  VALIDATE_ARGUMENT_TYPE(info, 0, IsArray);
  Local<Array> jsUuid = Local<Array>::Cast(info[0]);
  if (jsUuid->Length() != UUID_LENGTH) {
    Nan::ThrowRangeError("SetUuidForPinBasedOxm: Wrong UUID length");
    return;
  }
  OicUuid_t uuid = {{0}};
  if (!fillCArrayFromJSArray(uuid.id, UUID_LENGTH, jsUuid)) {
    return;
  }
  SetUuidForPinBasedOxm(&uuid);
}

NAN_METHOD(bind_GetDtlsPskForRandomPinOxm) {
  VALIDATE_ARGUMENT_COUNT(info, 3);
  VALIDATE_ARGUMENT_TYPE(info, 0, IsUint32);
  // info[1] type will be string, but is currently unused.
  VALIDATE_ARGUMENT_TYPE(info, 2, IsObject);

  CADtlsPskCredType_t credType = (CADtlsPskCredType_t)Nan::To<uint32_t>(info[0]).FromJust();
  Local<Object> outParam = Local<Object>::Cast(info[1]);
  size_t result_length = Nan::To<uint32_t>(Nan::Get(outParam, Nan::New("result_length").ToLocalChecked()).ToLocalChecked()).FromJust();
  unsigned char result[result_length];
  int32_t result = GetDtlsPskForRandomPinOxm(credType)
}
