/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#ifndef CELIX_PUBSUB_ZMQ_ADMIN_H
#define CELIX_PUBSUB_ZMQ_ADMIN_H

#include <mutex>
#include <map>
#include <pubsub_admin.h>
#include "celix_api.h"
#include "log_helper.h"
#include "pubsub_nanomsg_topic_receiver.h"
#include <pubsub_serializer.h>

#include "../../../shell/shell/include/command.h"
#include "pubsub_nanomsg_topic_sender.h"
#include "pubsub_nanomsg_topic_receiver.h"

#define PUBSUB_NANOMSG_ADMIN_TYPE       "zmq"
#define PUBSUB_NANOMSG_URL_KEY          "zmq.url"

#define PUBSUB_NANOMSG_VERBOSE_KEY      "PSA_ZMQ_VERBOSE"
#define PUBSUB_NANOMSG_VERBOSE_DEFAULT  true

#define PUBSUB_NANOMSG_PSA_IP_KEY       "PSA_IP"
#define PUBSUB_NANOMSG_PSA_ITF_KEY		"PSA_INTERFACE"

#define PUBSUB_NANOMSG_DEFAULT_IP       "127.0.0.1"

//typedef struct pubsub_nanomsg_admin pubsub_nanomsg_admin_t;

template <typename key, typename value>
struct ProtectedMap {
    std::mutex mutex{};
    std::map<key, value> map{};
};

class pubsub_nanomsg_admin {
public:
    pubsub_nanomsg_admin(celix_bundle_context_t *ctx, log_helper_t *logHelper);
    pubsub_nanomsg_admin(const pubsub_nanomsg_admin&) = delete;
    pubsub_nanomsg_admin& operator=(const pubsub_nanomsg_admin&) = delete;
    ~pubsub_nanomsg_admin();
    void start();
    void stop();

private:
    void addSerializerSvc(void *svc, const celix_properties_t *props);
    void removeSerializerSvc(void */*svc*/, const celix_properties_t *props);
    celix_status_t matchPublisher(long svcRequesterBndId, const celix_filter_t *svcFilter,
                                                      double *score, long *serializerSvcId);
    celix_status_t matchSubscriber(long svcProviderBndId,
                                                       const celix_properties_t *svcProperties, double *score,
                                                       long *serializerSvcId);
    celix_status_t matchEndpoint(const celix_properties_t *endpoint, bool *match);

    celix_status_t setupTopicSender(const char *scope, const char *topic,
                                                        long serializerSvcId, celix_properties_t **publisherEndpoint);
    celix_status_t teardownTopicSender(const char *scope, const char *topic);

    celix_status_t setupTopicReceiver(const char *scope, const char *topic,
                                                          long serializerSvcId, celix_properties_t **subscriberEndpoint);
    celix_status_t teardownTopicReceiver(const char *scope, const char *topic);

    celix_status_t addEndpoint(const celix_properties_t *endpoint);
    celix_status_t removeEndpoint(const celix_properties_t *endpoint);

    celix_status_t executeCommand(char *commandLine __attribute__((unused)), FILE *out,
                                                        FILE *errStream __attribute__((unused)));

    celix_status_t connectEndpointToReceiver(pubsub::nanomsg::topic_receiver *receiver,
                                                                   const celix_properties_t *endpoint);

    celix_status_t disconnectEndpointFromReceiver(pubsub::nanomsg::topic_receiver *receiver,
                                                                        const celix_properties_t *endpoint);
    celix_bundle_context_t *ctx;
    log_helper_t *log;
    pubsub_admin_service_t adminService{};
    long adminSvcId = -1L;
    long cmdSvcId = -1L;
    command_service_t cmdSvc{};
    long serializersTrackerId = -1L;

    const char *fwUUID{};

    char* ipAddress{};

    unsigned int basePort{};
    unsigned int maxPort{};

    double qosSampleScore{};
    double qosControlScore{};
    double defaultScore{};

    bool verbose{};

    typedef struct psa_nanomsg_serializer_entry {
        const char *serType;
        long svcId;
        pubsub_serializer_service_t *svc;
    } psa_nanomsg_serializer_entry_t;
    ProtectedMap<long, psa_nanomsg_serializer_entry_t*> serializers{};
    ProtectedMap<char*, pubsub_nanomsg_topic_sender_t*> topicSenders{};
    ProtectedMap<char*, pubsub::nanomsg::topic_receiver*> topicReceivers{};
    ProtectedMap<const char*, celix_properties_t *> discoveredEndpoints{};
};

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif


#endif //CELIX_PUBSUB_ZMQ_ADMIN_H
