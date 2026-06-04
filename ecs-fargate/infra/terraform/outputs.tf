output "cloudfront_url" {
  description = "CloudFront URL for the static UI and /api/* proxy."
  value       = "https://${aws_cloudfront_distribution.ui.domain_name}"
}

output "api_gateway_url" {
  description = "Direct API Gateway URL."
  value       = aws_apigatewayv2_api.manager.api_endpoint
}

output "rtms_webhook_url" {
  description = "Zoom RTMS webhook URL."
  value       = "${aws_apigatewayv2_api.manager.api_endpoint}/api/zoom/rtms/webhook"
}

output "ui_bucket" {
  description = "S3 bucket for static UI files."
  value       = aws_s3_bucket.ui.id
}

output "ecs_cluster" {
  description = "ECS cluster used for runner tasks."
  value       = aws_ecs_cluster.runner.name
}

output "ecs_task_definition" {
  description = "Fargate runner task definition ARN."
  value       = aws_ecs_task_definition.runner.arn
}

output "runner_ecr_repository_url" {
  description = "Push the runner image here with tag latest."
  value       = aws_ecr_repository.runner.repository_url
}

output "runner_image" {
  description = "Image URI used by the ECS task definition."
  value       = local.runner_image
}

output "ecs_container_name" {
  description = "Runner container name used for RunTask overrides."
  value       = local.runner_container
}

output "runner_security_group" {
  description = "Outbound-only runner security group ID."
  value       = aws_security_group.runner.id
}

output "vpc_id" {
  description = "Created VPC ID."
  value       = aws_vpc.main.id
}

output "runner_subnet_ids" {
  description = "Created public subnet IDs used by Fargate runner tasks."
  value       = aws_subnet.public[*].id
}

output "status_table" {
  description = "DynamoDB status table."
  value       = aws_dynamodb_table.status.name
}

output "ssm_parameter_prefix" {
  description = "Prefix where manager secrets/config should be stored."
  value       = var.ssm_parameter_prefix
}
