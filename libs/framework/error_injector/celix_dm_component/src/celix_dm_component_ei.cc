/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include "celix_dm_component_ei.h"
#include "celix_error_injector.h"

extern "C" {

celix_dm_component_t* __real_celix_dmComponent_create(bundle_context_t *context, const char* name);
CELIX_EI_DEFINE(celix_dmComponent_create, celix_dm_component_t*)
celix_dm_component_t* __wrap_celix_dmComponent_create(bundle_context_t *context, const char* name) {
    CELIX_EI_IMPL(celix_dmComponent_create);
    return __real_celix_dmComponent_create(context, name);
}

celix_dm_service_dependency_t* __real_celix_dmServiceDependency_create();
CELIX_EI_DEFINE(celix_dmServiceDependency_create, celix_dm_service_dependency_t*)
celix_dm_service_dependency_t* __wrap_celix_dmServiceDependency_create() {
    CELIX_EI_IMPL(celix_dmServiceDependency_create);
    return __real_celix_dmServiceDependency_create();
}

}