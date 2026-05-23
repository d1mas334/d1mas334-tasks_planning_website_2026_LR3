# Лабораторная работа 02. Разработка REST API сервиса

## Дисциплина

Архитектура информационных систем / Программная инженерия.

## Вариант

Вариант 10 — планирование задач, аналог LeaderTask.

Сервис содержит три основные сущности:

- пользователь;
- цель;
- задача.

## Технологии

- C++20;
- Yandex Userver;
- REST API;
- in-memory storage на `std::unordered_map` и `std::vector`;
- Docker и Docker Compose.

PostgreSQL в этой лабораторной не используется. Данные живут только в памяти процесса и удаляются после перезапуска контейнера.

## Структура проекта

```text
.
├── CMakeLists.txt
├── Dockerfile
├── docker-compose.yaml
├── openapi.yaml
├── configs/
│   └── static_config.yaml
├── src/
│   └── main.cpp
└── tests/
    └── curl_examples.md
```

Файлы лабораторной работы №1 (`README.md`, `workspace.dsl`) оставлены без удаления.

## Запуск

Сборка и запуск через Docker Compose:

```bash
docker compose up --build
```

После запуска API доступен по адресу:

```text
http://localhost:8080
```

Проверка работоспособности:

```bash
curl http://localhost:8080/ping
```

Ожидаемый ответ:

```json
{"status":"ok"}
```

## Endpoints

| Метод | URL | Назначение | Auth |
|---|---|---|---|
| GET | `/ping` | Проверка сервиса | Нет |
| POST | `/api/users` | Создание пользователя | Нет |
| POST | `/api/auth/login` | Получение bearer token | Нет |
| GET | `/api/users/by-login?login=ivan` | Поиск пользователя по логину | Да |
| GET | `/api/users/search?mask=iv` | Поиск пользователей по маске имени и фамилии | Да |
| POST | `/api/goals` | Создание цели | Да |
| GET | `/api/goals` | Получение всех целей | Да |
| POST | `/api/goals/{goalId}/tasks` | Создание задачи в цели | Да |
| GET | `/api/goals/{goalId}/tasks` | Получение задач цели | Да |
| PATCH | `/api/goals/{goalId}/tasks/{taskId}/status` | Изменение статуса задачи | Да |

Для защищенных endpoints нужен заголовок:

```text
Authorization: Bearer token-1
```

Токен учебный: `token-{userId}`.

## Пример сценария

Создать пользователя:

```bash
curl -i -X POST http://localhost:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"login":"ivan","password":"12345","firstName":"Ivan","lastName":"Ivanov","email":"ivan@example.com","phone":"+79990000000","role":"manager"}'
```

Получить токен:

```bash
curl -i -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"login":"ivan","password":"12345"}'
```

Создать цель:

```bash
curl -i -X POST http://localhost:8080/api/goals \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer token-1" \
  -d '{"title":"Сдать лабораторные","description":"Закрыть все лабораторные по архитектуре ИС"}'
```

Создать задачу:

```bash
curl -i -X POST http://localhost:8080/api/goals/1/tasks \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer token-1" \
  -d '{"title":"Сделать REST API","description":"Реализовать endpoints для варианта 10","assigneeId":1,"dueDate":"2026-06-01"}'
```

Изменить статус задачи:

```bash
curl -i -X PATCH http://localhost:8080/api/goals/1/tasks/1/status \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer token-1" \
  -d '{"status":"in_progress"}'
```

## Обработка ошибок

Сервис возвращает JSON вида:

```json
{"error":"message"}
```

Поддерживаемые статусы:

- `400 Bad Request` — невалидный JSON, пустые обязательные поля, неправильный статус задачи;
- `401 Unauthorized` — отсутствует `Authorization` header или токен неверный;
- `404 Not Found` — пользователь, цель или задача не найдены;
- `409 Conflict` — логин уже существует.

Допустимые статусы задачи:

- `new`;
- `in_progress`;
- `done`;
- `cancelled`.
