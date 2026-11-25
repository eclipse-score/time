#!/bin/bash
# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************

# Prepares a Debian/Ubuntu host to run the QEMU integration tests, e.g.
#   bazel test --config=qemu //score/tests/feature:test_feature_qemu_linux_x86_64
#
# - installs the QEMU and cloud-image tools used by the image build and test plugin
# - makes /dev/kvm accessible to the current user by adding them to the group that
#   owns /dev/kvm (creating a local group with the same GID if needed)
#
# Re-running the script is safe. Group membership only applies to new login
# sessions, so log out and back in (or use `sg kvm -c "<command>"`) afterwards.

set -euo pipefail

PACKAGES=(
  qemu-system-x86
  qemu-system-arm
  qemu-utils
  cloud-image-utils
  python3
)

SUDO=""
if [[ "$(id -u)" -ne 0 ]]; then
  if ! command -v sudo >/dev/null 2>&1; then
    echo "error: run as root or install sudo" >&2
    exit 1
  fi
  SUDO="sudo"
fi

echo "Installing packages: ${PACKAGES[*]}"
${SUDO} apt-get update
${SUDO} env DEBIAN_FRONTEND=noninteractive apt-get install -y "${PACKAGES[@]}"

if [[ ! -e /dev/kvm ]]; then
  echo "warning: /dev/kvm does not exist; QEMU will fall back to software emulation (TCG)" >&2
  echo "         and the test boot may exceed the ping timeout. Check virtualization support." >&2
  exit 0
fi

KVM_GID="$(stat -c '%g' /dev/kvm)"
KVM_GROUP="$(getent group "${KVM_GID}" | cut -d: -f1 || true)"

if [[ -z "${KVM_GROUP}" ]]; then
  # The device's group is not known inside this system (e.g. a host GID that is not
  # mapped into the container). Create a local group with the same numeric GID.
  KVM_GROUP="kvm"
  if getent group "${KVM_GROUP}" >/dev/null; then
    echo "error: group name '${KVM_GROUP}' already exists with a different GID than /dev/kvm (${KVM_GID})" >&2
    exit 1
  fi
  ${SUDO} groupadd -g "${KVM_GID}" "${KVM_GROUP}"
fi

CURRENT_USER="$(id -un)"
if [[ " $(id -nG "${CURRENT_USER}") " == *" ${KVM_GROUP} "* ]]; then
  echo "${CURRENT_USER} is already in group ${KVM_GROUP} (GID ${KVM_GID})"
else
  ${SUDO} usermod -aG "${KVM_GROUP}" "${CURRENT_USER}"
  echo "Added ${CURRENT_USER} to group ${KVM_GROUP} (GID ${KVM_GID})"
fi

echo "Done. Log out and back in, or run commands with: sg ${KVM_GROUP} -c \"<command>\""
