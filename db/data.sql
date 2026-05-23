INSERT INTO users (login, password_hash, first_name, last_name, email, phone, role)
VALUES
    ('alexey', 'pass123', 'Alexey', 'Sokolov', 'alexey.sokolov@example.com', '+79000000001', 'manager'),
    ('maria', 'pass123', 'Maria', 'Orlova', 'maria.orlova@example.com', '+79000000002', 'worker'),
    ('dmitry', 'pass123', 'Dmitry', 'Kuznetsov', 'dmitry.kuznetsov@example.com', '+79000000003', 'worker'),
    ('olga', 'pass123', 'Olga', 'Smirnova', 'olga.smirnova@example.com', '+79000000004', 'manager'),
    ('sergey', 'pass123', 'Sergey', 'Volkov', 'sergey.volkov@example.com', '+79000000005', 'worker'),
    ('anna', 'pass123', 'Anna', 'Petrova', 'anna.petrova@example.com', '+79000000006', 'admin'),
    ('pavel', 'pass123', 'Pavel', 'Ivanov', 'pavel.ivanov@example.com', '+79000000007', 'worker'),
    ('elena', 'pass123', 'Elena', 'Morozova', 'elena.morozova@example.com', '+79000000008', 'manager'),
    ('nikita', 'pass123', 'Nikita', 'Lebedev', 'nikita.lebedev@example.com', '+79000000009', 'worker'),
    ('irina', 'pass123', 'Irina', 'Fedorova', 'irina.fedorova@example.com', '+79000000010', 'worker');

INSERT INTO goals (title, description, author_id, status)
VALUES
    ('Launch task planning module', 'Prepare the core task planning workflow for the team.', 1, 'active'),
    ('Improve onboarding', 'Collect and organize onboarding tasks for new employees.', 4, 'active'),
    ('Prepare quarterly report', 'Plan report preparation and review steps.', 8, 'active'),
    ('Migrate documentation', 'Move internal process documents to the new workspace.', 6, 'active'),
    ('Release mobile backlog', 'Prioritize mobile application backlog items.', 1, 'active'),
    ('Optimize support process', 'Reduce time for resolving frequent support requests.', 4, 'active'),
    ('Design analytics dashboard', 'Define tasks for the first dashboard version.', 8, 'active'),
    ('Plan security audit', 'Prepare checklist and responsibilities for security audit.', 6, 'active'),
    ('Update API contracts', 'Synchronize task planning API contracts with clients.', 1, 'completed'),
    ('Archive obsolete goals', 'Review and close outdated planning goals.', 6, 'cancelled');

INSERT INTO tasks (
    goal_id, title, description, assignee_id, author_id, status, due_date
)
VALUES
    (1, 'Describe task statuses', 'Document allowed task statuses and transitions.', 2, 1, 'done', '2026-06-01'),
    (1, 'Implement goal task endpoint', 'Create endpoint for adding tasks to a goal.', 3, 1, 'in_progress', '2026-06-05'),
    (2, 'Create onboarding checklist', 'Prepare checklist for the first working week.', 5, 4, 'new', '2026-06-10'),
    (3, 'Collect source metrics', 'Gather source metrics for the quarterly report.', 7, 8, 'new', '2026-06-12'),
    (4, 'Inventory documents', 'List documents that should be migrated.', 9, 6, 'done', '2026-06-03'),
    (5, 'Rank mobile issues', 'Sort mobile backlog issues by priority.', 10, 1, 'in_progress', '2026-06-15'),
    (6, 'Analyze support tickets', 'Find repeated support requests from the last month.', 2, 4, 'new', '2026-06-18'),
    (7, 'Draft dashboard layout', 'Prepare the first analytics dashboard layout.', 3, 8, 'new', '2026-06-20'),
    (8, 'Prepare audit checklist', 'Create checklist for security audit preparation.', 5, 6, 'new', '2026-06-22'),
    (9, 'Review API examples', 'Check request and response examples in API contracts.', 7, 1, 'done', '2026-05-30');
