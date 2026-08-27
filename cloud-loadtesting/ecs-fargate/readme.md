# Zoom Meeting SDK Load Test - ECS Fargate

This is the AWS-specific load-testing copy. It keeps only the unified Meeting
SDK runner, the manager UI/API, and Terraform for the AWS control plane.

## AWS Architecture

```text
Operator
  |
  v
CloudFront static UI
  |
  v
API Gateway -> Lambda manager
  |             |-- Zoom APIs: users, meetings, ZAK, RTMS
  |             |-- SSM parameters for runtime config
  |             `-- ECS RunTask / StopTask
  |
  `-- ECS Fargate one-off runner tasks (0..N)
        |
        `-- outbound connection to Zoom meeting
```

The runner scales to zero because there is no always-on ECS service. The manager
launches Fargate tasks only when load-test instances are requested.

## Folders

- `zoom_sendraw_loadtest-meeting`: unified start/join Meeting SDK runner image.
- `zoom_loadtest_manager`: Lambda-compatible management API and static UI.
- `infra/terraform`: minimal AWS infrastructure for the hosted manager and runner.

## First Deployment

1. Install AWS CLI v2, Terraform `>= 1.6.0`, and Docker.
2. Configure AWS credentials for the target account and region.
3. Apply `infra/terraform` to create the AWS control-plane resources.
4. Build and push the runner image to the Terraform-created ECR repository.
5. Configure Zoom and token-service settings through SSM parameters or the manager.
6. Open `terraform output -raw cloudfront_url` or `terraform output -raw api_gateway_url`.

See [infra/terraform/readme.md](infra/terraform/readme.md) for detailed commands.

## Defaults

- Runner size: `0.25 vCPU / 0.5GB RAM`
- Default task cap: `10`
- Resource grouping: optional AWS Resource Groups group from `aws_resource_group_name`
- Excluded by design: CloudWatch log groups, NAT Gateway, ALB, Secrets Manager, and always-on ECS services

## Sensitive Files

Do not commit local credentials, generated media, SDK binaries, Terraform state,
dependency folders, or local key material. The repo `.gitignore` excludes local
env files, Terraform state/cache, media outputs, node modules, and key/cert files.
