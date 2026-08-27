# Zoom Load Test Manager

Node/Express management website for launching the unified load-test image as
one-off ECS Fargate tasks:

- `<account-id>.dkr.ecr.<region>.amazonaws.com/zoom-sendraw-loadtest-meeting:latest`

It uses Zoom Server-to-Server OAuth for REST API control-plane actions and the
AWS ECS API to start, stop, and inspect Fargate runner tasks. The browser UI is
optimized for load testing: each custCreate user row has inline resolve, start,
join, delete, and instance-count controls.

## What It Does

- Edit operational environment settings from the browser page; hosted Lambda saves writable values to SSM Parameter Store.
- Auto-fill fake email, first name, last name, and a Licensed user type for `custCreate` user creation.
- List selectable custCreate candidates from manager-created users, fake-domain users, and Zoom API/login type data when available.
- Resolve the selected user's PMI and PMI passcode from `GET /users/{userId}` and `GET /users/{userId}/settings`.
- Schedule a fallback meeting just before launch if Zoom does not expose an instant/PMI meeting ID for that user.
- Delete a custCreate user from Zoom with `DELETE /users/{userId}?action=delete` and remove it from the manager's DynamoDB-backed user list.
- Get a user's ZAK just in time when starting a meeting as host.
- Start or stop RTMS inline for running Fargate tasks with `PATCH /live_meetings/{meetingId}/rtms_app/status`.
- Track RTMS command state per container and confirm actual started/stopped state from Zoom RTMS webhooks when configured.
- Fetch Meeting SDK JWT/signature from the configured `MEETING_TOKEN_ENDPOINT`.
- Start join tasks from the selected user's instant meeting ID/passcode and runtime JWT with `MEETING_MODE=join`.
- Start host tasks from the selected user's instant meeting ID, runtime JWT, and just-in-time ZAK with `MEETING_MODE=start`.
- Show ongoing ECS tasks with meeting/user tags, then stop one task, join tasks, start tasks, or all load-test tasks.

The RTMS start request body is:

```json
{
  "action": "start",
  "settings": {
    "client_id": "<rtms-app-client-id>",
    "participant_user_id": "ZOOM_HOST_OR_ALTERNATIVE_HOST_USER_ID"
  }
}
```

The manager fills `participant_user_id` from the selected custCreate user or the
ECS task tag `zoom-loadtest.user-id`. If this field is omitted, Zoom falls
back to the OAuth token user, which can return error `2308` when that user is
not the meeting host or an alternative host.

## Setup

```bash
cd zoom_loadtest_manager
npm install
cp .env.example .env
```

Edit `.env`, or start the app and edit these values from the Environment card:

```dotenv
MANAGER_AUTH_USERNAME=admin
MANAGER_AUTH_PASSWORD=change-me
```

These credentials are verified server-side for management API requests. For the
hosted Lambda deployment, Terraform reads them from this ignored `.env`; run
`terraform apply` after changing them.

```bash
ZOOM_ACCOUNT_ID=
ZOOM_CLIENT_ID=
ZOOM_CLIENT_SECRET=

AWS_REGION=us-east-1
ECS_CLUSTER=
ECS_TASK_DEFINITION=
ECS_CONTAINER_NAME=zoom-sendraw-loadtest-meeting
ECS_SUBNETS=subnet-a,subnet-b
ECS_SECURITY_GROUPS=sg-runner
```

Do not commit `.env`. It is intentionally ignored.

In the hosted Lambda deployment, the Environment card cannot write a local file,
so editable settings are persisted under the configured SSM parameter prefix and
loaded on Lambda cold start. Terraform-owned infrastructure wiring remains
read-only, but these operational values are editable from the page:

```bash
CUSTCREATE_EMAIL_DOMAIN=
ECS_TASK_CPU=
ECS_TASK_MEMORY=
ECS_MAX_TASKS=
DOCKER_REGISTRY_URL=
DOCKER_REGISTRY_USERNAME=
DOCKER_REGISTRY_PASSWORD=
DOCKER_IMAGE=
DOCKER_SHM_SIZE=
DOCKER_CPU_MIN=
DOCKER_CPU_MAX=
DOCKER_MEMORY_MIN=
DOCKER_MEMORY_MAX=
```

The `DOCKER_*` values are retained as compatibility/display settings. ECS runner
image, CPU, memory, networking, and task definition are controlled by the ECS and
Terraform settings, not by local Docker flags.

For webhook-confirmed RTMS status, add the Zoom webhook secret token and set the
Zoom app webhook URL to this manager endpoint:

```bash
ZOOM_WEBHOOK_SECRET_TOKEN=
```

```text
https://YOUR_MANAGER_HOST/api/zoom/rtms/webhook
```

Zoom validates the endpoint with `ZOOM_WEBHOOK_SECRET_TOKEN`. Subscribe the app
to `meeting.rtms_started` and `meeting.rtms_stopped`. The UI schedules a delayed
status check 60 seconds after `Start RTMS`; without webhook delivery it can only
show that the REST start command was accepted, not that the RTMS stream is live.

## Run

```bash
npm start
```

Open:

```text
http://localhost:3090
```

## Required Zoom Scopes

Exact scope names depend on the app's scope model, but the manager uses these
operation families:

- Users: create/read users and read user token/ZAK.
- Meetings: read user PMI/settings and optionally create/read meetings through the retained API endpoints.
- RTMS: update participant RTMS app status.

Relevant granular scopes may include:

- `user:read:token` or `user:read:zak`
- `user:write:user` or corresponding admin user-create scope
- `meeting:write:meeting` / `meeting:read:meeting`
- `meeting:update:participant_rtms_app_status`

Verify exact scopes in the Zoom Marketplace/API Hub for the app type you use.

## Runtime Notes

The runner images themselves do not fetch Meeting SDK JWTs. This manager fetches
the JWT/signature from `MEETING_TOKEN_ENDPOINT` and passes it to the containers
as `JWT_TOKEN`.

The inline launcher does not ask for meeting number, passcode, JWT, or ZAK. It
uses the custCreate user from that row:

- Join mode: resolves PMI/passcode or creates a scheduled fallback meeting, fetches Meeting SDK JWT with role `0`, and starts the unified image with `MEETING_MODE=join`.
- Start mode: resolves PMI or creates a scheduled fallback meeting, fetches Meeting SDK JWT with role `1`, fetches the selected user's ZAK, and starts the unified image with `MEETING_MODE=start`.

Fargate task-size defaults are:

- CPU: `ECS_TASK_CPU=256`, equivalent to `0.25 vCPU`.
- Memory: `ECS_TASK_MEMORY=512`, equivalent to `0.5GB`.
- Maximum concurrent tasks: `ECS_MAX_TASKS=10`.

The ECS task definition should be registered with the same Fargate-compatible
size: `cpu=256`, `memory=512`. The manager also passes these values as task
overrides, but the task definition must already be compatible with Fargate.
The manager rejects launches that would exceed `ECS_MAX_TASKS`, counting both
pending and running tasks. Set Terraform `manager_reserved_concurrency = 1` to
serialize manager requests. Accounts with low Lambda concurrency quotas can use
`-1`, but then a narrow concurrent-launch race remains. Direct ECS API calls
outside the manager are not covered by this limit.

Zoom exposes the user's `type` as the plan type. The manager still shows it, but
it does not rely on plan type alone to identify custCreate users. It treats
manager-created users, users matching `CUSTCREATE_EMAIL_DOMAIN`, and API/login
type matches from Zoom as selectable custCreate candidates.

For join tasks:

```bash
MEETING_MODE=join
MEETING_NUMBER=1234567890
MEETING_PASSWORD=passcode-if-required
JWT_TOKEN=meeting-sdk-jwt-or-signature
```

For start-meeting tasks:

```bash
MEETING_MODE=start
MEETING_NUMBER=1234567890
JWT_TOKEN=meeting-sdk-jwt-or-signature
USER_ZAK=host-zak-token
```

The Terraform template under `../infra/terraform` creates the ECS cluster, task
definition, ECR repository, API Gateway, Lambda manager, CloudFront/S3 UI,
DynamoDB manager-user table, SSM SecureString placeholders, disposable VPC, public
subnets, and IAM roles. It intentionally does not create CloudWatch log groups,
NAT Gateway, ALB, Secrets Manager, or an always-on ECS service.

The app invokes ECS APIs. The IAM role running the manager needs permission for
`ecs:RunTask`, `ecs:StopTask`, `ecs:ListTasks`, `ecs:DescribeTasks`, and
`iam:PassRole` for the ECS task execution/task roles.
