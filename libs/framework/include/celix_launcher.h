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


#ifndef CELIX_LAUNCHER_H
#define CELIX_LAUNCHER_H

#include "celix_framework.h"
#include "celix_framework_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Celix launcher environment property to specify interval of the periodic shutdown check performed by launcher.
 *
 * The launcher will perform a periodic check to see whether to perform a shutdown, and if so, the launcher will
 * stop and destroy the framework. The interval of this check can be specified in seconds using this property.
 */
#define CELIX_LAUNCHER_SHUTDOWN_PERIOD_IN_SECONDS           "CELIX_LAUNCHER_SHUTDOWN_PERIOD_IN_SECONDS"
#define CELIX_LAUNCHER_SHUTDOWN_PERIOD_IN_SECONDS_DEFAULT   1.0

/**
 * @brief Launch a celix framework, wait (block) until the framework is stopped and destroy the framework when stopped.
 *
 * The launcher will also setup signal handlers for SIGINT, SIGTERM, SIGUSR1 and SIGUSR2 and initialize libcurl.
 * For SIGINT and SIGTERM the framework will be stopped.
 * SIGUSR1 and SIGUSR2 will be ignored.
 *
 * The Celix launcher can only controls a single framework instance. If multiple frameworks are needed,
 * `celix_frameworkFactory_createFramework` or `celix::createFramework` should be used. If the celix launcher is called
 * while a framework is already running or being launched, the launcher will print an error message to stderr and
 * return 1.
 *
 * @param argc argc as provided in a main function.
 * @param argv argv as provided in a main function.
 * @param embeddedConfig The optional embedded config text.
 * @return 0 if successful and return 1 if the framework could not be launched. Reason for not launching the framework
 * will be logged.
 */
CELIX_FRAMEWORK_EXPORT int celix_launcher_launchAndWait(int argc, char* argv[], const char* embeddedConfig);

/**
 * @brief Trigger the stop of the Celix framework.
 *
 * Will trigger the global framework instance to stop. This will result in the framework to stop and the
 * celix_launcher_launch function to return.
 *
 * If no global framework instance is available, this function will print an error message to stderr.
 */
CELIX_FRAMEWORK_EXPORT void celix_launcher_triggerStop();

#ifdef __cplusplus
}
#endif

#endif //CELIX_LAUNCHER_H
