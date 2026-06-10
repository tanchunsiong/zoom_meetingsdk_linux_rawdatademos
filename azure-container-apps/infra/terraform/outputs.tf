output "resource_group_name" {
  value = azurerm_resource_group.main.name
}

output "acr_login_server" {
  value = azurerm_container_registry.runner.login_server
}

output "docker_registry_username" {
  value = azurerm_container_registry.runner.admin_username
}

output "docker_registry_password" {
  value     = azurerm_container_registry.runner.admin_password
  sensitive = true
}

output "runner_image_target" {
  value = "${azurerm_container_registry.runner.login_server}/zoom-sendraw-loadtest-meeting:latest"
}

output "container_app_job_name" {
  value = azurerm_container_app_job.runner.name
}

output "manager_identity_client_id" {
  value = azurerm_user_assigned_identity.manager.client_id
}

output "manager_key_vault_url" {
  value = azurerm_key_vault.manager_config.vault_uri
}

output "manager_url" {
  value = "https://${azurerm_container_app.manager.ingress[0].fqdn}"
}

output "manager_environment" {
  value = {
    AZURE_SUBSCRIPTION_ID        = data.azurerm_client_config.current.subscription_id
    AZURE_RESOURCE_GROUP         = azurerm_resource_group.main.name
    AZURE_CONTAINER_APP_JOB_NAME = azurerm_container_app_job.runner.name
    AZURE_CONTAINER_NAME         = "zoom-sendraw-loadtest-meeting"
    AZURE_RUNNER_IMAGE           = "${azurerm_container_registry.runner.login_server}/zoom-sendraw-loadtest-meeting:latest"
    AZURE_MAX_EXECUTIONS         = var.max_runner_executions
    AZURE_PROJECT                = var.project_name
    MANAGER_URL                  = "https://${azurerm_container_app.manager.ingress[0].fqdn}"
    DOCKER_REGISTRY_URL          = azurerm_container_registry.runner.login_server
    DOCKER_REGISTRY_USERNAME     = azurerm_container_registry.runner.admin_username
    DOCKER_REGISTRY_PASSWORD     = azurerm_container_registry.runner.admin_password
    DOCKER_IMAGE                 = "${azurerm_container_registry.runner.login_server}/zoom-sendraw-loadtest-meeting:latest"
  }
  sensitive = true
}

output "manager_image_target" {
  value = "${azurerm_container_registry.runner.login_server}/zoom-loadtest-manager:latest"
}
