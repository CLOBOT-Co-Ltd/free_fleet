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

#ifndef TEST__UNIT__MOCK_CLIENTMIDDLEWARE_HPP
#define TEST__UNIT__MOCK_CLIENTMIDDLEWARE_HPP

#include <free_fleet/transport/ClientMiddleware.hpp>

namespace free_fleet {

class MockClientMiddleware : public transport::ClientMiddleware
{
public:
  std::function<void(const messages::DockRequest&)> dock_request_callback;
  std::function<void(const messages::PauseRequest&)> pause_request_callback;
  std::function<void(const messages::ResumeRequest&)> resume_request_callback;
  std::function<void(const messages::NavigationRequest&)>
    navigation_request_callback;
  std::function<void(const messages::RelocalizationRequest&)>
    relocalization_request_callback;

  MockClientMiddleware()
  {}

  void send_state(const messages::RobotState&) override
  {}

  void set_dock_request_callback(
    std::function<void(const messages::DockRequest&)> callback) override
  {
    dock_request_callback = std::move(callback);
  }

  void set_pause_request_callback(
    std::function<void(const messages::PauseRequest&)> callback) override
  {
    pause_request_callback = std::move(callback);
  }
  
  void set_resume_request_callback(
    std::function<void(const messages::ResumeRequest&)> callback) override
  {
    resume_request_callback = std::move(callback);
  }

  void set_navigation_request_callback(
    std::function<void(const messages::NavigationRequest&)> callback) override
  {
    navigation_request_callback = std::move(callback);
  }

  void set_relocalization_request_callback(
    std::function<void(const messages::RelocalizationRequest&)> callback)
    override
  {
    relocalization_request_callback = std::move(callback);
  }
};

} // namespace free_fleet

#endif // TEST__UNIT__MOCK_CLIENTMIDDLEWARE_HPP
