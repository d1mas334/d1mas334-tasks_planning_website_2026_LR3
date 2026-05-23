DROP TABLE IF EXISTS tasks;
DROP TABLE IF EXISTS goals;
DROP TABLE IF EXISTS users;

CREATE TABLE users (
    id BIGSERIAL PRIMARY KEY,
    login VARCHAR(64) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    first_name VARCHAR(100) NOT NULL,
    last_name VARCHAR(100) NOT NULL,
    email VARCHAR(255) NOT NULL UNIQUE,
    phone VARCHAR(32),
    role VARCHAR(32) NOT NULL CHECK (role IN ('worker', 'manager', 'admin')),
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE goals (
    id BIGSERIAL PRIMARY KEY,
    title VARCHAR(255) NOT NULL,
    description TEXT NOT NULL,
    author_id BIGINT NOT NULL REFERENCES users(id),
    status VARCHAR(32) NOT NULL DEFAULT 'active'
        CHECK (status IN ('active', 'completed', 'cancelled')),
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE tasks (
    id BIGSERIAL PRIMARY KEY,
    goal_id BIGINT NOT NULL REFERENCES goals(id) ON DELETE CASCADE,
    title VARCHAR(255) NOT NULL,
    description TEXT NOT NULL,
    assignee_id BIGINT NOT NULL REFERENCES users(id),
    author_id BIGINT NOT NULL REFERENCES users(id),
    status VARCHAR(32) NOT NULL DEFAULT 'new'
        CHECK (status IN ('new', 'in_progress', 'done', 'cancelled')),
    due_date DATE,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_goals_author_id ON goals(author_id);
CREATE INDEX idx_tasks_goal_id ON tasks(goal_id);
CREATE INDEX idx_tasks_assignee_id ON tasks(assignee_id);
CREATE INDEX idx_tasks_author_id ON tasks(author_id);

CREATE INDEX idx_users_first_name_lower ON users(LOWER(first_name));
CREATE INDEX idx_users_last_name_lower ON users(LOWER(last_name));

CREATE INDEX idx_tasks_status ON tasks(status);
CREATE INDEX idx_goals_status ON goals(status);
CREATE INDEX idx_tasks_goal_id_status ON tasks(goal_id, status);
