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

#include <float.h>
#include <assert.h>
#include "celix_err.h"

extern "C" {
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <ffi.h>

#include "dyn_common.h"
#include "dyn_type.h"
#include "json_serializer.h"
#include "json_rpc.h"
#include "celix_errno.h"


    void prepareTest(void) {
        dyn_function_type *dynFunc = nullptr;
        int rc = dynFunction_parseWithStr("add(#am=handle;PDD#am=pre;*D)N", nullptr, &dynFunc);
        ASSERT_EQ(0, rc);

        char *result = nullptr;

        void *handle = nullptr;
        double arg1 = 1.0;
        double arg2 = 2.0;

        void *args[4];
        args[0] = &handle;
        args[1] = &arg1;
        args[2] = &arg2;

        rc = jsonRpc_prepareInvokeRequest(dynFunc, "add", args, &result);
        ASSERT_EQ(0, rc);

        //printf("result is %s\n", result);

        ASSERT_TRUE(strstr(result, "\"add\"") != nullptr);
        ASSERT_TRUE(strstr(result, "1.0") != nullptr);
        ASSERT_TRUE(strstr(result, "2.0") != nullptr);

        free(result);
        dynFunction_destroy(dynFunc);
    }

    void prepareCharTest(void) {
        dyn_function_type *dynFunc = nullptr;
        int rc = dynFunction_parseWithStr("setName(#am=handle;Pt)N", nullptr, &dynFunc);
        ASSERT_EQ(0, rc);

        char *result = nullptr;

        void *handle = nullptr;
        char *arg1 = strdup("hello char");
        void *args[2];
        args[0] = &handle;
        args[1] = &arg1;

        rc = jsonRpc_prepareInvokeRequest(dynFunc, "setName", args, &result);
        ASSERT_EQ(0, rc);

        ASSERT_TRUE(strstr(result, "\"setName\"") != nullptr);
        ASSERT_TRUE(strstr(result, "hello char") != nullptr);

        free(result);
        dynFunction_destroy(dynFunc);
    }

    void prepareConstCharTest(void) {
        dyn_function_type *dynFunc = nullptr;
        int rc = dynFunction_parseWithStr("setConstName(#am=handle;P#const=true;t)N", nullptr, &dynFunc);
        ASSERT_EQ(0, rc);

        char *result = nullptr;

        void *handle = nullptr;
        const char *arg1 = "hello const char";
        void *args[2];
        args[0] = &handle;
        args[1] = &arg1;

        rc = jsonRpc_prepareInvokeRequest(dynFunc, "setConstName", args, &result);
        ASSERT_EQ(0, rc);

        ASSERT_TRUE(strstr(result, "\"setConstName\"") != nullptr);
        ASSERT_TRUE(strstr(result, "hello const char") != nullptr);

        free(result);
        dynFunction_destroy(dynFunc);
    }

    void prepareTestFailed(void) {
        dyn_function_type *dynFunc = nullptr;
        int rc = dynFunction_parseWithStr("action(#am=handle;PP)N", nullptr, &dynFunc);
        ASSERT_EQ(0, rc);

        char *result = nullptr;

        void *handle = nullptr;
        void *arg1 = nullptr;
        void *args[2];
        args[0] = &handle;
        args[1] = &arg1;

        rc = jsonRpc_prepareInvokeRequest(dynFunc, "action", args, &result);
        ASSERT_EQ(1, rc);
        celix_err_printErrors(stderr, nullptr, nullptr);

        free(result);
        dynFunction_destroy(dynFunc);
    }

    void handleTestPre(void) {
        dyn_function_type *dynFunc = nullptr;
        int rc = dynFunction_parseWithStr("add(#am=handle;PDD#am=pre;*D)N", nullptr, &dynFunc);
        ASSERT_EQ(0, rc);

        const char *reply = "{\"r\":2.2}";
        double result = -1.0;
        double *out = &result;
        void *args[4];
        args[3] = &out;
        int rsErrno = 0;
        rc = jsonRpc_handleReply(dynFunc, reply, args, &rsErrno);
        ASSERT_EQ(0, rc);
        ASSERT_EQ(0, rsErrno);
        //ASSERT_EQ(2.2, result);

        dynFunction_destroy(dynFunc);
    }


    void handleTestInvalidReply(void) {
        dyn_function_type *dynFunc = nullptr;
        int rc = dynFunction_parseWithStr("add(#am=handle;PDD#am=pre;*D)N", nullptr, &dynFunc);
        ASSERT_EQ(0, rc);

        double result = -1.0;
        double *out = &result;
        void *args[4];
        args[3] = &out;
        int rsErrno = 0;
        rc = jsonRpc_handleReply(dynFunc, "invalid", args, &rsErrno);
        ASSERT_EQ(1, rc);
        celix_err_printErrors(stderr, nullptr, nullptr);

        dynFunction_destroy(dynFunc);
    }



    int add(void*, double a, double b, double *result) {
        *result = a + b;
        return 0;
    }

    int addFailed(void*, double , double , double *) {
        return CELIX_CUSTOMER_ERROR_MAKE(0,1);// return customer error
    }

    int getName_example4(void*, char** result) {
        *result = strdup("allocatedInFunction");
        return 0;
    }

    struct tst_seq {
        uint32_t cap;
        uint32_t len;
        double *buf;
    };


    //StatsResult={DDD[D average min max input}
    struct tst_StatsResult {
        double average;
        double min;
        double max;
        struct tst_seq input;
    };


    int stats(void*, struct tst_seq input, struct tst_StatsResult **out) {
        assert(out != nullptr);
        assert(*out == nullptr);
        double total = 0.0;
        unsigned int count = 0;
        auto max = DBL_MIN;
        auto min = DBL_MAX;

        unsigned int i;
        for (i = 0; i<input.len; i += 1) {
            total += input.buf[i];
            count += 1;
            if (input.buf[i] > max) {
                max = input.buf[i];
            }
            if (input.buf[i] < min) {
                min = input.buf[i];
            }
        }

        auto result = static_cast<tst_StatsResult *>(calloc(1, sizeof(tst_StatsResult)));
        if(count>0) {
		    result->average = total / count;
        }
        result->min = min;
        result->max = max;
        auto buf = static_cast<double *>(calloc(input.len, sizeof(double)));
        memcpy(buf, input.buf, input.len * sizeof(double));
        result->input.len = input.len;
        result->input.cap = input.len;
        result->input.buf = buf;

        *out = result;
        return 0;
    }

    struct item {
        double a;
        double b;
    };

    struct item_seq {
        uint32_t  cap;
        uint32_t  len;
        struct item **buf;
    };

    struct tst_serv {
        void *handle;
        int (*add)(void *, double, double, double *);
        int (*sub)(void *, double, double, double *);
        int (*sqrt)(void *, double, double *);
        int (*stats)(void *, struct tst_seq, struct tst_StatsResult **);
    };

    struct tst_serv_example4 {
        void *handle;
        int (*getName_example4)(void *, char** name);
        int (*setName_example4)(void *, char* name);
        int (*setConstName_example4)(void *, const char* name);
    };

    void callTestPreAllocated(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        char *result = nullptr;
        tst_serv serv {nullptr, add, nullptr, nullptr, nullptr};

        rc = jsonRpc_call(intf, &serv, R"({"m":"add(DD)D", "a": [1.0,2.0]})", &result);
        ASSERT_EQ(0, rc);
        ASSERT_TRUE(strstr(result, "3.0") != nullptr);

        free(result);
        dynInterface_destroy(intf);
    }

    void callFailedTestPreAllocated(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        char *result = nullptr;
        tst_serv serv {nullptr, addFailed, nullptr, nullptr, nullptr};

        rc = jsonRpc_call(intf, &serv, R"({"m":"add(DD)D", "a": [1.0,2.0]})", &result);
        ASSERT_EQ(0, rc);
        ASSERT_TRUE(strstr(result, "e") != nullptr);

        free(result);
        dynInterface_destroy(intf);
    }

    void callTestOutput(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        char *result = nullptr;
        tst_serv serv {nullptr, nullptr, nullptr, nullptr, stats};

        rc = jsonRpc_call(intf, &serv, R"({"m":"stats([D)LStatsResult;", "a": [[1.0,2.0]]})", &result);
        ASSERT_EQ(0, rc);
        ASSERT_TRUE(strstr(result, "1.5") != nullptr);

        free(result);
        dynInterface_destroy(intf);
    }

    void callTestInvalidRequest(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        char *result = nullptr;
        tst_serv serv {nullptr, addFailed, nullptr, nullptr, nullptr};

        rc = jsonRpc_call(intf, &serv, R"({)", &result);
        ASSERT_EQ(1, rc);

        rc = jsonRpc_call(intf, &serv, R"({"a": [1.0,2.0]})", &result);
        ASSERT_EQ(1, rc);

        //request argument type mismatch
        rc = jsonRpc_call(intf, &serv, R"({"m":"stats([D)LStatsResult;", "a": [1.0]})", &result);
        ASSERT_EQ(1, rc);

        dynInterface_destroy(intf);
    }

    void handleTestOut(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        struct methods_head *head;
        dynInterface_methods(intf, &head);
        dyn_function_type *func = nullptr;
        struct method_entry *entry = nullptr;
        TAILQ_FOREACH(entry, head, entries) {
            if (strcmp(entry->name, "stats") == 0) {
                func = entry->dynFunc;
                break;
            }
        }
        ASSERT_TRUE(func != nullptr);

        const char *reply = R"({"r":{"input":[1.0,2.0],"max":2.0,"average":1.5,"min":1.0}})";

        void *args[3];
        args[0] = nullptr;
        args[1] = nullptr;
        args[2] = nullptr;

        struct tst_StatsResult *result = nullptr;
        void *out = &result;
        args[2] = &out;

        int rsErrno = 0;
        rc = jsonRpc_handleReply(func, reply, args, &rsErrno);
        ASSERT_EQ(0, rc);
        ASSERT_EQ(0, rsErrno);
        ASSERT_EQ(1.5, result->average);

        free(result->input.buf);
        free(result);
        dynInterface_destroy(intf);
    }

    void handleTestOutWithEmptyReply(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        struct methods_head *head;
        dynInterface_methods(intf, &head);
        dyn_function_type *func = nullptr;
        struct method_entry *entry = nullptr;
        TAILQ_FOREACH(entry, head, entries) {
            if (strcmp(entry->name, "stats") == 0) {
                func = entry->dynFunc;
                break;
            }
        }
        ASSERT_TRUE(func != nullptr);

        const char *reply = R"({})";

        void *args[3];
        args[0] = nullptr;
        args[1] = nullptr;
        args[2] = nullptr;

        struct tst_StatsResult *result = nullptr;
        void *out = &result;
        args[2] = &out;

        int rsErrno = 0;
        rc = jsonRpc_handleReply(func, reply, args, &rsErrno);
        ASSERT_NE(0, rc);
        celix_err_printErrors(stderr, nullptr, nullptr);

        dynInterface_destroy(intf);
    }

    void handleTestReplyError(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        struct methods_head *head;
        dynInterface_methods(intf, &head);
        dyn_function_type *func = nullptr;
        struct method_entry *entry = nullptr;
        TAILQ_FOREACH(entry, head, entries) {
            if (strcmp(entry->name, "add") == 0) {
                func = entry->dynFunc;
                break;
            }
        }
        ASSERT_TRUE(func != nullptr);

        const char *reply = R"({"e":33554433})";

        void *args[4];
        args[0] = nullptr;
        args[1] = nullptr;
        args[2] = nullptr;
        args[3] = nullptr;

        double result = 0;
        void *out = &result;
        args[3] = &out;

        int rsErrno = 0;
        rc = jsonRpc_handleReply(func, reply, args, &rsErrno);
        ASSERT_EQ(0, rc);
        ASSERT_EQ(33554433, rsErrno);
        ASSERT_EQ(0, result);

        dynInterface_destroy(intf);
    }

    void callTestUnknownRequest(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        char *result = nullptr;
        tst_serv serv {nullptr, addFailed, nullptr, nullptr, nullptr};

        rc = jsonRpc_call(intf, &serv, R"({"m":"unknown", "a": [1.0,2.0]})", &result);
        ASSERT_EQ(1, rc);

        dynInterface_destroy(intf);
    }

    static void handleTestOutputSequence() {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example2.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        struct methods_head *head;
        dynInterface_methods(intf, &head);
        dyn_function_type *func = nullptr;
        struct method_entry *entry = nullptr;
        TAILQ_FOREACH(entry, head, entries) {
            if (strcmp(entry->name, "example1") == 0) {
                func = entry->dynFunc;
                break;
            }
        }
        ASSERT_TRUE(func != nullptr);

        //dyn_type *arg = dynFunction_argumentTypeForIndex(func, 1);
        //dynType_print(arg, stdout);

        const char *reply = R"({"r":[{"a":1.0,"b":1.5},{"a":2.0,"b":2.5}]})";

        void *args[2];
        args[0] = nullptr;
        args[1] = nullptr;

        struct item_seq *result = nullptr;
        void *out = &result;
        args[1] = &out;

        int rsErrno = 0;
        rc = jsonRpc_handleReply(func, reply, args, &rsErrno);
        ASSERT_EQ(0, rc);
        ASSERT_EQ(0, rsErrno);
        ASSERT_EQ(2, result->len);
        ASSERT_EQ(1.0, result->buf[0]->a);
        ASSERT_EQ(1.5, result->buf[0]->b);
        ASSERT_EQ(2.0, result->buf[1]->a);
        ASSERT_EQ(2.5, result->buf[1]->b);


        unsigned int i;
        for (i = 0; i < result->len; i +=1 ) {
            free(result->buf[i]);
        }
        free(result->buf);
        free(result);
        dynInterface_destroy(intf);
    }

    void handleTestAction(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example5.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        struct methods_head *head;
        dynInterface_methods(intf, &head);
        dyn_function_type *func = nullptr;
        struct method_entry *entry = nullptr;
        TAILQ_FOREACH(entry, head, entries) {
            if (strcmp(entry->name, "action") == 0) {
                func = entry->dynFunc;
                break;
            }
        }
        ASSERT_TRUE(func != nullptr);

        const char *reply = R"({})";

        void *args[1];
        args[0] = nullptr;

        int rsErrno = 0;
        rc = jsonRpc_handleReply(func, reply, args, &rsErrno);
        ASSERT_EQ(0, rc);

        dynInterface_destroy(intf);
    }

    //Current only support one out argument
    void handleTestMultiPreOut(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/invalids/multiOutArgs.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        struct methods_head *head;
        dynInterface_methods(intf, &head);
        dyn_function_type *func = nullptr;
        struct method_entry *entry = nullptr;
        TAILQ_FOREACH(entry, head, entries) {
            if (strcmp(entry->name, "multiPreOut") == 0) {
                func = entry->dynFunc;
                break;
            }
        }
        ASSERT_TRUE(func != nullptr);

        const char *reply = R"({"r":2.0})";

        void *args[3];
        args[0] = nullptr;
        args[1] = nullptr;
        args[2] = nullptr;

        double result1 = 0;
        void *out = &result1;
        args[1] = &out;
        int result2 = 0;
        void *out2 = &result2;
        args[2] = &out2;

        int rsErrno = 0;
        rc = jsonRpc_handleReply(func, reply, args, &rsErrno);
        ASSERT_NE(0, rc);
        celix_err_printErrors(stderr, nullptr, nullptr);

        dynInterface_destroy(intf);
    }

    //Current only support one out argument
    void handleTestMultiOut(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/invalids/multiOutArgs.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        struct methods_head *head;
        dynInterface_methods(intf, &head);
        dyn_function_type *func = nullptr;
        struct method_entry *entry = nullptr;
        TAILQ_FOREACH(entry, head, entries) {
            if (strcmp(entry->name, "multiOut") == 0) {
                func = entry->dynFunc;
                break;
            }
        }
        ASSERT_TRUE(func != nullptr);

        const char *reply = R"({"r":2.0})";

        void *args[3];
        args[0] = nullptr;
        args[1] = nullptr;
        args[2] = nullptr;

        double *result1 = nullptr;
        void *out = &result1;
        args[1] = &out;
        int *result2 = nullptr;
        void *out2 = &result2;
        args[2] = &out2;

        int rsErrno = 0;
        rc = jsonRpc_handleReply(func, reply, args, &rsErrno);
        ASSERT_NE(0, rc);
        celix_err_printErrors(stderr, nullptr, nullptr);

        dynInterface_destroy(intf);
    }




    void callTestOutChar(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example4.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        char *result = nullptr;
        tst_serv_example4 serv {nullptr, getName_example4, nullptr, nullptr};

        rc = jsonRpc_call(intf, &serv, R"({"m": "getName(V)t", "a": []})", &result);
        ASSERT_EQ(0, rc);

        ASSERT_TRUE(strstr(result, "allocatedInFunction") != nullptr);

        free(result);
        dynInterface_destroy(intf);
    }


    void handleTestOutChar(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example4.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        struct methods_head *head;
        dynInterface_methods(intf, &head);
        dyn_function_type *func = nullptr;
        struct method_entry *entry = nullptr;
        TAILQ_FOREACH(entry, head, entries) {
            if (strcmp(entry->name, "getName") == 0) {
                func = entry->dynFunc;
                break;
            }
        }

        ASSERT_TRUE(func != nullptr);

        const char *reply = R"({"r": "this is a test string"})";
        char *result = nullptr;
        void *out = &result;

        void *args[2];
        args[0] = nullptr;
        args[1] = &out;

        if (func != nullptr) { // Check needed just to satisfy Coverity
            int rsErrno = 0;
		     jsonRpc_handleReply(func, reply, args, &rsErrno);
		     ASSERT_EQ(0, rsErrno);
        }

        ASSERT_STREQ("this is a test string", result);

        free(result);
        dynInterface_destroy(intf);
    }

    void handleTestPreChar(void) {
        dyn_function_type *dynFunc = nullptr;
        int rc = dynFunction_parseWithStr("getName(#am=handle;P#am=pre;t)N", nullptr, &dynFunc);
        ASSERT_EQ(0, rc);

        const char *reply = "{\"r\":\"this is a test pre string\"}";
        char result[32] = {0};
        void *out = result;
        void *args[2];
        args[0] = nullptr;
        args[1] = &out;
        int rsErrno = 0;
        rc = jsonRpc_handleReply(dynFunc, reply, args, &rsErrno);
        ASSERT_EQ(0, rc);
        ASSERT_EQ(0, rsErrno);
        ASSERT_STREQ("this is a test pre string", result);

        dynFunction_destroy(dynFunc);
    }

    void callTestChar(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example4.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        char *result = nullptr;
        tst_serv_example4 serv {nullptr, nullptr,[](void*, char *name)->int {
            EXPECT_STREQ(name, "hello");
            free(name);
            return 0;
        }, nullptr};

        rc = jsonRpc_call(intf, &serv, R"({"m": "setName", "a":["hello"]})", &result);
        ASSERT_EQ(0, rc);
        free(result);

        dynInterface_destroy(intf);
    }

    void callTestConstChar(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example4.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        char *result = nullptr;
        tst_serv_example4 serv {nullptr, nullptr, nullptr, [](void*, const char *name)->int {
            EXPECT_STREQ(name, "hello const char");
            return 0;
        }};

        rc = jsonRpc_call(intf, &serv, R"({"m": "setConstName", "a":["hello const char"]})", &result);
        ASSERT_EQ(0, rc);
        free(result);

        dynInterface_destroy(intf);
    }

    enum example6_enum{
            v1 = 1,
            v2 = 2,
    };

    struct tst_CptData {
        double d;
        char *t;
        struct tst_seq s;
        enum example6_enum e;
    };

    struct tst_serv_example6 {
        void *handle;
        int (*cpt)(void *, struct tst_CptData *input, struct tst_CptData *output);
    };

    void testRequestBackwardCompatibility(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example6.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);


        char *result = nullptr;

        tst_serv_example6 serv {nullptr, nullptr};

        serv.cpt = [](void *, struct tst_CptData *input, struct tst_CptData *)->int {
            EXPECT_EQ(input->d , 0.0);
            EXPECT_EQ(input->t , nullptr);
            EXPECT_EQ(input->s.len , 0);
            EXPECT_EQ(input->s.cap , 0);
            EXPECT_EQ(input->s.buf , nullptr);
            EXPECT_EQ(input->e , 0);
            return 0;
        };
        rc = jsonRpc_call(intf, &serv, R"({"m": "compatibility", "a": [{}]})", &result);
        ASSERT_EQ(0, rc);
        free(result);

        serv.cpt = [](void *, struct tst_CptData *input, struct tst_CptData *)->int {
            EXPECT_EQ(input->d , 1.0);
            EXPECT_EQ(input->t , nullptr);
            EXPECT_EQ(input->s.len , 0);
            EXPECT_EQ(input->s.cap , 0);
            EXPECT_EQ(input->s.buf , nullptr);
            EXPECT_EQ(input->e , 0);
            return 0;
        };
        rc = jsonRpc_call(intf, &serv, R"({"m": "compatibility", "a": [{"d":1.0}]})", &result);
        ASSERT_EQ(0, rc);
        free(result);

        serv.cpt = [](void *, struct tst_CptData *input, struct tst_CptData *)->int {
            EXPECT_EQ(input->d , 1.0);
            EXPECT_STREQ(input->t , "hello compatibility");
            EXPECT_EQ(input->s.len , 0);
            EXPECT_EQ(input->s.cap , 0);
            EXPECT_EQ(input->s.buf , nullptr);
            EXPECT_EQ(input->e , 0);
            return 0;
        };
        rc = jsonRpc_call(intf, &serv, R"({"m": "compatibility", "a": [{"d":1.0, "t":"hello compatibility"}]})", &result);
        ASSERT_EQ(0, rc);
        free(result);

        serv.cpt = [](void *, struct tst_CptData *input, struct tst_CptData *)->int {
            EXPECT_EQ(input->d , 1.0);
            EXPECT_STREQ(input->t , "hello compatibility");
            EXPECT_EQ(input->s.len , 3);
            EXPECT_EQ(input->s.cap , 3);
            EXPECT_EQ(input->s.buf[0] , 1.0);
            EXPECT_EQ(input->s.buf[1] , 2.0);
            EXPECT_EQ(input->s.buf[2] , 3.0);
            EXPECT_EQ(input->e , 0);
            return 0;
        };
        rc = jsonRpc_call(intf, &serv, R"({"m": "compatibility", "a": [{"d":1.0, "t":"hello compatibility", "s":[1.0,2.0,3.0]}]})", &result);
        ASSERT_EQ(0, rc);
        free(result);


        serv.cpt = [](void *, struct tst_CptData *input, struct tst_CptData *)->int {
            EXPECT_EQ(input->d , 1.0);
            EXPECT_STREQ(input->t , "hello compatibility");
            EXPECT_EQ(input->s.len , 3);
            EXPECT_EQ(input->s.cap , 3);
            EXPECT_EQ(input->s.buf[0] , 1.0);
            EXPECT_EQ(input->s.buf[1] , 2.0);
            EXPECT_EQ(input->s.buf[2] , 3.0);
            EXPECT_EQ(input->e , v2);
            return 0;
        };
        rc = jsonRpc_call(intf, &serv, R"({"m": "compatibility", "a": [{"d":1.0, "t":"hello compatibility", "s":[1.0,2.0,3.0], "e":"v2"}]})", &result);
        ASSERT_EQ(0, rc);
        free(result);


        dynInterface_destroy(intf);
    }

    void testResponseForwardCompatibility(void) {
        dyn_interface_type *intf = nullptr;
        FILE *desc = fopen("descriptors/example6.descriptor", "r");
        ASSERT_TRUE(desc != nullptr);
        int rc = dynInterface_parse(desc, &intf);
        ASSERT_EQ(0, rc);
        fclose(desc);

        struct methods_head *head;
        dynInterface_methods(intf, &head);
        dyn_function_type *func = nullptr;
        struct method_entry *entry = nullptr;
        TAILQ_FOREACH(entry, head, entries) {
            if (strcmp(entry->name, "compatibility") == 0) {
                func = entry->dynFunc;
                break;
            }
        }
        ASSERT_TRUE(func != nullptr);

        struct tst_CptData *cptData{nullptr};
        void *out = &cptData;

        void *args[3];
        args[0] = nullptr;
        args[1] = nullptr;
        args[2] = &out;
        int rsErrno = 0;

        //provider has more reply
        rc = jsonRpc_handleReply(func, R"({"r":{"d":1.0, "t":"hello compatibility", "s":[1.0,2.0,3.0], "e":"v1", "e2":"v2"}})", args, &rsErrno);
        EXPECT_EQ(0, rc);
        EXPECT_EQ(0, rsErrno);
        EXPECT_NE(cptData , nullptr);
        EXPECT_EQ(cptData->d , 1.0);
        EXPECT_STREQ(cptData->t , "hello compatibility");
        EXPECT_EQ(cptData->s.len , 3);
        EXPECT_EQ(cptData->s.cap , 3);
        EXPECT_EQ(cptData->s.buf[0] , 1.0);
        EXPECT_EQ(cptData->s.buf[1] , 2.0);
        EXPECT_EQ(cptData->s.buf[2] , 3.0);
        EXPECT_EQ(cptData->e , v1);
        free(cptData->t);
        free(cptData->s.buf);
        free(cptData);

        dynInterface_destroy(intf);
    }
}

class JsonRpcTests : public ::testing::Test {
public:
    JsonRpcTests() {

    }
    ~JsonRpcTests() override {
    }

};

TEST_F(JsonRpcTests, prepareTest) {
    prepareTest();
}

TEST_F(JsonRpcTests, prepareCharTest) {
    prepareCharTest();
}

TEST_F(JsonRpcTests, prepareConstCharTest) {
    prepareConstCharTest();
}

TEST_F(JsonRpcTests, prepareTestFailed) {
    prepareTestFailed();
}

TEST_F(JsonRpcTests, handleTestPre) {
    handleTestPre();
}

TEST_F(JsonRpcTests, handleTestInvalidReply) {
    handleTestInvalidReply();
}

TEST_F(JsonRpcTests, handleTestOut) {
    handleTestOut();
}

TEST_F(JsonRpcTests, callPre) {
    callTestPreAllocated();
}

TEST_F(JsonRpcTests, callFailedPre) {
    callFailedTestPreAllocated();
}

TEST_F(JsonRpcTests, callOut) {
    callTestOutput();
}

TEST_F(JsonRpcTests, callTestInvalidRequest) {
    callTestInvalidRequest();
}

TEST_F(JsonRpcTests, callTestUnknownRequest) {
    callTestUnknownRequest();
}

TEST_F(JsonRpcTests, handleOutSeq) {
    handleTestOutputSequence();
}

TEST_F(JsonRpcTests, callTestOutChar) {
    callTestOutChar();
}

TEST_F(JsonRpcTests, handleOutChar) {
    handleTestOutChar();
}

TEST_F(JsonRpcTests, handlePreChar) {
    handleTestPreChar();
}

TEST_F(JsonRpcTests, handleReplyError) {
    handleTestReplyError();
}

TEST_F(JsonRpcTests, handleTestOutWithEmptyReply) {
    handleTestOutWithEmptyReply();
}

TEST_F(JsonRpcTests, handleTestAction) {
    handleTestAction();
}

TEST_F(JsonRpcTests, handleTestMultiPreOut) {
    handleTestMultiPreOut();
}

TEST_F(JsonRpcTests, handleTestMultiOut) {
    handleTestMultiOut();
}

TEST_F(JsonRpcTests, callTestChar) {
    callTestChar();
}

TEST_F(JsonRpcTests, callTestConstChar) {
    callTestConstChar();
}

TEST_F(JsonRpcTests, testRequestBackwardCompatibility) {
    testRequestBackwardCompatibility();
}

TEST_F(JsonRpcTests, testResponseForwardCompatibility) {
    testResponseForwardCompatibility();
}
