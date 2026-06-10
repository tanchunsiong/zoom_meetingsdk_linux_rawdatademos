# ECS Fargate Terraform

Minimal Terraform for the scale-to-zero load-test shape:

- S3 bucket for static UI files.
- CloudFront distribution with `/api/*` routed to API Gateway.
- API Gateway HTTP API to invoke the manager Lambda.
- Actual Lambda-packaged manager API.
- Real static manager UI uploaded to the private S3 bucket.
- ECS cluster and Fargate task definition for one-off runner tasks.
- ECR repository for the runner image.
- Disposable VPC with public subnets and an internet gateway.
- DynamoDB on-demand status table with TTL.
- Outbound-only runner security group.
- IAM permissions needed for Lambda to run/stop/inspect ECS tasks and read SSM parameters.
- SSM SecureString placeholder parameter names.

Intentionally not included:

- CloudWatch log groups, dashboards, alarms, or ECS log configuration.
- ALB.
- NAT Gateway.
- Secrets Manager.
- Always-on ECS services.
- Autoscaling policies.
- S3 transcript/media storage.
- AWS Budgets.

This is a disposable test stack. Terraform is configured to delete objects from
the UI S3 bucket and images from the ECR repository during `terraform destroy`.
Do not use those resources for data or images that must be retained.

CloudFront is enabled by default. Set `enable_cloudfront = false` when testing
with an AWS account that has not been verified to create CloudFront resources.
The direct API Gateway URL remains available; the private UI S3 bucket will not
be browser-accessible until CloudFront or another frontend host is configured.

## Deployment Machine Prerequisites

Install these tools on the machine that will deploy and test this stack:

- AWS CLI v2, configured with credentials for the target AWS account.
- Terraform `>= 1.6.0`.
- Docker, required to build and push the runner image to ECR.

Verify the tools and AWS identity before running Terraform:

```bash
aws --version
terraform version
docker version
AWS_PROFILE=zoom-loadtest AWS_REGION=us-east-1 aws sts get-caller-identity
```

Do not use AWS root-user access keys. Use a dedicated IAM user or temporary role
credentials with permission to create the resources in this template.

For the first disposable-stack validation, the deployment identity needs broad
permissions across EC2/VPC, IAM, ECS, ECR, Lambda, API Gateway, CloudFront, S3,
DynamoDB, and SSM, including `iam:PassRole`. Attaching AWS managed
`AdministratorAccess` temporarily is the simplest way to validate the template.
Remove it after the test and replace it with a restricted deployment policy.

Run Terraform with the same named profile:

```bash
AWS_PROFILE=zoom-loadtest AWS_REGION=us-east-1 terraform plan
AWS_PROFILE=zoom-loadtest AWS_REGION=us-east-1 terraform apply
```

## Fargate Size

The default runner task size is:

```text
runner_cpu    = 256
runner_memory = 512
```

That maps to `0.25 vCPU / 0.5GB`.

The manager has a hard cost-control ceiling:

```text
max_runner_tasks = 10
```

It rejects a request above 10 and rejects launches that would make the project
exceed 10 pending or running Fargate tasks. Set
`manager_reserved_concurrency = 1` to prevent simultaneous manager requests from
racing past this ceiling. Some new accounts have a Lambda quota too low to
reserve concurrency; use `-1` for those accounts. Without serialization, a
narrow concurrent-launch race remains. Direct `ecs:RunTask` calls made outside
the manager are not covered.

## Setup

```bash
cp terraform.tfvars.example terraform.tfvars
```

At minimum, set:

- `aws_region`
- `project_name`, if you want a different resource prefix

Then:

```bash
terraform init
terraform plan
terraform apply
```

Push the runner image to the output `runner_ecr_repository_url` with tag
`latest`.

Terraform uploads the real browser UI and packages the real manager API for
Lambda. The manager uses SSM Parameter Store for runtime Zoom configuration and
DynamoDB for its custCreate user records.

The hosted management APIs use server-verified HTTP Basic Authentication. Set
the credentials in the ignored manager `.env` before applying:

```bash
cp ../../zoom_loadtest_manager/.env.example ../../zoom_loadtest_manager/.env
```

```dotenv
MANAGER_AUTH_USERNAME=admin
MANAGER_AUTH_PASSWORD=change-me
```

Terraform reads these two values from `zoom_loadtest_manager/.env` and configures
the Lambda environment. Run `terraform apply` after changing them. If the keys
are absent, the temporary test defaults are `admin` / `admin`. The static UI
files remain public, but management data and actions require authentication.
The Zoom RTMS webhook endpoint is exempt because Zoom cannot send these
credentials. Treat the local Terraform state as sensitive because Lambda
environment values are recorded in state.

The same ignored `.env` can set the displayed ECR values:

```dotenv
DOCKER_REGISTRY_URL=<account-id>.dkr.ecr.<region>.amazonaws.com
DOCKER_REGISTRY_USERNAME=AWS
DOCKER_REGISTRY_PASSWORD=
DOCKER_IMAGE=<account-id>.dkr.ecr.<region>.amazonaws.com/zoom-sendraw-loadtest-meeting:latest
```

Leave `DOCKER_REGISTRY_PASSWORD` blank. ECR login passwords expire, and ECS
Fargate pulls the runner using the task execution IAM role.

## Secrets

Do not put real secret values in Terraform variables. Terraform creates
placeholder SecureString parameter names under `ssm_parameter_prefix`, for
example:

```text
/zoom-loadtest/zoom/account-id
/zoom-loadtest/zoom/client-id
/zoom-loadtest/zoom/client-secret
/zoom-loadtest/zoom/rtms-client-id
/zoom-loadtest/zoom/webhook-secret-token
/zoom-loadtest/meeting-token-endpoint
```

The Lambda role can read parameters under that prefix.
Update the placeholder values outside Terraform after apply.

## Notes

This template does not push Docker images. Build and push the runner separately
before starting meeting tasks.

Deployment test results and known blockers are recorded in `TESTING.md`.
