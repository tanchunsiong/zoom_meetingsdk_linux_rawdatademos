# Terraform Deployment Test Log

## 2026-06-09 - Initial AWS Plan

Target:

```text
AWS profile: <aws-profile>
AWS region: us-east-1
AWS account: <aws-account-id>
Project name: zoom-loadtest-test
Maximum runner tasks: 10
Runner size: 0.25 vCPU / 0.5GB
```

Completed successfully:

- AWS credentials authenticated with `sts:GetCallerIdentity`.
- AWS CLI v2, Terraform, and Docker were available on the deployment machine.
- `terraform init` completed.
- `terraform validate` completed.
- Node syntax checks completed for the ECS manager.
- Secret scan found no committed AWS, Zoom, or Docker credentials.
- Terraform parsed a plan containing 27 additions, 0 changes, and 0 destroys.

Blocked:

```text
ec2:DescribeAvailabilityZones
```

AWS returned `UnauthorizedOperation` for the deployment IAM principal.
No AWS resources were created and no Fargate tasks were launched.

The deployment identity also could not call:

```text
iam:GetUser
iam:ListAttachedUserPolicies
iam:ListUserPolicies
servicequotas:GetServiceQuota
```

These denials indicate the deployment identity has not yet received the broad
temporary permissions needed to validate the complete disposable stack.

## Local Fixes From Review

- Added a manager limit of 10 pending or running Fargate tasks.
- Serialized manager Lambda requests to avoid concurrent launch races.
- Enabled ECR `force_delete` so pushed test images do not block destruction.
- Enabled S3 `force_destroy` so uploaded UI files do not block destruction.
- Set the CloudFront minimum viewer protocol to TLS 1.2 (2021 policy).

Known incomplete component:

- Terraform currently deploys a placeholder manager Lambda. The complete Node
  manager still needs Lambda packaging/adaptation before the hosted control
  plane can launch tasks.

## 2026-06-09 - First Apply

The first apply partially created the stack and exposed:

- Lambda rejected the explicitly configured `AWS_REGION` environment variable
  because Lambda reserves it. The template now relies on Lambda's automatically
  supplied region variable.
- CloudFront returned `AccessDenied` because the AWS account must be verified
  before creating CloudFront resources. CloudFront is now optional and disabled
  in the local test tfvars; direct API Gateway testing can proceed.
- Lambda rejected reserved concurrency of one because the account's Lambda quota
  would fall below AWS's required unreserved concurrency minimum of five. Manager
  reserved concurrency is now configurable and set to `-1` for this test. A
  DynamoDB atomic launch lock is still needed to make the 10-task ceiling
  race-proof when requests are not serialized.

## 2026-06-09 - Successful Reconciliation And Smoke Test

After applying the local fixes:

- Terraform apply completed successfully.
- A post-apply Terraform plan reported no changes.
- API Gateway invoked the placeholder Lambda successfully at both `/` and
  `/api/status`.
- Lambda was active and received `ECS_MAX_TASKS=10`.
- The task definition used Fargate, `awsvpc`, `256` CPU units, and `512` MiB.
- The task definition had no CloudWatch log configuration.
- ECS initially had no pending or running tasks.
- CloudFront remained disabled for this unverified AWS account.

The real local runner image was about `18.7 GB`, so it was not pushed merely for
an infrastructure smoke test. A small Alpine image was pushed temporarily to the
disposable ECR repository and used for one short Fargate task.

The smoke task:

- Was accepted without ECS failures.
- Reached `RUNNING`.
- Received an ENI and private IPv4 address in a created public subnet.
- Used `256` CPU units and `512` MiB.
- Exited intentionally with code `0`.

Remaining end-to-end gaps:

- Replace the Alpine smoke image with a reviewed Meeting SDK runner image.
- Package/adapt the real manager API for Lambda instead of the placeholder.
- Add an atomic DynamoDB launch lock if Lambda requests cannot be serialized.
- Verify the actual Meeting SDK join/start path and Zoom API/RTMS configuration.

## 2026-06-09 - Successful Destroy

The saved destroy plan contained 33 managed resources. `terraform apply` of that
destroy plan completed successfully:

```text
Resources: 0 added, 0 changed, 33 destroyed
```

The ECR repository containing the temporary smoke image and the S3 UI bucket
were deleted successfully, confirming `force_delete` and `force_destroy` work
for this disposable stack.

A final fresh plan against the corrected, empty stack completed successfully:

```text
Plan: 33 to add, 0 to change, 0 to destroy
```

## 2026-06-09 - CloudFront Verification Retry

CloudFront was enabled again after an AWS Support request. AWS accepted creation
of the CloudFront origin access control, but rejected the actual distribution
with the same account-level error:

```text
AccessDenied: Your account must be verified before you can add new CloudFront resources.
```

The AWS account is therefore still not enabled for new CloudFront distributions.
The partially created test stack was destroyed after this retry.

## Next Run

Temporarily attach AWS managed `AdministratorAccess` to the deployment identity,
then verify:

```bash
AWS_PROFILE=<aws-profile> AWS_REGION=us-east-1 aws ec2 describe-availability-zones
AWS_PROFILE=<aws-profile> AWS_REGION=us-east-1 terraform plan
```

Review the plan before running:

```bash
AWS_PROFILE=<aws-profile> AWS_REGION=us-east-1 terraform apply
```

After validation, run `terraform destroy` and replace `AdministratorAccess` with
a restricted deployment policy.

## 2026-06-10 - Actual Manager Deployment

Target account `<aws-account-id>`, region `us-east-1`, using an AWS SSO
administrator role.

Completed:

- Applied the base stack with 33 additions and no destroys.
- Enabled CloudFront with an incremental creation-only apply.
- Made Terraform upload the real static UI to private S3.
- Replaced the placeholder Lambda with the actual Express manager using
  `serverless-http`.
- Moved Lambda runtime Zoom configuration to SSM Parameter Store.
- Moved managed custCreate user records to the existing DynamoDB status table.
- Confirmed CloudFront serves HTML, CSS, and JavaScript with correct MIME types.
- Confirmed API Gateway and CloudFront `/api/status` invoke the real manager.
- Confirmed `/api/env` reads the generated ECS configuration and ignores
  `REPLACE_ME` SSM placeholders.
- Confirmed the manager sees the ECS cluster with no running tasks.
- Pushed the actual Meeting SDK runner image to ECR as `latest`.
- Started one credential-free Fargate smoke task using the actual image.
- Confirmed the hosted manager listed the pending/stopped smoke task and tags.
- Confirmed Fargate pulled and started the image successfully; the task exited
  with code `64` because no meeting parameters were intentionally supplied.
- Confirmed a final Terraform plan reported no changes.

Issues found and fed back into Terraform/code:

- The original Terraform deployed only a placeholder Lambda and did not upload
  UI files. Terraform now packages the real manager and manages the UI objects.
- Express cannot listen on a port in Lambda. The manager now exports the app and
  uses a Lambda handler adapter.
- Lambda cannot write `.env` or local managed-user JSON reliably. Lambda uses
  SSM and DynamoDB instead.
- `ssm:GetParametersByPath` requires permission on both the prefix ARN and its
  descendants. The Lambda policy now includes both.
- CloudFront ignores `minimum_protocol_version` when using its default
  certificate, causing perpetual Terraform drift. The unsupported setting was
  removed.
- The local runner is about `18.7 GB` expanded and includes a single very large
  media layer. ECR reports about `1.04 GB` compressed, but the first Fargate pull
  took about `2m38s`. This materially limits burst-start speed across regions.

Remaining validation:

- Populate the SSM Zoom credentials through the hosted UI.
- Run a controlled single-task join/start meeting test.
- Add authentication before treating the public CloudFront manager as anything
  beyond a temporary test deployment. API Gateway routes currently use
  `authorization_type = NONE`.
- RTMS command confirmation is still held in Lambda process memory and may be
  lost on a cold start; persist it in DynamoDB before relying on it operationally.

## Temporary Manager Authentication

The hosted manager now verifies HTTP Basic Authentication server-side for every
management API route. The RTMS webhook endpoint remains unauthenticated so Zoom
can deliver webhook events. Test defaults are `admin` / `admin`; these are
intentionally weak and must be changed before entering real Zoom credentials.
CloudFront forwards the `Authorization` header only for `/api/*`. Static UI
objects use `Cache-Control: no-cache` so future UI/auth updates are revalidated.
Terraform reads the hosted username/password from the ignored
`zoom_loadtest_manager/.env`; changing that file followed by `terraform apply`
updates the Lambda environment.
