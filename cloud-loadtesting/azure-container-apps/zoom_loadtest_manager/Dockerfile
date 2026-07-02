FROM node:22-bookworm-slim

ENV NODE_ENV=production

WORKDIR /app

COPY package*.json ./
RUN npm ci --omit=dev

COPY public/ ./public/
COPY src/ ./src/

EXPOSE 3090

CMD ["node", "src/server.js"]
