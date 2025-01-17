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

#include "celix_bundle_manifest.h"
#include "celix_bundle_private.h"
#include "celix_module.h"
#include "celix_properties.h"
#include "celix_properties_type.h"
#include "framework_private.h"
#include "utils.h"
#include "celix_file_utils.h"
#include "bundle_context_private.h"
#include "service_tracker_private.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <celix_constants.h>
#include <unistd.h>

static char* celix_bundle_getBundleOrPersistentStoreEntry(const celix_bundle_t* bnd, bool bundleEntry, const char* name);
celix_status_t bundle_createModule(bundle_pt bundle, celix_module_t** module);
celix_status_t bundle_closeRevisions(const_bundle_pt bundle);

celix_status_t celix_bundle_createFromArchive(celix_framework_t *framework, celix_bundle_archive_t* archive, celix_bundle_t **bundleOut) {
    celix_status_t status = CELIX_SUCCESS;
    celix_bundle_t* bundle = calloc(1, sizeof(*bundle));

    if (!bundle) {
        status = CELIX_ENOMEM;
        fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot create bundle from archive, out of memory.");
        return status;
    }

    bundle->framework = framework;
    bundle->archive = archive;
    bundle->modules = celix_arrayList_create();
    bundle->state = OSGI_FRAMEWORK_BUNDLE_INSTALLED;
    bundle->handle = NULL;
    bundle->activator = NULL;
    bundle->context = NULL;

    if (bundle->modules == NULL) {
        status = CELIX_ENOMEM;
        fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot create bundle from archive, out of memory.");
        free(bundle);
        return status;
    }

    celix_module_t* module;
    status = bundle_createModule(bundle, &module);
    if (status != CELIX_SUCCESS) {
        fw_logCode(framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot create bundle from archive, cannot create module.");
        celix_arrayList_destroy(bundle->modules);
        free(bundle);
        return status;
    } else {
        bundle_addModule(bundle, module);
    }

    *bundleOut = bundle;
    return status;
}

celix_status_t bundle_destroy(bundle_pt bundle) {
    for (int i = 0; i < celix_arrayList_size(bundle->modules); ++i) {
        celix_module_t* module = celix_arrayList_get(bundle->modules, i);
        module_destroy(module);
    }
    celix_arrayList_destroy(bundle->modules);

    free(bundle);

    return CELIX_SUCCESS;
}

celix_status_t bundle_getArchive(const_bundle_pt bundle, celix_bundle_archive_t** archive) {
	celix_status_t status = CELIX_SUCCESS;
	if (bundle != NULL && *archive == NULL) {
		*archive = bundle->archive;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}
	return status;
}

celix_status_t bundle_getCurrentModule(const_bundle_pt bundle, celix_module_t** module) {
	celix_status_t status = CELIX_SUCCESS;

	if (bundle == NULL || celix_arrayList_size(bundle->modules)==0 ) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*module = celix_arrayList_get(bundle->modules, celix_arrayList_size(bundle->modules) - 1);
	}

	return status;
}

void * bundle_getHandle(bundle_pt bundle) {
	return bundle->handle;
}

void bundle_setHandle(bundle_pt bundle, void * handle) {
	bundle->handle = handle;
}

celix_bundle_activator_t* bundle_getActivator(const_bundle_pt bundle) {
	return bundle->activator;
}

celix_status_t bundle_setActivator(bundle_pt bundle, celix_bundle_activator_t *activator) {
	bundle->activator = activator;
	return CELIX_SUCCESS;
}

celix_bundle_context_t* celix_bundle_getContext(const_bundle_pt bundle) { return bundle->context; }

void celix_bundle_setContext(bundle_pt bundle, bundle_context_pt context) { bundle->context = context; }

celix_status_t bundle_getState(const_bundle_pt bundle, bundle_state_e *state) {
	if (bundle==NULL) {
		*state = CELIX_BUNDLE_STATE_UNKNOWN;
		return CELIX_BUNDLE_EXCEPTION;
	}
    __atomic_load(&bundle->state, state, __ATOMIC_ACQUIRE);
	return CELIX_SUCCESS;
}

celix_status_t bundle_setState(bundle_pt bundle, bundle_state_e state) {
    __atomic_store_n(&bundle->state, state, __ATOMIC_RELEASE);
	return CELIX_SUCCESS;
}

celix_status_t bundle_createModule(bundle_pt bundle, celix_module_t** moduleOut) {
    celix_status_t status = CELIX_SUCCESS;

    celix_module_t* module = module_create(bundle);
    if (!module) {
        status = CELIX_BUNDLE_EXCEPTION;
        fw_logCode(bundle->framework->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot create module.");
        return status;
    }

    const char* symName = NULL;
    status = module_getSymbolicName(module, &symName);
    assert(status == CELIX_SUCCESS);
    /*
     * NOTE only allowing a single bundle with a symbolic name.
     * OSGi spec allows same symbolic name and different versions, but this is risky with
     * the behaviour of dlopen when opening shared libraries with the same SONAME.
     */
    bool alreadyInstalled = celix_framework_isBundleAlreadyInstalled(bundle->framework, symName);
    if (alreadyInstalled) {
        status = CELIX_BUNDLE_EXCEPTION;
        fw_logCode(bundle->framework->logger,
                   CELIX_LOG_LEVEL_ERROR,
                   status,
                   "Cannot create module, bundle with symbolic name '%s' already installed.",
                   symName);
    }
    if (status == CELIX_SUCCESS) {
        *moduleOut = module;
    } else {
        module_destroy(module);
    }

    return status;
}

celix_status_t bundle_addModule(bundle_pt bundle, celix_module_t* module) {
    celix_arrayList_add(bundle->modules, module);

    //set new module info
    const char *sn = NULL;
    const char *n = NULL;
    const char *g = NULL;
    const char *d = NULL;
    module_getSymbolicName(module, &sn);
    module_getGroup(module, &g);
    module_getName(module, &n);
    module_getDescription(module, &d);

    return CELIX_SUCCESS;
}

celix_status_t bundle_isSystemBundle(const_bundle_pt bundle, bool* systemBundle) {
    celix_bundle_archive_t* archive = celix_bundle_getArchive(bundle);
    if (archive) {
        long bundleId = celix_bundleArchive_getId(archive);
        *systemBundle = (bundleId == CELIX_FRAMEWORK_BUNDLE_ID);
    } else {
        fw_log(bundle->framework->logger, CELIX_LOG_LEVEL_ERROR, "Failed to check if bundle is the systembundle");
        return CELIX_BUNDLE_EXCEPTION;
    }
    return CELIX_SUCCESS;
}

celix_status_t bundle_getBundleId(const bundle_t *bundle, long *bndId) {
	celix_status_t status = CELIX_SUCCESS;
	long id = celix_bundle_getId(bundle);
	if (id >= 0) {
		*bndId = id;
	} else {
		status = CELIX_BUNDLE_EXCEPTION;
		*bndId = -1;
	}
	return status;
}

celix_status_t bundle_getFramework(const_bundle_pt bundle, framework_pt *framework) {
    if (bundle == NULL || framework == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    *framework = bundle->framework;
    return CELIX_SUCCESS;
}


/**********************************************************************************************************************
 **********************************************************************************************************************
 * Updated API
 **********************************************************************************************************************
 **********************************************************************************************************************/

long celix_bundle_getId(const bundle_t* bnd) {
    long bndId = -1;
    celix_bundle_archive_t* archive = celix_bundle_getArchive(bnd);
    if (archive) {
        bndId = celix_bundleArchive_getId(archive);
    } else {
        framework_logIfError(celix_frameworkLogger_globalLogger(),
                             CELIX_BUNDLE_EXCEPTION,
                             NULL,
                             "Failed to get bundle id");
    }
    return bndId;
}

celix_bundle_state_e celix_bundle_getState(const celix_bundle_t *bnd) {
    return __atomic_load_n(&bnd->state, __ATOMIC_ACQUIRE);
}

static char* celix_bundle_getBundleOrPersistentStoreEntry(const celix_bundle_t* bnd, bool bundleEntry, const char* name) {
    celix_bundle_archive_t* archive = NULL;
    celix_status_t status = bundle_getArchive(bnd, &archive);
    if (status != CELIX_SUCCESS) {
        fw_logCode(bnd->framework->logger, CELIX_BUNDLE_EXCEPTION, status, "Failed to get bundle archive");
        return NULL;
    }

    const char *root;
    if (bundleEntry) {
        root = celix_bundleArchive_getCurrentRevisionRoot(archive);
    } else {
        root = celix_bundleArchive_getPersistentStoreRoot(archive);
    }

    char *entry = NULL;
    if (name == NULL || strnlen(name, 1) == 0) { //NULL or ""
        entry = celix_utils_strdup(root);
    } else if ((strnlen(name, 1) > 0) && (name[0] == '/')) {
        asprintf(&entry, "%s%s", root, name);
    } else {
        asprintf(&entry, "%s/%s", root, name);
    }

    if (celix_utils_fileExists(entry)) {
        return entry;
    } else {
        free(entry);
        return NULL;
    }
}

char* celix_bundle_getEntry(const celix_bundle_t* bnd, const char *path) {
	char *entry = NULL;
	if (bnd != NULL && bnd->framework != NULL) {
        entry = celix_bundle_getBundleOrPersistentStoreEntry(bnd, true, path);
	}
	return entry;
}

char* celix_bundle_getDataFile(const celix_bundle_t* bnd, const char *path) {
    char *entry = NULL;
    if (bnd != NULL && bnd->framework != NULL) {
        entry = celix_bundle_getBundleOrPersistentStoreEntry(bnd, false, path);
    }
    return entry;
}

static celix_bundle_manifest_t* celix_bundle_getManifest(const celix_bundle_t* bnd) {
    celix_bundle_manifest_t* man = celix_bundleArchive_getManifest(bnd->archive);
    assert(man); //bundles always have a manifest
    return man;
}

const char* celix_bundle_getManifestValue(const celix_bundle_t* bnd, const char* attribute) {
    celix_bundle_manifest_t* man = celix_bundle_getManifest(bnd);
    assert(man); //bundle always has a manifest
    const celix_properties_t* attr = celix_bundleManifest_getAttributes(man);
    return celix_properties_getAsString(attr, attribute, NULL);
}

const char* celix_bundle_getGroup(const celix_bundle_t *bnd) {
    celix_bundle_manifest_t* man = celix_bundle_getManifest(bnd);
    return celix_bundleManifest_getBundleGroup(man);
}

const char* celix_bundle_getSymbolicName(const celix_bundle_t *bnd) {
    celix_bundle_manifest_t* man = celix_bundle_getManifest(bnd);
    return celix_bundleManifest_getBundleSymbolicName(man);
}

const char* celix_bundle_getName(const celix_bundle_t* bnd) {
    celix_bundle_manifest_t* man = celix_bundle_getManifest(bnd);
    return celix_bundleManifest_getBundleName(man);
}

const char* celix_bundle_getDescription(const celix_bundle_t* bnd) {
    celix_bundle_manifest_t* man = celix_bundle_getManifest(bnd);
    return celix_bundleManifest_getBundleDescription(man);
}

char* celix_bundle_getLocation(const celix_bundle_t *bnd) {
    char* result = NULL;
    if (bnd->archive != NULL) {
        const char* loc = celix_bundleArchive_getLocation(bnd->archive);
        result = celix_utils_strdup(loc);
        if (!result) {
            fw_log(bnd->framework->logger, CELIX_BUNDLE_EXCEPTION, "Failed to allocate memory for bundle location");
        }
    }
    return result;
}

const celix_version_t* celix_bundle_getVersion(const celix_bundle_t *bnd) {
    const celix_version_t* result = NULL;
    celix_module_t* mod = NULL;
    bundle_getCurrentModule(bnd, &mod);
    if (mod != NULL) {
        result = module_getVersion(mod);
    }
    return result;
}

bool celix_bundle_isSystemBundle(const celix_bundle_t *bnd) {
    return bnd != NULL && celix_bundle_getId(bnd) == 0;
}

celix_array_list_t* celix_bundle_listRegisteredServices(const celix_bundle_t *bnd) {
    long bndId = celix_bundle_getId(bnd);
    celix_array_list_t* result = celix_arrayList_create();
    celix_array_list_t *svcIds = celix_serviceRegistry_listServiceIdsForOwner(bnd->framework->registry, bndId);
    for (int i = 0; i < celix_arrayList_size(svcIds); ++i) {
        long svcId = celix_arrayList_getLong(svcIds, i);
        celix_bundle_service_list_entry_t* entry = calloc(1, sizeof(*entry));
        entry->serviceId = svcId;
        entry->bundleOwner = bndId;
        celix_serviceRegistry_getServiceInfo(bnd->framework->registry, svcId, bndId, &entry->serviceName, &entry->serviceProperties, &entry->factory);
        celix_arrayList_add(result, entry);
    }
    celix_arrayList_destroy(svcIds);
    return result;
}

void celix_bundle_destroyRegisteredServicesList(celix_array_list_t* list) {
    if (list != NULL) {
        for (int i = 0; i < celix_arrayList_size(list); ++i) {
            celix_bundle_service_list_entry_t *entry = celix_arrayList_get(list, i);
            free(entry->serviceName);
            celix_properties_destroy(entry->serviceProperties);
            free(entry);
        }
        celix_arrayList_destroy(list);
    }
}

static void celix_bundle_destroyServiceTrackerListCallback(void *data) {
    celix_bundle_service_tracker_list_entry_t *entry = data;
    free(entry->filter);
    free(entry->serviceName);
    free(entry);
}

celix_array_list_t* celix_bundle_listServiceTrackers(const celix_bundle_t *bnd) {
    celix_array_list_create_options_t opts = CELIX_EMPTY_ARRAY_LIST_CREATE_OPTIONS;
    opts.simpleRemovedCallback = celix_bundle_destroyServiceTrackerListCallback;
    celix_array_list_t* result = celix_arrayList_createWithOptions(&opts);
    celixThreadRwlock_readLock(&bnd->context->lock);
    CELIX_LONG_HASH_MAP_ITERATE(bnd->context->serviceTrackers, iter) {
        celix_bundle_context_service_tracker_entry_t *trkEntry = iter.value.ptrValue;
        if (trkEntry->tracker != NULL) {
            celix_bundle_service_tracker_list_entry_t *entry = calloc(1, sizeof(*entry));
            entry->filter = celix_utils_strdup(trkEntry->tracker->filter);
            entry->nrOfTrackedServices = serviceTracker_nrOfTrackedServices(trkEntry->tracker);
            entry->serviceName = celix_utils_strdup(trkEntry->tracker->serviceName);
            entry->bundleOwner = celix_bundle_getId(bnd);

            if (entry->serviceName != NULL) {
                celix_arrayList_add(result, entry);
            } else {
                framework_logIfError(bnd->framework->logger, CELIX_BUNDLE_EXCEPTION, NULL,
                                     "Failed to get service name from tracker. filter is %s", entry->filter);
                celix_bundle_destroyServiceTrackerListCallback(entry);
            }
        }
    }
    celixThreadRwlock_unlock(&bnd->context->lock);
    return result;
}

celix_bundle_archive_t* celix_bundle_getArchive(const celix_bundle_t *bundle) {
    celix_bundle_archive_t* archive = NULL;
    (void)bundle_getArchive(bundle, &archive);
    return archive;
}
