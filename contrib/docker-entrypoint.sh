#!/usr/bin/env bash
set -euo pipefail
umask 077

DATA_DIR="${EQUALISHCOIN_DATA_DIR:-/home/equalishcoin/.equalishcoin}"
CONF_FILE="${EQUALISHCOIN_CONF:-$DATA_DIR/equalishcoin.conf}"

RPC_USER="${COIND_RPC_USER:-eqlsrpc}"
RPC_PASSWORD="${COIND_RPC_PASSWORD:-changeme}"
P2P_PORT="${COIND_P2P_PORT:-19901}"
RPC_PORT="${COIND_RPC_PORT:-19902}"
RPC_BIND="${COIND_RPC_BIND:-0.0.0.0}"
RPC_ALLOW_IP="${COIND_RPC_ALLOW_IP:-172.16.0.0/12}"
TXINDEX="${COIND_TXINDEX:-1}"
PRUNE="${COIND_PRUNE:-0}"
MAXCONNECTIONS="${COIND_MAXCONNECTIONS:-64}"
EXTRA_ARGS="${COIND_EXTRA_ARGS:-}"

DEFAULT_BOOTSTRAP_NODES=(
  "seed1.equalishcoin.invalid:19901"
  "seed2.equalishcoin.invalid:19901"
)

BOOTSTRAP_NODES=()
if [[ -n "${COIND_BOOTSTRAP_NODES:-}" ]]; then
  IFS=',' read -r -a BOOTSTRAP_NODES <<< "${COIND_BOOTSTRAP_NODES}"
else
  BOOTSTRAP_NODES=("${DEFAULT_BOOTSTRAP_NODES[@]}")
fi

mkdir -p "${DATA_DIR}"
if [[ ! -w "${DATA_DIR}" ]]; then
  echo "Data dir is not writable: ${DATA_DIR}" >&2
  exit 1
fi

if [[ ! -f "${CONF_FILE}" ]]; then
  cat > "${CONF_FILE}" <<EOF
server=1
daemon=0
listen=1
printtoconsole=1

rpcuser=${RPC_USER}
rpcpassword=${RPC_PASSWORD}
rpcbind=${RPC_BIND}
rpcallowip=${RPC_ALLOW_IP}
rpcport=${RPC_PORT}

port=${P2P_PORT}
txindex=${TXINDEX}
prune=${PRUNE}
maxconnections=${MAXCONNECTIONS}
wallet=wallet.dat
EOF
fi

for node in "${BOOTSTRAP_NODES[@]}"; do
  node="${node//[[:space:]]/}"
  [[ -z "${node}" ]] && continue
  line="addnode=${node}"
  if ! grep -Fxq "${line}" "${CONF_FILE}"; then
    echo "${line}" >> "${CONF_FILE}"
  fi
done

chmod 600 "${CONF_FILE}"

if [[ "${1:-}" == "equalishcoind" ]]; then
  exec equalishcoind \
    -datadir="${DATA_DIR}" \
    -conf="${CONF_FILE}" \
    ${EXTRA_ARGS}
fi

exec "$@"