#include <algorithm>
#include <charconv>
#include <cctype>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/formats/common/type.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/utils/daemon_run.hpp>

namespace task_planning {

namespace components = userver::components;
namespace formats = userver::formats;
namespace handlers = userver::server::handlers;
namespace http = userver::server::http;
namespace json = userver::formats::json;
namespace server = userver::server;

struct User {
  int id{};
  std::string login;
  std::string password;
  std::string first_name;
  std::string last_name;
  std::string email;
  std::string phone;
  std::string role;
};

struct Goal {
  int id{};
  std::string title;
  std::string description;
  int author_id{};
  std::string status;
};

struct Task {
  int id{};
  int goal_id{};
  std::string title;
  std::string description;
  int assignee_id{};
  int author_id{};
  std::string status;
  std::string due_date;
};

class InMemoryStorage final : public components::ComponentBase {
 public:
  static constexpr std::string_view kName = "in-memory-storage";

  using ComponentBase::ComponentBase;

  std::optional<User> CreateUser(User user) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (user_ids_by_login_.find(user.login) != user_ids_by_login_.end()) {
      return std::nullopt;
    }

    user.id = next_user_id_++;
    const auto id = user.id;
    user_ids_by_login_[user.login] = id;
    users_by_id_[id] = user;
    user_order_.push_back(id);
    return user;
  }

  std::optional<User> FindUserByLogin(const std::string& login) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto login_it = user_ids_by_login_.find(login);
    if (login_it == user_ids_by_login_.end()) {
      return std::nullopt;
    }

    return users_by_id_.at(login_it->second);
  }

  std::optional<User> FindUserById(int id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto it = users_by_id_.find(id);
    if (it == users_by_id_.end()) {
      return std::nullopt;
    }

    return it->second;
  }

  std::vector<User> SearchUsers(const std::string& mask) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto lower_mask = ToLower(mask);
    std::vector<User> result;
    for (const auto user_id : user_order_) {
      const auto& user = users_by_id_.at(user_id);
      const auto first_name = ToLower(user.first_name);
      const auto last_name = ToLower(user.last_name);
      if (first_name.find(lower_mask) != std::string::npos ||
          last_name.find(lower_mask) != std::string::npos) {
        result.push_back(user);
      }
    }
    return result;
  }

  std::optional<int> AuthenticateUser(const std::string& login,
                                      const std::string& password) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto login_it = user_ids_by_login_.find(login);
    if (login_it == user_ids_by_login_.end()) {
      return std::nullopt;
    }

    const auto& user = users_by_id_.at(login_it->second);
    if (user.password != password) {
      return std::nullopt;
    }

    return user.id;
  }

  std::optional<int> AuthenticateToken(std::string_view token) const {
    constexpr std::string_view kPrefix = "token-";
    if (token.substr(0, kPrefix.size()) != kPrefix) {
      return std::nullopt;
    }

    int user_id = 0;
    const auto id_view = token.substr(kPrefix.size());
    const auto* begin = id_view.data();
    const auto* end = id_view.data() + id_view.size();
    const auto [ptr, ec] = std::from_chars(begin, end, user_id);
    if (ec != std::errc{} || ptr != end || user_id <= 0) {
      return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (users_by_id_.find(user_id) == users_by_id_.end()) {
      return std::nullopt;
    }

    return user_id;
  }

  Goal CreateGoal(std::string title, std::string description, int author_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    Goal goal;
    goal.id = next_goal_id_++;
    goal.title = std::move(title);
    goal.description = std::move(description);
    goal.author_id = author_id;
    goal.status = "active";

    goals_by_id_[goal.id] = goal;
    goal_order_.push_back(goal.id);
    return goal;
  }

  std::vector<Goal> ListGoals() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<Goal> result;
    for (const auto goal_id : goal_order_) {
      result.push_back(goals_by_id_.at(goal_id));
    }
    return result;
  }

  std::optional<Goal> FindGoalById(int id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto it = goals_by_id_.find(id);
    if (it == goals_by_id_.end()) {
      return std::nullopt;
    }

    return it->second;
  }

  std::optional<Task> CreateTask(int goal_id, std::string title,
                                 std::string description, int assignee_id,
                                 int author_id, std::string due_date) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (goals_by_id_.find(goal_id) == goals_by_id_.end() ||
        users_by_id_.find(assignee_id) == users_by_id_.end()) {
      return std::nullopt;
    }

    Task task;
    task.id = next_task_id_++;
    task.goal_id = goal_id;
    task.title = std::move(title);
    task.description = std::move(description);
    task.assignee_id = assignee_id;
    task.author_id = author_id;
    task.status = "new";
    task.due_date = std::move(due_date);

    tasks_by_id_[task.id] = task;
    task_ids_by_goal_[goal_id].push_back(task.id);
    return task;
  }

  std::vector<Task> ListTasks(int goal_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<Task> result;
    const auto goal_it = task_ids_by_goal_.find(goal_id);
    if (goal_it == task_ids_by_goal_.end()) {
      return result;
    }

    for (const auto task_id : goal_it->second) {
      result.push_back(tasks_by_id_.at(task_id));
    }
    return result;
  }

  std::optional<Task> UpdateTaskStatus(int goal_id, int task_id,
                                       std::string status) {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto task_it = tasks_by_id_.find(task_id);
    if (task_it == tasks_by_id_.end() || task_it->second.goal_id != goal_id) {
      return std::nullopt;
    }

    task_it->second.status = std::move(status);
    return task_it->second;
  }

 private:
  static std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](char ch) {
      return static_cast<char>(
          std::tolower(static_cast<unsigned char>(ch)));
    });
    return value;
  }

  mutable std::mutex mutex_;
  int next_user_id_{1};
  int next_goal_id_{1};
  int next_task_id_{1};

  std::unordered_map<int, User> users_by_id_;
  std::unordered_map<std::string, int> user_ids_by_login_;
  std::vector<int> user_order_;

  std::unordered_map<int, Goal> goals_by_id_;
  std::vector<int> goal_order_;

  std::unordered_map<int, Task> tasks_by_id_;
  std::unordered_map<int, std::vector<int>> task_ids_by_goal_;
};

struct ApiError {
  http::HttpStatus status;
  std::string message;
};

json::ValueBuilder BuildError(std::string_view message) {
  json::ValueBuilder builder;
  builder["error"] = std::string{message};
  return builder;
}

json::ValueBuilder BuildUser(const User& user) {
  json::ValueBuilder builder;
  builder["id"] = user.id;
  builder["login"] = user.login;
  builder["firstName"] = user.first_name;
  builder["lastName"] = user.last_name;
  builder["email"] = user.email;
  builder["phone"] = user.phone;
  builder["role"] = user.role;
  return builder;
}

json::ValueBuilder BuildGoal(const Goal& goal) {
  json::ValueBuilder builder;
  builder["id"] = goal.id;
  builder["title"] = goal.title;
  builder["description"] = goal.description;
  builder["authorId"] = goal.author_id;
  builder["status"] = goal.status;
  return builder;
}

json::ValueBuilder BuildTask(const Task& task) {
  json::ValueBuilder builder;
  builder["id"] = task.id;
  builder["goalId"] = task.goal_id;
  builder["title"] = task.title;
  builder["description"] = task.description;
  builder["assigneeId"] = task.assignee_id;
  builder["authorId"] = task.author_id;
  builder["status"] = task.status;
  builder["dueDate"] = task.due_date;
  return builder;
}

std::string MakeJsonResponse(const http::HttpRequest& request,
                             http::HttpStatus status,
                             json::ValueBuilder builder) {
  request.SetResponseStatus(status);
  request.GetHttpResponse().SetHeader(std::string_view{"Content-Type"},
                                      std::string{"application/json"});
  return json::ToString(builder.ExtractValue());
}

std::string MakeErrorResponse(const http::HttpRequest& request,
                              http::HttpStatus status,
                              std::string_view message) {
  return MakeJsonResponse(request, status, BuildError(message));
}

json::Value ParseJsonObjectBody(const http::HttpRequest& request) {
  try {
    const auto body = json::FromString(request.RequestBody());
    if (!body.IsObject()) {
      throw ApiError{http::HttpStatus::kBadRequest,
                     "request body must be a JSON object"};
    }
    return body;
  } catch (const json::Exception&) {
    throw ApiError{http::HttpStatus::kBadRequest, "invalid JSON body"};
  }
}

std::string GetRequiredString(const json::Value& body,
                              std::string_view field_name) {
  const auto value = body[std::string{field_name}];
  if (value.IsMissing() || !value.IsString()) {
    throw ApiError{http::HttpStatus::kBadRequest,
                   std::string{field_name} + " must be a non-empty string"};
  }

  auto result = value.As<std::string>();
  if (result.empty()) {
    throw ApiError{http::HttpStatus::kBadRequest,
                   std::string{field_name} + " must be a non-empty string"};
  }

  return result;
}

int GetRequiredPositiveInt(const json::Value& body, std::string_view field_name) {
  const auto value = body[std::string{field_name}];
  if (value.IsMissing()) {
    throw ApiError{http::HttpStatus::kBadRequest,
                   std::string{field_name} + " is required"};
  }

  try {
    const auto result = value.As<int>();
    if (result <= 0) {
      throw ApiError{http::HttpStatus::kBadRequest,
                     std::string{field_name} + " must be positive"};
    }
    return result;
  } catch (const json::Exception&) {
    throw ApiError{http::HttpStatus::kBadRequest,
                   std::string{field_name} + " must be an integer"};
  }
}

int ParsePositiveId(std::string_view value, std::string_view field_name) {
  int result = 0;
  const auto* begin = value.data();
  const auto* end = value.data() + value.size();
  const auto [ptr, ec] = std::from_chars(begin, end, result);
  if (ec != std::errc{} || ptr != end || result <= 0) {
    throw ApiError{http::HttpStatus::kBadRequest,
                   std::string{field_name} + " must be positive integer"};
  }
  return result;
}

bool IsValidTaskStatus(std::string_view status) {
  return status == "new" || status == "in_progress" || status == "done" ||
         status == "cancelled";
}

class ApiHandlerBase : public handlers::HttpHandlerBase {
 public:
  ApiHandlerBase(const components::ComponentConfig& config,
                 const components::ComponentContext& context)
      : HttpHandlerBase(config, context),
        storage_(context.FindComponent<InMemoryStorage>()) {}

  std::string HandleRequestThrow(
      const http::HttpRequest& request,
      server::request::RequestContext& context) const final {
    try {
      return HandleApiRequest(request, context);
    } catch (const ApiError& error) {
      return MakeErrorResponse(request, error.status, error.message);
    }
  }

 protected:
  virtual std::string HandleApiRequest(
      const http::HttpRequest& request,
      server::request::RequestContext& context) const = 0;

  int RequireAuth(const http::HttpRequest& request) const {
    constexpr std::string_view kPrefix = "Bearer ";

    const auto& auth_header = request.GetHeader("Authorization");
    const auto auth_view = std::string_view{auth_header};
    if (auth_view.substr(0, kPrefix.size()) != kPrefix) {
      throw ApiError{http::HttpStatus::kUnauthorized,
                     "missing or invalid Authorization header"};
    }

    const auto user_id = storage_.AuthenticateToken(
        auth_view.substr(kPrefix.size()));
    if (!user_id.has_value()) {
      throw ApiError{http::HttpStatus::kUnauthorized, "invalid bearer token"};
    }

    return *user_id;
  }

  InMemoryStorage& storage_;
};

class PingHandler final : public ApiHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-ping";
  using ApiHandlerBase::ApiHandlerBase;

 private:
  std::string HandleApiRequest(
      const http::HttpRequest& request,
      server::request::RequestContext&) const override {
    json::ValueBuilder builder;
    builder["status"] = "ok";
    return MakeJsonResponse(request, http::HttpStatus::kOk, std::move(builder));
  }
};

class UsersHandler final : public ApiHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-users";
  using ApiHandlerBase::ApiHandlerBase;

 private:
  std::string HandleApiRequest(
      const http::HttpRequest& request,
      server::request::RequestContext&) const override {
    const auto body = ParseJsonObjectBody(request);

    User user;
    user.login = GetRequiredString(body, "login");
    user.password = GetRequiredString(body, "password");
    user.first_name = GetRequiredString(body, "firstName");
    user.last_name = GetRequiredString(body, "lastName");
    user.email = GetRequiredString(body, "email");
    user.phone = GetRequiredString(body, "phone");
    user.role = GetRequiredString(body, "role");

    const auto created = storage_.CreateUser(std::move(user));
    if (!created.has_value()) {
      throw ApiError{http::HttpStatus::kConflict, "login already exists"};
    }

    return MakeJsonResponse(request, http::HttpStatus::kCreated,
                            BuildUser(*created));
  }
};

class LoginHandler final : public ApiHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-login";
  using ApiHandlerBase::ApiHandlerBase;

 private:
  std::string HandleApiRequest(
      const http::HttpRequest& request,
      server::request::RequestContext&) const override {
    const auto body = ParseJsonObjectBody(request);
    const auto login = GetRequiredString(body, "login");
    const auto password = GetRequiredString(body, "password");

    const auto user_id = storage_.AuthenticateUser(login, password);
    if (!user_id.has_value()) {
      throw ApiError{http::HttpStatus::kUnauthorized,
                     "invalid login or password"};
    }

    json::ValueBuilder builder;
    builder["token"] = "token-" + std::to_string(*user_id);
    return MakeJsonResponse(request, http::HttpStatus::kOk, std::move(builder));
  }
};

class UserByLoginHandler final : public ApiHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-user-by-login";
  using ApiHandlerBase::ApiHandlerBase;

 private:
  std::string HandleApiRequest(
      const http::HttpRequest& request,
      server::request::RequestContext&) const override {
    RequireAuth(request);

    const auto& login = request.GetArg("login");
    if (login.empty()) {
      throw ApiError{http::HttpStatus::kBadRequest,
                     "login query parameter is required"};
    }

    const auto user = storage_.FindUserByLogin(login);
    if (!user.has_value()) {
      throw ApiError{http::HttpStatus::kNotFound, "user not found"};
    }

    return MakeJsonResponse(request, http::HttpStatus::kOk, BuildUser(*user));
  }
};

class UserSearchHandler final : public ApiHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-user-search";
  using ApiHandlerBase::ApiHandlerBase;

 private:
  std::string HandleApiRequest(
      const http::HttpRequest& request,
      server::request::RequestContext&) const override {
    RequireAuth(request);

    const auto& mask = request.GetArg("mask");
    if (mask.empty()) {
      throw ApiError{http::HttpStatus::kBadRequest,
                     "mask query parameter is required"};
    }

    json::ValueBuilder users(formats::common::Type::kArray);
    for (const auto& user : storage_.SearchUsers(mask)) {
      users.PushBack(BuildUser(user));
    }

    return MakeJsonResponse(request, http::HttpStatus::kOk, std::move(users));
  }
};

class GoalsHandler final : public ApiHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-goals";
  using ApiHandlerBase::ApiHandlerBase;

 private:
  std::string HandleApiRequest(
      const http::HttpRequest& request,
      server::request::RequestContext&) const override {
    const auto author_id = RequireAuth(request);

    if (request.GetMethod() == http::HttpMethod::kGet) {
      json::ValueBuilder goals(formats::common::Type::kArray);
      for (const auto& goal : storage_.ListGoals()) {
        goals.PushBack(BuildGoal(goal));
      }
      return MakeJsonResponse(request, http::HttpStatus::kOk,
                              std::move(goals));
    }

    const auto body = ParseJsonObjectBody(request);
    const auto title = GetRequiredString(body, "title");
    const auto description = GetRequiredString(body, "description");
    const auto goal = storage_.CreateGoal(title, description, author_id);

    return MakeJsonResponse(request, http::HttpStatus::kCreated,
                            BuildGoal(goal));
  }
};

class GoalTasksHandler final : public ApiHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-goal-tasks";
  using ApiHandlerBase::ApiHandlerBase;

 private:
  std::string HandleApiRequest(
      const http::HttpRequest& request,
      server::request::RequestContext&) const override {
    const auto author_id = RequireAuth(request);
    const auto goal_id = ParsePositiveId(request.GetPathArg("goalId"), "goalId");

    if (!storage_.FindGoalById(goal_id).has_value()) {
      throw ApiError{http::HttpStatus::kNotFound, "goal not found"};
    }

    if (request.GetMethod() == http::HttpMethod::kGet) {
      json::ValueBuilder tasks(formats::common::Type::kArray);
      for (const auto& task : storage_.ListTasks(goal_id)) {
        tasks.PushBack(BuildTask(task));
      }
      return MakeJsonResponse(request, http::HttpStatus::kOk,
                              std::move(tasks));
    }

    const auto body = ParseJsonObjectBody(request);
    const auto title = GetRequiredString(body, "title");
    const auto description = GetRequiredString(body, "description");
    const auto assignee_id = GetRequiredPositiveInt(body, "assigneeId");
    const auto due_date = GetRequiredString(body, "dueDate");

    if (!storage_.FindUserById(assignee_id).has_value()) {
      throw ApiError{http::HttpStatus::kNotFound, "assignee user not found"};
    }

    const auto task = storage_.CreateTask(goal_id, title, description,
                                          assignee_id, author_id, due_date);
    if (!task.has_value()) {
      throw ApiError{http::HttpStatus::kNotFound, "goal or assignee not found"};
    }

    return MakeJsonResponse(request, http::HttpStatus::kCreated,
                            BuildTask(*task));
  }
};

class TaskStatusHandler final : public ApiHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-task-status";
  using ApiHandlerBase::ApiHandlerBase;

 private:
  std::string HandleApiRequest(
      const http::HttpRequest& request,
      server::request::RequestContext&) const override {
    RequireAuth(request);

    const auto goal_id = ParsePositiveId(request.GetPathArg("goalId"), "goalId");
    const auto task_id = ParsePositiveId(request.GetPathArg("taskId"), "taskId");

    if (!storage_.FindGoalById(goal_id).has_value()) {
      throw ApiError{http::HttpStatus::kNotFound, "goal not found"};
    }

    const auto body = ParseJsonObjectBody(request);
    const auto status = GetRequiredString(body, "status");
    if (!IsValidTaskStatus(status)) {
      throw ApiError{http::HttpStatus::kBadRequest, "invalid task status"};
    }

    const auto task = storage_.UpdateTaskStatus(goal_id, task_id, status);
    if (!task.has_value()) {
      throw ApiError{http::HttpStatus::kNotFound, "task not found"};
    }

    return MakeJsonResponse(request, http::HttpStatus::kOk, BuildTask(*task));
  }
};

}  // namespace task_planning

int main(int argc, char* argv[]) {
  const auto component_list =
      userver::components::MinimalServerComponentList()
          .Append<task_planning::InMemoryStorage>()
          .Append<task_planning::PingHandler>()
          .Append<task_planning::UsersHandler>()
          .Append<task_planning::LoginHandler>()
          .Append<task_planning::UserByLoginHandler>()
          .Append<task_planning::UserSearchHandler>()
          .Append<task_planning::GoalsHandler>()
          .Append<task_planning::GoalTasksHandler>()
          .Append<task_planning::TaskStatusHandler>();

  return userver::utils::DaemonMain(argc, argv, component_list);
}
