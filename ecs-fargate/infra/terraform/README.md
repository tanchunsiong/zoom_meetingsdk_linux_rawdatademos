# ECS Fargate Terraform

Minimal Terraform for the scale-to-zero load-test shape:

- S3 bucket for static UI files.
- CloudFront distribution with `/api/*` routed to API Gateway.
- API Gateway HTTP API to invoke the manager Lambda.
- Lambda manager entry point with a placeholder package.
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

## Fargate Size

The default runner task size is:

```text
runner_cpu    = 256
runner_memory = 512
```

That maps to `0.25 vCPU / 0.5GB`.

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

Upload the static UI to the output `ui_bucket`.
Push the runner image to the output `runner_ecr_repository_url` with tag
`latest`.

The first apply deploys a placeholder Lambda response. Replace it later with
the packaged manager API.

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

This template does not package the real manager Lambda code or push Docker
images.
