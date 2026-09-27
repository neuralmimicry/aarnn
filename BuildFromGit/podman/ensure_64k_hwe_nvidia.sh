#!/usr/bin/env bash
set -euo pipefail

aarnn_ensure_64k_hwe_nvidia() {
  local requested="${1:-${AARNN_ENABLE_GPU:-false}}"
  [[ "$(getconf PAGESIZE 2>/dev/null || printf 0)" == "65536" ]] || return 0
  [[ -r /etc/os-release ]] || return 0
  # shellcheck disable=SC1091
  . /etc/os-release
  [[ "${ID:-}" == ubuntu && "${VERSION_ID:-}" == 24.04 ]] || return 0
  if [[ "${requested,,}" != true && "${requested,,}" != 1 ]] &&
     ! command -v nvidia-smi >/dev/null 2>&1 && [[ ! -e /dev/nvidiactl ]]; then
    return 0
  fi
  dpkg-query -W -f='${Status}' linux-nvidia-64k-hwe-24.04 2>/dev/null |
    grep -q 'install ok installed' && return 0
  if [[ "${EUID}" -eq 0 ]]; then
    apt-get update && apt-get install -y linux-nvidia-64k-hwe-24.04
  else
    sudo apt-get update && sudo apt-get install -y linux-nvidia-64k-hwe-24.04
  fi
}
