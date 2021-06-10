/*
 * Copyright (C) 2002 Open Source Robotics Foundation
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

#include <chrono>
#include <thread>
#include <future>
#include <iostream>
#include <unordered_set>

#include <rmf_traffic/Time.hpp>

#include <free_fleet/client/Client.hpp>
#include "internal_Client.hpp"

namespace free_fleet {
namespace client {

//==============================================================================
bool Client::Implementation::connected() const
{
  return command_handle && status_handle && middleware;
}

//==============================================================================
void Client::Implementation::set_callbacks()
{
  using namespace std::placeholders;
  middleware->set_pause_request_callback(
    std::bind(&Implementation::handle_pause_request, this, _1));
  middleware->set_resume_request_callback(
    std::bind(&Implementation::handle_resume_request, this, _1));
  middleware->set_dock_request_callback(
    std::bind(&Implementation::handle_dock_request, this, _1));
  middleware->set_navigation_request_callback(
    std::bind(&Implementation::handle_navigation_request, this, _1));
  middleware->set_relocalization_request_callback(
    std::bind(&Implementation::handle_relocalization_request, this, _1));
}

//==============================================================================
void Client::Implementation::run_once()
{
  // send state
  free_fleet::messages::RobotState new_state {
    robot_name,
    robot_model,
    task_id,
    status_handle->mode(),
    status_handle->battery_percent(),
    status_handle->location(),
    static_cast<uint32_t>(status_handle->target_path_waypoint_index())
  };
  middleware->send_state(new_state);
}

//==============================================================================
void Client::Implementation::run(uint32_t frequency)
{
  set_callbacks();

  const double seconds_per_iteration = 1.0 / frequency;
  const rmf_traffic::Duration duration_per_iteration =
    rmf_traffic::time::from_seconds(seconds_per_iteration);
  rmf_traffic::Time t_prev = std::chrono::steady_clock::now();

  while (connected())
  {
    if (std::chrono::steady_clock::now() - t_prev < duration_per_iteration)
      continue;
    t_prev = std::chrono::steady_clock::now();

    run_once();
  }
}

//==============================================================================
void Client::Implementation::start_async(uint32_t frequency)
{
  async_thread =
    std::thread(std::bind(&Client::Implementation::run, this, frequency));
}

//==============================================================================
void Client::Implementation::handle_pause_request(
  const messages::PauseRequest& request)
{
  if (!is_valid_request(request))
    return;
  task_ids.insert(request.task_id);
  task_id = request.task_id;
  command_handle->stop();
}

//==============================================================================
void Client::Implementation::handle_resume_request(
  const messages::ResumeRequest& request)
{
  if (!is_valid_request(request))
    return;
  task_ids.insert(request.task_id);
  task_id = request.task_id;
  command_handle->resume();
}

//==============================================================================
void Client::Implementation::handle_dock_request(
  const messages::DockRequest& request)
{
  if (!is_valid_request(request))
    return;
  task_ids.insert(request.task_id);
  task_id = request.task_id;
  free_fleet::client::CommandHandle::RequestCompleted callback =
    [this]() { task_id = 0; };
  command_handle->dock(request.dock_name, callback);
}

//==============================================================================
void Client::Implementation::handle_navigation_request(
  const messages::NavigationRequest& request)
{
  if (!is_valid_request(request))
    return;
  task_ids.insert(request.task_id);
  task_id = request.task_id;
  free_fleet::client::CommandHandle::RequestCompleted callback =
    [this]() { task_id = 0; };
  command_handle->follow_new_path(request.path, callback);
}

//==============================================================================
void Client::Implementation::handle_relocalization_request(
  const messages::RelocalizationRequest& request)
{
  if (!is_valid_request(request))
    return;
  task_ids.insert(request.task_id);
  task_id = request.task_id;
  free_fleet::client::CommandHandle::RequestCompleted callback =
    [this]() { task_id = 0; };
  command_handle->relocalize(request.location, callback);
}

//==============================================================================
auto Client::make(
  const std::string& robot_name,
  const std::string& robot_model,
  std::shared_ptr<CommandHandle> command_handle,
  std::shared_ptr<StatusHandle> status_handle,
  std::unique_ptr<transport::ClientMiddleware> middleware)
  -> std::shared_ptr<Client>
{
  auto make_error_fn = [](const std::string& error_msg)
  {
    std::cerr << error_msg << std::endl;
    return nullptr;
  };

  if (robot_name.empty())
    return make_error_fn("Provided robot name must not be empty.");
  if (robot_model.empty())
    return make_error_fn("Provided robot model must not be empty.");
  if (!command_handle)
    return make_error_fn("Provided command handle is invalid.");
  if (!status_handle)
    return make_error_fn("Provided status handle is invalid.");
  if (!middleware)
    return make_error_fn("Provided middleware is invalid.");

  std::shared_ptr<Client> new_client(new Client);
  new_client->_pimpl = rmf_utils::make_impl<Implementation>(Implementation());
  new_client->_pimpl->robot_name = robot_name;
  new_client->_pimpl->robot_model = robot_model;
  new_client->_pimpl->command_handle = std::move(command_handle);
  new_client->_pimpl->status_handle = std::move(status_handle);
  new_client->_pimpl->middleware = std::move(middleware);
  return new_client;
}

//==============================================================================
Client::Client()
: _pimpl(rmf_utils::make_impl<Implementation>(Implementation()))
{}

//==============================================================================
void Client::run(uint32_t frequency)
{
  if (frequency == 0)
    throw std::range_error("[Error]: Frequency has to be greater than 0.");
  if (started())
    throw std::runtime_error("[Error]: Client has already been started.");
  _pimpl->started = true;

  _pimpl->run(frequency);
}

//==============================================================================
void Client::start_async(uint32_t frequency)
{
  if (frequency == 0)
    throw std::range_error("[Error]: Frequency has to be greater than 0.");
  if (started())
    throw std::runtime_error("[Error]: Client has already been started.");

  _pimpl->start_async(frequency);
}

//==============================================================================
bool Client::started() const
{
  return _pimpl->started.load();
}

//==============================================================================

} // namespace client
} // namespace free_fleet
