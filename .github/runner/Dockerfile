ARG UBUNTU_VERSION=24.04
FROM ubuntu:${UBUNTU_VERSION}

# ARG section

# See https://github.com/actions/runner/releases
# ARCH should be in (x64 | arm64 | arm)
ARG ACTION_RUNNER_TAG=2.334.0
ARG ACTION_RUNNER_ARCH=x64

# See https://github.com/miurahr/aqtinstall
ARG QT_VERSION=6.8.3
ARG QT_ARCHIVES="qtbase qtdeclarative icu"
ARG QT_MODULES=""
ARG QT_DIR=/opt/qt

LABEL org.opencontainers.image.vendor="Infomaniak"
LABEL org.opencontainers.image.title="matomo-sdk-qt-runner-ubuntu"
LABEL org.opencontainers.image.source="https://github.com/Infomaniak/matomo-sdk-qt"
LABEL org.opencontainers.image.description="Docker image for the Matomo SDK Qt runner on Ubuntu"

ENV TZ=Europe/Zurich
ENV LANG=fr_CH.UTF-8
ENV LC_ALL=fr_CH.UTF-8

ENV WORKING_DIR=/opt/actions-runner
ENV USERNAME=runner

RUN ln -snf "/usr/share/zoneinfo/$TZ" /etc/localtime && echo "$TZ" > /etc/timezone
RUN apt update --fix-missing && apt upgrade --yes && apt clean && rm -rf /var/lib/apt/lists/*
RUN echo '#!/bin/bash\napt update --fix-missing && apt install --yes "$@" && apt clean && rm -rf /var/lib/apt/lists/*' > /usr/local/bin/apt-install && chmod +x /usr/local/bin/apt-install


RUN apt-install locales && \
    locale-gen $LANG && \
    update-locale LANG=$LANG

RUN apt-install \
    curl \
    tar \
    gzip \
    xz-utils \
    unzip \
    ca-certificates \
    jq \
    libdigest-sha-perl \
    python3-venv \
    python3-pip

RUN apt-install \
    git \
    cmake \
    ninja-build \
    build-essential \
    clang \
    lld \
    clang-format \
    pkg-config \
    ccache \
    openssl \
    libssl-dev \
    libglib2.0-dev \
    libgl1-mesa-dev \
    libegl1-mesa-dev \
    libgl1-mesa-dri \
    libxkbcommon-dev \
    libxkbcommon-x11-0 \
    libfontconfig1 \
    libfreetype6 \
    libdbus-1-3 \
    libx11-6 \
    libx11-xcb1 \
    libxcb1 \
    libxcb-cursor0 \
    libxcb-glx0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render0 \
    libxcb-render-util0 \
    libxcb-shape0 \
    libxcb-shm0 \
    libxcb-sync1 \
    libxcb-xfixes0 \
    libxcb-xinerama0 \
    libxcb-xkb1 \
    xvfb

RUN python3 -m venv /opt/venv

RUN /opt/venv/bin/pip install "aqtinstall<3.3"

RUN /opt/venv/bin/pip install "conan>=2,<3"

ENV PATH="/opt/venv/bin:${PATH}"

RUN case "$ACTION_RUNNER_ARCH" in \
        x64) \
            if [ "$QT_VERSION" = "6.2.3" ]; then \
                AQT_ARCH="gcc_64"; \
            else \
                AQT_ARCH="linux_gcc_64"; \
            fi ;; \
        arm64) AQT_ARCH="linux_gcc_arm64" ;; \
        *)     echo "Unsupported arch: $ACTION_RUNNER_ARCH" && exit 1 ;; \
    esac && \
    if [ "$QT_VERSION" = "6.2.3" ]; then \
        aqt install-qt linux desktop "$QT_VERSION" "$AQT_ARCH" -O "$QT_DIR" \
            ${QT_MODULES:+--modules $QT_MODULES}; \
    else \
        aqt install-qt linux desktop "$QT_VERSION" "$AQT_ARCH" -O "$QT_DIR" \
            ${QT_ARCHIVES:+--archives $QT_ARCHIVES} \
            ${QT_MODULES:+--modules $QT_MODULES}; \
    fi && \
    ln -s "$(find "$QT_DIR/$QT_VERSION" -maxdepth 1 -mindepth 1 -type d)" "$QT_DIR/current"

ENV QT_VERSION=${QT_VERSION}
ENV QT_DIR=${QT_DIR}
ENV PATH="${QT_DIR}/current/bin:${PATH}"
ENV LD_LIBRARY_PATH="${QT_DIR}/current/lib"
ENV CMAKE_PREFIX_PATH="${QT_DIR}/current"

# Prepare the runner user and working directory
RUN mkdir -p "$WORKING_DIR"
RUN useradd --home-dir "$WORKING_DIR" -M -s /bin/bash $USERNAME
RUN chown $USERNAME:$USERNAME $WORKING_DIR
WORKDIR $WORKING_DIR

USER $USERNAME

# The github token is here to avoid getting rate limited by the GitHub API when downloading the runner (60 requests per hour for unauthenticated requests, see https://docs.github.com/en/rest/using-the-rest-api/rate-limits-for-the-rest-api?apiVersion=2026-03-10#primary-rate-limit-for-unauthenticated-users)
RUN --mount=type=secret,id=github_token,mode=0444 \
    export RUNNER_ARCHIVE="actions-runner-linux-$ACTION_RUNNER_ARCH-$ACTION_RUNNER_TAG.tar.gz" && \
    export RUNNER_RELEASE_API="https://api.github.com/repos/actions/runner/releases/tags/v$ACTION_RUNNER_TAG" && \
    export RUNNER_SHA256=$(curl -fsSL \
        -H "Accept: application/vnd.github+json" \
        -H "Authorization: Bearer $(cat /run/secrets/github_token)" \
        "$RUNNER_RELEASE_API" | jq -r --arg name "$RUNNER_ARCHIVE" '.assets[] | select(.name == $name) | .digest | sub("^sha256:"; "")') && \
    curl -O -L "https://github.com/actions/runner/releases/download/v$ACTION_RUNNER_TAG/$RUNNER_ARCHIVE" && \
    echo "$RUNNER_SHA256  $RUNNER_ARCHIVE" | shasum -a 256 -c && \
    tar xzf "./$RUNNER_ARCHIVE" && \
    rm "./$RUNNER_ARCHIVE"

# Install runner dependencies as root
USER root
RUN ./bin/installdependencies.sh
USER $USERNAME

COPY container-entrypoint.sh entrypoint.sh
ENTRYPOINT ["/opt/actions-runner/entrypoint.sh"]
