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

#include <node_jsvmpapi.h>
#include <map>
#include <node_api_helpers.h>
extern "C" {
#include <ocstack.h>
}

template<typename handleType>
struct JSHandleData {
	const char *typeName;
	handleType data;
};

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

  static JSHandleData<handleType> *getMetaData(napi_env env, napi_value jsObject) {
  	struct JSHandleData<handleType> *metaData =
		(struct JSHandleData<handleType> *)napi_unwrap(env, jsObject);
	if (metaData && metaData->typeName == jsName::jsClassName()) {
		return metaData;
	}
	napi_throw_type_error(env, 
      	(std::string("Object is not of type ") + jsName::jsClassName())
          .c_str());
	return 0;
  }s

 public:
  static napi_value New(napi_env env, handleType data) {
    napi_value returnValue =
		napi_new_instance(env, napi_get_persistent_value(theTemplate(env)),
			0, 0);
	struct JSHandleData<handleType> *metaData = new struct JSHandleHandleData<handleType>;
	metaData->typeName = jsName::jsClassName();
	metaData->data = data;
    napi_wrap(env, returnValue, metaData, 0, 0);

    return returnValue;
  }

  // If the object is not of the expected type, or if the pointer inside the
  // object has already been removed, then we must throw an error
  static handleType Resolve(napi_env env, napi_value jsObject) {
	struct JSHandleData<handleType> *metaData = getMetaData(env, jsObject);
	return metaData ? metaData->data : 0;
  }

  static void Invalidate(napi_value jsObject) {
	struct JSHandleData<handleType> *metaData = getMetaData(env, jsObject);
	delete metaData;
	napi_wrap(env, jsObject, 0, 0, 0);
  }
};

class JSOCRequestHandle : public JSHandle<JSOCRequestHandle, OCRequestHandle> {
 public:
  static const char *jsClassName() { return "OCRequestHandle"; }
};

template <typename handleType>
class CallbackInfo {
 public:
  napi_env env;
  handleType handle;
  napi_persistent callback;
  napi_persistent jsHandle;
  napi_value Init(napi_value _jsHandle, napi_value jsCallback) {
  	callback = napi_create_persistent(env, jsCallback);
	jsHandle = napi_create_persistent(env, _jsHandle);
    return _jsHandle;
  }
  CallbackInfo(napi_env _env) : env(_env), handle(0), callback(0), jsHandle(0) {}
  virtual ~CallbackInfo() {
    if (jsHandle) {
      napi_value theObject = napi_get_persistent_value(env, jsHandle);
	  napi_wrap(env, theObject, 0, 0, 0);
	  napi_set_property(env, theObject, napi_property_name(env, "stale"),
	  	napi_create_boolean(env, true));
      napi_release_persistent(env, jsHandle);
	  napi_release_persistent(env, callback);
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
  static std::map<OCResourceHandle, napi_persistent>
      handles;
};

napi_value jsArrayFromBytes(napi_env env, unsigned char *bytes, uint32_t length);

bool fillCArrayFromJSArray(unsigned char *bytes, uint32_t length,
                           napi_value array);

#endif /* __IOTIVITY_NODE_HANDLES_H__ */
