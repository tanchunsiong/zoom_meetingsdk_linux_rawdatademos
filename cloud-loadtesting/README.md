# Cloud Load Testing

This folder isolates the load-testing projects from the upstream Meeting SDK raw-data demos.

- `on-premise/zoom_sendraw_loadtest-meeting`: unified local Docker runner for join/start meeting load tests.
- `on-premise/zoom_loadtest_manager`: local Docker-based manager UI.
- `ecs-fargate`: AWS ECS/Fargate scale-to-zero deployment.
- `azure-container-apps`: Azure Container Apps Jobs scale-to-zero deployment.

Local environment files, Terraform state, dependency folders, SDK binaries, and generated media remain ignored.
