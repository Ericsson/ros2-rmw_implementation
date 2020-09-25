// Copyright 2020 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include "osrf_testing_tools_cpp/scope_exit.hpp"

#include "rcutils/allocator.h"
#include "rcutils/strdup.h"

#include "rmw/rmw.h"
#include "rmw/error_handling.h"

#include "test_msgs/srv/basic_types.h"

#include "./config.hpp"
#include "./testing_macros.hpp"

#ifdef RMW_IMPLEMENTATION
# define CLASSNAME_(NAME, SUFFIX) NAME ## __ ## SUFFIX
# define CLASSNAME(NAME, SUFFIX) CLASSNAME_(NAME, SUFFIX)
#else
# define CLASSNAME(NAME, SUFFIX) NAME
#endif

class CLASSNAME (TestService, RMW_IMPLEMENTATION) : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rmw_init_options_t init_options = rmw_get_zero_initialized_init_options();
    rmw_ret_t ret = rmw_init_options_init(&init_options, rcutils_get_default_allocator());
    ASSERT_EQ(RMW_RET_OK, ret) << rcutils_get_error_string().str;
    OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
    {
      rmw_ret_t ret = rmw_init_options_fini(&init_options);
      EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    });
    init_options.enclave = rcutils_strdup("/", rcutils_get_default_allocator());
    ASSERT_STREQ("/", init_options.enclave);
    ret = rmw_init(&init_options, &context);
    ASSERT_EQ(RMW_RET_OK, ret) << rcutils_get_error_string().str;
    constexpr char node_name[] = "my_test_node";
    constexpr char node_namespace[] = "/my_test_ns";
    node = rmw_create_node(&context, node_name, node_namespace);
    ASSERT_NE(nullptr, node) << rcutils_get_error_string().str;
  }

  void TearDown() override
  {
    rmw_ret_t ret = rmw_destroy_node(node);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_shutdown(&context);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_context_fini(&context);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  }

  rmw_context_t context{rmw_get_zero_initialized_context()};
  rmw_node_t * node{nullptr};
};

TEST_F(CLASSNAME(TestService, RMW_IMPLEMENTATION), create_and_destroy) {
  constexpr char service_name[] = "/test";
  const rosidl_service_type_support_t * ts =
    ROSIDL_GET_SRV_TYPE_SUPPORT(test_msgs, srv, BasicTypes);
  rmw_service_t * srv =
    rmw_create_service(node, ts, service_name, &rmw_qos_profile_default);
  ASSERT_NE(nullptr, srv) << rmw_get_error_string().str;
  rmw_ret_t ret = rmw_destroy_service(node, srv);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
}

TEST_F(CLASSNAME(TestService, RMW_IMPLEMENTATION), create_and_destroy_native) {
  constexpr char service_name[] = "/test";
  const rosidl_service_type_support_t * ts =
    ROSIDL_GET_SRV_TYPE_SUPPORT(test_msgs, srv, BasicTypes);
  rmw_qos_profile_t native_qos_profile = rmw_qos_profile_default;
  native_qos_profile.avoid_ros_namespace_conventions = true;
  rmw_service_t * srv =
    rmw_create_service(node, ts, service_name, &native_qos_profile);
  ASSERT_NE(nullptr, srv) << rmw_get_error_string().str;
  rmw_ret_t ret = rmw_destroy_service(node, srv);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
}

TEST_F(CLASSNAME(TestService, RMW_IMPLEMENTATION), create_with_bad_arguments) {
  constexpr char service_name[] = "/test";
  const rosidl_service_type_support_t * ts =
    ROSIDL_GET_SRV_TYPE_SUPPORT(test_msgs, srv, BasicTypes);

  rmw_service_t * srv =
    rmw_create_service(nullptr, ts, service_name, &rmw_qos_profile_default);
  EXPECT_EQ(nullptr, srv);
  rmw_reset_error();

  srv = rmw_create_service(node, nullptr, service_name, &rmw_qos_profile_default);
  EXPECT_EQ(nullptr, srv);
  rmw_reset_error();

  const char * implementation_identifier = node->implementation_identifier;
  node->implementation_identifier = "not-an-rmw-implementation-identifier";
  srv = rmw_create_service(node, ts, service_name, &rmw_qos_profile_default);
  node->implementation_identifier = implementation_identifier;
  EXPECT_EQ(nullptr, srv);
  rmw_reset_error();

  srv = rmw_create_service(node, ts, nullptr, &rmw_qos_profile_default);
  EXPECT_EQ(nullptr, srv);
  rmw_reset_error();

  srv = rmw_create_service(node, ts, "", &rmw_qos_profile_default);
  EXPECT_EQ(nullptr, srv);
  rmw_reset_error();

  constexpr char service_name_with_spaces[] = "/foo bar";
  srv = rmw_create_service(node, ts, service_name_with_spaces, &rmw_qos_profile_default);
  EXPECT_EQ(nullptr, srv);
  rmw_reset_error();

  constexpr char relative_service_name[] = "foo";
  srv = rmw_create_service(node, ts, relative_service_name, &rmw_qos_profile_default);
  EXPECT_EQ(nullptr, srv);
  rmw_reset_error();

  srv = rmw_create_service(node, ts, service_name, nullptr);
  EXPECT_EQ(nullptr, srv);
  rmw_reset_error();

  srv = rmw_create_service(node, ts, service_name, &rmw_qos_profile_unknown);
  EXPECT_EQ(nullptr, srv);
  rmw_reset_error();

  // Creating and destroying a service still succeeds.
  srv = rmw_create_service(node, ts, service_name, &rmw_qos_profile_default);
  ASSERT_NE(nullptr, srv) << rmw_get_error_string().str;
  rmw_ret_t ret = rmw_destroy_service(node, srv);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
}


TEST_F(CLASSNAME(TestService, RMW_IMPLEMENTATION), create_with_internal_errors) {
  constexpr char service_name[] = "/test";
  const rosidl_service_type_support_t * ts =
    ROSIDL_GET_SRV_TYPE_SUPPORT(test_msgs, srv, BasicTypes);
  ScopedFaultySystemMemory sfsm;
  RCUTILS_FAULT_INJECTION_TEST(
  {
    rmw_service_t * srv =
    rmw_create_service(node, ts, service_name, &rmw_qos_profile_default);
    if (srv) {
      int64_t count = rcutils_fault_injection_get_count();
      rcutils_fault_injection_set_count(RCUTILS_FAULT_INJECTION_NEVER_FAIL);
      rmw_ret_t ret = rmw_destroy_service(node, srv);
      EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
      rcutils_fault_injection_set_count(count);
    } else {
      rmw_reset_error();
    }
  });
}

TEST_F(CLASSNAME(TestService, RMW_IMPLEMENTATION), destroy_with_internal_errors) {
  constexpr char service_name[] = "/test";
  const rosidl_service_type_support_t * ts =
    ROSIDL_GET_SRV_TYPE_SUPPORT(test_msgs, srv, BasicTypes);
  ScopedFaultySystemMemory sfsm;
  RCUTILS_FAULT_INJECTION_TEST(
  {
    int64_t count = rcutils_fault_injection_get_count();
    rcutils_fault_injection_set_count(RCUTILS_FAULT_INJECTION_NEVER_FAIL);
    rmw_service_t * srv =
    rmw_create_service(node, ts, service_name, &rmw_qos_profile_default);
    ASSERT_NE(nullptr, srv) << rmw_get_error_string().str;
    rcutils_fault_injection_set_count(count);
    if (RMW_RET_OK != rmw_destroy_service(node, srv)) {
      rmw_reset_error();
    }
  });
}

class CLASSNAME (TestServiceUse, RMW_IMPLEMENTATION)
  : public CLASSNAME(TestService, RMW_IMPLEMENTATION)
{
protected:
  using Base = CLASSNAME(TestService, RMW_IMPLEMENTATION);

  void SetUp() override
  {
    Base::SetUp();
    constexpr char service_name[] = "/test";
    const rosidl_service_type_support_t * ts =
      ROSIDL_GET_SRV_TYPE_SUPPORT(test_msgs, srv, BasicTypes);
    srv = rmw_create_service(node, ts, service_name, &rmw_qos_profile_default);
    ASSERT_NE(nullptr, srv) << rmw_get_error_string().str;
  }

  void TearDown() override
  {
    rmw_ret_t ret = rmw_destroy_service(node, srv);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    Base::TearDown();
  }

  rmw_service_t * srv{nullptr};
};

TEST_F(CLASSNAME(TestServiceUse, RMW_IMPLEMENTATION), destroy_with_null_node) {
  rmw_ret_t ret = rmw_destroy_service(nullptr, srv);
  EXPECT_EQ(RMW_RET_INVALID_ARGUMENT, ret);
  rmw_reset_error();
}

TEST_F(CLASSNAME(TestServiceUse, RMW_IMPLEMENTATION), destroy_null_service) {
  rmw_ret_t ret = rmw_destroy_service(node, nullptr);
  EXPECT_EQ(RMW_RET_INVALID_ARGUMENT, ret);
  rmw_reset_error();
}

TEST_F(CLASSNAME(TestServiceUse, RMW_IMPLEMENTATION), destroy_with_node_of_another_impl) {
  const char * implementation_identifier = node->implementation_identifier;
  node->implementation_identifier = "not-an-rmw-implementation-identifier";
  rmw_ret_t ret = rmw_destroy_service(node, srv);
  node->implementation_identifier = implementation_identifier;
  EXPECT_EQ(RMW_RET_INCORRECT_RMW_IMPLEMENTATION, ret);
  rmw_reset_error();
}

TEST_F(CLASSNAME(TestServiceUse, RMW_IMPLEMENTATION), destroy_service_of_another_impl) {
  const char * implementation_identifier = srv->implementation_identifier;
  srv->implementation_identifier = "not-an-rmw-implementation-identifier";
  rmw_ret_t ret = rmw_destroy_service(node, srv);
  srv->implementation_identifier = implementation_identifier;
  EXPECT_EQ(RMW_RET_INCORRECT_RMW_IMPLEMENTATION, ret);
  rmw_reset_error();
}
