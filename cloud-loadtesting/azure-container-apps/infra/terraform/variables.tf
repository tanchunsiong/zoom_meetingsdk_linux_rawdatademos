variable "azure_location" {
  description = "Azure region for this deployment."
  type        = string
  default     = "eastus"
}

variable "project_name" {
  description = "Lowercase resource name prefix."
  type        = string
  default     = "zoom-loadtest"

  validation {
    condition     = can(regex("^[a-z][a-z0-9-]{2,20}$", var.project_name))
    error_message = "project_name must start with a letter and contain 3-21 lowercase letters, numbers, or hyphens."
  }
}

variable "runner_image" {
  description = "Runner image. Use the public placeholder for the first apply, then push to ACR and set its ACR URL."
  type        = string
  default     = "mcr.microsoft.com/k8se/quickstart-jobs:latest"
}

variable "manager_image" {
  description = "Hosted manager web app image."
  type        = string
  default     = "mcr.microsoft.com/azuredocs/containerapps-helloworld:latest"
}

variable "manager_auth_username" {
  description = "HTTP Basic Auth username for the hosted manager."
  type        = string
  default     = "admin"
}

variable "manager_auth_password" {
  description = "HTTP Basic Auth password for the hosted manager."
  type        = string
  sensitive   = true
  default     = "admin"
}

variable "manager_cpu" {
  description = "vCPU assigned to the hosted manager Container App."
  type        = number
  default     = 0.25
}

variable "manager_memory" {
  description = "Memory assigned to the hosted manager Container App."
  type        = string
  default     = "0.5Gi"
}

variable "runner_cpu" {
  description = "vCPU assigned to each Container Apps Job execution."
  type        = number
  default     = 0.25
}

variable "runner_memory" {
  description = "Memory assigned to each Container Apps Job execution."
  type        = string
  default     = "0.5Gi"
}

variable "max_runner_executions" {
  description = "Cost-control limit enforced by the manager."
  type        = number
  default     = 10

  validation {
    condition     = var.max_runner_executions >= 1 && var.max_runner_executions <= 10
    error_message = "max_runner_executions must be between 1 and 10 for this test stack."
  }
}

variable "runner_timeout_seconds" {
  description = "Maximum duration of one runner execution."
  type        = number
  default     = 7200
}

variable "tags" {
  description = "Additional tags applied to resources."
  type        = map(string)
  default     = {}
}
