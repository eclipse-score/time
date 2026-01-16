#!/bin/bash
# (c) 2025 ETAS GmbH. All rights reserved.

SESSION_NAME="score_time_demo"
APP_DIR="$(pwd)"

# --- Step 1: Build all applications with Bazel ---
echo "Building all applications with Bazel..."
bazel build ///...

if [ $? -ne 0 ]; then
    echo "Bazel build failed. Exiting."
    exit 1
fi

echo "Bazel build successful!"

echo "Cleaning up in case there are leftovers..."
sudo killall ptpd
sudo killall tsync_daemon
sudo rm -f /dev/shm/sem.time_domain_1
sudo rm -f /dev/shm/sysclock
sudo rm -f /dev/shm/tsync_id_mappings

echo "Starting tmux session..."
sleep 2

# --- Step 2: Create a new tmux session and window ---
tmux new-session -d -s $SESSION_NAME -n "Eclipse S-Core: TIME demo"

tmux set -g pane-border-status top

# --- Step 3: Split the window into 4 panes ---
# Layout:
# --------------------
# |      P1          |
# |                  |
# --------------------
# | P2   | P3   | P4 |
# --------------------

# Split top/bottom (splits into two horizontal panes)
# Pane 0 (top), Pane 1 (bottom)
tmux split-window -v -t $SESSION_NAME:0.0

# Select the bottom pane (Pane 1) and split it vertically into three
# This will result in:
# Pane 0 (top)
# Pane 1 (bottom left)
# Pane 2 (bottom middle)
tmux split-window -h -t $SESSION_NAME:0.1
tmux split-window -h -t $SESSION_NAME:0.2

# Now we have 4 panes:
# Pane 0: Top (for get_current_time)
# Pane 1: Bottom-Left (for tsync-daemon)
# Pane 2: Bottom-Middle (for ptpd_master)
# Pane 3: Bottom-Right (for ptpd_slave)

# --- Step 4: Run each application in its respective pane ---

# Pane 0: get_current_time
tmux select-pane -t $SESSION_NAME:0.0 -T "get_current_time.cpp"
tmux send-keys -t $SESSION_NAME:0.0 "echo '--- get_current_time (Application) ---'" C-m
tmux send-keys -t $SESSION_NAME:0.0 "cd $APP_DIR && sleep 2" C-m
tmux send-keys -t $SESSION_NAME:0.0 "sudo -E bazel-bin/examples/get_current_time" C-m

# Pane 1: tsync-daemon
tmux select-pane -t $SESSION_NAME:0.1 -T "TSYNC-DAEMON"
tmux send-keys -t $SESSION_NAME:0.1 "echo '--- tsync-daemon ---'" C-m
tmux send-keys -t $SESSION_NAME:0.1 "cd $APP_DIR" C-m
tmux send-keys -t $SESSION_NAME:0.1 "export ECUCFG_ENV_VAR_ROOTFOLDER=$APP_DIR/bazel-out/k8-fastbuild/bin/src/tsync-daemon/src/score/time/daemon/" C-m
tmux send-keys -t $SESSION_NAME:0.1 "sudo -E bazel-bin/src/tsync-daemon/tsync_daemon" C-m

# Pane 2: ptpd_master
tmux select-pane -t $SESSION_NAME:0.2 -T "PTP MASTER (e.g. on µC Classic domain)"
tmux send-keys -t $SESSION_NAME:0.2 "echo '--- ptpd master ---'" C-m
tmux send-keys -t $SESSION_NAME:0.2 "cd $APP_DIR && sleep 1" C-m
tmux send-keys -t $SESSION_NAME:0.2 "sudo bazel-bin/src/ptpd/ptpd -i eth0 -d 1 --global:foreground=Y -M --ptpengine:transport=ethernet --ptpengine:delay_mechanism=DELAY_DISABLED --ptpengine:disable_bmca=y --score:globaltimepropagationdelay=0.0 -L --ptpengine:dot1as=1 --clock:no_adjust=Y -V" C-m

# Pane 3: ptpd_slave
tmux select-pane -t $SESSION_NAME:0.3 -T "PTP SLAVE (e.g. on S-Core domain)"
tmux send-keys -t $SESSION_NAME:0.3 "echo '--- ptpd slave ---'" C-m
tmux send-keys -t $SESSION_NAME:0.3 "cd $APP_DIR && sleep 1" C-m
tmux send-keys -t $SESSION_NAME:0.3 "sudo bazel-bin/src/ptpd/ptpd -i eth0 -d 1 --global:foreground=Y -s --ptpengine:transport=ethernet --ptpengine:delay_mechanism=DELAY_DISABLED --ptpengine:disable_bmca=y --score:globaltimepropagationdelay=0.0 -L --ptpengine:dot1as=1 --clock:no_adjust=Y -V" C-m

# --- Attach to the tmux session ---
echo "Attaching to tmux session '$SESSION_NAME'. Press Ctrl+B D to detach."
tmux attach-session -t $SESSION_NAME
