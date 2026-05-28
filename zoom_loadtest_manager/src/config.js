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
  MEETING_TOKEN_ENDPOINT: 'https://nodejs.asdc.cc/meeting',
  ZOOM_RTMS_CLIENT_ID: 'bnLICtNSlytlF35PKrpQ',
  DOCKER_REGISTRY_URL: 'dcr.asdc.cc',
  DOCKER_REGISTRY_USERNAME: '',
  DOCKER_REGISTRY_PASSWORD: '',
  DOCKER_IMAGE: 'dcr.asdc.cc/zoom-sendraw-loadtest-meeting:latest',
  DOCKER_PROJECT: 'zoom-loadtest-meeting',
  DOCKER_RESTART_POLICY: 'no',
  DOCKER_SHM_SIZE: '256m',
  DOCKER_CPUS: '',
  DOCKER_MEMORY: '',
  DOCKER_NETWORK: '',
  DOCKER_LOGIN_BEFORE_RUN: 'false'
};

function env(name, fallback = '') {
  return process.env[name] ?? fallback;
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
    tokenEndpoint: httpsUrlEnv('MEETING_TOKEN_ENDPOINT', 'https://nodejs.asdc.cc/meeting'),
    rtmsClientId: env('ZOOM_RTMS_CLIENT_ID', 'bnLICtNSlytlF35PKrpQ')
  },
  docker: {
    registryUrl: env('DOCKER_REGISTRY_URL', 'dcr.asdc.cc'),
    registryUsername: env('DOCKER_REGISTRY_USERNAME'),
    registryPassword: env('DOCKER_REGISTRY_PASSWORD'),
    image: env('DOCKER_IMAGE', 'dcr.asdc.cc/zoom-sendraw-loadtest-meeting:latest'),
    project: env('DOCKER_PROJECT', 'zoom-loadtest-meeting'),
    restartPolicy: env('DOCKER_RESTART_POLICY', 'no'),
    shmSize: env('DOCKER_SHM_SIZE', '256m'),
    cpus: env('DOCKER_CPUS'),
    memory: env('DOCKER_MEMORY'),
    network: env('DOCKER_NETWORK')
  },
  features: {
    dockerLoginBeforeRun: boolEnv('DOCKER_LOGIN_BEFORE_RUN', false)
  }
};

export const envKeys = Object.keys(envDefaults);

export const secretEnvKeys = new Set([
  'ZOOM_CLIENT_SECRET',
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
  config.zoom.tokenEndpoint = httpsUrlEnv('MEETING_TOKEN_ENDPOINT', 'https://nodejs.asdc.cc/meeting');
  config.zoom.rtmsClientId = env('ZOOM_RTMS_CLIENT_ID', 'bnLICtNSlytlF35PKrpQ');

  config.docker.registryUrl = env('DOCKER_REGISTRY_URL', 'dcr.asdc.cc');
  config.docker.registryUsername = env('DOCKER_REGISTRY_USERNAME');
  config.docker.registryPassword = env('DOCKER_REGISTRY_PASSWORD');
  config.docker.image = env('DOCKER_IMAGE', 'dcr.asdc.cc/zoom-sendraw-loadtest-meeting:latest');
  config.docker.project = env('DOCKER_PROJECT', 'zoom-loadtest-meeting');
  config.docker.restartPolicy = env('DOCKER_RESTART_POLICY', 'no');
  config.docker.shmSize = env('DOCKER_SHM_SIZE', '256m');
  config.docker.cpus = env('DOCKER_CPUS');
  config.docker.memory = env('DOCKER_MEMORY');
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
      rtmsClientId: config.zoom.rtmsClientId
    },
    docker: {
      registryUrl: config.docker.registryUrl,
      hasRegistryUsername: Boolean(config.docker.registryUsername),
      hasRegistryPassword: Boolean(config.docker.registryPassword),
      image: config.docker.image,
      project: config.docker.project
    }
  };
}
