# Build environment for the CH569 SD/UART USB3 bridge firmware.
#
# Provides the xPack riscv-none-elf-gcc toolchain + make. The firmware tree is
# mounted at /work (see docker-compose.yml), so the image never needs rebuilding
# when source changes.
#
#   docker compose run --rm build          # = make
#   docker compose run --rm build make clean
#
FROM debian:bookworm-slim

ARG XPACK_VERSION=12.2.0-3

RUN apt-get update && \
    apt-get install -y --no-install-recommends ca-certificates curl make python3 && \
    rm -rf /var/lib/apt/lists/*

RUN curl -L -o /tmp/toolchain.tar.gz \
      "https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v${XPACK_VERSION}/xpack-riscv-none-elf-gcc-${XPACK_VERSION}-linux-x64.tar.gz" && \
    mkdir -p /opt/xpack && \
    tar -xzf /tmp/toolchain.tar.gz -C /opt/xpack --strip-components=1 && \
    rm /tmp/toolchain.tar.gz

ENV PATH="/opt/xpack/bin:${PATH}"

WORKDIR /work
CMD ["make"]
