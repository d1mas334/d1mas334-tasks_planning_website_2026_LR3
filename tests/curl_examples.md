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
LOGIN="ivan_$(date +%s)"

curl -i -X POST "$BASE_URL/api/users" \
  -H "Content-Type: application/json" \
  -d "{\"login\":\"$LOGIN\",\"password\":\"12345\",\"firstName\":\"Ivan\",\"lastName\":\"Ivanov\",\"email\":\"$LOGIN@example.com\",\"phone\":\"+79990000000\",\"role\":\"manager\"}"
```

## 3. Login

```bash
curl -i -X POST "$BASE_URL/api/auth/login" \
  -H "Content-Type: application/json" \
  -d "{\"login\":\"$LOGIN\",\"password\":\"12345\"}"
```

Для дальнейших примеров можно использовать seed-пользователя из `db/data.sql`:

```bash
TOKEN=$(curl -s -X POST "$BASE_URL/api/auth/login" \
  -H "Content-Type: application/json" \
  -d '{"login":"alexey","password":"pass123"}' | sed -E 's/.*"token":"([^"]+)".*/\1/')
```

## 4. Find user by login

```bash
curl -i "$BASE_URL/api/users/by-login?login=alexey" \
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
  -d '{"title":"Сдать лабораторную №3","description":"Подключить PostgreSQL storage"}'
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
  -d '{"title":"Проверить PostgreSQL storage","description":"Убедиться, что API пишет в БД","assigneeId":1,"dueDate":"2026-06-01"}'
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
