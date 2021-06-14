/*
 * Copyright (C) 2020 Open Source Robotics Foundation
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
 *
 */

#include <memory>
#include <iostream>

#include <rmf_utils/catch.hpp>

#include <free_fleet/manager/Manager.hpp>
#include <free_fleet/messages/RobotMode.hpp>
#include <free_fleet/messages/Waypoint.hpp>
#include <free_fleet/messages/Location.hpp>
#include <free_fleet/manager/SimpleCoordinateTransformer.hpp>

#include "src/manager/internal_Manager.hpp"
#include "src/manager/internal_RobotInfo.hpp"

#include "mock_ServerMiddleware.hpp"

SCENARIO("Test Manager API")
{
  const std::string fleet_name = "test_fleet";

  const std::string test_map_name = "test_level";
  std::shared_ptr<rmf_traffic::agv::Graph> graph(new rmf_traffic::agv::Graph);
  graph->add_waypoint(test_map_name, {0, 0});
  graph->add_waypoint(test_map_name, {10, 0});
  graph->add_waypoint(test_map_name, {-10, 0});
  graph->add_waypoint(test_map_name, {0, 10});
  graph->add_waypoint(test_map_name, {0, -10});
  graph->add_lane(0, 1);
  graph->add_lane(1, 0);
  graph->add_lane(0, 2);
  graph->add_lane(2, 0);
  graph->add_lane(0, 3);
  graph->add_lane(3, 0);
  graph->add_lane(0, 4);
  graph->add_lane(4, 0);

  std::unique_ptr<free_fleet::transport::ServerMiddleware> m(
    new free_fleet::MockServerMiddleware());
  auto ct = free_fleet::manager::SimpleCoordinateTransformer::make(
    1.0,
    0.0,
    0.0,
    0.0);
  free_fleet::Manager::TimeNow time_now_fn =
    [](){ return std::chrono::steady_clock::now(); };
  free_fleet::Manager::RobotUpdatedCallback cb =
    [](const free_fleet::manager::RobotInfo&){};

  auto manager = free_fleet::Manager::make(
    fleet_name,
    graph,
    std::move(m),
    ct,
    time_now_fn,
    cb);
  REQUIRE(manager);

  GIVEN("Starting with initial conditions, running 5 times")
  {
    for (int i = 0; i < 5; ++i)
      CHECK_NOTHROW(
        free_fleet::Manager::Implementation::get(*manager).run_once());
  }

  GIVEN("Started with no robots")
  {
    for (int i = 0; i < 5; ++i)
      CHECK_NOTHROW(
        free_fleet::Manager::Implementation::get(*manager).run_once());

    auto robot_names = manager->robot_names();
    CHECK(robot_names.empty());

    auto info = manager->robot("random");
    CHECK(!info);

    auto all_info = manager->all_robots();
    CHECK(all_info.empty());
  }

  GIVEN("Sending requests with no robots")
  {
    auto robot_names = manager->robot_names();
    CHECK(robot_names.empty());

    std::string rn = "test_robot";

    auto id = manager->request_pause(rn);
    CHECK(!id.has_value());

    id = manager->request_resume(rn);
    CHECK(!id.has_value());

    id = manager->request_dock(rn, "dock_name");
    CHECK(!id.has_value());

    id = manager->request_relocalization(
      rn,
      free_fleet::messages::Location(),
      0);
    CHECK(!id.has_value());

    id = manager->request_navigation(
      rn,
      {free_fleet::messages::Waypoint(), free_fleet::messages::Waypoint()});
    CHECK(!id.has_value());
  }
}

SCENARIO("Testing manager API with dummy robots")
{
  const std::string fleet_name = "test_fleet";

  const std::string test_map_name = "test_level";
  std::shared_ptr<rmf_traffic::agv::Graph> graph(new rmf_traffic::agv::Graph);
  graph->add_waypoint(test_map_name, {0, 0});
  graph->add_waypoint(test_map_name, {10, 0});
  graph->add_waypoint(test_map_name, {-10, 0});
  graph->add_waypoint(test_map_name, {0, 10});
  graph->add_waypoint(test_map_name, {0, -10});
  graph->add_lane(0, 1);
  graph->add_lane(1, 0);
  graph->add_lane(0, 2);
  graph->add_lane(2, 0);
  graph->add_lane(0, 3);
  graph->add_lane(3, 0);
  graph->add_lane(0, 4);
  graph->add_lane(4, 0);
  graph->add_waypoint(test_map_name, {100, 100});

  std::unique_ptr<free_fleet::transport::ServerMiddleware> m(
    new free_fleet::MockServerMiddleware());
  auto ct = free_fleet::manager::SimpleCoordinateTransformer::make(
    1.0,
    0.0,
    0.0,
    0.0);
  free_fleet::Manager::TimeNow time_now_fn =
    [](){ return std::chrono::steady_clock::now(); };
  free_fleet::Manager::RobotUpdatedCallback cb =
    [](const free_fleet::manager::RobotInfo&){};

  auto manager = free_fleet::Manager::make(
    fleet_name,
    graph,
    std::move(m),
    ct,
    time_now_fn,
    cb);
  REQUIRE(manager);

  free_fleet::messages::RobotState initial_state {
    "test_robot",
    "test_model",
    0,
    free_fleet::messages::RobotMode(),
    1.0,
    free_fleet::messages::Location(),
    0
  };
  rmf_traffic::Time initial_time = std::chrono::steady_clock::now();

  auto& impl = free_fleet::Manager::Implementation::get(*manager);

  initial_state.name = "test_robot_1";
  auto robot_info_1 =
    free_fleet::manager::RobotInfo::Implementation::make(
      initial_state,
      graph,
      initial_time);
  REQUIRE(robot_info_1);

  initial_state.name = "test_robot_2";
  auto robot_info_2 =
    free_fleet::manager::RobotInfo::Implementation::make(
      initial_state,
      graph,
      initial_time);
  REQUIRE(robot_info_2);

  initial_state.name = "test_robot_3";
  auto robot_info_3 =
    free_fleet::manager::RobotInfo::Implementation::make(
      initial_state,
      graph,
      initial_time);
  REQUIRE(robot_info_3);

  impl.robots[robot_info_1->name()] = robot_info_1;
  impl.robots[robot_info_2->name()] = robot_info_2;
  impl.robots[robot_info_3->name()] = robot_info_3;

  GIVEN("Basic API with dummy robots")
  {
    auto names = manager->robot_names();
    CHECK(names.size() == 3);
    
    auto all_info = manager->all_robots();
    CHECK(all_info.size() == 3);

    auto info = manager->robot("test_robot_1");
    REQUIRE(info);
    CHECK(info->name() == "test_robot_1");

    info = manager->robot("test_robot_2");
    REQUIRE(info);
    CHECK(info->name() == "test_robot_2");
  }

  GIVEN("Sending pause request to dummy robots")
  {
    // Valid mode
    auto id_1 = manager->request_pause("test_robot_1");
    REQUIRE(id_1.has_value());
    CHECK(id_1.value() == 1);

    // Invalid robot
    auto id_2 = manager->request_pause("test_robot_10");
    CHECK(!id_2.has_value());

    // Subsequent mode requests
    auto id_3 = manager->request_pause("test_robot_2");
    REQUIRE(id_3.has_value());
    CHECK(id_3.value() == 2);

    auto id_4 = manager->request_pause("test_robot_3");
    REQUIRE(id_4.has_value());
    CHECK(id_4.value() == 3);
  }

  GIVEN("Sending resume request to dummy robots")
  {
    // Valid mode
    auto id_1 = manager->request_resume("test_robot_1");
    REQUIRE(id_1.has_value());
    CHECK(id_1.value() == 1);

    // Invalid robot
    auto id_2 = manager->request_resume("test_robot_10");
    CHECK(!id_2.has_value());

    // Subsequent mode requests
    auto id_3 = manager->request_resume("test_robot_2");
    REQUIRE(id_3.has_value());
    CHECK(id_3.value() == 2);

    auto id_4 = manager->request_resume("test_robot_3");
    REQUIRE(id_4.has_value());
    CHECK(id_4.value() == 3);
  }

  GIVEN("Sending dock request to dummy robots")
  {
    // Valid mode
    auto id_1 = manager->request_dock("test_robot_1", "mock_dock");
    REQUIRE(id_1.has_value());
    CHECK(id_1.value() == 1);

    // Invalid robot
    auto id_2 = manager->request_dock("test_robot_10", "mock_dock");
    CHECK(!id_2.has_value());

    // Subsequent mode requests
    auto id_3 = manager->request_dock("test_robot_2", "mock_dock");
    REQUIRE(id_3.has_value());
    CHECK(id_3.value() == 2);

    auto id_4 = manager->request_dock("test_robot_3", "mock_dock");
    REQUIRE(id_4.has_value());
    CHECK(id_4.value() == 3);
  }

  GIVEN("Sending relocalization requests to dummy robots")
  {
    // Valid relocalization
    free_fleet::messages::Location valid_location {
      0,
      0,
      0.0,
      0.0,
      0.0,
      test_map_name,
    };
    auto id_1 = manager->request_relocalization(
      "test_robot_1",
      valid_location,
      0);
    REQUIRE(id_1.has_value());
    CHECK(id_1.value() == 1);

    // Invalid waypoint index
    auto id_2 = manager->request_relocalization(
      "test_robot_2",
      valid_location,
      100);
    CHECK(!id_2.has_value());

    // Last visited waypoint too far away
    auto id_3 = manager->request_relocalization(
      "test_robot_3",
      valid_location,
      5);
    CHECK(!id_3.has_value());

    // Subsequent requests
    auto id_4 = manager->request_relocalization(
      "test_robot_2",
      valid_location,
      0);
    REQUIRE(id_4.has_value());
    CHECK(id_4.value() == 2);

    auto id_5 = manager->request_relocalization(
      "test_robot_3",
      valid_location,
      0);
    REQUIRE(id_5.has_value());
    CHECK(id_5.value() == 3);
  }

  GIVEN("Sending navigation request to dummy robots")
  {
    // Valid waypoints
    free_fleet::messages::Location valid_location {
      0,
      0,
      0.0,
      0.0,
      0.0,
      test_map_name};
    free_fleet::messages::Waypoint valid_waypoint_1 {
      0,
      valid_location};

    valid_location.x = 10.0;
    free_fleet::messages::Waypoint valid_waypoint_2 {
      1,
      valid_location};
    
    auto id_1 = manager->request_navigation(
      "test_robot_2",
      {valid_waypoint_1, valid_waypoint_2});
    REQUIRE(id_1.has_value());
    CHECK(id_1.value() == 1);

    // Invalid waypoints
    free_fleet::messages::Waypoint invalid_waypoint {
      100,
      free_fleet::messages::Location()};
    auto id_2 = manager->request_navigation(
      "test_robot_3",
      {free_fleet::messages::Waypoint(), invalid_waypoint});
    CHECK(!id_2.has_value());

    // Empty path
    auto id_3 = manager->request_navigation(
      "test_robot_3",
      {});
    CHECK(!id_3.has_value());

    // Invalid robot
    auto id_4 = manager->request_navigation(
      "test_robot_30",
      {free_fleet::messages::Waypoint(), free_fleet::messages::Waypoint()});
    CHECK(!id_4.has_value());

    // Subsequent navigation request
    auto id_5 = manager->request_navigation(
      "test_robot_1",
      {valid_waypoint_1, valid_waypoint_2});
    REQUIRE(id_5.has_value());
    CHECK(id_5.value() == 2);

    auto id_6 = manager->request_navigation(
      "test_robot_3",
      {valid_waypoint_1, valid_waypoint_2});
    REQUIRE(id_6.has_value());
    CHECK(id_6.value() == 3);
  }

  GIVEN("Sending subsequent requests of different types")
  {
    // Dock
    auto id_1 = manager->request_dock("test_robot_1", "mock_dock");
    REQUIRE(id_1.has_value());
    CHECK(id_1.value() == 1);

    // Pause
    auto id_2 = manager->request_pause("test_robot_1");
    REQUIRE(id_2.has_value());
    CHECK(id_2.value() == 2);

    // Resume
    auto id_3 = manager->request_resume("test_robot_1");
    REQUIRE(id_3.has_value());
    CHECK(id_3.value() == 3);

    // Navigation
    free_fleet::messages::Location valid_location {
      0,
      0,
      0.0,
      0.0,
      0.0,
      test_map_name};
    free_fleet::messages::Waypoint valid_waypoint_1 {
      0,
      valid_location};
    valid_location.x = 10.0;
    free_fleet::messages::Waypoint valid_waypoint_2 {
      1,
      valid_location};
    auto id_4 = manager->request_navigation(
      "test_robot_2",
      {valid_waypoint_1, valid_waypoint_2});
    REQUIRE(id_4.has_value());
    CHECK(id_4.value() == 4);

    // Relocalization
    valid_location.x = 0.0;
    valid_location.y = 10.0;
    auto id_5 = manager->request_relocalization(
      "test_robot_3",
      valid_location,
      3);
    REQUIRE(id_5.has_value());
    CHECK(id_5.value() == 5);

    // Invalid navigation
    free_fleet::messages::Waypoint invalid_waypoint {
      100,
      free_fleet::messages::Location()};
    auto id_6 = manager->request_navigation(
      "test_robot_1",
      {free_fleet::messages::Waypoint(), invalid_waypoint});
    CHECK(!id_6.has_value());

    // Valid Relocalization
    valid_location.x = 0.0;
    valid_location.y = 0.0;
    auto id_7 = manager->request_relocalization(
      "test_robot_1",
      valid_location,
      0);
    REQUIRE(id_7.has_value());
    CHECK(id_7.value() == 6);
  }
}

class MockServerMiddlewareWithRobot : public free_fleet::MockServerMiddleware
{
public:

  MockServerMiddlewareWithRobot()
  {}

  std::vector<free_fleet::messages::RobotState> read_states() final
  {
    const std::string test_map_name = "test_level";
    free_fleet::messages::Location valid_location {
      0,
      0,
      0.0,
      0.0,
      0.0,
      test_map_name};
    free_fleet::messages::RobotState valid_state {
      "test_robot",
      "test_model",
      0,
      free_fleet::messages::RobotMode(),
      1.0,
      valid_location,
      0
    };
    return {valid_state};
  }
};

SCENARIO("Testing update robot callback with dummy robot")
{
  const std::string fleet_name = "test_fleet";

  const std::string test_map_name = "test_level";
  std::shared_ptr<rmf_traffic::agv::Graph> graph(new rmf_traffic::agv::Graph);
  graph->add_waypoint(test_map_name, {0, 0});
  graph->add_waypoint(test_map_name, {10, 0});
  graph->add_waypoint(test_map_name, {-10, 0});
  graph->add_waypoint(test_map_name, {0, 10});
  graph->add_waypoint(test_map_name, {0, -10});
  graph->add_lane(0, 1);
  graph->add_lane(1, 0);
  graph->add_lane(0, 2);
  graph->add_lane(2, 0);
  graph->add_lane(0, 3);
  graph->add_lane(3, 0);
  graph->add_lane(0, 4);
  graph->add_lane(4, 0);
  graph->add_waypoint(test_map_name, {100, 100});

  std::unique_ptr<free_fleet::transport::ServerMiddleware> m(
    new MockServerMiddlewareWithRobot());
  auto ct = free_fleet::manager::SimpleCoordinateTransformer::make(
    1.0,
    0.0,
    0.0,
    0.0);
  free_fleet::Manager::TimeNow time_now_fn =
    [](){ return std::chrono::steady_clock::now(); };
  free_fleet::Manager::RobotUpdatedCallback cb =
    [](const free_fleet::manager::RobotInfo& updated_robot_info)
  {
    CHECK(updated_robot_info.name() == "test_robot");
  };

  auto manager = free_fleet::Manager::make(
    fleet_name,
    graph,
    std::move(m),
    ct,
    time_now_fn,
    cb);
  REQUIRE(manager);

  free_fleet::messages::RobotState initial_state {
    "test_robot",
    "test_model",
    1,
    free_fleet::messages::RobotMode(),
    1.0,
    free_fleet::messages::Location(),
    0
  };
  rmf_traffic::Time initial_time = std::chrono::steady_clock::now();

  auto& impl = free_fleet::Manager::Implementation::get(*manager);

  auto robot_info =
    free_fleet::manager::RobotInfo::Implementation::make(
      initial_state,
      graph,
      initial_time);
  REQUIRE(robot_info);

  impl.robots[robot_info->name()] = robot_info;
  
  impl.run_once();  
}
