locals {
  name             = var.project_name
  lambda_function  = "${local.name}-manager"
  ecs_cluster      = "${local.name}-cluster"
  runner_family    = "${local.name}-runner"
  runner_container = "zoom-sendraw-loadtest-meeting"
  status_table     = "${local.name}-status"
  ui_bucket_name   = var.ui_bucket_name != "" ? var.ui_bucket_name : null
  api_domain_name  = replace(aws_apigatewayv2_api.manager.api_endpoint, "https://", "")
  runner_image     = "${aws_ecr_repository.runner.repository_url}:latest"
  ui_source_dir    = "${path.module}/../../zoom_loadtest_manager/public"
  manager_env_file = "${path.module}/../../zoom_loadtest_manager/.env"
  manager_env_lines = fileexists(local.manager_env_file) ? [
    for line in split("\n", file(local.manager_env_file)) : trimspace(line)
    if trimspace(line) != "" && !startswith(trimspace(line), "#") && strcontains(line, "=")
  ] : []
  manager_env = {
    for line in local.manager_env_lines :
    trimspace(regex("^([^=]+)=(.*)$", line)[0]) => trim(trimspace(regex("^([^=]+)=(.*)$", line)[1]), "\"'")
  }
  ui_files = fileset(local.ui_source_dir, "**")
  ui_content_types = {
    css  = "text/css; charset=utf-8"
    html = "text/html; charset=utf-8"
    js   = "text/javascript; charset=utf-8"
    json = "application/json; charset=utf-8"
    svg  = "image/svg+xml"
  }
}

data "aws_availability_zones" "available" {
  state = "available"
}

data "archive_file" "manager_lambda" {
  type        = "zip"
  source_dir  = "${path.module}/../../zoom_loadtest_manager"
  output_path = "${path.module}/.terraform/manager.zip"
  excludes    = [".env", ".data", "manager.log", "npm-debug.log"]
}

resource "aws_vpc" "main" {
  cidr_block           = var.vpc_cidr
  enable_dns_hostnames = true
  enable_dns_support   = true

  tags = {
    Name = "${local.name}-vpc"
  }
}

resource "aws_resourcegroups_group" "stack" {
  count       = var.aws_resource_group_name == "" ? 0 : 1
  name        = var.aws_resource_group_name
  description = "Resources for Zoom RTMS load testing"

  resource_query {
    type = "TAG_FILTERS_1_0"
    query = jsonencode({
      ResourceTypeFilters = ["AWS::AllSupported"]
      TagFilters = [{
        Key    = "Project"
        Values = [var.project_name]
      }]
    })
  }
}

resource "aws_internet_gateway" "main" {
  vpc_id = aws_vpc.main.id

  tags = {
    Name = "${local.name}-igw"
  }
}

resource "aws_subnet" "public" {
  count = length(var.public_subnet_cidrs)

  vpc_id                  = aws_vpc.main.id
  cidr_block              = var.public_subnet_cidrs[count.index]
  availability_zone       = data.aws_availability_zones.available.names[count.index % length(data.aws_availability_zones.available.names)]
  map_public_ip_on_launch = true

  tags = {
    Name = "${local.name}-public-${count.index + 1}"
  }
}

resource "aws_route_table" "public" {
  vpc_id = aws_vpc.main.id

  route {
    cidr_block = "0.0.0.0/0"
    gateway_id = aws_internet_gateway.main.id
  }

  tags = {
    Name = "${local.name}-public"
  }
}

resource "aws_route_table_association" "public" {
  count = length(aws_subnet.public)

  subnet_id      = aws_subnet.public[count.index].id
  route_table_id = aws_route_table.public.id
}

resource "aws_ecr_repository" "runner" {
  name                 = "zoom-sendraw-loadtest-meeting"
  image_tag_mutability = "MUTABLE"
  force_delete         = true

  image_scanning_configuration {
    scan_on_push = true
  }
}

resource "aws_ecr_lifecycle_policy" "runner" {
  repository = aws_ecr_repository.runner.name

  policy = jsonencode({
    rules = [{
      rulePriority = 1
      description  = "Keep last 5 images"
      selection = {
        tagStatus   = "any"
        countType   = "imageCountMoreThan"
        countNumber = 5
      }
      action = {
        type = "expire"
      }
    }]
  })
}

resource "aws_s3_bucket" "ui" {
  bucket        = local.ui_bucket_name
  bucket_prefix = local.ui_bucket_name == null ? "${local.name}-ui-" : null
  force_destroy = true
}

resource "aws_s3_bucket_public_access_block" "ui" {
  bucket = aws_s3_bucket.ui.id

  block_public_acls       = true
  block_public_policy     = true
  ignore_public_acls      = true
  restrict_public_buckets = true
}

resource "aws_s3_object" "ui" {
  for_each = local.ui_files

  bucket        = aws_s3_bucket.ui.id
  key           = each.value
  source        = "${local.ui_source_dir}/${each.value}"
  etag          = filemd5("${local.ui_source_dir}/${each.value}")
  content_type  = lookup(local.ui_content_types, reverse(split(".", each.value))[0], "application/octet-stream")
  cache_control = "no-cache"
}

resource "aws_cloudfront_origin_access_control" "ui" {
  count = var.enable_cloudfront ? 1 : 0

  name                              = "${local.name}-ui-oac"
  description                       = "Access ${aws_s3_bucket.ui.id} from CloudFront only"
  origin_access_control_origin_type = "s3"
  signing_behavior                  = "always"
  signing_protocol                  = "sigv4"
}

resource "aws_cloudfront_distribution" "ui" {
  count = var.enable_cloudfront ? 1 : 0

  enabled             = true
  default_root_object = "index.html"
  comment             = "${local.name} static UI and API"
  price_class         = "PriceClass_100"

  origin {
    origin_id                = "ui"
    domain_name              = aws_s3_bucket.ui.bucket_regional_domain_name
    origin_access_control_id = aws_cloudfront_origin_access_control.ui[0].id
  }

  origin {
    origin_id   = "api"
    domain_name = local.api_domain_name

    custom_origin_config {
      http_port              = 80
      https_port             = 443
      origin_protocol_policy = "https-only"
      origin_ssl_protocols   = ["TLSv1.2"]
    }
  }

  default_cache_behavior {
    target_origin_id       = "ui"
    viewer_protocol_policy = "redirect-to-https"
    allowed_methods        = ["GET", "HEAD", "OPTIONS"]
    cached_methods         = ["GET", "HEAD"]
    compress               = true

    forwarded_values {
      query_string = false
      cookies {
        forward = "none"
      }
    }
  }

  ordered_cache_behavior {
    path_pattern           = "/api/*"
    target_origin_id       = "api"
    viewer_protocol_policy = "redirect-to-https"
    allowed_methods        = ["GET", "HEAD", "OPTIONS", "POST", "PUT", "PATCH", "DELETE"]
    cached_methods         = ["GET", "HEAD"]
    compress               = true

    forwarded_values {
      query_string = true
      headers      = ["Authorization", "Content-Type"]
      cookies {
        forward = "all"
      }
    }

    min_ttl     = 0
    default_ttl = 0
    max_ttl     = 0
  }

  restrictions {
    geo_restriction {
      restriction_type = "none"
    }
  }

  viewer_certificate {
    cloudfront_default_certificate = true
  }
}

resource "aws_s3_bucket_policy" "ui_cloudfront" {
  count = var.enable_cloudfront ? 1 : 0

  bucket = aws_s3_bucket.ui.id

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Sid       = "AllowCloudFrontRead"
      Effect    = "Allow"
      Principal = { Service = "cloudfront.amazonaws.com" }
      Action    = "s3:GetObject"
      Resource  = "${aws_s3_bucket.ui.arn}/*"
      Condition = {
        StringEquals = {
          "AWS:SourceArn" = aws_cloudfront_distribution.ui[0].arn
        }
      }
    }]
  })
}

resource "aws_dynamodb_table" "status" {
  name         = local.status_table
  billing_mode = "PAY_PER_REQUEST"
  hash_key     = "pk"
  range_key    = "sk"

  attribute {
    name = "pk"
    type = "S"
  }

  attribute {
    name = "sk"
    type = "S"
  }

  ttl {
    attribute_name = "expiresAt"
    enabled        = true
  }
}

resource "aws_ssm_parameter" "placeholders" {
  for_each = var.create_ssm_placeholders ? toset([
    "zoom/account-id",
    "zoom/client-id",
    "zoom/client-secret",
    "zoom/rtms-client-id",
    "zoom/webhook-secret-token",
    "meeting-token-endpoint"
  ]) : toset([])

  name  = "${var.ssm_parameter_prefix}/${each.key}"
  type  = "SecureString"
  value = "REPLACE_ME"

  lifecycle {
    ignore_changes = [value]
  }
}

resource "aws_ecs_cluster" "runner" {
  name = local.ecs_cluster
}

resource "aws_security_group" "runner" {
  name        = "${local.name}-runner"
  description = "Outbound-only security group for Zoom load-test Fargate tasks"
  vpc_id      = aws_vpc.main.id

  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
}

resource "aws_iam_role" "ecs_task_execution" {
  name = "${local.name}-ecs-task-execution"

  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect    = "Allow"
      Principal = { Service = "ecs-tasks.amazonaws.com" }
      Action    = "sts:AssumeRole"
    }]
  })
}

resource "aws_iam_role_policy" "ecs_task_execution_ecr" {
  name = "ecr-pull"
  role = aws_iam_role.ecs_task_execution.id

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect = "Allow"
      Action = [
        "ecr:GetAuthorizationToken",
        "ecr:BatchCheckLayerAvailability",
        "ecr:GetDownloadUrlForLayer",
        "ecr:BatchGetImage"
      ]
      Resource = "*"
    }]
  })
}

resource "aws_iam_role" "ecs_task" {
  name = "${local.name}-ecs-task"

  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect    = "Allow"
      Principal = { Service = "ecs-tasks.amazonaws.com" }
      Action    = "sts:AssumeRole"
    }]
  })
}

resource "aws_ecs_task_definition" "runner" {
  family                   = local.runner_family
  requires_compatibilities = ["FARGATE"]
  network_mode             = "awsvpc"
  cpu                      = tostring(var.runner_cpu)
  memory                   = tostring(var.runner_memory)
  execution_role_arn       = aws_iam_role.ecs_task_execution.arn
  task_role_arn            = aws_iam_role.ecs_task.arn

  container_definitions = jsonencode([{
    name      = local.runner_container
    image     = local.runner_image
    essential = true
  }])
}

resource "aws_iam_role" "lambda" {
  name = "${local.name}-manager-lambda"

  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect    = "Allow"
      Principal = { Service = "lambda.amazonaws.com" }
      Action    = "sts:AssumeRole"
    }]
  })
}

resource "aws_iam_role_policy" "lambda_manager" {
  name = "manager"
  role = aws_iam_role.lambda.id

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Effect = "Allow"
        Action = [
          "ecs:RunTask",
          "ecs:StopTask",
          "ecs:ListTasks",
          "ecs:DescribeTasks",
          "ecs:TagResource"
        ]
        Resource = "*"
      },
      {
        Effect = "Allow"
        Action = "iam:PassRole"
        Resource = [
          aws_iam_role.ecs_task_execution.arn,
          aws_iam_role.ecs_task.arn
        ]
      },
      {
        Effect = "Allow"
        Action = [
          "dynamodb:GetItem",
          "dynamodb:PutItem",
          "dynamodb:UpdateItem",
          "dynamodb:DeleteItem",
          "dynamodb:Query",
          "dynamodb:Scan"
        ]
        Resource = aws_dynamodb_table.status.arn
      },
      {
        Effect = "Allow"
        Action = [
          "ssm:GetParameter",
          "ssm:GetParameters",
          "ssm:GetParametersByPath",
          "ssm:PutParameter"
        ]
        Resource = [
          "arn:aws:ssm:${var.aws_region}:*:parameter${var.ssm_parameter_prefix}",
          "arn:aws:ssm:${var.aws_region}:*:parameter${var.ssm_parameter_prefix}/*"
        ]
      }
    ]
  })
}

resource "aws_lambda_function" "manager" {
  function_name                  = local.lambda_function
  description                    = "Zoom load-test manager API"
  role                           = aws_iam_role.lambda.arn
  filename                       = data.archive_file.manager_lambda.output_path
  source_code_hash               = data.archive_file.manager_lambda.output_base64sha256
  handler                        = "lambda.handler"
  runtime                        = var.lambda_runtime
  timeout                        = 30
  memory_size                    = 512
  reserved_concurrent_executions = var.manager_reserved_concurrency

  environment {
    variables = {
      ECS_CLUSTER              = aws_ecs_cluster.runner.name
      ECS_TASK_DEFINITION      = aws_ecs_task_definition.runner.arn
      ECS_TASK_FAMILY          = aws_ecs_task_definition.runner.family
      ECS_CONTAINER_NAME       = local.runner_container
      ECS_SUBNETS              = join(",", aws_subnet.public[*].id)
      ECS_SECURITY_GROUPS      = aws_security_group.runner.id
      ECS_ASSIGN_PUBLIC_IP     = tostring(var.assign_public_ip)
      ECS_TASK_CPU             = tostring(var.runner_cpu)
      ECS_TASK_MEMORY          = tostring(var.runner_memory)
      ECS_MAX_TASKS            = tostring(var.max_runner_tasks)
      ECS_PROJECT              = local.name
      STATUS_TABLE_NAME        = aws_dynamodb_table.status.name
      SSM_PARAMETER_PREFIX     = var.ssm_parameter_prefix
      MANAGER_AUTH_USERNAME    = sensitive(lookup(local.manager_env, "MANAGER_AUTH_USERNAME", "admin"))
      MANAGER_AUTH_PASSWORD    = sensitive(lookup(local.manager_env, "MANAGER_AUTH_PASSWORD", "admin"))
      DOCKER_REGISTRY_URL      = lookup(local.manager_env, "DOCKER_REGISTRY_URL", aws_ecr_repository.runner.repository_url)
      DOCKER_REGISTRY_USERNAME = lookup(local.manager_env, "DOCKER_REGISTRY_USERNAME", "AWS")
      DOCKER_IMAGE             = lookup(local.manager_env, "DOCKER_IMAGE", local.runner_image)
      DOCKER_PROJECT           = lookup(local.manager_env, "DOCKER_PROJECT", local.name)
    }
  }
}

resource "aws_apigatewayv2_api" "manager" {
  name          = "${local.name}-api"
  protocol_type = "HTTP"

  cors_configuration {
    allow_origins = var.allowed_cors_origins
    allow_methods = ["GET", "POST", "PUT", "PATCH", "DELETE", "OPTIONS"]
    allow_headers = ["Authorization", "Content-Type"]
  }
}

resource "aws_apigatewayv2_integration" "manager" {
  api_id                 = aws_apigatewayv2_api.manager.id
  integration_type       = "AWS_PROXY"
  integration_uri        = aws_lambda_function.manager.invoke_arn
  payload_format_version = "2.0"
}

resource "aws_apigatewayv2_route" "proxy" {
  api_id    = aws_apigatewayv2_api.manager.id
  route_key = "ANY /{proxy+}"
  target    = "integrations/${aws_apigatewayv2_integration.manager.id}"
}

resource "aws_apigatewayv2_route" "root" {
  api_id    = aws_apigatewayv2_api.manager.id
  route_key = "ANY /"
  target    = "integrations/${aws_apigatewayv2_integration.manager.id}"
}

resource "aws_apigatewayv2_stage" "default" {
  api_id      = aws_apigatewayv2_api.manager.id
  name        = "$default"
  auto_deploy = true
}

resource "aws_lambda_permission" "api_gateway" {
  statement_id  = "AllowApiGatewayInvoke"
  action        = "lambda:InvokeFunction"
  function_name = aws_lambda_function.manager.function_name
  principal     = "apigateway.amazonaws.com"
  source_arn    = "${aws_apigatewayv2_api.manager.execution_arn}/*/*"
}
