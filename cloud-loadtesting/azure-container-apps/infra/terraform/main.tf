data "azurerm_client_config" "current" {}

resource "random_string" "suffix" {
  length  = 6
  upper   = false
  special = false
}

locals {
  compact_name = replace(var.project_name, "-", "")
  suffix       = random_string.suffix.result
  common_tags = merge(var.tags, {
    Project   = var.project_name
    Stack     = "azure-container-apps"
    ManagedBy = "terraform"
  })
}

resource "azurerm_resource_group" "main" {
  name     = "${var.project_name}-rg"
  location = var.azure_location
  tags     = local.common_tags
}

resource "azurerm_container_registry" "runner" {
  name                = substr("${local.compact_name}${local.suffix}", 0, 50)
  resource_group_name = azurerm_resource_group.main.name
  location            = azurerm_resource_group.main.location
  sku                 = "Basic"
  admin_enabled       = true
  tags                = local.common_tags
}

resource "azurerm_user_assigned_identity" "runner" {
  name                = "${var.project_name}-runner-identity"
  resource_group_name = azurerm_resource_group.main.name
  location            = azurerm_resource_group.main.location
  tags                = local.common_tags
}

resource "azurerm_role_assignment" "runner_acr_pull" {
  scope                = azurerm_container_registry.runner.id
  role_definition_name = "AcrPull"
  principal_id         = azurerm_user_assigned_identity.runner.principal_id
}

resource "azurerm_container_app_environment" "main" {
  name                = "${var.project_name}-environment"
  resource_group_name = azurerm_resource_group.main.name
  location            = azurerm_resource_group.main.location
  tags                = local.common_tags
}

resource "azurerm_container_app_job" "runner" {
  name                         = "${var.project_name}-runner"
  resource_group_name          = azurerm_resource_group.main.name
  location                     = azurerm_resource_group.main.location
  container_app_environment_id = azurerm_container_app_environment.main.id
  replica_timeout_in_seconds   = var.runner_timeout_seconds
  replica_retry_limit          = 0
  tags                         = local.common_tags

  identity {
    type         = "UserAssigned"
    identity_ids = [azurerm_user_assigned_identity.runner.id]
  }

  registry {
    server   = azurerm_container_registry.runner.login_server
    identity = azurerm_user_assigned_identity.runner.id
  }

  manual_trigger_config {
    parallelism              = 1
    replica_completion_count = 1
  }

  template {
    container {
      name   = "zoom-sendraw-loadtest-meeting"
      image  = var.runner_image
      cpu    = var.runner_cpu
      memory = var.runner_memory

      env {
        name  = "USE_JWT_TOKEN_FROM_WEB_SERVICE"
        value = "false"
      }
    }
  }

  depends_on = [azurerm_role_assignment.runner_acr_pull]
}

resource "azurerm_user_assigned_identity" "manager" {
  name                = "${var.project_name}-manager-identity"
  resource_group_name = azurerm_resource_group.main.name
  location            = azurerm_resource_group.main.location
  tags                = local.common_tags
}

resource "azurerm_role_assignment" "manager_job_contributor" {
  scope                = azurerm_container_app_job.runner.id
  role_definition_name = "Contributor"
  principal_id         = azurerm_user_assigned_identity.manager.principal_id
}

resource "azurerm_role_assignment" "manager_acr_pull" {
  scope                = azurerm_container_registry.runner.id
  role_definition_name = "AcrPull"
  principal_id         = azurerm_user_assigned_identity.manager.principal_id
}

resource "azurerm_key_vault" "manager_config" {
  name                       = substr("${local.compact_name}${local.suffix}cfgkv", 0, 24)
  resource_group_name        = azurerm_resource_group.main.name
  location                   = azurerm_resource_group.main.location
  tenant_id                  = data.azurerm_client_config.current.tenant_id
  sku_name                   = "standard"
  rbac_authorization_enabled = true
  purge_protection_enabled   = false
  soft_delete_retention_days = 7
  tags                       = local.common_tags
}

resource "azurerm_role_assignment" "manager_key_vault_secrets" {
  scope                = azurerm_key_vault.manager_config.id
  role_definition_name = "Key Vault Secrets Officer"
  principal_id         = azurerm_user_assigned_identity.manager.principal_id
}

resource "azurerm_container_app" "manager" {
  name                         = "${var.project_name}-manager"
  resource_group_name          = azurerm_resource_group.main.name
  container_app_environment_id = azurerm_container_app_environment.main.id
  revision_mode                = "Single"
  tags                         = local.common_tags

  identity {
    type         = "UserAssigned"
    identity_ids = [azurerm_user_assigned_identity.manager.id]
  }

  registry {
    server   = azurerm_container_registry.runner.login_server
    identity = azurerm_user_assigned_identity.manager.id
  }

  secret {
    name  = "manager-auth-password"
    value = var.manager_auth_password
  }

  secret {
    name  = "acr-admin-password"
    value = azurerm_container_registry.runner.admin_password
  }

  ingress {
    external_enabled = true
    target_port      = 3090
    transport        = "auto"

    traffic_weight {
      latest_revision = true
      percentage      = 100
    }
  }

  template {
    min_replicas = 0
    max_replicas = 1

    container {
      name   = "zoom-loadtest-manager"
      image  = var.manager_image
      cpu    = var.manager_cpu
      memory = var.manager_memory

      env {
        name  = "PORT"
        value = "3090"
      }

      env {
        name  = "HOST"
        value = "0.0.0.0"
      }

      env {
        name  = "AZURE_HOSTED_MANAGER"
        value = "true"
      }

      env {
        name  = "MANAGER_AUTH_USERNAME"
        value = var.manager_auth_username
      }

      env {
        name        = "MANAGER_AUTH_PASSWORD"
        secret_name = "manager-auth-password"
      }

      env {
        name  = "AZURE_CLIENT_ID"
        value = azurerm_user_assigned_identity.manager.client_id
      }

      env {
        name  = "AZURE_KEY_VAULT_URL"
        value = azurerm_key_vault.manager_config.vault_uri
      }

      env {
        name  = "AZURE_SUBSCRIPTION_ID"
        value = data.azurerm_client_config.current.subscription_id
      }

      env {
        name  = "AZURE_RESOURCE_GROUP"
        value = azurerm_resource_group.main.name
      }

      env {
        name  = "AZURE_CONTAINER_APP_JOB_NAME"
        value = azurerm_container_app_job.runner.name
      }

      env {
        name  = "AZURE_CONTAINER_NAME"
        value = "zoom-sendraw-loadtest-meeting"
      }

      env {
        name  = "AZURE_RUNNER_IMAGE"
        value = "${azurerm_container_registry.runner.login_server}/zoom-sendraw-loadtest-meeting:latest"
      }

      env {
        name  = "AZURE_MANAGEMENT_API_VERSION"
        value = "2026-01-01"
      }

      env {
        name  = "AZURE_RUNNER_CPU"
        value = tostring(var.runner_cpu)
      }

      env {
        name  = "AZURE_RUNNER_MEMORY"
        value = var.runner_memory
      }

      env {
        name  = "AZURE_MAX_EXECUTIONS"
        value = tostring(var.max_runner_executions)
      }

      env {
        name  = "AZURE_PROJECT"
        value = var.project_name
      }

      env {
        name  = "DOCKER_REGISTRY_URL"
        value = azurerm_container_registry.runner.login_server
      }

      env {
        name  = "DOCKER_REGISTRY_USERNAME"
        value = azurerm_container_registry.runner.admin_username
      }

      env {
        name        = "DOCKER_REGISTRY_PASSWORD"
        secret_name = "acr-admin-password"
      }

      env {
        name  = "DOCKER_IMAGE"
        value = "${azurerm_container_registry.runner.login_server}/zoom-sendraw-loadtest-meeting:latest"
      }
    }
  }

  depends_on = [
    azurerm_role_assignment.manager_acr_pull,
    azurerm_role_assignment.manager_job_contributor,
    azurerm_role_assignment.manager_key_vault_secrets
  ]
}
