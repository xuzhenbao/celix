/*
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing,
  software distributed under the License is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  KIND, either express or implied.  See the License for the
  specific language governing permissions and limitations
  under the License.
 */

#include "asprintf_ei.h"
#include "bundle_archive_private.h"
#include "celix_bundle_cache.h"
#include "celix_constants.h"
#include "celix_file_utils.h"
#include "celix_hash_map_ei.h"
#include "celix_log.h"
#include "celix_properties.h"
#include "celix_utils_ei.h"
#include "framework_private.h"
#include "malloc_ei.h"
#include "gtest/gtest.h"
#include <string>

class CelixBundleCacheErrorInjectionTestSuite : public ::testing::Test {
public:
    CelixBundleCacheErrorInjectionTestSuite() {
        fw.configurationMap = celix_properties_create();
        fw.logger = celix_frameworkLogger_create(CELIX_LOG_LEVEL_TRACE);
    }

    ~CelixBundleCacheErrorInjectionTestSuite() override {
        celix_ei_expect_celix_utils_writeOrCreateString(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_createDirectory(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_celix_utils_deleteDirectory(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_asprintf(nullptr, 0, -1);
        celix_ei_expect_celix_stringHashMap_create(nullptr, 0, nullptr);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_frameworkLogger_destroy(fw.logger);
        celix_properties_destroy(fw.configurationMap);
    }

    struct celix_framework fw {};
};

TEST_F(CelixBundleCacheErrorInjectionTestSuite, CacheCreateErrorTest) {
    celix_bundle_cache_t *cache = nullptr;
    celix_ei_expect_calloc((void*) celix_bundleCache_create, 0, nullptr);
    EXPECT_EQ(CELIX_ENOMEM, celix_bundleCache_create(&fw, &cache));
    EXPECT_EQ(nullptr, cache);
    celix_ei_expect_celix_stringHashMap_create((void*) celix_bundleCache_create, 0, nullptr);
    EXPECT_EQ(CELIX_ENOMEM, celix_bundleCache_create(&fw, &cache));
    EXPECT_EQ(nullptr, cache);
    celix_properties_setBool(fw.configurationMap, CELIX_FRAMEWORK_CACHE_USE_TMP_DIR, true);
    celix_ei_expect_asprintf((void*) celix_bundleCache_create, 0, -1);
    EXPECT_EQ(CELIX_ENOMEM, celix_bundleCache_create(&fw, &cache));
    EXPECT_EQ(nullptr, cache);
    celix_properties_setBool(fw.configurationMap, CELIX_FRAMEWORK_CACHE_USE_TMP_DIR, false);
    celix_ei_expect_celix_utils_strdup((void*) celix_bundleCache_create, 0, nullptr);
    EXPECT_EQ(CELIX_ENOMEM, celix_bundleCache_create(&fw, &cache));
    EXPECT_EQ(nullptr, cache);
    celix_properties_setBool(fw.configurationMap, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, true);
    celix_ei_expect_celix_utils_deleteDirectory((void*) celix_bundleCache_create, 1, CELIX_FILE_IO_EXCEPTION);
    EXPECT_EQ(CELIX_FILE_IO_EXCEPTION, celix_bundleCache_create(&fw, &cache));
    EXPECT_EQ(nullptr, cache);
    celix_properties_setBool(fw.configurationMap, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, false);
    celix_ei_expect_celix_utils_createDirectory((void*) celix_bundleCache_create, 0, CELIX_FILE_IO_EXCEPTION);
    EXPECT_EQ(CELIX_FILE_IO_EXCEPTION, celix_bundleCache_create(&fw, &cache));
    EXPECT_EQ(nullptr, cache);
}

TEST_F(CelixBundleCacheErrorInjectionTestSuite, CacheDeleteErrorTest) {
    celix_bundle_cache_t *cache = nullptr;
    celix_properties_setBool(fw.configurationMap, CELIX_FRAMEWORK_CACHE_USE_TMP_DIR, true);
    EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_create(&fw, &cache));
    celix_ei_expect_celix_utils_deleteDirectory((void*) celix_bundleCache_deleteCacheDir, 0, CELIX_FILE_IO_EXCEPTION);
    EXPECT_EQ(CELIX_FILE_IO_EXCEPTION, celix_bundleCache_deleteCacheDir(cache));
    EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_destroy(cache));
}

TEST_F(CelixBundleCacheErrorInjectionTestSuite, ArchiveCreateErrorTest) {
    celix_bundle_cache_t *cache = nullptr;
    bundle_archive_t* archive = nullptr;
    celix_properties_setBool(fw.configurationMap, CELIX_FRAMEWORK_CACHE_USE_TMP_DIR, true);
    EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_create(&fw, &cache));
    fw.cache = cache;
    celix_ei_expect_celix_utils_writeOrCreateString((void*) celix_bundleCache_createArchive, 0, nullptr);
    EXPECT_EQ(CELIX_ENOMEM, celix_bundleCache_createArchive(cache, 1, SIMPLE_TEST_BUNDLE1_LOCATION, &archive));
    EXPECT_EQ(nullptr, archive);
    EXPECT_EQ(-1, celix_bundleCache_findBundleIdForLocation(cache, SIMPLE_TEST_BUNDLE1_LOCATION));
    EXPECT_FALSE(celix_bundleCache_isBundleIdAlreadyUsed(cache, 1));
    EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_destroy(cache));

    EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_create(&fw, &cache));
    fw.cache = cache;
    celix_ei_expect_calloc((void*) celix_bundleArchive_create, 0, nullptr);
    EXPECT_EQ(CELIX_ENOMEM, celix_bundleCache_createArchive(cache, 1, SIMPLE_TEST_BUNDLE1_LOCATION, &archive));
    EXPECT_EQ(nullptr, archive);
    EXPECT_EQ(-1, celix_bundleCache_findBundleIdForLocation(cache, SIMPLE_TEST_BUNDLE1_LOCATION));
    EXPECT_FALSE(celix_bundleCache_isBundleIdAlreadyUsed(cache, 1));
    EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_destroy(cache));
}

TEST_F(CelixBundleCacheErrorInjectionTestSuite, ArchiveDestroyErrorTest) {
    celix_bundle_cache_t *cache = nullptr;
    bundle_archive_t* archive = nullptr;
    celix_properties_setBool(fw.configurationMap, CELIX_FRAMEWORK_CACHE_USE_TMP_DIR, true);
    EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_create(&fw, &cache));
    fw.cache = cache;
    EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_createArchive(cache, 1, SIMPLE_TEST_BUNDLE1_LOCATION, &archive));
    celix_ei_expect_celix_utils_deleteDirectory((void*) celix_bundleCache_destroyArchive, 1, CELIX_FILE_IO_EXCEPTION);
    std::string storeRoot = celix_bundleArchive_getPersistentStoreRoot(archive);
    EXPECT_EQ(CELIX_FILE_IO_EXCEPTION, celix_bundleCache_destroyArchive(cache, archive));
    EXPECT_TRUE(celix_utils_directoryExists(storeRoot.c_str()));
    EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_destroy(cache));
}

TEST_F(CelixBundleCacheErrorInjectionTestSuite, CreateBundleArchivesCacheErrorTest) {
    celix_bundle_cache_t *cache = nullptr;
    celix_properties_set(fw.configurationMap, CELIX_AUTO_START_1, SIMPLE_TEST_BUNDLE1_LOCATION);
    celix_properties_setBool(fw.configurationMap, CELIX_FRAMEWORK_CACHE_USE_TMP_DIR, true);
    EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_create(&fw, &cache));
    fw.cache = cache;
    celix_ei_expect_celix_utils_deleteDirectory((void*) celix_bundleCache_createBundleArchivesCache, 0, CELIX_FILE_IO_EXCEPTION);
    EXPECT_EQ(CELIX_FILE_IO_EXCEPTION, celix_bundleCache_createBundleArchivesCache(&fw, true));
    celix_ei_expect_celix_utils_writeOrCreateString((void*) celix_bundleCache_createBundleArchivesCache, 1, nullptr);
    EXPECT_EQ(CELIX_ENOMEM, celix_bundleCache_createBundleArchivesCache(&fw, true));
    celix_properties_unset(fw.configurationMap, CELIX_AUTO_START_1);
    celix_properties_set(fw.configurationMap, CELIX_AUTO_INSTALL, SIMPLE_TEST_BUNDLE1_LOCATION);
    celix_ei_expect_celix_utils_writeOrCreateString((void*) celix_bundleCache_createBundleArchivesCache, 1, nullptr);
    EXPECT_EQ(CELIX_ENOMEM, celix_bundleCache_createBundleArchivesCache(&fw, true));
    celix_ei_expect_celix_utils_writeOrCreateString((void*) celix_bundleCache_createArchive, 0, nullptr);
    EXPECT_EQ(CELIX_ENOMEM, celix_bundleCache_createBundleArchivesCache(&fw, true));
    EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_destroy(cache));
}