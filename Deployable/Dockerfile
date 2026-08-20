# --- Build stage -----------------------------------------------------
# Compiles fastsearch_server only (tests are skipped here deliberately:
# FASTSEARCH_BUILD_TESTS=OFF avoids fetching + compiling GoogleTest in
# the image build, which is dead weight in a deployment artifact and
# meaningfully slows the image down. Tests still run in CI, see
# .github/workflows/ci.yml -- this Dockerfile is not where correctness
# is established, it's where an already-tested binary gets packaged.
FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake libsqlite3-dev ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY CMakeLists.txt ./
COPY src ./src
COPY third_party ./third_party

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFASTSEARCH_BUILD_TESTS=OFF \
    && cmake --build build -j"$(nproc)" --target fastsearch_server

# --- Runtime stage -----------------------------------------------------
# A separate, much smaller image: only the shared libs the binary
# actually links against at runtime, not the whole compiler toolchain.
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
        libsqlite3-0 ca-certificates curl \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --create-home --uid 1000 appuser \
    && mkdir -p /data && chown appuser:appuser /data

WORKDIR /app
COPY --from=builder /app/build/fastsearch_server ./fastsearch_server

USER appuser
EXPOSE 8080
VOLUME ["/data"]

# /data is the mounted volume (see docker-compose.yml) so the SQLite
# file -- and therefore the dictionary -- survives container restarts
# and image rebuilds. Losing it would mean losing every word/frequency
# added since the seed dataset, which defeats the point of Phase 4.
HEALTHCHECK --interval=10s --timeout=3s --start-period=5s --retries=5 \
    CMD curl -f http://localhost:8080/api/health || exit 1

ENTRYPOINT ["./fastsearch_server"]
CMD ["/data/fastsearch.db", "8080"]
