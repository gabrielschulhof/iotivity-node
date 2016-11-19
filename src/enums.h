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

#ifndef __IOTIVITY_NODE_ENUMS_H__
#define __IOTIVITY_NODE_ENUMS_H__

#include <node_jsvmapi.h>
#include <node_api_helpers.h>

NAPI_MODULE_INIT(InitEnums);

#define SET_CONSTANT(env, destination, name, type)                          \
  napi_set_property((env), (destination), napi_property_name((env), #name), \
                    napi_create_##type((env), name))
#endif /* __IOTIVITY_NODE_ENUMS_H__ */
