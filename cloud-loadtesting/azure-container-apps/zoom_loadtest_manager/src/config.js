import { DefaultAzureCredential } from '@azure/identity';
import { SecretClient } from '@azure/keyvault-secrets';
import dotenv from 'dotenv';
import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

dotenv.config();

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const appRoot = path.join(__dirname, '..');
const envPath = path.join(appRoot, '.env');
const persistedEnvSecretName = 'manager-environment';

export const envDefaults = {
  PORT: '3090',
  HOST: '0.0.0.0',
  MANAGER_AUTH_USERNAME: 'admin',
  MANAGER_AUTH_PASSWORD: 'admin',
  CUSTCREATE_EMAIL_DOMAIN: 'loadtest.invalid',
  ZOOM_ACCOUNT_ID: '',
  ZOOM_CLIENT_ID: '',
  ZOOM_CLIENT_SECRET: '',
  ZOOM_API_BASE_URL: 'https://api.zoom.us/v2',
  MEETING_TOKEN_ENDPOINT: '',
  ZOOM_RTMS_CLIENT_ID: '',
  ZOOM_WEBHOOK_SECRET_TOKEN: '',
  AZURE_SUBSCRIPTION_ID: '',
  AZURE_RESOURCE_GROUP: '',
  AZURE_CONTAINER_APP_JOB_NAME: 'zoom-loadtest-runner',
  AZURE_CONTAINER_NAME: 'zoom-sendraw-loadtest-meeting',
  AZURE_RUNNER_IMAGE: '',
  AZURE_MANAGEMENT_API_VERSION: '2026-01-01',
  AZURE_RUNNER_CPU: '0.25',
  AZURE_RUNNER_MEMORY: '0.5Gi',
  AZURE_MAX_EXECUTIONS: '10',
  AZURE_PROJECT: 'zoom-loadtest-meeting',
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
    rtmsClientId: env('ZOOM_RTMS_CLIENT_ID'),
    webhookSecretToken: env('ZOOM_WEBHOOK_SECRET_TOKEN')
  },
  azure: {
    subscriptionId: env('AZURE_SUBSCRIPTION_ID'),
    resourceGroup: env('AZURE_RESOURCE_GROUP'),
    jobName: env('AZURE_CONTAINER_APP_JOB_NAME', 'zoom-loadtest-runner'),
    containerName: env('AZURE_CONTAINER_NAME', 'zoom-sendraw-loadtest-meeting'),
    runnerImage: env('AZURE_RUNNER_IMAGE'),
    apiVersion: env('AZURE_MANAGEMENT_API_VERSION', '2026-01-01'),
    runnerCpu: Number(env('AZURE_RUNNER_CPU', '0.25')),
    runnerMemory: env('AZURE_RUNNER_MEMORY', '0.5Gi'),
    maxExecutions: intEnv('AZURE_MAX_EXECUTIONS', 10),
    project: env('AZURE_PROJECT', 'zoom-loadtest-meeting')
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
  'MANAGER_AUTH_PASSWORD',
  'ZOOM_CLIENT_SECRET',
  'ZOOM_WEBHOOK_SECRET_TOKEN',
  'DOCKER_REGISTRY_PASSWORD'
]);

const hostedReadonlyEnvKeys = new Set([
  'PORT',
  'HOST',
  'MANAGER_AUTH_USERNAME',
  'MANAGER_AUTH_PASSWORD',
  'AZURE_SUBSCRIPTION_ID',
  'AZURE_RESOURCE_GROUP',
  'AZURE_CONTAINER_APP_JOB_NAME',
  'AZURE_CONTAINER_NAME',
  'AZURE_RUNNER_IMAGE',
  'AZURE_MANAGEMENT_API_VERSION',
  'AZURE_RUNNER_CPU',
  'AZURE_RUNNER_MEMORY',
  'AZURE_MAX_EXECUTIONS',
  'AZURE_PROJECT',
  'DOCKER_REGISTRY_URL',
  'DOCKER_REGISTRY_USERNAME',
  'DOCKER_REGISTRY_PASSWORD',
  'DOCKER_IMAGE'
]);

function isReadonlyEnvKey(key) {
  return process.env.AZURE_HOSTED_MANAGER === 'true' && hostedReadonlyEnvKeys.has(key);
}

function editableEnvValues() {
  return Object.fromEntries(envKeys
    .filter(key => !isReadonlyEnvKey(key))
    .map(key => [key, process.env[key] ?? envDefaults[key] ?? '']));
}

function keyVaultClient() {
  const vaultUrl = process.env.AZURE_KEY_VAULT_URL;
  return vaultUrl ? new SecretClient(vaultUrl, new DefaultAzureCredential()) : null;
}

export async function loadPersistedEnv() {
  const client = keyVaultClient();
  if (!client) return;

  try {
    const secret = await client.getSecret(persistedEnvSecretName);
    const values = JSON.parse(secret.value || '{}');
    for (const [key, value] of Object.entries(values)) {
      if (envKeys.includes(key) && !isReadonlyEnvKey(key)) {
        process.env[key] = String(value ?? '');
      }
    }
    reloadConfigFromEnv();
    console.log(`Loaded persisted manager environment from ${process.env.AZURE_KEY_VAULT_URL}`);
  } catch (error) {
    if (error.statusCode === 404 || error.code === 'SecretNotFound') {
      console.log('No persisted manager environment found in Key Vault; using blank/default values.');
      return;
    }
    throw error;
  }
}

export function envSnapshot() {
  return envKeys.map(key => {
    const isSecret = secretEnvKeys.has(key);
    const hasExplicitValue = process.env[key] !== undefined && process.env[key] !== '';
    const effectiveValue = process.env[key] ?? envDefaults[key] ?? '';
    const readonly = isReadonlyEnvKey(key);
    return {
      key,
      isSecret,
      readonly,
      readonlyReason: readonly ? 'Managed by Terraform and the hosted Azure Container App.' : '',
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
    if (isReadonlyEnvKey(key)) {
      continue;
    }
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

  const client = keyVaultClient();
  if (client) {
    await client.setSecret(persistedEnvSecretName, JSON.stringify(editableEnvValues()), {
      contentType: 'application/json'
    });
  } else {
    const lines = envKeys.map(key => `${key}=${quoteEnvValue(process.env[key] ?? envDefaults[key] ?? '')}`);
    await fs.writeFile(envPath, `${lines.join('\n')}\n`, 'utf8');
  }
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
  config.zoom.rtmsClientId = env('ZOOM_RTMS_CLIENT_ID');
  config.zoom.webhookSecretToken = env('ZOOM_WEBHOOK_SECRET_TOKEN');

  config.azure.subscriptionId = env('AZURE_SUBSCRIPTION_ID');
  config.azure.resourceGroup = env('AZURE_RESOURCE_GROUP');
  config.azure.jobName = env('AZURE_CONTAINER_APP_JOB_NAME', 'zoom-loadtest-runner');
  config.azure.containerName = env('AZURE_CONTAINER_NAME', 'zoom-sendraw-loadtest-meeting');
  config.azure.runnerImage = env('AZURE_RUNNER_IMAGE');
  config.azure.apiVersion = env('AZURE_MANAGEMENT_API_VERSION', '2026-01-01');
  config.azure.runnerCpu = Number(env('AZURE_RUNNER_CPU', '0.25'));
  config.azure.runnerMemory = env('AZURE_RUNNER_MEMORY', '0.5Gi');
  config.azure.maxExecutions = intEnv('AZURE_MAX_EXECUTIONS', 10);
  config.azure.project = env('AZURE_PROJECT', 'zoom-loadtest-meeting');

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
    azure: {
      subscriptionId: config.azure.subscriptionId,
      resourceGroup: config.azure.resourceGroup,
      jobName: config.azure.jobName,
      containerName: config.azure.containerName,
      runnerImage: config.azure.runnerImage,
      apiVersion: config.azure.apiVersion,
      runnerCpu: config.azure.runnerCpu,
      runnerMemory: config.azure.runnerMemory,
      maxExecutions: config.azure.maxExecutions,
      project: config.azure.project
    }
  };
}
