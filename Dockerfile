# Build stage for C binary
FROM gcc:latest AS builder
WORKDIR /build
COPY src/ src/
COPY include/ include/
COPY Makefile .
RUN make LDFLAGS="-static"

# Runtime stage with Node.js for WebSocket bridge
FROM node:20-slim
WORKDIR /app

# Install procps for testing
RUN apt-get update && apt-get install -y --no-install-recommends procps && rm -rf /var/lib/apt/lists/*

# Copy syswatch binary
COPY --from=builder /build/syswatch /usr/local/bin/syswatch

# Copy and install WebSocket bridge server
COPY dashboard/server/ ./server/
WORKDIR /app/server
RUN npm install --production

EXPOSE 3001
CMD ["node", "index.js"]
