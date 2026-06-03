# Self-hosted runner

Docker image for the GitHub Actions self-hosted runner, with Qt pre-installed.

## Prerequisites

- Docker with BuildKit (default since Docker 23+)
- A GitHub Personal Access Token (PAT) with the following permissions:
  - **Classic PAT** : `repo`
  - **Fine-grained PAT** : Repository > `Administration` (Read and write)

## Build the image

Use the provided script:

```bash
.github/runner/build.sh -u <ubuntu-version> -q <qt-version>
```

The script reads the GitHub token from the `GITHUB_TOKEN` environment variable, or from `.github/runner/github_pat.env` if the variable is not set.

```bash
# Using an env var
GITHUB_TOKEN=ghp_xxx .github/runner/build.sh -u 24.04 -q 6.8.3

# Using a file (place your raw token in this file)
echo "ghp_xxx" > .github/runner/github_pat.env
.github/runner/build.sh -u 24.04 -q 6.8.3
```

Run `.github/runner/build.sh -h` for the full list of supported versions.

## Build and run the runner fleet

The Compose file starts the CI runners used by the current matrix:

- Ubuntu 22.04 + Qt 6.2.3
- Ubuntu 22.04 + Qt 6.8.3
- Ubuntu 22.04 + Qt 6.11.1
- Ubuntu 26.04 + Qt 6.2.3
- Ubuntu 26.04 + Qt 6.8.3
- Ubuntu 26.04 + Qt 6.11.1

Each runner image needs roughly 5 GB of Docker/containerd storage while building.
Make sure the Docker root directory and the containerd snapshotter storage have
enough free space for the number of runners you build.

```bash
export GITHUB_TOKEN=ghp_xxx
docker compose -f .github/runner/compose.yaml up -d --build
```

If you keep the token in `.github/runner/github_pat.env`, export it first:

```bash
export GITHUB_TOKEN="$(cat .github/runner/github_pat.env)"
docker compose -f .github/runner/compose.yaml up -d --build
```

Stop and deregister the runners:

```bash
docker compose -f .github/runner/compose.yaml down
```

## Run the container

```bash
docker run -d \
  -e GITHUB_TOKEN=ghp_xxx \
  -e RUNNER_REPO=<your-github-username>/matomo-sdk-qt \
  matomo-sdk-qt-runner:ubuntu26.04-qt6.8.3
```

The container registers itself as a self-hosted runner on startup and deregisters cleanly on `docker stop`.

## CI setup on a fork

The CI workflow targets runners by label. Start one container per matrix entry you want to support:

| Image tag              | Labels matched                         |
|------------------------|----------------------------------------|
| `ubuntu22.04-qt6.2.3`  | `self-hosted, ubuntu-22.04, qt-6.2.3`  |
| `ubuntu22.04-qt6.8.3`  | `self-hosted, ubuntu-22.04, qt-6.8.3`  |
| `ubuntu22.04-qt6.11.1` | `self-hosted, ubuntu-22.04, qt-6.11.1` |
| `ubuntu26.04-qt6.2.3`  | `self-hosted, ubuntu-26.04, qt-6.2.3`  |
| `ubuntu26.04-qt6.8.3`  | `self-hosted, ubuntu-26.04, qt-6.8.3`  |
| `ubuntu26.04-qt6.11.1` | `self-hosted, ubuntu-26.04, qt-6.11.1` |

To skip a combination, remove the corresponding entry from `matrix.include` in `.github/workflows/ci.yml`.
