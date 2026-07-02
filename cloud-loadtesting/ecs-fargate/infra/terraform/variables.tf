variable "aws_region" {
  description = "AWS region for this deployment."
  type        = string
  default     = "us-east-1"
}

variable "project_name" {
  description = "Resource name prefix."
  type        = string
  default     = "zoom-loadtest"
}

variable "aws_resource_group_name" {
  description = "Optional AWS Resource Groups group name for browsing all tagged stack resources. Empty disables it."
  type        = string
  default     = "zoom_rtms_loadtest"
}

variable "owner_name" {
  description = "Optional owner name tag for a specific deployment. Leave blank in reusable templates."
  type        = string
  default     = ""
}

variable "vpc_cidr" {
  description = "CIDR for the disposable VPC Terraform creates."
  type        = string
  default     = "10.80.0.0/16"
}

variable "public_subnet_cidrs" {
  description = "CIDRs for public subnets used by Fargate tasks."
  type        = list(string)
  default     = ["10.80.1.0/24", "10.80.2.0/24"]
}

variable "assign_public_ip" {
  description = "Whether Fargate runner tasks receive public IPs."
  type        = bool
  default     = true
}

variable "runner_cpu" {
  description = "Fargate task CPU units. 256 = 0.25 vCPU."
  type        = number
  default     = 256
}

variable "runner_memory" {
  description = "Fargate task memory MiB. 512 = 0.5GB."
  type        = number
  default     = 512
}

variable "max_runner_tasks" {
  description = "Hard maximum number of concurrent runner tasks accepted by the manager."
  type        = number
  default     = 10

  validation {
    condition     = var.max_runner_tasks >= 1 && var.max_runner_tasks <= 10
    error_message = "max_runner_tasks must be between 1 and 10 for this cost-controlled test stack."
  }
}

variable "lambda_handler" {
  description = "Lambda handler for the packaged manager API."
  type        = string
  default     = "index.handler"
}

variable "lambda_runtime" {
  description = "Lambda runtime."
  type        = string
  default     = "nodejs20.x"
}

variable "manager_reserved_concurrency" {
  description = "Reserved concurrency for the manager Lambda. Use -1 for unreserved concurrency when the account quota is too low."
  type        = number
  default     = -1

  validation {
    condition     = var.manager_reserved_concurrency == -1 || var.manager_reserved_concurrency >= 1
    error_message = "manager_reserved_concurrency must be -1 or at least 1."
  }
}

variable "ssm_parameter_prefix" {
  description = "Prefix for SSM SecureString parameters read by the manager Lambda."
  type        = string
  default     = "/zoom-loadtest"
}

variable "allowed_cors_origins" {
  description = "CORS origins allowed to call the API. Use the CloudFront URL/custom domain in production."
  type        = list(string)
  default     = ["*"]
}

variable "enable_cloudfront" {
  description = "Create CloudFront in front of the UI and API. Disable when the AWS account is not verified for CloudFront."
  type        = bool
  default     = true
}

variable "ui_bucket_name" {
  description = "Optional fixed S3 bucket name for the static UI. Empty lets AWS generate a name."
  type        = string
  default     = ""
}

variable "create_ssm_placeholders" {
  description = "Create non-secret placeholder SecureString parameters under ssm_parameter_prefix."
  type        = bool
  default     = true
}
