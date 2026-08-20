# --- Build stage -----------------------------------------------------
FROM node:22-bookworm-slim AS builder

WORKDIR /app
COPY frontend/package.json frontend/package-lock.json ./
RUN npm ci

COPY frontend ./

# Vite bakes VITE_* variables into the bundle AT BUILD TIME, not at
# container start time -- there is no "runtime env var" for a static
# SPA once it's built. That means the backend URL has to be known here,
# as a build arg, rather than passed to `docker run` later. Default
# assumes both services are reached via the host machine's published
# ports (see docker-compose.yml), which is correct for local/dev use;
# a real production deployment behind a real domain would override this
# at build time, e.g. --build-arg VITE_API_BASE_URL=https://api.example.com
ARG VITE_API_BASE_URL=http://localhost:8080
ENV VITE_API_BASE_URL=$VITE_API_BASE_URL

RUN npm run build

# --- Runtime stage -----------------------------------------------------
# The build stage's ~500MB of node_modules never reaches this image --
# only the static dist/ output does. Final image is nginx + ~200KB of
# HTML/CSS/JS.
FROM nginx:1.27-alpine

COPY --from=builder /app/dist /usr/share/nginx/html
COPY frontend/nginx.conf /etc/nginx/conf.d/default.conf

EXPOSE 80
HEALTHCHECK --interval=10s --timeout=3s --start-period=5s --retries=5 \
    CMD wget -qO- http://localhost:80/ >/dev/null || exit 1
