import dotenv from 'dotenv';
import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

dotenv.config();

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const appRoot = path.join(__dirname, '..');
const envPath = path.join(appRoot, '.env');

export const envDefaults = {
  PORT: '3090',
  HOST: '0.0.0.0',
  CUSTCREATE_EMAIL_DOMAIN: 'loadtest.invalid',
  ZOOM_ACCOUNT_ID: '',
  ZOOM_CLIENT_ID: '',
  ZOOM_CLIENT_SECRET: '',
  ZOOM_API_BASE_URL: 'https://api.zoom.us/v2',
  MEETING_TOKEN_ENDPOINT: '',
  ZOOM_RTMS_CLIENT_ID: 'bnLICtNSlytlF35PKrpQ',
  ZOOM_WEBHOOK_SECRET_TOKEN: '',
  AWS_REGION: 'us-east-1',
  ECS_CLUSTER: '',
  ECS_TASK_DEFINITION: '',
  ECS_TASK_FAMILY: 'zoom-sendraw-loadtest-meeting',
  ECS_CONTAINER_NAME: 'zoom-sendraw-loadtest-meeting',
  ECS_SUBNETS: '',
  ECS_SECURITY_GROUPS: '',
  ECS_ASSIGN_PUBLIC_IP: 'true',
  ECS_LAUNCH_TYPE: 'FARGATE',
  ECS_PLATFORM_VERSION: 'LATEST',
  ECS_TASK_CPU: '256',
  ECS_TASK_MEMORY: '512',
  ECS_PROJECT: 'zoom-loadtest-meeting',
  DOCKER_REGISTRY_URL: 'dcr.asdc.cc',
  DOCKER_REGISTRY_USERNAME: '',
  DOCKER_REGISTRY_PASSWORD: '',
  DOCKER_IMAGE: 'dcr.asdc.cc/zoom-sendraw-loadtest-meeting:latest',
  DOCKER_PROJECT: 'zoom-loadtest-meeting',
  DOCKER_RESTART_POLICY: 'no',
  DOCKER_SHM_SIZE: '256m',
  DOCKER_CPU_MIN: '0.1',
  DOCKER_CPU_MAX: '0.5',
  DOCKER_MEMORY_MIN: '200m',
  DOCKER_MEMORY_MAX: '500m',
  DOCKER_NETWORK: '',
  DOCKER_LOGIN_BEFORE_RUN: 'false'
};

function env(name, fallback = '') {
  return process.env[name] ?? fallback;
}

function envNonEmpty(name, fallback = '') {
  const raw = process.env[name];
  return raw === undefined || raw === '' ? fallback : raw;
}

function intEnv(name, fallback) {
  const raw = env(name);
  if (!raw) return fallback;
  const parsed = Number.parseInt(raw, 10);
  return Number.isFinite(parsed) ? parsed : fallback;
}

function boolEnv(name, fallback = false) {
  const raw = env(name);
  if (!raw) return fallback;
  return ['1', 'true', 'yes', 'y', 'on'].includes(raw.toLowerCase());
}

function httpsUrlEnv(name, fallback) {
  const raw = env(name, fallback).trim();
  if (!raw) return fallback;
  if (/^https?:\/\//i.test(raw)) return raw;
  return `https://${raw}`;
}

export const config = {
  server: {
    host: env('HOST', '0.0.0.0'),
    port: intEnv('PORT', 3090),
    custCreateEmailDomain: env('CUSTCREATE_EMAIL_DOMAIN', 'loadtest.invalid')
  },
  zoom: {
    accountId: env('ZOOM_ACCOUNT_ID'),
    clientId: env('ZOOM_CLIENT_ID'),
    clientSecret: env('ZOOM_CLIENT_SECRET'),
    apiBaseUrl: httpsUrlEnv('ZOOM_API_BASE_URL', 'https://api.zoom.us/v2').replace(/\/$/, ''),
    tokenEndpoint: httpsUrlEnv('MEETING_TOKEN_ENDPOINT', ''),
    rtmsClientId: env('ZOOM_RTMS_CLIENT_ID', 'bnLICtNSlytlF35PKrpQ'),
    webhookSecretToken: env('ZOOM_WEBHOOK_SECRET_TOKEN')
  },
  ecs: {
    region: env('AWS_REGION', 'us-east-1'),
    cluster: env('ECS_CLUSTER'),
    taskDefinition: env('ECS_TASK_DEFINITION'),
    taskFamily: env('ECS_TASK_FAMILY', 'zoom-sendraw-loadtest-meeting'),
    containerName: env('ECS_CONTAINER_NAME', 'zoom-sendraw-loadtest-meeting'),
    subnets: env('ECS_SUBNETS').split(',').map(item => item.trim()).filter(Boolean),
    securityGroups: env('ECS_SECURITY_GROUPS').split(',').map(item => item.trim()).filter(Boolean),
    assignPublicIp: boolEnv('ECS_ASSIGN_PUBLIC_IP', true),
    launchType: env('ECS_LAUNCH_TYPE', 'FARGATE'),
    platformVersion: env('ECS_PLATFORM_VERSION', 'LATEST'),
    taskCpu: env('ECS_TASK_CPU', '256'),
    taskMemory: env('ECS_TASK_MEMORY', '512'),
    project: env('ECS_PROJECT', 'zoom-loadtest-meeting')
  },
  docker: {
    registryUrl: env('DOCKER_REGISTRY_URL', 'dcr.asdc.cc'),
    registryUsername: env('DOCKER_REGISTRY_USERNAME'),
    registryPassword: env('DOCKER_REGISTRY_PASSWORD'),
    image: env('DOCKER_IMAGE', 'dcr.asdc.cc/zoom-sendraw-loadtest-meeting:latest'),
    project: env('DOCKER_PROJECT', 'zoom-loadtest-meeting'),
    restartPolicy: env('DOCKER_RESTART_POLICY', 'no'),
    shmSize: env('DOCKER_SHM_SIZE', '256m'),
    cpuMin: envNonEmpty('DOCKER_CPU_MIN', '0.1'),
    cpuMax: envNonEmpty('DOCKER_CPU_MAX', envNonEmpty('DOCKER_CPUS', '0.5')),
    memoryMin: envNonEmpty('DOCKER_MEMORY_MIN', '200m'),
    memoryMax: envNonEmpty('DOCKER_MEMORY_MAX', envNonEmpty('DOCKER_MEMORY', '500m')),
    network: env('DOCKER_NETWORK')
  },
  features: {
    dockerLoginBeforeRun: boolEnv('DOCKER_LOGIN_BEFORE_RUN', false)
  }
};

export const envKeys = Object.keys(envDefaults);

export const secretEnvKeys = new Set([
  'ZOOM_CLIENT_SECRET',
  'ZOOM_WEBHOOK_SECRET_TOKEN',
  'DOCKER_REGISTRY_PASSWORD'
]);

export function envSnapshot() {
  return envKeys.map(key => {
    const isSecret = secretEnvKeys.has(key);
    const hasExplicitValue = process.env[key] !== undefined && process.env[key] !== '';
    const effectiveValue = process.env[key] ?? envDefaults[key] ?? '';
    return {
      key,
      isSecret,
      isSet: hasExplicitValue,
      fromDefault: process.env[key] === undefined && effectiveValue !== '',
      value: isSecret && hasExplicitValue ? '' : String(effectiveValue),
      placeholder: isSecret && hasExplicitValue ? 'Configured; enter a new value to replace' : ''
    };
  });
}

function quoteEnvValue(value) {
  const stringValue = String(value ?? '');
  if (/^[A-Za-z0-9_./:@-]*$/.test(stringValue)) {
    return stringValue;
  }
  return JSON.stringify(stringValue);
}

export async function saveEnv(updates) {
  for (const key of envKeys) {
    if (Object.prototype.hasOwnProperty.call(updates, key)) {
      const nextValue = String(updates[key] ?? '');
      if (secretEnvKeys.has(key) && nextValue === '' && process.env[key]) {
        continue;
      }
      process.env[key] = nextValue;
    } else if (process.env[key] === undefined && envDefaults[key] !== undefined) {
      process.env[key] = envDefaults[key];
    }
  }

  const lines = envKeys.map(key => `${key}=${quoteEnvValue(process.env[key] ?? envDefaults[key] ?? '')}`);
  await fs.writeFile(envPath, `${lines.join('\n')}\n`, 'utf8');
  reloadConfigFromEnv();
}

export function reloadConfigFromEnv() {
  config.server.host = env('HOST', '0.0.0.0');
  config.server.port = intEnv('PORT', 3090);
  config.server.custCreateEmailDomain = env('CUSTCREATE_EMAIL_DOMAIN', 'loadtest.invalid');

  config.zoom.accountId = env('ZOOM_ACCOUNT_ID');
  config.zoom.clientId = env('ZOOM_CLIENT_ID');
  config.zoom.clientSecret = env('ZOOM_CLIENT_SECRET');
  config.zoom.apiBaseUrl = httpsUrlEnv('ZOOM_API_BASE_URL', 'https://api.zoom.us/v2').replace(/\/$/, '');
  config.zoom.tokenEndpoint = httpsUrlEnv('MEETING_TOKEN_ENDPOINT', '');
  config.zoom.rtmsClientId = env('ZOOM_RTMS_CLIENT_ID', 'bnLICtNSlytlF35PKrpQ');
  config.zoom.webhookSecretToken = env('ZOOM_WEBHOOK_SECRET_TOKEN');

  config.ecs.region = env('AWS_REGION', 'us-east-1');
  config.ecs.cluster = env('ECS_CLUSTER');
  config.ecs.taskDefinition = env('ECS_TASK_DEFINITION');
  config.ecs.taskFamily = env('ECS_TASK_FAMILY', 'zoom-sendraw-loadtest-meeting');
  config.ecs.containerName = env('ECS_CONTAINER_NAME', 'zoom-sendraw-loadtest-meeting');
  config.ecs.subnets = env('ECS_SUBNETS').split(',').map(item => item.trim()).filter(Boolean);
  config.ecs.securityGroups = env('ECS_SECURITY_GROUPS').split(',').map(item => item.trim()).filter(Boolean);
  config.ecs.assignPublicIp = boolEnv('ECS_ASSIGN_PUBLIC_IP', true);
  config.ecs.launchType = env('ECS_LAUNCH_TYPE', 'FARGATE');
  config.ecs.platformVersion = env('ECS_PLATFORM_VERSION', 'LATEST');
  config.ecs.taskCpu = env('ECS_TASK_CPU', '256');
  config.ecs.taskMemory = env('ECS_TASK_MEMORY', '512');
  config.ecs.project = env('ECS_PROJECT', 'zoom-loadtest-meeting');

  config.docker.registryUrl = env('DOCKER_REGISTRY_URL', 'dcr.asdc.cc');
  config.docker.registryUsername = env('DOCKER_REGISTRY_USERNAME');
  config.docker.registryPassword = env('DOCKER_REGISTRY_PASSWORD');
  config.docker.image = env('DOCKER_IMAGE', 'dcr.asdc.cc/zoom-sendraw-loadtest-meeting:latest');
  config.docker.project = env('DOCKER_PROJECT', 'zoom-loadtest-meeting');
  config.docker.restartPolicy = env('DOCKER_RESTART_POLICY', 'no');
  config.docker.shmSize = env('DOCKER_SHM_SIZE', '256m');
  config.docker.cpuMin = envNonEmpty('DOCKER_CPU_MIN', '0.1');
  config.docker.cpuMax = envNonEmpty('DOCKER_CPU_MAX', envNonEmpty('DOCKER_CPUS', '0.5'));
  config.docker.memoryMin = envNonEmpty('DOCKER_MEMORY_MIN', '200m');
  config.docker.memoryMax = envNonEmpty('DOCKER_MEMORY_MAX', envNonEmpty('DOCKER_MEMORY', '500m'));
  config.docker.network = env('DOCKER_NETWORK');

  config.features.dockerLoginBeforeRun = boolEnv('DOCKER_LOGIN_BEFORE_RUN', false);
}

export function publicStatus() {
  return {
    zoom: {
      hasAccountId: Boolean(config.zoom.accountId),
      hasClientId: Boolean(config.zoom.clientId),
      hasClientSecret: Boolean(config.zoom.clientSecret),
      apiBaseUrl: config.zoom.apiBaseUrl,
      tokenEndpoint: config.zoom.tokenEndpoint,
      rtmsClientId: config.zoom.rtmsClientId,
      hasWebhookSecretToken: Boolean(config.zoom.webhookSecretToken)
    },
    docker: {
      registryUrl: config.docker.registryUrl,
      hasRegistryUsername: Boolean(config.docker.registryUsername),
      hasRegistryPassword: Boolean(config.docker.registryPassword),
      image: config.docker.image,
      project: config.docker.project,
      cpuMin: config.docker.cpuMin,
      cpuMax: config.docker.cpuMax,
      memoryMin: config.docker.memoryMin,
      memoryMax: config.docker.memoryMax
    },
    ecs: {
      region: config.ecs.region,
      cluster: config.ecs.cluster,
      taskDefinition: config.ecs.taskDefinition,
      taskFamily: config.ecs.taskFamily,
      containerName: config.ecs.containerName,
      hasSubnets: config.ecs.subnets.length > 0,
      hasSecurityGroups: config.ecs.securityGroups.length > 0,
      assignPublicIp: config.ecs.assignPublicIp,
      launchType: config.ecs.launchType,
      platformVersion: config.ecs.platformVersion,
      taskCpu: config.ecs.taskCpu,
      taskMemory: config.ecs.taskMemory,
      project: config.ecs.project
    }
  };
}
