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
