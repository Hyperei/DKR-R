# Android/Linux build toolchain for DKR-R (fork: Hyperei/DKR-R)
# Replaces the Windows-only PowerShell packaging path with a reproducible
# container that can produce an arm64 APK on Linux.
FROM debian:trixie-slim

ARG NDK_VERSION=29.0.14206865
ARG BUILD_TOOLS=35.0.0
ARG PLATFORM=android-35
ENV DEBIAN_FRONTEND=noninteractive

# Host tools: C++ toolchain, CMake/Ninja, JDK 21 for Gradle, plus the
# texinfo/bison/flex set needed to build mips64-elf binutils for the decomp.
RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential clang lld cmake ninja-build git curl ca-certificates \
      python3 python3-venv python3-pip rsync unzip zip xz-utils file \
      openjdk-21-jdk-headless pkg-config \
      texinfo bison flex libgmp-dev libmpfr-dev libmpc-dev zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

ENV ANDROID_HOME=/opt/android-sdk
ENV ANDROID_SDK_ROOT=${ANDROID_HOME}
ENV ANDROID_NDK_HOME=${ANDROID_HOME}/ndk/${NDK_VERSION}

# Android command-line tools, then the exact pinned NDK the project's CI uses.
RUN mkdir -p ${ANDROID_HOME}/cmdline-tools \
    && curl -fsSL -o /tmp/cmdline-tools.zip \
       https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip \
    && unzip -q /tmp/cmdline-tools.zip -d ${ANDROID_HOME}/cmdline-tools \
    && mv ${ANDROID_HOME}/cmdline-tools/cmdline-tools ${ANDROID_HOME}/cmdline-tools/latest \
    && rm /tmp/cmdline-tools.zip

ENV PATH=${ANDROID_HOME}/cmdline-tools/latest/bin:${ANDROID_HOME}/platform-tools:${PATH}

RUN yes | sdkmanager --licenses >/dev/null 2>&1 || true
RUN sdkmanager --install "platform-tools" "platforms;${PLATFORM}" \
      "build-tools;${BUILD_TOOLS}" "ndk;${NDK_VERSION}" >/dev/null

# Gradle: the project ships no wrapper under packaging/android, so install one.
# AGP 8.11.0 (packaging/android/build.gradle) requires Gradle 8.13 or newer.
ARG GRADLE_VERSION=8.13
RUN curl -fsSL -o /tmp/gradle.zip \
      https://services.gradle.org/distributions/gradle-${GRADLE_VERSION}-bin.zip \
    && unzip -q /tmp/gradle.zip -d /opt && rm /tmp/gradle.zip
ENV PATH=/opt/gradle-${GRADLE_VERSION}/bin:${PATH}

# The decomp's dkr_assets_tool links -lpcre2-8 on Linux. Kept as a late layer
# so amending it never re-downloads the NDK.
USER root
RUN apt-get update && apt-get install -y --no-install-recommends \
      libpcre2-dev \
    && rm -rf /var/lib/apt/lists/*

# Build as a non-root user matching the host so bind-mounted output is writable.
ARG UID=1000
ARG GID=1000
RUN groupadd -g ${GID} builder 2>/dev/null || true \
    && useradd -m -u ${UID} -g ${GID} -s /bin/bash builder 2>/dev/null || true \
    && mkdir -p /work && chown -R ${UID}:${GID} /work ${ANDROID_HOME}
USER builder
WORKDIR /work
