-- 1. Create user
INSERT INTO users (
    login, password_hash, first_name, last_name, email, phone, role
)
VALUES ($1, $2, $3, $4, $5, NULLIF($6, ''), $7)
ON CONFLICT DO NOTHING
RETURNING id, login, password_hash, first_name, last_name, email,
          COALESCE(phone, '') AS phone, role;

-- 2. Find user by login
SELECT id, login, password_hash, first_name, last_name, email,
       COALESCE(phone, '') AS phone, role
FROM users
WHERE login = $1;

-- 3. Search users by first/last name mask
SELECT id, login, password_hash, first_name, last_name, email,
       COALESCE(phone, '') AS phone, role
FROM users
WHERE LOWER(first_name) LIKE '%' || LOWER($1::text) || '%'
   OR LOWER(last_name) LIKE '%' || LOWER($1::text) || '%'
ORDER BY id;

-- 4. Login/authentication by login and password hash
SELECT id
FROM users
WHERE login = $1 AND password_hash = $2;

-- 5. Create goal
INSERT INTO goals (title, description, author_id)
VALUES ($1, $2, $3)
RETURNING id, title, description, author_id, status;

-- 6. List goals
SELECT id, title, description, author_id, status
FROM goals
ORDER BY id;

-- 7. Create task for goal
INSERT INTO tasks (
    goal_id, title, description, assignee_id, author_id, due_date
)
VALUES ($1, $2, $3, $4, $5, NULLIF($6, '')::date)
RETURNING id, goal_id, title, description, assignee_id, author_id,
          status, COALESCE(due_date::text, '') AS due_date;

-- 8. List tasks by goal
SELECT id, goal_id, title, description, assignee_id, author_id,
       status, COALESCE(due_date::text, '') AS due_date
FROM tasks
WHERE goal_id = $1
ORDER BY id;

-- 9. Update task status
UPDATE tasks
SET status = $3,
    updated_at = CURRENT_TIMESTAMP
WHERE goal_id = $1 AND id = $2
RETURNING id, goal_id, title, description, assignee_id, author_id,
          status, COALESCE(due_date::text, '') AS due_date;

-- 10. Check goal existence
SELECT id, title, description, author_id, status
FROM goals
WHERE id = $1;

-- 11. Check user existence by id
SELECT id, login, password_hash, first_name, last_name, email,
       COALESCE(phone, '') AS phone, role
FROM users
WHERE id = $1;
