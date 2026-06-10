import serverless from 'serverless-http';

import { loadRuntimeConfig } from './src/config.js';
import { app } from './src/server.js';

const expressHandler = serverless(app);
let configLoaded;

export async function handler(event, context) {
  configLoaded ||= loadRuntimeConfig();
  await configLoaded;
  return expressHandler(event, context);
}
