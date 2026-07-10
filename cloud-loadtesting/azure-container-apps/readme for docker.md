# Docker notes

Run these commands from `zoom_sendraw_loadtest-meeting/`.

The production runner is an Ubuntu 22.04 image for Azure Container Apps Jobs.
Docker is used on the deployment machine to build and push that image to the
Terraform-created Azure Container Registry.

## Build

```bash
IMAGE="$(terraform -chdir=../infra/terraform output -raw runner_image_target)" ./scripts/build-image.sh
```

The build stages the Meeting SDK from `SDK_SOURCE`, which defaults to the local
Meeting SDK path configured by this Azure copy.

## Push To ACR

```bash
ACR="$(terraform -chdir=../infra/terraform output -raw acr_login_server)"
az acr login --name "${ACR%%.*}"
PUSH_IMAGE=true IMAGE="$(terraform -chdir=../infra/terraform output -raw runner_image_target)" ./scripts/build-image.sh
```

## Local Smoke Test

```bash
MEETING_MODE=join ./scripts/start-loadtest.sh 1 1234567890 "$JWT_TOKEN" "passcode" LoadBot
```

The local helper uses Docker only. Normal Azure runs are launched and stopped as
Container Apps Job executions by `zoom_loadtest_manager`.

## Stop Local Containers

```bash
./scripts/stop-loadtest.sh
```

## Useful Local Commands

```bash
docker images -a
docker ps -a
docker rmi <image-id>
```
