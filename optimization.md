# Оптимизация PostgreSQL для варианта 10

## Hot paths

Основные частые запросы API:

- поиск пользователя по `login` при `/api/users/by-login` и `/api/auth/login`;
- проверка пользователя по `id` при bearer token `token-{id}`;
- получение всех задач цели по `goal_id`;
- изменение статуса задачи по паре `goal_id + task_id`;
- поиск пользователей по маске имени и фамилии;
- фильтрация задач и целей по статусу.

## Индексы

В [db/schema.sql](db/schema.sql) созданы индексы:

```sql
CREATE INDEX idx_goals_author_id ON goals(author_id);
CREATE INDEX idx_tasks_goal_id ON tasks(goal_id);
CREATE INDEX idx_tasks_assignee_id ON tasks(assignee_id);
CREATE INDEX idx_tasks_author_id ON tasks(author_id);
CREATE INDEX idx_users_first_name_lower ON users(LOWER(first_name));
CREATE INDEX idx_users_last_name_lower ON users(LOWER(last_name));
CREATE INDEX idx_tasks_status ON tasks(status);
CREATE INDEX idx_goals_status ON goals(status);
CREATE INDEX idx_tasks_goal_id_status ON tasks(goal_id, status);
```

`users.login` и `users.email` индексируются автоматически через `UNIQUE`.

Индексы по FK нужны, потому что внешние ключи часто используются в `JOIN`, проверках существования и выборках дочерних сущностей. Индекс `tasks(goal_id, status)` полезен для развития API: например, для списка задач конкретной цели только в статусе `new` или `in_progress`.

Функциональные индексы `LOWER(first_name)` и `LOWER(last_name)` поддерживают регистронезависимый поиск. В текущем API используется маска вида `%mask%`; для промышленного поиска по вхождению лучше добавить расширение `pg_trgm` и GIN-индексы, но для учебной лабораторной оставлены только требуемые функциональные индексы.

## EXPLAIN ANALYZE

Планы сняты в PostgreSQL-контейнере командой:

```bash
docker compose exec -T postgres psql -U task_planning -d task_planning \
  -c "EXPLAIN (ANALYZE, BUFFERS) ..."
```

На учебном наборе данных время выполнения мало и может колебаться. Важнее форма плана: `Seq Scan` до индекса и `Index Scan` после индекса.

## Запрос 1: поиск пользователя по login

Запрос:

```sql
SELECT id, login, password_hash, first_name, last_name, email,
       COALESCE(phone, '') AS phone, role
FROM users
WHERE login = 'alexey';
```

Для сравнения план до оптимизации был снят на временной версии таблицы без UNIQUE(login):

```text
Seq Scan on users  (cost=0.00..10.50 rows=1 width=1736) (actual time=0.009..0.011 rows=1 loops=1)
  Filter: ((login)::text = 'alexey'::text)
  Rows Removed by Filter: 10
  Buffers: shared hit=1
Planning Time: 0.326 ms
Execution Time: 0.033 ms
```

После оптимизации:

```text
Index Scan using users_login_key on users  (cost=0.14..8.16 rows=1 width=1736) (actual time=0.039..0.040 rows=1 loops=1)
  Index Cond: ((login)::text = 'alexey'::text)
  Buffers: shared hit=2
Planning Time: 0.320 ms
Execution Time: 0.062 ms
```

Вывод: до индекса PostgreSQL просматривал таблицу и отбрасывал строки фильтром. После создания `UNIQUE(login)` используется `Index Scan`, и поиск по логину остается быстрым при росте таблицы.

## Запрос 2: получение задач цели по goal_id

Запрос:

```sql
SELECT id, goal_id, title, description, assignee_id, author_id,
       status, COALESCE(due_date::text, '') AS due_date
FROM tasks
WHERE goal_id = 1
ORDER BY id;
```

До оптимизации, без `idx_tasks_goal_id` и `idx_tasks_goal_id_status`:

```text
Sort  (cost=11.39..11.40 rows=1 width=694) (actual time=0.035..0.036 rows=2 loops=1)
  Sort Key: id
  Sort Method: quicksort  Memory: 25kB
  Buffers: shared hit=4 dirtied=1
  ->  Seq Scan on tasks  (cost=0.00..11.38 rows=1 width=694) (actual time=0.008..0.009 rows=2 loops=1)
        Filter: (goal_id = 1)
        Rows Removed by Filter: 9
        Buffers: shared hit=1 dirtied=1
Planning Time: 0.322 ms
Execution Time: 0.062 ms
```

После оптимизации:

```text
Sort  (cost=8.18..8.18 rows=1 width=694) (actual time=0.038..0.039 rows=2 loops=1)
  Sort Key: id
  Sort Method: quicksort  Memory: 25kB
  Buffers: shared hit=8
  ->  Index Scan using idx_tasks_goal_id_status on tasks  (cost=0.14..8.17 rows=1 width=694) (actual time=0.016..0.017 rows=2 loops=1)
        Index Cond: (goal_id = 1)
        Buffers: shared hit=5
Planning Time: 0.422 ms
Execution Time: 0.073 ms
```

Вывод: до индекса выполнялся полный просмотр `tasks`. После оптимизации используется индекс с первым столбцом `goal_id`, поэтому запрос масштабируется лучше при большом количестве задач.

## Итог

Для текущей лабораторной основные оптимизации закрывают реальные маршруты API:

- `users.login` ускоряет login и поиск пользователя;
- `tasks.goal_id` и `tasks(goal_id, status)` ускоряют список задач цели;
- индексы по FK поддерживают связи `users -> goals -> tasks`;
- CHECK constraints защищают допустимые роли и статусы;
- FK constraints сохраняют ссылочную целостность.
