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
#include "../common.h"

using namespace v8;

#define RETURN_ON_THROW(callback, context, argumentCount, arguments) \
  ({                                                               \
    Nan::TryCatch tryCatch;                                        \
    Local<Value> returnValue =                                     \
        (callback)->Call((context), (argumentCount), (arguments)); \
    if (tryCatch.HasCaught()) {                                    \
	  Local<Value> caughtValue = tryCatch.Exception();             \
	  if (!caughtValue->IsNull()) {                                \
	  	tryCatch.ReThrow();                                        \
	  }                                                            \
      return;                                                      \
    }                                                              \
    returnValue;                                                   \
  })

#define CALL_GETTER(argument)                                          \
  do {                                                                 \
    Local<Value> jsIndex = (argument);                                 \
    Nan::Callback jsCallback(Local<Function>::Cast(                    \
        Nan::Get(Nan::To<Object>(info.Data()).ToLocalChecked(),        \
                 Nan::New("getter").ToLocalChecked())                  \
            .ToLocalChecked()));                                       \
    info.GetReturnValue().Set(                                         \
        RETURN_ON_THROW(&jsCallback, info.Holder(), 1, (&jsIndex))); \
  } while (0)

#define CALL_SETTER(argument, value)                                  \
  do {                                                                \
    Local<Value> jsIndex = (argument);                                \
    Nan::Callback jsCallback(Local<Function>::Cast(                   \
        Nan::Get(Nan::To<Object>(info.Data()).ToLocalChecked(),       \
                 Nan::New("setter").ToLocalChecked())                 \
            .ToLocalChecked()));                                      \
    Local<Value> arguments[2] = {jsIndex, (value)};                   \
    info.GetReturnValue().Set(                                        \
        RETURN_ON_THROW(&jsCallback, info.Holder(), 2, arguments)); \
  } while (0)

#define CALL_AND_VALIDATE(argument, name, typecheck, convertType) \
  do {                                                                       \
    Local<Value> jsIndex = (argument);                                       \
    Nan::Callback jsCallback(Local<Function>::Cast(                          \
        Nan::Get(Nan::To<Object>(info.Data()).ToLocalChecked(),              \
                 Nan::New((name)).ToLocalChecked())                         \
            .ToLocalChecked()));                                             \
    Local<Value> returnValue =                                               \
        RETURN_ON_THROW(&jsCallback, info.Holder(), 1, (&jsIndex));        \
    VALIDATE_VALUE_TYPE(returnValue, typecheck, name " return type", return ); \
    info.GetReturnValue().Set(Nan::To<convertType>(returnValue).ToLocalChecked()); \
  } while (0)

#define CALL_QUERY(argument) \
	CALL_AND_VALIDATE(argument, "query", IsInt32, Int32)

#define CALL_DELETER(argument) \
	CALL_AND_VALIDATE(argument, "deleter", IsBoolean, Boolean)

#define CALL_ENUMERATOR(name) \
do { \
    Nan::Callback jsCallback(Local<Function>::Cast(                          \
        Nan::Get(Nan::To<Object>(info.Data()).ToLocalChecked(),              \
                 Nan::New((name)).ToLocalChecked())                          \
            .ToLocalChecked()));                                             \
    Local<Value> returnValue =                                               \
        RETURN_ON_THROW(&jsCallback, info.Holder(), 0, 0);        \
    VALIDATE_VALUE_TYPE(returnValue, IsArray, name " return type", return ); \
	info.GetReturnValue().Set(Local<Array>::Cast(returnValue)); \
} while(0)

#define NAN_HAS(object, property) \
  Nan::Has((object), Nan::New((property)).ToLocalChecked()).FromJust()

NAN_INDEX_GETTER(indexedGetter) { CALL_GETTER(Nan::New(index)); }

NAN_INDEX_SETTER(indexedSetter) { CALL_SETTER(Nan::New(index), value); }

NAN_INDEX_QUERY(indexedQuery) { CALL_QUERY(Nan::New(index)); }

NAN_INDEX_DELETER(indexedDeleter) { CALL_DELETER(Nan::New(index)); }

NAN_INDEX_ENUMERATOR(indexedEnumerator) { CALL_ENUMERATOR("indexedEnumerator");}

NAN_PROPERTY_GETTER(namedGetter) {
  if (!(strcmp(*String::Utf8Value(property), "_asyncQueue"))) {
  	return;
  }
  CALL_GETTER(property);
}

NAN_PROPERTY_SETTER(namedSetter) {
  if (!(strcmp(*String::Utf8Value(property), "_asyncQueue"))) {
  	return;
  }
  CALL_SETTER(property, value);
}

NAN_PROPERTY_QUERY(namedQuery) { CALL_QUERY(property); }

NAN_PROPERTY_DELETER(namedDeleter) { CALL_DELETER(property); }

NAN_PROPERTY_ENUMERATOR(namedEnumerator) {CALL_ENUMERATOR("namedEnumerator");}

/* This comment avoids attaching this method */NAN_METHOD(Interceptable) {
  if (!(info.Data()->IsFunction())) {
    return;
  }
  size_t index, length = info.Length();
  Nan::Callback jsCallback(Local<Function>::Cast(info.Data()));
  Local<Value> arguments[length];
  for (index = 0; index < length; index++) {
    arguments[index] = info[index];
  }
  info.GetReturnValue().Set(
      RETURN_ON_THROW(&jsCallback, info.Holder(), length, arguments));
}

// 0: Name
// 1: Constructor
// 2: Handlers
NAN_METHOD(bind_DeclareInterceptableClass) {
  VALIDATE_ARGUMENT_COUNT(info, 3);
  VALIDATE_ARGUMENT_TYPE(info, 0, IsString);
  VALIDATE_ARGUMENT_TYPE_OR_NULL(info, 1, IsFunction);
  VALIDATE_ARGUMENT_TYPE(info, 2, IsObject);

  Local<Object> jsHandlers = Nan::To<Object>(info[2]).ToLocalChecked();
  bool hasGetter = NAN_HAS(jsHandlers, "getter");
  bool hasSetter = NAN_HAS(jsHandlers, "setter");
  bool hasQuery = NAN_HAS(jsHandlers, "query");
  bool hasDeleter = NAN_HAS(jsHandlers, "deleter");
  bool hasIndexedEnumerator = NAN_HAS(jsHandlers, "indexedEnumerator");
  bool hasNamedEnumerator = NAN_HAS(jsHandlers, "namedEnumerator");
  if (!hasGetter) {
    Nan::ThrowError("Missing 'getter'");
    return;
  }

  Local<String> jsClassName = Nan::To<String>(info[0]).ToLocalChecked();
  Local<FunctionTemplate> interceptableTemplate = Nan::New<FunctionTemplate>();

  interceptableTemplate->SetClassName(jsClassName);
  Nan::SetCallHandler(interceptableTemplate, Interceptable, info[1]);
  Nan::SetIndexedPropertyHandler(
      interceptableTemplate->InstanceTemplate(), indexedGetter,
      (hasSetter ? indexedSetter : 0), (hasQuery ? indexedQuery : 0),
      (hasDeleter ? indexedDeleter : 0),
      (hasIndexedEnumerator ? indexedEnumerator : 0), jsHandlers);
  Nan::SetNamedPropertyHandler(
      interceptableTemplate->InstanceTemplate(), namedGetter,
      (hasSetter ? namedSetter : 0), (hasQuery ? namedQuery : 0),
      (hasDeleter ? namedDeleter : 0),
	  (hasNamedEnumerator ? namedEnumerator : 0),
      jsHandlers);

  Local<Function> interceptableFunction =
      Nan::GetFunction(interceptableTemplate).ToLocalChecked();
  Nan::Set(interceptableFunction, Nan::New("displayName").ToLocalChecked(),
           jsClassName);
  info.GetReturnValue().Set(interceptableFunction);
}
