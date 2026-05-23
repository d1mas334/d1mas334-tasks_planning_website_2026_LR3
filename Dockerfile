FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest AS build

WORKDIR /app
COPY . .

RUN cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --target task_planning_api

FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest

WORKDIR /app
COPY --from=build /app/build/task_planning_api /app/task_planning_api
COPY configs/static_config.yaml /app/configs/static_config.yaml

EXPOSE 8080

CMD ["/app/task_planning_api", "-c", "/app/configs/static_config.yaml"]
