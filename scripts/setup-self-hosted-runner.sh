#!/usr/bin/env bash
# Register this machine as a GitHub Actions self-hosted runner for the
# cpp-runtime-inspector repo. The CI workflow's `pull-local` job runs on this
# runner so newly-built images are pulled the moment the publish job finishes.
#
# Prerequisites:
#   - docker installed and the current user can run `docker` without sudo
#   - `docker compose` plugin installed
#   - A registration token from:
#       https://github.com/imkiptoo/cpp-runtime-inspector/settings/actions/runners/new
#     (The token is short-lived; grab it right before running this script.)

set -euo pipefail

REPO_URL="https://github.com/imkiptoo/cpp-runtime-inspector"
RUNNER_DIR="${HOME}/actions-runner"
RUNNER_LABEL="local-deploy"
RUNNER_NAME="$(hostname)"
RUNNER_VERSION="2.321.0"
RUNNER_ARCH="x64"

if ! command -v docker >/dev/null; then
  echo "ERROR: docker is required but not found in PATH" >&2
  exit 1
fi

if ! docker info >/dev/null 2>&1; then
  echo "ERROR: cannot talk to the docker daemon as $(whoami)." >&2
  echo "       Add yourself to the 'docker' group: sudo usermod -aG docker \$USER" >&2
  exit 1
fi

read -rsp "Paste the GitHub runner registration token: " REG_TOKEN
echo
if [[ -z "${REG_TOKEN}" ]]; then
  echo "ERROR: empty token" >&2
  exit 1
fi

mkdir -p "${RUNNER_DIR}"
cd "${RUNNER_DIR}"

if [[ ! -f ./config.sh ]]; then
  echo "==> Downloading actions-runner v${RUNNER_VERSION}"
  TARBALL="actions-runner-linux-${RUNNER_ARCH}-${RUNNER_VERSION}.tar.gz"
  curl -fsSL -o "${TARBALL}" \
    "https://github.com/actions/runner/releases/download/v${RUNNER_VERSION}/${TARBALL}"
  tar xzf "${TARBALL}"
  rm "${TARBALL}"
fi

echo "==> Configuring runner '${RUNNER_NAME}' with label '${RUNNER_LABEL}'"
./config.sh \
  --unattended \
  --url "${REPO_URL}" \
  --token "${REG_TOKEN}" \
  --name "${RUNNER_NAME}" \
  --labels "${RUNNER_LABEL}" \
  --replace

echo "==> Installing systemd service"
sudo ./svc.sh install "$(whoami)"
sudo ./svc.sh start

echo
echo "Runner registered. Status:"
sudo ./svc.sh status || true

cat <<EOF

Next steps:
  1. Confirm the runner shows as 'Idle' at:
       ${REPO_URL}/settings/actions/runners
  2. Push to main. The 'pull-local' job will run on this machine and:
       - docker pull both images from ghcr.io
       - docker compose up -d (using docker-compose.yml in the workflow checkout)

Service control:
  cd ${RUNNER_DIR}
  sudo ./svc.sh status   # check
  sudo ./svc.sh stop     # pause runner
  sudo ./svc.sh start    # resume
  sudo ./svc.sh uninstall  # remove service (then ./config.sh remove --token <token>)
EOF
