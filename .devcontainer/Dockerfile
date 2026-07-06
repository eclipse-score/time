# Use Dockerfile to get Dependabot version bumps after new image is released
FROM ghcr.io/eclipse-score/devcontainer:v1.8.0

ARG USERNAME=vscode

# Rename 'vscode' to the host username for QNX-related user expectations.
# In environments using a license server, the QNX license might be bound to a username.
RUN if [ "$USERNAME" != "vscode" ]; then \
    usermod -l ${USERNAME} vscode \
    && groupmod -n ${USERNAME} vscode \
    && usermod -d /home/${USERNAME} -m ${USERNAME} \
    && ln -s /home/${USERNAME} /home/vscode \
    && echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/${USERNAME} \
    && chmod 0440 /etc/sudoers.d/${USERNAME}; \
  fi

USER ${USERNAME}
