workspace "Task Planning System" "Лабораторная работа 01. Вариант 10 — Планирование задач" {
    !identifiers hierarchical

    model {
        user = person "Исполнитель" "Пользователь системы, который создает личные цели, просматривает доступные задачи и изменяет статус задач, назначенных ему." "User"
        manager = person "Менеджер" "Пользователь, который создает цели, создает задачи, назначает исполнителей и контролирует выполнение задач." "User"
        admin = person "Администратор" "Технический пользователь для сопровождения системы и управления учетными записями." "Administrator"

        emailSystem = softwareSystem "Email-сервер" "Внешняя система для отправки email-уведомлений пользователям." "External System"
        smsSystem = softwareSystem "SMS-сервер" "Внешняя система для отправки срочных SMS-уведомлений исполнителям." "External System"

        taskPlanningSystem = softwareSystem "Система планирования задач" "Веб-приложение для планирования целей, создания задач, назначения исполнителей и контроля статусов." {
            webApp = container "Web Application" "Клиентское веб-приложение для работы с пользователями, целями и задачами." "React, TypeScript"

            apiApp = container "Task Planning API" "Основной REST API сервис. Содержит логические модули пользователей, целей, задач, аутентификации и формирования событий уведомлений." "C++ / Yandex Userver, REST API"

            notificationWorker = container "Notification Worker" "Фоновый обработчик событий задач и отправки уведомлений пользователям." "C++ / Yandex Userver Worker"

            database = container "PostgreSQL Database" "Хранит пользователей, цели, задачи, статусы и права доступа." "PostgreSQL" "Database"
        }

        user -> taskPlanningSystem.webApp "Работает с целями и своими задачами" "HTTPS"
        manager -> taskPlanningSystem.webApp "Создает цели, задачи и назначает исполнителей" "HTTPS"
        admin -> taskPlanningSystem.webApp "Управляет пользователями и выполняет поддержку" "HTTPS"

        taskPlanningSystem.webApp -> taskPlanningSystem.apiApp "Вызывает API системы" "HTTPS/REST"

        taskPlanningSystem.apiApp -> taskPlanningSystem.database "Читает и записывает пользователей, цели, задачи и статусы" "PostgreSQL protocol"
        taskPlanningSystem.apiApp -> taskPlanningSystem.notificationWorker "Передает событие о создании или изменении задачи" "Async message / HTTP"

        taskPlanningSystem.notificationWorker -> emailSystem "Отправляет email-уведомления" "SMTP"
        taskPlanningSystem.notificationWorker -> smsSystem "Отправляет SMS-уведомления" "HTTPS/API"
    }

    views {
        systemContext taskPlanningSystem "SystemContext" "Контекст системы" {
            include *
            autoLayout
            title "C1 — System Context: система планирования задач"
            description "Контекстная диаграмма показывает пользователей системы и внешние системы уведомлений."
        }

        container taskPlanningSystem "Containers" "Контейнеры системы планирования задач" {
            include *
            autoLayout
            title "C2 — Container: контейнеры системы планирования задач"
            description "Контейнерная диаграмма показывает веб-приложение, REST API на Yandex Userver, PostgreSQL и фоновый обработчик уведомлений."
        }

        dynamic taskPlanningSystem "CreateTaskDynamic" "Диаграмма последовательности: создание новой задачи на пути к цели" {
            manager -> taskPlanningSystem.webApp "Заполняет форму создания задачи"
            taskPlanningSystem.webApp -> taskPlanningSystem.apiApp "POST /goals/{goalId}/tasks"
            taskPlanningSystem.apiApp -> taskPlanningSystem.database "Проверяет цель, исполнителя и права доступа"
            taskPlanningSystem.apiApp -> taskPlanningSystem.database "Сохраняет новую задачу"
            taskPlanningSystem.apiApp -> taskPlanningSystem.notificationWorker "Передает событие TaskCreated"
            taskPlanningSystem.notificationWorker -> emailSystem "Отправляет email-уведомление исполнителю"
            taskPlanningSystem.notificationWorker -> smsSystem "Отправляет SMS-уведомление для важной задачи"
            autoLayout
            title "Dynamic — создание задачи в цели"
            description "Диаграмма показывает последовательность взаимодействия контейнеров при создании задачи и уведомлении исполнителя."
        }

        styles {
            element "User" {
                shape person
                background #08427b
                color #ffffff
            }
            element "Administrator" {
                shape person
                background #5f021f
                color #ffffff
            }
            element "External System" {
                background #999999
                color #ffffff
            }
            element "Database" {
                shape cylinder
                background #1168bd
                color #ffffff
            }
            element "Container" {
                background #438dd5
                color #ffffff
            }
            element "Software System" {
                background #1168bd
                color #ffffff
            }
            relationship "Relationship" {
                thickness 4
            }
        }
    }
}
