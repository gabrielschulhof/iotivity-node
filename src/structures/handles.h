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

#ifndef __IOTIVITY_NODE_HANDLES_H__
#define __IOTIVITY_NODE_HANDLES_H__

#include <map>
#include <node_jsvmpapi.h>
#include <node_api_helpers.h>
extern "C" {
#include <ocstack.h>
}

template <class jsName, typename handleType>
class JSHandle {
  static NAPI_METHOD(constructor_callback) {}
  static napi_persistent &theTemplate(napi_env env) {
    static napi_persistent returnValue = 0;

    if (!returnValue) {
	  napi_value constructor =
	  	napi_create_constructor_for_wrap(env, constructor_callback);
	  napi_set_function_name(env, constructor,
	  	napi_property_name(env, jsName::jsClassName()));
      returnValue = napi_create_persistent(env, constructor);
    }
    return returnValue;
  }

 public:
  static napi_value New(napi_env env, handleType data) {
    napi_value returnValue =
		napi_new_instance(env, napi_get_persistent_value(theTemplate(env)),
			0, 0);
    napi_wrap(env, returnValue, data, 0, 0);

    return returnValue;
  }

  // If the object is not of the expected type, or if the pointer inside the
  // object has already been removed, then we must throw an error
  static handleType Resolve(napi_env env, napi_value jsObject) {
    handleType returnValue = 0;

    if (napi_instanceof(env, jsObject, napi_get_persistent_value(theTemplate())) {
      returnValue = (handleType)napi_unwrap(env, jsObject);
    }
    if (!returnValue) {
		napi_throw_type_error(env, 
      		(std::string("Object is not of type ") + jsName::jsClassName())
              .c_str());
    }
    return returnValue;
  }
};

class JSOCRequestHandle : public JSHandle<JSOCRequestHandle, OCRequestHandle> {
 public:
  static const char *jsClassName() { return "OCRequestHandle"; }
};

template <typename handleType>
class CallbackInfo {
 public:
  handleType handle;
  napi_persistent callback;
  napi_persistent jsHandle;
  napi_value Init(napi_value _jsHandle, napi_value jsCallback) {
    callback.Reset(jsCallback);
    jsHandle.Reset(_jsHandle);
    return _jsHandle;
  }
  CallbackInfo() : handle(0) {}
  virtual ~CallbackInfo() {
    if (!jsHandle.IsEmpty()) {
      v8::Local<v8::Object> theObject = Nan::New(jsHandle);
      Nan::SetInternalFieldPointer(theObject, 0, 0);
      Nan::ForceSet(theObject, Nan::New("stale").ToLocalChecked(),
                    Nan::New(true),
                    (v8::PropertyAttribute)(v8::ReadOnly | v8::DontDelete));
      jsHandle.Reset();
    }
  }
};

#define JSCALLBACKHANDLE_RESOLVE(type, info, object, ...) \
  do {                                                    \
    info = type::Resolve((object));                       \
    if (!info) {                                          \
      return __VA_ARGS__;                                 \
    }                                                     \
  } while (0)

class JSOCDoHandle : public JSHandle<JSOCDoHandle, CallbackInfo<OCDoHandle> *> {
 public:
  static const char *jsClassName() { return "OCDoHandle"; }
};

class JSOCResourceHandle
    : public JSHandle<JSOCResourceHandle, CallbackInfo<OCResourceHandle> *> {
 public:
  static const char *jsClassName() { return "OCResourceHandle"; }
  static std::map<OCResourceHandle, Nan::Persistent<v8::Object> *> handles;
};

v8::Local<v8::Array> jsArrayFromBytes(unsigned char *bytes, uint32_t length);

bool fillCArrayFromJSArray(unsigned char *bytes, uint32_t length,
                           v8::Local<v8::Array> array);

#endif /* __IOTIVITY_NODE_HANDLES_H__ */
