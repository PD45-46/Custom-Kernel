FROM ubuntu:22.04

# Install dependencies
RUN apt update && apt install -y \
    build-essential bison flex libgmp-dev libmpc-dev libmpfr-dev \
    texinfo nasm qemu-system-x86 grub-pc-bin xorriso mtools wget

# Copy project
WORKDIR /kernel
COPY . .

# Build toolchain
RUN chmod +x scripts/setup_toolchain.sh && \
    scripts/setup_toolchain.sh

# Add toolchain to PATH
ENV PATH="/opt/cross/bin:$PATH"

# Default command
CMD ["make", "run"]