#!/usr/bin/env bash
set -euo pipefail

. /etc/os-release

RUNNER_REPO="${RUNNER_REPO:-Infomaniak/matomo-sdk-qt}"
GITHUB_TOKEN="${GITHUB_TOKEN:?GITHUB_TOKEN environment variable is required}"
RUNNER_NAME="${RUNNER_NAME:-${ID}-${VERSION_ID}-qt${QT_VERSION}}"
RUNNER_LABELS="${RUNNER_LABELS:-${ID}-${VERSION_ID},qt-${QT_VERSION}}"

EXISTING_RUNNER_ID=$(curl -fsSL \
    -H "Authorization: Bearer $GITHUB_TOKEN" \
    -H "Accept: application/vnd.github+json" \
    "https://api.github.com/repos/$RUNNER_REPO/actions/runners" \
    | jq -r --arg name "$RUNNER_NAME" '.runners[] | select(.name == $name) | .id')

if [ -n "$EXISTING_RUNNER_ID" ]; then
    curl -fsSL -X DELETE \
        -H "Authorization: Bearer $GITHUB_TOKEN" \
        -H "Accept: application/vnd.github+json" \
        "https://api.github.com/repos/$RUNNER_REPO/actions/runners/$EXISTING_RUNNER_ID"
fi

RUNNER_TOKEN=$(curl -fsSL -X POST \
    -H "Authorization: Bearer $GITHUB_TOKEN" \
    -H "Accept: application/vnd.github+json" \
    "https://api.github.com/repos/$RUNNER_REPO/actions/runners/registration-token" \
    | jq -r .token)

./config.sh \
    --unattended \
    --url "https://github.com/$RUNNER_REPO" \
    --token "$RUNNER_TOKEN" \
    --name "$RUNNER_NAME" \
    --labels "$RUNNER_LABELS"

cleanup() {
    REMOVAL_TOKEN=$(curl -fsSL -X POST \
        -H "Authorization: Bearer $GITHUB_TOKEN" \
        -H "Accept: application/vnd.github+json" \
        "https://api.github.com/repos/$RUNNER_REPO/actions/runners/remove-token" \
        | jq -r .token)
    ./config.sh remove --token "$REMOVAL_TOKEN"
}
trap cleanup EXIT

./run.sh
