#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMPOSE_FILE="$SCRIPT_DIR/compose.yaml"

usage() {
    cat <<EOF
Usage: $0 [service ...]

Build runner images one after another to avoid Docker/BuildKit disk pressure.

If no service is provided, every service from compose.yaml is built sequentially.

Examples:
  $0
  $0 ubuntu-22-04-qt-6-2-3 ubuntu-26-04-qt-6-8-3
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

if [[ -z "${GITHUB_TOKEN:-}" && -f "$SCRIPT_DIR/github_pat.env" ]]; then
    export GITHUB_TOKEN
    GITHUB_TOKEN="$(<"$SCRIPT_DIR/github_pat.env")"
fi

if [[ "$#" -gt 0 ]]; then
    services=("$@")
else
    services=()
    while IFS= read -r service; do
        services+=("$service")
    done < <(docker compose -f "$COMPOSE_FILE" config --services)
fi

if [[ "${#services[@]}" -eq 0 ]]; then
    echo "Error: no services to build." >&2
    exit 1
fi

for service in "${services[@]}"; do
    echo
    echo "==> Building $service"
    docker compose -f "$COMPOSE_FILE" build "$service"
    echo "==> Done $service"
done

echo
echo "All requested runner images were built."
