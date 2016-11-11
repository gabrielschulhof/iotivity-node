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

#ifndef __IOTIVITY_NODE_FUNCTIONS_INTERNAL_H__
#define __IOTIVITY_NODE_FUNCTIONS_INTERNAL_H__

#include <node_jsvmapi.h>
#include <string>
extern "C" {
#include <ocstack.h>
}

#define TRY_CALL(callback, context, argumentCount, arguments, jsReturnValue,   \
                 exceptionReturn)                                              \
  do {                                                                         \
    Nan::TryCatch tryCatch;                                                    \
    jsReturnValue = (callback)->Call((context), (argumentCount), (arguments)); \
    if (tryCatch.HasCaught()) {                                                \
      tryCatch.ReThrow();                                                      \
      return (exceptionReturn);                                                \
    }                                                                          \
  } while (0)

#define VALIDATE_ARGUMENT_COUNT(env, args, length)          \
  if (napi_get_cb_args_length((env), (args)) < length) {    \
  	napi_throw_error((env),                                 \
		(char *)"Argument count must be exactly " #length); \
	return;                                                 \
  }

#define VALIDATE_ARGUMENT_TYPE(env, args, index, typecheck)            \
  if (napi_get_type_of_value((env), (args)[(index)]) != (typecheck)) {   \
    napi_throw_type_error((env),                                       \
      (char *)"Type of argument " #index " must be " #typecheck);      \
    return;                                                            \
  }

#define VALIDATE_VALUE_TYPE(env, value, typecheck, message, ...)                 \
	if (napi_get_type_of_value((env), (value)) != (typecheck)) { \
	napi_throw_type_error((env), \
        (std::string(message) + " must satisfy " #typecheck "()").c_str()); \
    __VA_ARGS__;                                                            \
  }

#define VALIDATE_ARGUMENT_TYPE_OR_NULL(env, args, index, typecheck)      \
  if (!(napi_get_type_of_value((env), (args)[(index)]) == typecheck ||   \
  		napi_get_type_of_value((env), (args)[(index)]) == napi_null)) {  \
    napi_throw_type_error((env),                                         \
      (char *)"Type of argument " #index " must be " #typecheck " or napi_null"); \
    return;                                                              \
  }

#define SET_STRING_IF_NOT_NULL(env, destination, source, memberName) \
  if ((source)->memberName) {                                        \
    napi_set_property((env), (destination), \
		napi_property_name((env), #memberName), \
		napi_create_string((env), (source)->memberName)); \
  }

#define SET_VALUE_ON_OBJECT(destination, source, memberName, type) \
  Nan::Set((destination), Nan::New(#memberName).ToLocalChecked(),  \
           Nan::New<type>((source)->memberName));

#define VALIDATE_AND_ASSIGN_STRING(destination, source, memberName, ...)     \
  do {                                                                       \
    char *resultingValue = 0;                                                \
    Local<String> jsMemberName = Nan::New(#memberName).ToLocalChecked();     \
    if ((source)->Has(jsMemberName)) {                                       \
      Local<Value> memberName =                                              \
          Nan::Get((source), jsMemberName).ToLocalChecked();                 \
      VALIDATE_VALUE_TYPE(memberName, IsString, #memberName, __VA_ARGS__);   \
      resultingValue = strdup((const char *)*String::Utf8Value(memberName)); \
      if (!resultingValue) {                                                 \
        Nan::ThrowError("Failed to allocate memory for " #memberName);       \
        __VA_ARGS__;                                                         \
      }                                                                      \
      (destination)->memberName = resultingValue;                            \
    }                                                                        \
  } while (0)

#define VALIDATE_AND_ASSIGN(destination, source, memberName, destinationType, typecheck, message, convertType, ...)             \
  Local<Value> memberName =                                                   \
      Nan::Get(source, Nan::New(#memberName).ToLocalChecked())                \
          .ToLocalChecked();                                                  \
  VALIDATE_VALUE_TYPE(memberName, typecheck, message "." #memberName,         \
                      __VA_ARGS__);                                           \
  destination.memberName =                                                    \
      (destinationType)Nan::To<convertType>(memberName).FromJust();

void addStringArray(napi_env env, napi_value destination, OCStringLL *source,
                    const char *name);
#define ADD_STRING_ARRAY(env, destination, source, memberName) \
  addStringArray((env), (destination), (source)->memberName, #memberName)

#ifdef DEBUG
void console_log(v8::Local<v8::Value> argument);
v8::Local<v8::Value> json_stringify(v8::Local<v8::Object> jsObject);
void debug_print(const char *message, ...);
#else
#define debug_print()
#endif /* def DEBUG */

#endif /* __IOTIVITY_NODE_FUNCTIONS_INTERNAL_H__ */
