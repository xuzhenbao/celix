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

#include "gtest/gtest.h"

#include <cstdlib>
#include <cstdio>

#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_version.h"

extern "C" {

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <jansson.h>

#include "celix_launcher.h"
#include "celix_constants.h"
#include "endpoint_description.h"
#include "framework.h"
#include "topology_manager.h"
#include "calculator_service.h"
#include "tm_scope.h"
#include "scope.h"
#include "remote_service_admin.h"
#include "remote_constants.h"
#include "disc_mock_service.h"
#include "tst_service.h"

#define JSON_EXPORT_SERVICES  "exportServices"
#define JSON_IMPORT_SERVICES  "importServices"

#define JSON_SERVICE_NAME     "filter"
#define JSON_SERVICE_ZONE     "zone"
#define JSON_SERVICE_KEY1     "key1"
#define JSON_SERVICE_KEY2     "key2"

#define TST_CONFIGURATION_TYPE "org.amdatu.remote.admin.http"

    static framework_pt framework = nullptr;
    static celix_bundle_context_t *context = nullptr;

    static service_reference_pt scopeServiceRef = nullptr;
    static tm_scope_service_pt tmScopeService = nullptr;

    static service_reference_pt calcRef = nullptr;
    static calculator_service_t *calc = nullptr;

    static service_reference_pt rsaRef = nullptr;
    static remote_service_admin_service_t *rsa = nullptr;

    static service_reference_pt discRef = nullptr;
    static disc_mock_service_t *discMock = nullptr;

    static service_reference_pt testRef = nullptr;
    static tst_service_t *testImport = nullptr;

    static service_reference_pt eplRef = nullptr;
    static endpoint_listener_t *eplService = nullptr; // actually this is the topology manager

    static void setupFm() {
        celix_status_t rc;
        celix_properties_t* config;
        ASSERT_EQ(CELIX_SUCCESS, celix_properties_load("config.properties", 0, &config));

        framework = celix_frameworkFactory_createFramework(config);
        ASSERT_NE(nullptr, framework);

        celix_bundle_t *bundle = celix_framework_getFrameworkBundle(framework);
        EXPECT_NE(nullptr, bundle);

        context = celix_framework_getFrameworkContext(framework);
        EXPECT_NE(nullptr, context);

        rc = bundleContext_getServiceReference(context, (char *)CELIX_RSA_REMOTE_SERVICE_ADMIN, &rsaRef);
        EXPECT_EQ(CELIX_SUCCESS, rc);
        EXPECT_TRUE(rsaRef != nullptr);

        rc = bundleContext_getService(context, rsaRef, (void **)&rsa);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        rc = bundleContext_getServiceReference(context, (char *)TOPOLOGYMANAGER_SCOPE_SERVICE, &scopeServiceRef);
        EXPECT_EQ(CELIX_SUCCESS, rc);
        EXPECT_TRUE(scopeServiceRef != nullptr);

        rc = bundleContext_getService(context, scopeServiceRef, (void **)&tmScopeService);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        rc = bundleContext_getServiceReference(context, (char *)CALCULATOR_SERVICE, &calcRef);
        EXPECT_EQ(CELIX_SUCCESS, rc);
        EXPECT_TRUE(calcRef != nullptr);

        rc = bundleContext_getService(context, calcRef, (void **)&calc);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        rc = bundleContext_getServiceReference(context, (char *)DISC_MOCK_SERVICE_NAME, &discRef);
        EXPECT_EQ(CELIX_SUCCESS, rc);
        EXPECT_TRUE(discRef != nullptr);

        rc = bundleContext_getService(context, discRef, (void **)&discMock);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        printf("==> Finished setup.\n");
    }

    static void teardownFm() {
        printf("==> Starting teardown.\n");
        int rc = 0;

        rc = bundleContext_ungetService(context, scopeServiceRef, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, rc);
        rc = bundleContext_ungetServiceReference(context,scopeServiceRef);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        rc = bundleContext_ungetService(context, calcRef, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, rc);
        rc = bundleContext_ungetServiceReference(context,calcRef);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        rc = bundleContext_ungetService(context, rsaRef, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, rc);
        rc = bundleContext_ungetServiceReference(context,rsaRef);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        rc = bundleContext_ungetService(context, discRef, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, rc);
        rc = bundleContext_ungetServiceReference(context,discRef);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        celix_frameworkFactory_destroyFramework(framework);

        scopeServiceRef = nullptr;
        tmScopeService = nullptr;
        calcRef = nullptr;
        calc = nullptr;

        rsaRef = nullptr;
        rsa = nullptr;
        discRef = nullptr;
        discMock = nullptr;

        testRef = nullptr;
        testImport = nullptr;

        eplRef = nullptr;
        eplService = nullptr;

        context = nullptr;
        framework = nullptr;
    }

    static void setupFmImport() {
        celix_status_t rc;
        celix_properties_t* config;
        ASSERT_EQ(CELIX_SUCCESS, celix_properties_load("config_import.properties", 0, &config));

        framework = celix_frameworkFactory_createFramework(config);
        ASSERT_NE(nullptr, framework);

        celix_bundle_t *bundle = celix_framework_getFrameworkBundle(framework);
        ASSERT_NE(nullptr, bundle);

        context = celix_framework_getFrameworkContext(framework);
        ASSERT_NE(nullptr, context);

        celix_array_list_t* bundles = celix_bundleContext_listBundles(context);
        EXPECT_EQ(celix_arrayList_size(bundles), 4); //rsa, calculator, topman, test bundle
        celix_arrayList_destroy(bundles);

        rc = bundleContext_getServiceReference(context, (char *)CELIX_RSA_REMOTE_SERVICE_ADMIN, &rsaRef);
        EXPECT_EQ(CELIX_SUCCESS, rc);
        EXPECT_TRUE(rsaRef != nullptr);

        rc = bundleContext_getService(context, rsaRef, (void **)&rsa);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        rc = bundleContext_getServiceReference(context, (char *)TOPOLOGYMANAGER_SCOPE_SERVICE, &scopeServiceRef);
        EXPECT_EQ(CELIX_SUCCESS, rc);
        EXPECT_TRUE(scopeServiceRef != nullptr);

        rc = bundleContext_getService(context, scopeServiceRef, (void **)&tmScopeService);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        rc = bundleContext_getServiceReference(context, (char *)TST_SERVICE_NAME, &testRef);
        EXPECT_EQ(CELIX_SUCCESS, rc);
        EXPECT_TRUE(testRef != nullptr);

        rc = bundleContext_getService(context, testRef, (void **)&testImport);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        rc = bundleContext_getServiceReference(context, (char*) CELIX_RSA_ENDPOINT_LISTENER_SERVICE_NAME, &eplRef);
        EXPECT_EQ(CELIX_SUCCESS, rc);
        EXPECT_TRUE(eplRef != nullptr);

        rc = bundleContext_getService(context, eplRef, (void **)&eplService);
        EXPECT_EQ(CELIX_SUCCESS, rc);
    }

    static void teardownFmImport() {
        int rc = 0;

        rc = bundleContext_ungetService(context, rsaRef, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, rc);
        rc = bundleContext_ungetServiceReference(context,rsaRef);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        rc = bundleContext_ungetService(context, scopeServiceRef, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, rc);
        rc = bundleContext_ungetServiceReference(context,scopeServiceRef);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        rc = bundleContext_ungetService(context, testRef, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, rc);
        rc = bundleContext_ungetServiceReference(context,testRef);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        rc = bundleContext_ungetService(context, eplRef, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, rc);
        rc = bundleContext_ungetServiceReference(context,eplRef);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        celix_frameworkFactory_destroyFramework(framework);

        scopeServiceRef = nullptr;
        tmScopeService = nullptr;
        calcRef = nullptr;
        calc = nullptr;

        rsaRef = nullptr;
        rsa = nullptr;
        discRef = nullptr;
        discMock = nullptr;

        testRef = nullptr;
        testImport = nullptr;

        eplRef = nullptr;
        eplService = nullptr;

        context = nullptr;
        framework = nullptr;
    }

    /// \TEST_CASE_ID{1}
    /// \TEST_CASE_TITLE{Test register scope service}
    /// \TEST_CASE_REQ{REQ-1}
    /// \TEST_CASE_DESC Checks if 3 bundles are installed after the framework setup
    static void testBundles(void) {
        printf("Begin: %s\n", __func__);
            celix_array_list_t* bundles = nullptr;

            int rc = bundleContext_getBundles(context, &bundles);
            EXPECT_EQ(0, rc);
            EXPECT_EQ(5, celix_arrayList_size(bundles)); //framework, scopeService & calc & rsa

            /*
            int size = arrayList_size(bundles);
            int i;
            for (i = 0; i < size; i += 1) {
                celix_bundle_t *bundle = nullptr;
                module_pt module = nullptr;
                char *name = nullptr;

                bundle = (celix_bundle_t *) arrayList_get(bundles, i);
                bundle_getCurrentModule(bundle, &module);
                module_getSymbolicName(module, &name);
                printf("got bundle with symbolic name '%s'", name);
            }*/

            celix_arrayList_destroy(bundles);
            printf("End: %s\n", __func__);
    }

    static void scopeInit(const char *fileName, int *nr_exported, int *nr_imported)
    {
        celix_status_t status = CELIX_SUCCESS;
        celix_status_t added;

        json_t *js_root;
        json_error_t error;
        celix_properties_t *properties;

        *nr_exported = 0;
        *nr_imported = 0;
        js_root = json_load_file(fileName, 0, &error);

        if (js_root != nullptr) {
            json_t *js_exportServices = json_object_get(js_root, JSON_EXPORT_SERVICES);
            json_t *js_importServices = json_object_get(js_root, JSON_IMPORT_SERVICES);

            if (js_exportServices != nullptr)  {
                if (json_is_array(js_exportServices)) {
                    int i = 0;
                    int size = json_array_size(js_exportServices);

                    for (; i < size; ++i) {
                        json_t* js_service = json_array_get(js_exportServices, i);

                        if (json_is_object(js_service)) {
                            json_t* js_filter = json_object_get(js_service, JSON_SERVICE_NAME);
                            json_t* js_serviceZone = json_object_get(js_service, JSON_SERVICE_ZONE);
                            json_t* js_key1 = json_object_get(js_service, JSON_SERVICE_KEY1);
                            json_t* js_key2 = json_object_get(js_service, JSON_SERVICE_KEY2);

                            properties = celix_properties_create();
                            if (js_serviceZone != nullptr) {
                                celix_properties_set(properties, (char*)JSON_SERVICE_ZONE,
                                                                 (char*)json_string_value(js_serviceZone));
                            }
                            if (js_key1 != nullptr) {
                                celix_properties_set(properties, (char*)JSON_SERVICE_KEY1,
                                                                 (char*)json_string_value(js_key1));
                            }
                            if (js_key2 != nullptr) {
                                celix_properties_set(properties, (char*)JSON_SERVICE_KEY2,
                                                                 (char*)json_string_value(js_key2));
                            }

                            added = tmScopeService->addExportScope(tmScopeService->handle, (char*)json_string_value(js_filter), properties);
                            if (added == CELIX_SUCCESS) {
                                (*nr_exported)++;
                            }
                         }
                     }
                 }
             }

            if (js_importServices != nullptr)  {
                if (json_is_array(js_importServices)) {
                    int i = 0;
                    int size = json_array_size(js_importServices);

                    for (; i < size; ++i) {
                        json_t* js_service = json_array_get(js_importServices, i);

                        if (json_is_object(js_service)) {
                            json_t* js_filter = json_object_get(js_service, JSON_SERVICE_NAME);

                            added = tmScopeService->addImportScope(tmScopeService->handle, (char*)json_string_value(js_filter));
                            if (added == CELIX_SUCCESS) {
                                (*nr_imported)++;
                            }
                        }
                    }
                }
            }

            json_decref(js_root);
        }
        else
        {
            printf("File error: %s\n", error.text);
            printf("File error: source %s\n", error.source);
            printf("File error: line %d position %d\n", error.line, error.position);
            status = CELIX_FILE_IO_EXCEPTION;
        }
        EXPECT_EQ(CELIX_SUCCESS, status);
    }

    /// \TEST_CASE_ID{2}
    /// \TEST_CASE_TITLE{Test scope initialisation}
    /// \TEST_CASE_REQ{REQ-2}
    /// \TEST_CASE_DESC Checks if scopes can be added, but not twice
    static void testScope(void) {
        int nr_exported;
        int nr_imported;
        celix_array_list_t* epList;

        printf("\nBegin: %s\n", __func__);
        scopeInit("scope.json", &nr_exported, &nr_imported);
        EXPECT_EQ(2, nr_exported);
        EXPECT_EQ(0, nr_imported);

        discMock->getEPDescriptors(discMock->handle, &epList);
        // We export one service: Calculator, which has DFI bundle info
        EXPECT_EQ(1, celix_arrayList_size(epList));
        for (int i = 0; i < celix_arrayList_size(epList); i++) {
            endpoint_description_t *ep = (endpoint_description_t *) celix_arrayList_get(epList, i);
            celix_properties_t *props = ep->properties;
            const char* value = celix_properties_get(props, "key2", "");
            EXPECT_STREQ("inaetics", value);
            /*
            printf("Service: %s ", ep->service);
            hash_map_iterator_pt iter = hashMapIterator_create(props);
            while (hashMapIterator_hasNext(iter)) {
                hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
                printf("%s - %s\n", (char*)hashMapEntry_getKey(entry),
                                   (char*)hashMapEntry_getValue(entry));
            }
            printf("\n");
            hashMapIterator_destroy(iter);
            */
        }
        printf("End: %s\n", __func__);
    }

    /// \TEST_CASE_ID{3}
    /// \TEST_CASE_TITLE{Test scope initialisation}
    /// \TEST_CASE_REQ{REQ-3}
    /// \TEST_CASE_DESC Checks if scopes can be added, but not twice
    static void testScope2(void) {
        int nr_exported;
        int nr_imported;
        celix_array_list_t* epList;
        printf("\nBegin: %s\n", __func__);
        scopeInit("scope2.json", &nr_exported, &nr_imported);
        EXPECT_EQ(3, nr_exported);
        EXPECT_EQ(1, nr_imported);
        discMock->getEPDescriptors(discMock->handle, &epList);
        // We export one service: Calculator, which has DFI bundle info
        EXPECT_EQ(1, celix_arrayList_size(epList));
        for (int i = 0; i < celix_arrayList_size(epList); i++) {
            endpoint_description_t *ep = (endpoint_description_t *) celix_arrayList_get(epList, i);
            celix_properties_t *props = ep->properties;
            const char* value = celix_properties_get(props, "key2", "");
            EXPECT_STREQ("inaetics", value);
        }
        printf("End: %s\n", __func__);
    }

    /// \TEST_CASE_ID{4}
    /// \TEST_CASE_TITLE{Test scope initialisation}
    /// \TEST_CASE_REQ{REQ-4}
    /// \TEST_CASE_DESC Checks if scopes can be added, but not twice
    static void testScope3(void) {
        int nr_exported;
        int nr_imported;
        celix_array_list_t* epList;
        printf("\nBegin: %s\n", __func__);
        scopeInit("scope3.json", &nr_exported, &nr_imported);
        EXPECT_EQ(3, nr_exported);
        EXPECT_EQ(1, nr_imported);
        discMock->getEPDescriptors(discMock->handle, &epList);
        // We export one service: Calculator, which has DFI bundle info
        EXPECT_EQ(1, celix_arrayList_size(epList));
        for (int i = 0; i < celix_arrayList_size(epList); i++) {
            endpoint_description_t *ep = (endpoint_description_t *) celix_arrayList_get(epList, i);
            celix_properties_t *props = ep->properties;
            const char* value = celix_properties_get(props, "key2", "");
            EXPECT_STREQ("inaetics", value);
        }
        printf("End: %s\n", __func__);
    }

    /// \TEST_CASE_ID{6}
    /// \TEST_CASE_TITLE{Test import scope}
    /// \TEST_CASE_REQ{REQ-3}
    /// \TEST_CASE_DESC Checks if import succeeds if there is no import scope defined
    static void testImportScope(void) {
        int nr_exported;
        int nr_imported;
        printf("\nBegin: %s\n", __func__);

        scopeInit("scope.json", &nr_exported, &nr_imported);
        EXPECT_EQ(0, nr_imported);
        int rc = 0;

        endpoint_description_t *endpoint = nullptr;

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, CELIX_RSA_ENDPOINT_SERVICE_ID, "42");
        celix_properties_set(props, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, "eec5404d-51d0-47ef-8d86-c825a8beda42");
        celix_properties_set(props, CELIX_RSA_ENDPOINT_ID, "eec5404d-51d0-47ef-8d86-c825a8beda42-42");
        celix_properties_set(props, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, TST_CONFIGURATION_TYPE);
        celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_NAME, "org.apache.celix.test.MyBundle");
        auto* v = celix_version_create(1, 0, 0, nullptr);
        celix_properties_assignVersion(props, "service.version", v);
        celix_properties_set(props, "zone", "a_zone");

        rc = endpointDescription_create(props, &endpoint);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        rc = eplService->endpointAdded(eplService->handle, endpoint, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        celix_framework_waitForEmptyEventQueue(framework);

        //Then endpointImported becomes true within 1 second
        bool imported;
        int iteration = 0;
        do {
            imported = testImport->IsImported(testImport);
            usleep(1000);
        } while (!imported && iteration++ < 1000);

        rc = eplService->endpointRemoved(eplService->handle, endpoint, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        celix_framework_waitForEmptyEventQueue(framework);

        rc = endpointDescription_destroy(endpoint);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        printf("*****After importService\n");
        printf("End: %s\n", __func__);
    }

    /// \TEST_CASE_ID{7}
    /// \TEST_CASE_TITLE{Test import scope}
    /// \TEST_CASE_REQ{REQ-3}
    /// \TEST_CASE_DESC Checks if import succeeds if there is a matching import scope defined
    static void testImportScopeMatch(void) {
        int nr_exported;
        int nr_imported;
        printf("\nBegin: %s\n", __func__);

        scopeInit("scope2.json", &nr_exported, &nr_imported);
        EXPECT_EQ(1, nr_imported);
        int rc = 0;

        endpoint_description_t *endpoint = nullptr;

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, CELIX_RSA_ENDPOINT_SERVICE_ID, "42");
        celix_properties_set(props, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, "eec5404d-51d0-47ef-8d86-c825a8beda42");
        celix_properties_set(props, CELIX_RSA_ENDPOINT_ID, "eec5404d-51d0-47ef-8d86-c825a8beda42-42");
        celix_properties_set(props, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, TST_CONFIGURATION_TYPE);
        celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_NAME, "org.apache.celix.test.MyBundle");
        auto* v = celix_version_create(1, 0, 0, nullptr);
        celix_properties_assignVersion(props, "service.version", v);
        celix_properties_set(props, "zone", "a_zone");

        rc = endpointDescription_create(props, &endpoint);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        rc = eplService->endpointAdded(eplService->handle, endpoint, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        celix_framework_waitForEmptyEventQueue(framework);

        //Then endpointImported becomes true within 1 second
        bool imported;
        int iteration = 0;
        do {
            imported = testImport->IsImported(testImport);
            usleep(1000);
        } while (!imported && iteration++ < 1000);

        rc = eplService->endpointRemoved(eplService->handle, endpoint, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        celix_framework_waitForEmptyEventQueue(framework);

        rc = endpointDescription_destroy(endpoint);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        printf("End: %s\n", __func__);
    }

    /// \TEST_CASE_ID{8}
    /// \TEST_CASE_TITLE{Test import scope block}
    /// \TEST_CASE_REQ{REQ-3}
    /// \TEST_CASE_DESC Checks if import fails with non matching import scope defined
    static void testImportScopeFail(void) {
        int nr_exported;
        int nr_imported;
        printf("\nBegin: %s\n", __func__);

        scopeInit("scope3.json", &nr_exported, &nr_imported);
        EXPECT_EQ(1, nr_imported);
        int rc = 0;

        endpoint_description_t *endpoint = nullptr;

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, CELIX_RSA_ENDPOINT_SERVICE_ID, "42");
        celix_properties_set(props, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, "eec5404d-51d0-47ef-8d86-c825a8beda42");
        celix_properties_set(props, CELIX_RSA_ENDPOINT_ID, "eec5404d-51d0-47ef-8d86-c825a8beda42-42");
        celix_properties_set(props, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, TST_CONFIGURATION_TYPE);
        celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_NAME, "org.apache.celix.test.MyBundle");
        auto*v = celix_version_create(1, 0, 0, nullptr);
        celix_properties_assignVersion(props, "service.version", v);
        celix_properties_set(props, "zone", "a_zone");

        rc = endpointDescription_create(props, &endpoint);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        rc = eplService->endpointAdded(eplService->handle, endpoint, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        celix_framework_waitForEmptyEventQueue(framework);

        bool imported = testImport->IsImported(testImport);
        EXPECT_EQ(false, imported);

        rc = eplService->endpointRemoved(eplService->handle, endpoint, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        celix_framework_waitForEmptyEventQueue(framework);

        rc = endpointDescription_destroy(endpoint);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        printf("End: %s\n", __func__);
    }

    /// \TEST_CASE_ID{9}
    /// \TEST_CASE_TITLE{Test import scope block}
    /// \TEST_CASE_REQ{REQ-3}
    /// \TEST_CASE_DESC Checks if import fails with non matching import scope defined
    static void testImportScopeMultiple(void) {
        int nr_exported;
        int nr_imported;
        printf("\nBegin: %s\n", __func__);

        scopeInit("scope4.json", &nr_exported, &nr_imported);
        EXPECT_EQ(2, nr_imported);
        int rc = 0;

        endpoint_description_t *endpoint = nullptr;

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, CELIX_RSA_ENDPOINT_SERVICE_ID, "42");
        celix_properties_set(props, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, "eec5404d-51d0-47ef-8d86-c825a8beda42");
        celix_properties_set(props, CELIX_RSA_ENDPOINT_ID, "eec5404d-51d0-47ef-8d86-c825a8beda42-42");
        celix_properties_set(props, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, TST_CONFIGURATION_TYPE);
        celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_NAME, "org.apache.celix.test.MyBundle");
        auto*v = celix_version_create(1, 0, 0, nullptr);
        celix_properties_assignVersion(props, "service.version", v);
        celix_properties_set(props, "zone", "a_zone");

        rc = endpointDescription_create(props, &endpoint);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        rc = eplService->endpointAdded(eplService->handle, endpoint, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        celix_framework_waitForEmptyEventQueue(framework);

        //Then endpointImported becomes true within 1 second
        bool imported;
        int iteration = 0;
        do {
            imported = testImport->IsImported(testImport);
            usleep(1000);
        } while (!imported && iteration++ < 1000);

        EXPECT_EQ(true, imported);

        rc = eplService->endpointRemoved(eplService->handle, endpoint, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        celix_framework_waitForEmptyEventQueue(framework);

        rc = endpointDescription_destroy(endpoint);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        printf("End: %s\n", __func__);
    }
}

class RemoteServiceTopologyAdminExportTestSuite : public ::testing::Test {
public:
    RemoteServiceTopologyAdminExportTestSuite() {
        setupFm();
    }
    ~RemoteServiceTopologyAdminExportTestSuite() override {
        teardownFm();
    }

};

class RemoteServiceTopologyAdminImportTestSuite : public ::testing::Test {
public:
    RemoteServiceTopologyAdminImportTestSuite() {
        setupFmImport();
    }
    ~RemoteServiceTopologyAdminImportTestSuite() override {
        teardownFmImport();
    }

};

TEST_F(RemoteServiceTopologyAdminImportTestSuite, scope_import_multiple) {
    testImportScopeMultiple();
}

TEST_F(RemoteServiceTopologyAdminImportTestSuite, scope_import_fail) {
    testImportScopeFail();
}

TEST_F(RemoteServiceTopologyAdminImportTestSuite, scope_import_match) {
    testImportScopeMatch();
}

TEST_F(RemoteServiceTopologyAdminImportTestSuite, scope_import) {
    testImportScope();
}

TEST_F(RemoteServiceTopologyAdminExportTestSuite, scope_init3) {
    testScope3();
}

TEST_F(RemoteServiceTopologyAdminExportTestSuite, scope_init2) {
    testScope2();
}

TEST_F(RemoteServiceTopologyAdminExportTestSuite, scope_init) {
    testScope();
}

TEST_F(RemoteServiceTopologyAdminExportTestSuite, init_test) {
    testBundles();
}
