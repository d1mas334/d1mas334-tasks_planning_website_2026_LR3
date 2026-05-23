# Лабораторная работа 03. Проектирование и оптимизация реляционной базы данных

## Дисциплина

Архитектура информационных систем / Программная инженерия.

## Вариант

Вариант 10 — планирование задач.

Приложение содержит три основные сущности:

- пользователь;
- цель;
- задача.

Лабораторная работа №3 продолжает REST API из лабораторной №2, но вместо in-memory storage использует PostgreSQL через компонент `userver::components::Postgres`.

## Технологии

- C++20;
- Yandex Userver;
- PostgreSQL 16;
- REST API;
- Docker и Docker Compose.

MongoDB, Redis и RabbitMQ в этой лабораторной не используются.

## Структура проекта

```text
.
├── CMakeLists.txt
├── Dockerfile
├── docker-compose.yaml
├── configs/
│   └── static_config.yaml
├── db/
│   ├── schema.sql
│   ├── data.sql
│   └── queries.sql
├── src/
│   └── main.cpp
├── tests/
│   └── curl_examples.md
├── optimization.md
└── openapi.yaml
```

## Схема БД

Таблица `users`:

| Поле | Тип | Ограничения |
|---|---|---|
| `id` | `BIGSERIAL` | `PRIMARY KEY` |
| `login` | `VARCHAR(64)` | `NOT NULL`, `UNIQUE` |
| `password_hash` | `VARCHAR(255)` | `NOT NULL` |
| `first_name` | `VARCHAR(100)` | `NOT NULL` |
| `last_name` | `VARCHAR(100)` | `NOT NULL` |
| `email` | `VARCHAR(255)` | `NOT NULL`, `UNIQUE` |
| `phone` | `VARCHAR(32)` | nullable |
| `role` | `VARCHAR(32)` | `NOT NULL`, `CHECK ('worker', 'manager', 'admin')` |
| `created_at` | `TIMESTAMP` | `NOT NULL DEFAULT CURRENT_TIMESTAMP` |

Таблица `goals`:

| Поле | Тип | Ограничения |
|---|---|---|
| `id` | `BIGSERIAL` | `PRIMARY KEY` |
| `title` | `VARCHAR(255)` | `NOT NULL` |
| `description` | `TEXT` | `NOT NULL` |
| `author_id` | `BIGINT` | `NOT NULL`, `FOREIGN KEY users(id)` |
| `status` | `VARCHAR(32)` | `NOT NULL DEFAULT 'active'`, `CHECK ('active', 'completed', 'cancelled')` |
| `created_at` | `TIMESTAMP` | `NOT NULL DEFAULT CURRENT_TIMESTAMP` |

Таблица `tasks`:

| Поле | Тип | Ограничения |
|---|---|---|
| `id` | `BIGSERIAL` | `PRIMARY KEY` |
| `goal_id` | `BIGINT` | `NOT NULL`, `FOREIGN KEY goals(id) ON DELETE CASCADE` |
| `title` | `VARCHAR(255)` | `NOT NULL` |
| `description` | `TEXT` | `NOT NULL` |
| `assignee_id` | `BIGINT` | `NOT NULL`, `FOREIGN KEY users(id)` |
| `author_id` | `BIGINT` | `NOT NULL`, `FOREIGN KEY users(id)` |
| `status` | `VARCHAR(32)` | `NOT NULL DEFAULT 'new'`, `CHECK ('new', 'in_progress', 'done', 'cancelled')` |
| `due_date` | `DATE` | nullable |
| `created_at` | `TIMESTAMP` | `NOT NULL DEFAULT CURRENT_TIMESTAMP` |
| `updated_at` | `TIMESTAMP` | `NOT NULL DEFAULT CURRENT_TIMESTAMP` |

DDL находится в [db/schema.sql](db/schema.sql), тестовые данные — в [db/data.sql](db/data.sql), SQL-шаблоны операций — в [db/queries.sql](db/queries.sql).

## Индексы

Созданы индексы:

- `users.login` — через `UNIQUE`, быстрый поиск пользователя по логину;
- `users.email` — через `UNIQUE`, контроль уникальности email;
- `idx_goals_author_id` — ускоряет выборки целей по автору;
- `idx_tasks_goal_id` — ускоряет получение задач цели;
- `idx_tasks_assignee_id` — ускоряет выборки задач исполнителя;
- `idx_tasks_author_id` — ускоряет выборки задач автора;
- `idx_users_first_name_lower`, `idx_users_last_name_lower` — функциональные индексы для регистронезависимого поиска по имени и фамилии;
- `idx_tasks_status`, `idx_goals_status` — фильтрация по статусам;
- `idx_tasks_goal_id_status` — частый составной фильтр задач по цели и статусу.

Подробности и планы запросов: [optimization.md](optimization.md).

## Запуск

Сборка и запуск API вместе с PostgreSQL:

```bash
docker compose up --build
```

API доступен по адресу:

```text
http://localhost:8080
```

PostgreSQL доступен внутри compose-сети как сервис `postgres`. Схема и тестовые данные автоматически подключаются через `/docker-entrypoint-initdb.d`.

## Проверка API

Проверить сервис:

```bash
curl http://localhost:8080/ping
```

Ожидаемый ответ:

```json
{"status":"ok"}
```

Создать пользователя:

```bash
LOGIN="ivan_$(date +%s)"
curl -i -X POST http://localhost:8080/api/users \
  -H "Content-Type: application/json" \
  -d "{\"login\":\"$LOGIN\",\"password\":\"12345\",\"firstName\":\"Ivan\",\"lastName\":\"Ivanov\",\"email\":\"$LOGIN@example.com\",\"phone\":\"+79990000000\",\"role\":\"manager\"}"
```

Получить учебный bearer token:

```bash
curl -i -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d "{\"login\":\"$LOGIN\",\"password\":\"12345\"}"
```

Для быстрых ручных проверок можно использовать seed-пользователя:

```bash
TOKEN=$(curl -s -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"login":"alexey","password":"pass123"}' | sed -E 's/.*"token":"([^"]+)".*/\1/')
```

Создать цель:

```bash
curl -i -X POST http://localhost:8080/api/goals \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{"title":"Сдать лабораторную №3","description":"Подключить PostgreSQL и описать оптимизацию"}'
```

Создать задачу:

```bash
curl -i -X POST http://localhost:8080/api/goals/1/tasks \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{"title":"Проверить PostgreSQL storage","description":"Убедиться, что API пишет в БД","assigneeId":1,"dueDate":"2026-06-01"}'
```

Получить задачи цели:

```bash
curl -i http://localhost:8080/api/goals/1/tasks \
  -H "Authorization: Bearer $TOKEN"
```

Изменить статус задачи:

```bash
curl -i -X PATCH http://localhost:8080/api/goals/1/tasks/1/status \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{"status":"in_progress"}'
```

## Endpoints

| Метод | URL | Назначение | Auth |
|---|---|---|---|
| GET | `/ping` | Проверка сервиса | Нет |
| POST | `/api/users` | Создание пользователя | Нет |
| POST | `/api/auth/login` | Получение bearer token | Нет |
| GET | `/api/users/by-login?login=alexey` | Поиск пользователя по логину | Да |
| GET | `/api/users/search?mask=iv` | Поиск пользователей по маске имени и фамилии | Да |
| POST | `/api/goals` | Создание цели | Да |
| GET | `/api/goals` | Получение всех целей | Да |
| POST | `/api/goals/{goalId}/tasks` | Создание задачи в цели | Да |
| GET | `/api/goals/{goalId}/tasks` | Получение задач цели | Да |
| PATCH | `/api/goals/{goalId}/tasks/{taskId}/status` | Изменение статуса задачи | Да |

## SQL-проверки

Открыть `psql` внутри контейнера:

```bash
docker compose exec postgres psql -U task_planning -d task_planning
```

Проверить таблицы:

```bash
docker compose exec -T postgres psql -U task_planning -d task_planning \
  -c "\dt"
```

Проверить количество записей:

```bash
docker compose exec -T postgres psql -U task_planning -d task_planning \
  -c "SELECT 'users' AS table_name, COUNT(*) FROM users UNION ALL SELECT 'goals', COUNT(*) FROM goals UNION ALL SELECT 'tasks', COUNT(*) FROM tasks;"
```

Получить план запроса:

```bash
docker compose exec -T postgres psql -U task_planning -d task_planning \
  -c "EXPLAIN (ANALYZE, BUFFERS) SELECT * FROM tasks WHERE goal_id = 1 ORDER BY id;"
```

Файл [db/queries.sql](db/queries.sql) содержит шаблоны SQL-запросов с параметрами `$1`, `$2`, `$3`, которые использует API-код.
