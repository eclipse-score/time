#!/bin/bash

# Configuration
TIME_SHIFT_MINUTES=5
NTP_SERVICE="systemd-timesyncd" # Change this to your NTP service: chronyd, ntp, systemd-timesyncd

# --- Function to display usage ---
usage() {
  echo "Usage: $0 [forward|backward]"
  echo "  'forward'  : Moves the system clock forward by ${TIME_SHIFT_MINUTES} minutes."
  echo "  'backward' : Moves the system clock backward by ${TIME_SHIFT_MINUTES} minutes."
  exit 1
}

# --- Ensure script is run as root ---
if [ "$EUID" -ne 0 ]; then
  echo "Error: Please run as root (sudo)."
  exit 1
fi

# --- Process command-line arguments ---
if [ -z "$1" ]; then
  usage
fi

DIRECTION="$1"
if [ "$DIRECTION" != "forward" ] && [ "$DIRECTION" != "backward" ]; then
  usage
fi

# --- Stop NTP/Time Sync Service ---
echo "Attempting to stop time synchronization service: ${NTP_SERVICE}..."
systemctl stop "${NTP_SERVICE}" &>/dev/null || {
  echo "Warning: Could not stop ${NTP_SERVICE}. It might not be installed, running, or you need to adjust the NTP_SERVICE variable."
  echo "Continuing without stopping NTP, but time changes might be reverted quickly."
}
sleep 1 # Give it a moment to stop

# --- Get Current Time in seconds since epoch ---
CURRENT_EPOCH_SECONDS=$(date +%s)
echo "Current system time: $(date -d "@${CURRENT_EPOCH_SECONDS}")"

# --- Calculate New Time ---
SHIFT_SECONDS=$((TIME_SHIFT_MINUTES * 60))
NEW_EPOCH_SECONDS=0

if [ "$DIRECTION" == "forward" ]; then
  NEW_EPOCH_SECONDS=$((CURRENT_EPOCH_SECONDS + SHIFT_SECONDS))
  echo "Calculating new time: ${TIME_SHIFT_MINUTES} minutes forward..."
elif [ "$DIRECTION" == "backward" ]; then
  NEW_EPOCH_SECONDS=$((CURRENT_EPOCH_SECONDS - SHIFT_SECONDS))
  echo "Calculating new time: ${TIME_SHIFT_MINUTES} minutes backward..."
fi

NEW_TIME_STRING=$(date -d "@${NEW_EPOCH_SECONDS}" +"%Y-%m-%d %H:%M:%S")

# --- Set New System Time ---
echo "Changing system time to: ${NEW_TIME_STRING}"
date -s "$NEW_TIME_STRING"
echo "Verification: $(date)"

# --- Prompt for user action or a short wait ---
echo ""
echo "The system time has been shifted. Perform your tests now."
echo "Press Enter to revert the time, or wait 10 seconds for automatic revert..."
read -t 10 -p "" && REVERT_NOW=true

if [ -z "$REVERT_NOW" ]; then
  echo "Automatic revert initiated after 10 seconds."
fi

# --- Restart NTP/Time Sync Service ---
echo ""
echo "Attempting to restart time synchronization service: ${NTP_SERVICE}..."
systemctl start "${NTP_SERVICE}" &>/dev/null || {
  echo "Error: Could not restart ${NTP_SERVICE}. You may need to manually restart it or set time with 'sudo date -s \"$(date)\"' to current."
}

echo "Time synchronization service restarted. System time should now be accurate."
echo "Final verification: $(date)"
echo "Done."
