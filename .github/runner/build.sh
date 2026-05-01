#!/usr/bin/env bash
set -euo pipefail

VALID_QT_VERSIONS=("6.2.3" "6.5.3" "6.8.3")
VALID_UBUNTU_VERSIONS=("22.04" "24.04" "26.04")

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

usage() {
    cat <<EOF
Usage: $0 -u <ubuntu-version> -q <qt-version>

  -u  Ubuntu version. Valid values: ${VALID_UBUNTU_VERSIONS[*]}
  -q  Qt version.     Valid values: ${VALID_QT_VERSIONS[*]}

The GitHub token is read from the GITHUB_TOKEN environment variable,
or from $SCRIPT_DIR/github_pat.env if the variable is not set.

Required GitHub PAT permissions:
  Classic PAT  : repo
  Fine-grained : Repository > Administration (Read and write)

Example:
  $0 -u 24.04 -q 6.8.3
  GITHUB_TOKEN=ghp_xxx $0 -u 22.04 -q 6.5.3
EOF
    exit 1
}

UBUNTU_VERSION=""
QT_VERSION=""

while getopts "u:q:h" opt; do
    case $opt in
        u) UBUNTU_VERSION="$OPTARG" ;;
        q) QT_VERSION="$OPTARG" ;;
        h) usage ;;
        *) usage ;;
    esac
done

if [[ -z "$UBUNTU_VERSION" || -z "$QT_VERSION" ]]; then
    echo "Error: -u and -q are both required" >&2
    usage
fi

contains() {
    local value="$1"; shift
    for item in "$@"; do [[ "$item" == "$value" ]] && return 0; done
    return 1
}

if ! contains "$UBUNTU_VERSION" "${VALID_UBUNTU_VERSIONS[@]}"; then
    echo "Error: invalid Ubuntu version '$UBUNTU_VERSION'" >&2
    echo "Valid values: ${VALID_UBUNTU_VERSIONS[*]}" >&2
    exit 1
fi

if ! contains "$QT_VERSION" "${VALID_QT_VERSIONS[@]}"; then
    echo "Error: invalid Qt version '$QT_VERSION'" >&2
    echo "Valid values: ${VALID_QT_VERSIONS[*]}" >&2
    exit 1
fi

SECRET_ARG=""
if [[ -n "${GITHUB_TOKEN:-}" ]]; then
    SECRET_ARG="--secret id=github_token,env=GITHUB_TOKEN"
elif [[ -f "$SCRIPT_DIR/github_pat.env" ]]; then
    SECRET_ARG="--secret id=github_token,src=$SCRIPT_DIR/github_pat.env"
else
    echo "Error: GITHUB_TOKEN is not set and $SCRIPT_DIR/github_pat.env does not exist." >&2
    echo "       Provide a GitHub PAT via GITHUB_TOKEN env var or place it in $SCRIPT_DIR/github_pat.env" >&2
    exit 1
fi

TAG="matomo-sdk-qt-runner:ubuntu${UBUNTU_VERSION}-qt${QT_VERSION}"

echo "Building $TAG..."
docker build \
    $SECRET_ARG \
    --build-arg UBUNTU_VERSION="$UBUNTU_VERSION" \
    --build-arg QT_VERSION="$QT_VERSION" \
    --tag "$TAG" \
    -f "$SCRIPT_DIR/Dockerfile" \
    "$SCRIPT_DIR/"

echo "Done: $TAG"
