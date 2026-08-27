# Zoom Load Test Manager for Azure

This Node manager keeps the existing Zoom custCreate user, meeting, token, RTMS,
start, join, and stop controls while using manual Azure Container Apps Job
executions for the runner workload.

Terraform hosts the manager as an Azure Container App with external HTTPS
ingress. The browser UI includes server-verified sign-in, popup results for
control-plane actions, and a bulk `Resolve All PMI and Passcode` action.

## Runtime

```bash
cp .env.example .env
npm install
az login
npm start
```

Open `http://localhost:3090`. Locally, `DefaultAzureCredential` uses the Azure
CLI login. When hosted in Azure, assign the Terraform-created manager identity
and set `AZURE_CLIENT_ID` to that identity's client ID.

Required Azure values are available from:

```bash
cd ../infra/terraform
terraform output -json manager_environment
```

Each start/join request starts an independent execution of the same manual
Container Apps Job and overrides its environment variables with the meeting
details and tokens. `AZURE_MAX_EXECUTIONS` limits active manager-controlled
executions. The default is 10.

Hosted Azure, ACR, image, and manager-auth values are injected by Terraform and
are read-only in both the UI and API. Zoom configuration remains editable.
When `AZURE_KEY_VAULT_URL` is set, saving the Environment form writes editable
values to the `manager-environment` Key Vault secret instead of the container
filesystem. The manager loads that secret at startup, preserving values across
redeployments. A missing secret is treated as a normal first run and leaves the
editable values blank or at their documented defaults.

Stopped executions remain in Azure's execution history but are excluded from the
manager container list; only active executions are shown.

The ARM identity needs Contributor permission on the Container Apps Job to start,
list, and stop executions. It also needs Key Vault Secrets Officer on the
persistence vault. The runner identity only receives `AcrPull` on ACR. No Azure
or Zoom secrets belong in `.env.example` or Terraform variables.
