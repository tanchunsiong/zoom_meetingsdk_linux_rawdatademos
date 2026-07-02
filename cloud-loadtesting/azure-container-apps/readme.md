# Zoom Meeting SDK Linux Load Test - Azure Container Apps

This is the Azure-specific copy of the raw-data load-test project. It keeps the
unified Meeting SDK runner and replaces ECS Fargate with manually triggered
Azure Container Apps Jobs.

## Azure Architecture

```text
Operator
  |
  v
Hosted Manager Container App
  |-- Zoom APIs: users, meetings, ZAK, RTMS
  |
  `-- Azure Resource Manager
        |
        `-- Container Apps Job executions (0..N)
              |
              |-- pull runner image from ACR using managed identity
              `-- outbound connection to Zoom meeting

Resource group
  |-- Azure Container Registry Basic
  |-- Container Apps Environment
  |-- hosted manager Container App
  |-- manual runner Job
  |-- managed identities and narrow role assignments
  `-- no Log Analytics or always-on compute
```

Container Apps Jobs are the closest Azure equivalent to one-off ECS Fargate
tasks: executions are started on demand, can receive per-run environment
overrides, and scale back to zero after they finish. The runners need outbound
internet access but do not need public inbound IP addresses.

The hosted manager uses external HTTPS ingress, `0.25` vCPU / `0.5Gi`, and a
maximum of one replica. Platform-managed Azure and ACR fields are read-only in
the manager UI. Stopped job executions remain in Azure history but are hidden
from the manager container list.

The resource group is the Azure cleanup boundary. Terraform tags all resources
with `Project`, `Stack`, and `ManagedBy`; no personal owner name is included.

## Folders

- `zoom_sendraw_loadtest-meeting`: unified start/join Meeting SDK runner image
- `zoom_loadtest_manager`: Azure ARM-enabled management website
- `infra/terraform`: minimal Azure runner infrastructure

## First Deployment

1. Install Azure CLI, Terraform `>= 1.6`, and Docker.
2. Run `az login` and select a subscription.
3. Apply `infra/terraform` with the public placeholder image.
4. Build and push the runner to the created ACR.
5. Set the ACR runner image in Terraform and apply again.
6. Build and push the manager image, set `manager_image`, and apply again.
7. Open `terraform output -raw manager_url`.

See [infra/terraform/README.md](infra/terraform/README.md) and
[zoom_loadtest_manager/README.md](zoom_loadtest_manager/README.md).

Terraform populates the generated ACR URL, username, password, and runner image
for the hosted manager. It does not populate Zoom credentials.
