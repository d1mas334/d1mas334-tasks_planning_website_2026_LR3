# Curl examples

Базовый адрес:

```bash
BASE_URL=http://localhost:8080
```

## 1. Ping

```bash
curl -i "$BASE_URL/ping"
```

## 2. Create user

```bash
curl -i -X POST "$BASE_URL/api/users" \
  -H "Content-Type: application/json" \
  -d '{"login":"ivan","password":"12345","firstName":"Ivan","lastName":"Ivanov","email":"ivan@example.com","phone":"+79990000000","role":"manager"}'
```

## 3. Login

```bash
curl -i -X POST "$BASE_URL/api/auth/login" \
  -H "Content-Type: application/json" \
  -d '{"login":"ivan","password":"12345"}'
```

Дальше используется токен:

```bash
TOKEN=token-1
```

## 4. Find user by login

```bash
curl -i "$BASE_URL/api/users/by-login?login=ivan" \
  -H "Authorization: Bearer $TOKEN"
```

## 5. Search users by name mask

```bash
curl -i "$BASE_URL/api/users/search?mask=iv" \
  -H "Authorization: Bearer $TOKEN"
```

## 6. Create goal

```bash
curl -i -X POST "$BASE_URL/api/goals" \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{"title":"Сдать лабораторные","description":"Закрыть все лабораторные по архитектуре ИС"}'
```

## 7. List goals

```bash
curl -i "$BASE_URL/api/goals" \
  -H "Authorization: Bearer $TOKEN"
```

## 8. Create task

```bash
curl -i -X POST "$BASE_URL/api/goals/1/tasks" \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{"title":"Сделать REST API","description":"Реализовать endpoints для варианта 10","assigneeId":1,"dueDate":"2026-06-01"}'
```

## 9. List goal tasks

```bash
curl -i "$BASE_URL/api/goals/1/tasks" \
  -H "Authorization: Bearer $TOKEN"
```

## 10. Update task status

```bash
curl -i -X PATCH "$BASE_URL/api/goals/1/tasks/1/status" \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{"status":"in_progress"}'
```
