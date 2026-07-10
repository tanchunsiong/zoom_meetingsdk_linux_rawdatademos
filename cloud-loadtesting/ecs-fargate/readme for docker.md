# Docker notes

Run these commands from `zoom_sendraw_loadtest-meeting/`.

The production runner is an Ubuntu 22.04 image for ECS Fargate. Docker is used on
the deployment machine to build and push that image to the Terraform-created ECR
repository.

## Build

```bash
IMAGE="$(terraform -chdir=../infra/terraform output -raw runner_image)" ./scripts/build-image.sh
```

The build stages the Meeting SDK from `SDK_SOURCE`, which defaults to
`/opt/zoom-meeting-sdk` in this AWS copy.

## Push To ECR

```bash
REGISTRY="$(terraform -chdir=../infra/terraform output -raw runner_ecr_repository_url)"
aws ecr get-login-password --region "$AWS_REGION" | docker login --username AWS --password-stdin "${REGISTRY%/*}"
PUSH_IMAGE=true IMAGE="${REGISTRY}:latest" ./scripts/build-image.sh
```

## Local Smoke Test

```bash
MEETING_MODE=join ./scripts/start-loadtest.sh 1 1234567890 "$JWT_TOKEN" "passcode" LoadBot
```

The local helper uses Docker only. Normal AWS runs are launched and stopped as
one-off ECS Fargate tasks by `zoom_loadtest_manager`.

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
