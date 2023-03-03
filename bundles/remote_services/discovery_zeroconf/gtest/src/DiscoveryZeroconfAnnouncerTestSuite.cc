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
#include <discovery_zeroconf_announcer.h>
#include <discovery_zeroconf_constants.h>
#include <dns_sd.h>
#include <remote_constants.h>
extern "C" {
#include <endpoint_description.h>
}
#include <endpoint_listener.h>
#include <celix_log_helper.h>
#include <celix_api.h>
#include <celix_errno.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <string.h>
#include <stdbool.h>

static void KillMDNSDeamon(void) {
    system("kill -TERM `cat /var/run/mdnsd.pid`");
}

static void StartMDNSDeamon(void) {
    system(MDNSD);
}

class DiscoveryZeroconfAnnouncerTestSuite : public ::testing::Test {
public:
    static void SetUpTestCase() {
        StartMDNSDeamon();
    }

    static void TearDownTestCase() {
        KillMDNSDeamon();
    }
    DiscoveryZeroconfAnnouncerTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_FRAMEWORK_STORAGE_CLEAN_NAME, "true");
        celix_properties_set(props, OSGI_FRAMEWORK_FRAMEWORK_STORAGE, ".dzc_announcer_test_cache");
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};
        auto* logHelperPtr = celix_logHelper_create(ctxPtr,"DiscoveryZeroconf");
        logHelper = std::shared_ptr<celix_log_helper_t>{logHelperPtr, [](auto*l){ celix_logHelper_destroy(l);}};
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    std::shared_ptr<celix_log_helper_t> logHelper{};
    int ifIndex{0};
    bool restartMDNSDeamon{false};
};

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, CreateAndDestroyAnnouncer) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

static void OnServiceResolveCallback(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *fullname, const char *host, uint16_t port, uint16_t txtLen, const unsigned char *txtRecord, void *context) {
    (void)sdRef;
    (void)flags;
    (void)interfaceIndex;
    (void)fullname;
    (void)context;
    EXPECT_EQ(errorCode, kDNSServiceErr_NoError);

    EXPECT_STREQ(host, DZC_HOST_DEFAULT);
    EXPECT_EQ(port, ntohs(DZC_PORT_DEFAULT));
    celix_properties_t *prop = (celix_properties_t *)context;
    int cnt = TXTRecordGetCount(txtLen, txtRecord);
    for (int i = 0; i < cnt; ++i) {
        char key[UINT8_MAX+1] = {0};
        char val[UINT8_MAX+1] = {0};
        const void *valPtr = NULL;
        uint8_t valLen = 0;
        DNSServiceErrorType err = TXTRecordGetItemAtIndex(txtLen, txtRecord, i, sizeof(key), key, &valLen, &valPtr);
        EXPECT_EQ(err, kDNSServiceErr_NoError);
        memcpy(val, valPtr, valLen);
        celix_properties_set(prop, key, val);
    }
}

static void OnServiceBrowseCallback(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *instanceName, const char *regtype, const char *replyDomain, void *context) {
    EXPECT_NE(nullptr, sdRef);
    DiscoveryZeroconfAnnouncerTestSuite *t = (DiscoveryZeroconfAnnouncerTestSuite *)context;
    EXPECT_EQ(errorCode, kDNSServiceErr_NoError);
    if ((flags & kDNSServiceFlagsAdd) && (strstr(instanceName, "dzc_test_service") != nullptr) && (int)interfaceIndex == t->ifIndex) {
        DNSServiceRef dsRef{};
        celix_properties_t *prop = celix_properties_create();
        DNSServiceErrorType dnsErr = DNSServiceResolve(&dsRef, 0, interfaceIndex, instanceName, regtype, replyDomain, OnServiceResolveCallback, prop);
        EXPECT_EQ(dnsErr, kDNSServiceErr_NoError);
        DNSServiceProcessResult(dsRef);
        EXPECT_TRUE(celix_properties_getAsLong(prop, DZC_SERVICE_PROPERTIES_SIZE_KEY, 0) > 0);
        //The txt record should not include DZC_SERVICE_ANNOUNCED_IF_INDEX_KEY,DZC_SERVICE_TYPE_KEY
        EXPECT_EQ(nullptr, celix_properties_get(prop, RSA_DISCOVERY_ZEROCONF_SERVICE_ANNOUNCED_IF_INDEX, nullptr));
        EXPECT_EQ(nullptr, celix_properties_get(prop, DZC_SERVICE_TYPE_KEY, nullptr));
        DNSServiceRefDeallocate(dsRef);
        celix_properties_destroy(prop);
    }
}

static void OnUseServiceCallback(void *handle, void *svc) {
    DiscoveryZeroconfAnnouncerTestSuite *t = (DiscoveryZeroconfAnnouncerTestSuite *)handle;
    endpoint_listener_t *epl = (endpoint_listener_t *)svc;
    const char *fwUuid = celix_bundleContext_getProperty(t->ctx.get(), OSGI_FRAMEWORK_FRAMEWORK_UUID, nullptr);
    celix_properties_t *properties = celix_properties_create();
    celix_properties_setLong(properties, RSA_DISCOVERY_ZEROCONF_SERVICE_ANNOUNCED_IF_INDEX, t->ifIndex);
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid);
    celix_properties_set(properties, OSGI_FRAMEWORK_OBJECTCLASS, "dzc_test_service");
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_ID, "60f49d89-d105-430c-b12b-93fbb54b1d19");
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_SERVICE_ID, "100");
    celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED, "true");
    celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, "dzc_test_config_type");
    endpoint_description_t *endpoint{};
    auto status = endpointDescription_create(properties,&endpoint);
    EXPECT_EQ(status, CELIX_SUCCESS);

    epl->endpointAdded(epl->handle, endpoint, nullptr);

    DNSServiceRef dsRef{};
    DNSServiceErrorType dnsErr = DNSServiceBrowse(&dsRef, 0, 0, DZC_SERVICE_PRIMARY_TYPE, "local.", OnServiceBrowseCallback, t);
    EXPECT_EQ(dnsErr, kDNSServiceErr_NoError);
    DNSServiceProcessResult(dsRef);
    DNSServiceRefDeallocate(dsRef);

    if (t->restartMDNSDeamon) {
        KillMDNSDeamon();
        StartMDNSDeamon();
    }

    epl->endpointRemoved(epl->handle, endpoint, nullptr);

    if (t->restartMDNSDeamon) {
        KillMDNSDeamon();
        StartMDNSDeamon();
    }

    endpointDescription_destroy(endpoint);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddAndRemoveEndpoint) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    ifIndex = kDNSServiceInterfaceIndexAny;
    auto found = celix_bundleContext_useService(ctx.get(), OSGI_ENDPOINT_LISTENER_SERVICE, this, OnUseServiceCallback);
    EXPECT_TRUE(found);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddAndRemoveLocalOnlyEndpoint) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    ifIndex = kDNSServiceInterfaceIndexLocalOnly;
    auto found = celix_bundleContext_useService(ctx.get(), OSGI_ENDPOINT_LISTENER_SERVICE, this, OnUseServiceCallback);
    EXPECT_TRUE(found);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

static int GetLoopBackIfIndex(void) {
    int ifIndex = 0;

    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) != -1)
    {
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == NULL)
                continue;

            if ((getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0)) {
                if (strcmp(host, "127.0.0.1") == 0) {
                    ifIndex = (int)if_nametoindex(ifa->ifa_name);
                    break;
                }
            }
        }

        freeifaddrs(ifaddr);
    }

    return ifIndex;
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddAndRemoveLoopBackEndpoint) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    ifIndex = GetLoopBackIfIndex();
    auto found = celix_bundleContext_useService(ctx.get(), OSGI_ENDPOINT_LISTENER_SERVICE, this, OnUseServiceCallback);
    EXPECT_TRUE(found);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, RestartMDNSDeamon) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    ifIndex = kDNSServiceInterfaceIndexAny;
    restartMDNSDeamon = true;
    auto found = celix_bundleContext_useService(ctx.get(), OSGI_ENDPOINT_LISTENER_SERVICE, this, OnUseServiceCallback);
    EXPECT_TRUE(found);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

static void OnUseServiceWithJumboEndpointCallback(void *handle, void *svc) {
    DiscoveryZeroconfAnnouncerTestSuite *t = (DiscoveryZeroconfAnnouncerTestSuite *)handle;
    endpoint_listener_t *epl = (endpoint_listener_t *)svc;
    const char *fwUuid = celix_bundleContext_getProperty(t->ctx.get(), OSGI_FRAMEWORK_FRAMEWORK_UUID, nullptr);
    celix_properties_t *properties = celix_properties_create();
    celix_properties_setLong(properties, RSA_DISCOVERY_ZEROCONF_SERVICE_ANNOUNCED_IF_INDEX, t->ifIndex);
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid);
    celix_properties_set(properties, OSGI_FRAMEWORK_OBJECTCLASS, "dzc_test_service");
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_ID, "60f49d89-d105-430c-b12b-93fbb54b1d19");
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_SERVICE_ID, "100");
    celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED, "true");
    celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, "dzc_test_config_type");
    for (int i = 0; i < 500; ++i) {
        char key[20]{};
        sprintf(key,"custom_key%d", i);
        celix_properties_setLong(properties, key, i);
    }
    endpoint_description_t *endpoint{};
    auto status = endpointDescription_create(properties,&endpoint);
    EXPECT_EQ(status, CELIX_SUCCESS);

    epl->endpointAdded(epl->handle, endpoint, nullptr);

    DNSServiceRef dsRef{};
    DNSServiceErrorType dnsErr = DNSServiceBrowse(&dsRef, 0, 0, DZC_SERVICE_PRIMARY_TYPE, "local.", OnServiceBrowseCallback, t);
    EXPECT_EQ(dnsErr, kDNSServiceErr_NoError);
    DNSServiceProcessResult(dsRef);
    DNSServiceRefDeallocate(dsRef);

    epl->endpointRemoved(epl->handle, endpoint, nullptr);

    endpointDescription_destroy(endpoint);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddJumboEndpoint) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    ifIndex = kDNSServiceInterfaceIndexAny;
    auto found = celix_bundleContext_useService(ctx.get(), OSGI_ENDPOINT_LISTENER_SERVICE, this, OnUseServiceWithJumboEndpointCallback);
    EXPECT_TRUE(found);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddLocalOnlyJumboEndpoint) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    ifIndex = kDNSServiceInterfaceIndexLocalOnly;
    auto found = celix_bundleContext_useService(ctx.get(), OSGI_ENDPOINT_LISTENER_SERVICE, this, OnUseServiceWithJumboEndpointCallback);
    EXPECT_TRUE(found);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

static void OnUseServiceWithInvalidEndpointCallback(void *handle, void *svc) {
    DiscoveryZeroconfAnnouncerTestSuite *t = (DiscoveryZeroconfAnnouncerTestSuite *)handle;
    endpoint_listener_t *epl = (endpoint_listener_t *)svc;
    celix_status_t status;
    status = epl->endpointAdded(epl->handle, nullptr, nullptr);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    status = epl->endpointRemoved(epl->handle, nullptr, nullptr);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    const char *fwUuid = celix_bundleContext_getProperty(t->ctx.get(), OSGI_FRAMEWORK_FRAMEWORK_UUID, nullptr);
    celix_properties_t *properties = celix_properties_create();
    celix_properties_setLong(properties, RSA_DISCOVERY_ZEROCONF_SERVICE_ANNOUNCED_IF_INDEX, DZC_SERVICE_ANNOUNCED_IF_INDEX_DEFAULT);
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid);
    celix_properties_set(properties, OSGI_FRAMEWORK_OBJECTCLASS, "dzc_test_service");
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_ID, "60f49d89-d105-430c-b12b-93fbb54b1d19");
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_SERVICE_ID, "100");
    celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED, "true");
    celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, "dzc_test_config_type");
    celix_properties_set(properties, DZC_SERVICE_TYPE_KEY, "invalid_service_type_because_the_size_large_than_48");
    endpoint_description_t *endpoint{};
    status = endpointDescription_create(properties,&endpoint);
    EXPECT_EQ(status, CELIX_SUCCESS);

    //Invalid service type
    status = epl->endpointAdded(epl->handle, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    //lost service name
    celix_properties_unset(properties, DZC_SERVICE_TYPE_KEY);
    celix_properties_unset(properties, OSGI_FRAMEWORK_OBJECTCLASS);
    status = epl->endpointAdded(epl->handle, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    endpointDescription_destroy(endpoint);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddInvalidEndpoint) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    auto found = celix_bundleContext_useService(ctx.get(), OSGI_ENDPOINT_LISTENER_SERVICE, this, OnUseServiceWithInvalidEndpointCallback);
    EXPECT_TRUE(found);
    discoveryZeroconfAnnouncer_destroy(announcer);
}