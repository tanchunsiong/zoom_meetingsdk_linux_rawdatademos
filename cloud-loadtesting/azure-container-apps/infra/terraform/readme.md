# Azure Terraform

This template creates the minimal Azure foundation for the hosted Zoom load-test
manager and Meeting SDK runner:

- One resource group for simple cleanup
- Azure Container Registry Basic
- Azure Container Apps Environment and a manually triggered Container Apps Job
- Hosted Azure Container App for the manager website
- Azure Key Vault for manager environment persistence across deployments
- User-assigned identities for ACR pull and manager access

Log Analytics, Azure Monitor alerts, Front Door, private networking, and an
always-on manager replica are intentionally excluded from this disposable stack.

The runner scales to zero because the job runs only when explicitly triggered.
The hosted manager uses `0.25` vCPU / `0.5Gi`, `min_replicas = 0`, and
`max_replicas = 1`. It enforces a maximum of 10 active executions by default.

## Prerequisites

- Azure CLI authenticated with `az login`
- Terraform `>= 1.6`
- An Azure subscription selected with `az account set --subscription ...`
- Docker only when building and pushing the runner image

## Apply

```bash
cp terraform.tfvars.example terraform.tfvars
terraform init
terraform plan
terraform apply
```

The first apply uses public placeholder images so ACR can be created before the
real runner and manager images exist. Build and push both images, set
`runner_image` and `manager_image` in `terraform.tfvars` to their ACR output
targets, and apply again.

```bash
az acr login --name "$(terraform output -raw acr_login_server | cut -d. -f1)"
docker build -t "$(terraform output -raw runner_image_target)" ../../zoom_sendraw_loadtest-meeting
docker push "$(terraform output -raw runner_image_target)"

docker build -t "$(terraform output -raw manager_image_target)" ../../zoom_loadtest_manager
docker push "$(terraform output -raw manager_image_target)"
terraform apply
```

Set `runner_image` and `manager_image` to their ACR targets before the second
apply. The hosted manager uses its provisioned managed identity to control the
runner Job and read/write one Key Vault secret named `manager-environment`.
The secret is not created until environment values are first saved from the
manager UI, so a new deployment starts with blank/default editable fields.
Subsequent manager revisions load those saved values from Key Vault.

Terraform also injects these read-only manager fields:

- `DOCKER_REGISTRY_URL`
- `DOCKER_REGISTRY_USERNAME`
- `DOCKER_REGISTRY_PASSWORD`
- `DOCKER_IMAGE`

Use `terraform output -raw manager_url` for the hosted webpage URL. The registry
password output and combined `manager_environment` output are sensitive. Use
`terraform output -raw manager_key_vault_url` to inspect the persistence vault.

## Cleanup

```bash
terraform destroy
```

Deleting the generated resource group in the Azure portal is also a complete
cleanup path. Do not place unrelated resources in that resource group.
