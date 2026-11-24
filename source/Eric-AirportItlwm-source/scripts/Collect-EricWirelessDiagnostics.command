#!/bin/bash

# Eric wireless diagnostics for Wi-Fi, AirDrop, Instant Hotspot and Bluetooth.
export LC_ALL=C COPYFILE_DISABLE=1
umask 077

if [ "$(/usr/bin/uname -s)" != Darwin ]; then
    echo "Run this collector on the affected Mac."
    exit 1
fi
if [ "$(/usr/bin/id -u)" -ne 0 ]; then
    echo "Administrator access is used to read wireless diagnostics."
    exec /usr/bin/sudo /bin/bash "$0" "$@"
fi

case "${1:-}" in
    --bluetooth) focus=bluetooth ;;
    "")
        printf '1. Wi-Fi\n2. AirDrop or Instant Hotspot\n3. Bluetooth device discovery\nChoose [1-3]: '
        IFS= read -r answer || exit 1
        case "$answer" in
            1) focus=wifi ;;
            2) focus=continuity ;;
            3) focus=bluetooth ;;
            *) echo "Choose 1, 2 or 3."; exit 1 ;;
        esac ;;
    *) echo "Usage: $0 [--bluetooth]"; exit 1 ;;
esac
base="$(cd -- "$(dirname "$0")" && pwd)" || exit 1
if [ ! -w "$base" ]; then
    echo "Move this command to a writable folder and try again."
    exit 1
fi
work="$(/usr/bin/mktemp -d "${TMPDIR:-/tmp}/EricWirelessDiagnostics.XXXXXX")" || exit 1
stream_pid=
timeline_pid=

stop_recording() {
    local pid
    for pid in "$stream_pid" "$timeline_pid"; do
        if [ -n "$pid" ]; then
            /bin/kill "$pid" 2>/dev/null
            wait "$pid" 2>/dev/null
        fi
    done
    stream_pid=
    timeline_pid=
}
trap 'stop_recording; /bin/rm -r -- "$work"' EXIT
trap 'exit 130' INT TERM

bounded() {
    (ulimit -f 512; "$@")
}
stamp() {
    /bin/date '+%Y-%m-%d %H:%M:%S %z'
}

boot_build="$(/usr/bin/sw_vers -buildVersion)"
os_version="$(/usr/bin/sw_vers -productVersion)"
(
    ulimit -f 256
    echo "Collector revision: 2"
    echo "Report: $focus"
    stamp
    /usr/bin/sw_vers
    /usr/bin/uname -m -r
    /usr/sbin/sysctl kern.bootargs kern.boottime hw.model
    echo
    echo '[Loaded kext version and UUID]'
    /usr/bin/kmutil showloaded 2>&1 | /usr/bin/grep -iE 'Index|AirportItlwm|itlwm|Lilu|BlueToolFixup|BTLEAdvFixup|IntelBT|BrcmPatchRAM|IO80211|IOSkywalk|IOBluetooth'
    echo
    echo '[Boot source]'
    /usr/sbin/ioreg -p IODeviceTree -r -n chosen -d 1 -l -w 0 2>/dev/null |
        /usr/bin/grep -E '"(boot-path|boot-file|boot-device-path|boot-uuid)"'
    /usr/sbin/nvram opencore-version 2>/dev/null || :
    echo
    echo '[Wi-Fi PCI identity]'
    /usr/sbin/ioreg -p IOService -r -c IOPCIDevice -d 1 -l -w 0 2>/dev/null |
        /usr/bin/awk '
            function emit() { if (wireless) { print node; printf "%s", props } }
            /\+-o / { emit(); node=$0; props=""; wireless=0 }
            /"class-code" = <[[:xdigit:]][[:xdigit:]]800200>/ { wireless=1 }
            /"(vendor-id|device-id|subsystem-id|subsystem-vendor-id|class-code|IOName|model)"/ {
                props=props $0 "\n"
            }
            END { emit() }
        '
    echo
    echo '[System framework versions]'
    for name in IO80211Family IOSkywalkFamily IOBluetoothFamily; do
        printf '%s ' "$name"
        /usr/bin/plutil -extract CFBundleVersion raw -o - "/System/Library/Extensions/$name.kext/Contents/Info.plist" 2>/dev/null || echo unavailable
    done
    echo
    echo '[Adjacent package files]'
    for bundle in "$base"/AirportItlwm*.kext "$base"/BlueToolFixup.kext; do
        info="$bundle/Contents/Info.plist"
        [ -f "$info" ] || continue
        printf '%s ' "$(basename "$bundle")"
        /usr/bin/plutil -extract CFBundleVersion raw -o - "$info" 2>/dev/null || :
        binary="$(/usr/bin/plutil -extract CFBundleExecutable raw -o - "$info" 2>/dev/null)"
        [ -n "$binary" ] && /usr/bin/shasum -a 256 "$bundle/Contents/MacOS/$binary" | /usr/bin/awk '{ print $1 }'
    done
) > "$work/environment.txt" 2>&1

wifi_interfaces="$(/usr/sbin/networksetup -listallhardwareports 2>/dev/null |
    /usr/bin/awk '/Hardware Port: (Wi-Fi|AirPort)/ { getline; print $2 }')"

registry_state() {
    /usr/sbin/ioreg -p IOService -l -w 0 2>/dev/null |
        /usr/bin/grep -aE '"(AirportItlwm-[^"]+|AWDL[^"]*|ITLAWDL[^"]*|BTLEAdv[^"]*|BluetoothFirmwareVersion)"[[:space:]]*=' |
        /usr/bin/cut -c 1-2500
}
snapshot() {
    stamp
    echo '[Wi-Fi and AWDL]'
    /usr/bin/wdutil info
    for interface in $wifi_interfaces awdl0 llw0; do
        /sbin/ifconfig "$interface"
        /usr/sbin/netstat -I "$interface" -b
    done
    echo '[Bluetooth controller]'
    /usr/sbin/system_profiler -timeout 10 -detailLevel basic SPBluetoothDataType
    echo '[Driver properties]'
    registry_state
}
kernel_lines() {
    /sbin/dmesg 2>&1 | /usr/bin/grep -aiE 'itlwm|awdl|BlueToolFixup|btlfx|IntelBT|ibtfw|BTLEAdv|\[advsets\]|\[rpa\]|\[pagein\]|permission|not permitted' |
        /usr/bin/tail -n 900
}

echo "Eric Wireless Diagnostics. Reports can contain network and device names. Send them privately."
printf 'Describe the problem and, for AirDrop, the sending device: '
IFS= read -r problem
printf '%s\n' "$problem" > "$work/attempt.txt"
echo "Recording will start after the initial snapshot."
bounded snapshot > "$work/before.txt" 2>&1
bounded kernel_lines > "$work/kernel-before.txt" 2>&1

predicate='process IN {"bluetoothd","wifid","airportd","wifip2pd","sharingd","rapportd","nearbyd","eapolclient","IPConfiguration"} OR (process == "kernel" AND (eventMessage CONTAINS[c] "itlwm" OR eventMessage CONTAINS[c] "awdl" OR eventMessage CONTAINS[c] "btlfx" OR eventMessage CONTAINS[c] "BlueToolFixup" OR eventMessage CONTAINS[c] "IntelBT" OR eventMessage CONTAINS[c] "[advsets]" OR eventMessage CONTAINS[c] "[rpa]" OR eventMessage CONTAINS[c] "[pagein]"))'
printf 'Start: %s\n' "$(stamp)" >> "$work/attempt.txt"
(ulimit -f 3072; exec /usr/bin/log stream --level debug --style compact --predicate "$predicate") > "$work/live.txt" 2>&1 &
stream_pid=$!
(
    ulimit -f 384
    while :; do
        stamp
        for interface in $wifi_interfaces awdl0; do
            /usr/sbin/netstat -I "$interface" -b 2>&1 | /usr/bin/tail -n 2
        done
        registry_state | /usr/bin/grep -aE 'Listener|Peer|AirDrop|Frame|Data|Status|Error|Scan|Join|Associate|Country|WCL' |
            /usr/bin/cut -c 1-250
        /bin/sleep 5
    done
) > "$work/timeline.txt" 2>&1 &
timeline_pid=$!

case "$focus" in
    bluetooth) echo "Open System Settings > Bluetooth. Put a nearby device in pairing mode and wait about 30 seconds." ;;
    continuity) echo "Try one AirDrop transfer or join the iPhone hotspot once." ;;
    wifi) echo "Open the Wi-Fi list and reproduce the problem once." ;;
esac
echo "Press Enter after the result, or wait up to 120 seconds."
IFS= read -r -t 120 unused
stop_recording
printf 'End: %s\n' "$(stamp)" >> "$work/attempt.txt"
bounded snapshot > "$work/after.txt" 2>&1
bounded kernel_lines > "$work/kernel-after.txt" 2>&1

patch_predicate='eventMessage CONTAINS[c] "[advsets]" OR eventMessage CONTAINS[c] "[rpa]" OR eventMessage CONTAINS[c] "[pagein]" OR eventMessage CONTAINS[c] "BlueToolFixup" OR eventMessage CONTAINS[c] "enableExtendedAdvertising" OR eventMessage CONTAINS[c] "support for extended scans"'
bounded /usr/bin/log show --last boot --info --style compact --predicate "$patch_predicate" > "$work/patches.txt" 2>&1

if [ "$focus" = bluetooth ]; then
    /usr/bin/find /Library/Logs/DiagnosticReports -maxdepth 2 -type f -mtime -2 \
            -iname '*bluetoothd*.ips' -exec /usr/bin/stat -f '%m %N' {} + 2>/dev/null |
        /usr/bin/sort -rn | /usr/bin/head -n 3 | while IFS= read -r row; do
            report="${row#* }"
            printf '\n--- %s ---\n' "$(basename "$report")"
            /usr/bin/head -c 131072 "$report"
        done > "$work/bluetoothd-crashes.txt"
    /usr/bin/file /usr/sbin/bluetoothd > "$work/bluetoothd-binary.txt" 2>&1
else
    /usr/bin/find /Library/Logs/DiagnosticReports -maxdepth 2 -type f -mtime -2 \
            \( -name '*.panic' -o -name 'panic-full*.ips' \) -exec /usr/bin/stat -f '%m %N' {} + 2>/dev/null |
        /usr/bin/sort -rn | /usr/bin/head -n 3 | while IFS= read -r row; do
            report="${row#* }"
            if /usr/bin/grep -aqiE 'AirportItlwm|BlueToolFixup|IntelBTPatcher|IO80211AWDL' "$report"; then
                printf '\n--- %s ---\n' "$(basename "$report")"
                /usr/bin/head -c 131072 "$report"
            fi
        done > "$work/wireless-panics.txt"
fi

if [ "$focus" = bluetooth ] && [[ "$os_version" = 15.* ]]; then
    # Read the Intel slice without requiring Xcode or Command Line Tools.
    /usr/bin/perl - "$work/bluetoothd-x86_64.bin" >> "$work/bluetoothd-binary.txt" 2>&1 <<'PERL'
use strict;
use warnings;
my ($output) = @ARGV;
my $path = '/usr/sbin/bluetoothd';
open my $input, '<:raw', $path or die "$path: $!";
read($input, my $head, 8) == 8 or die 'Incomplete Mach-O header';
my ($magic, $count) = unpack('NN', $head);
my ($offset, $size);
if ($magic == 0xcffaedfe && unpack('V', substr($head, 4)) == 0x01000007) {
    ($offset, $size) = (0, -s $path);
} elsif (($magic == 0xcafebabe || $magic == 0xcafebabf) && $count <= 32) {
    my $width = $magic == 0xcafebabf ? 32 : 20;
    for (1 .. $count) {
        read($input, my $entry, $width) == $width or die 'Incomplete FAT table';
        next unless unpack('N', $entry) == 0x01000007;
        ($offset, $size) = unpack($width == 32 ? 'Q>Q>' : 'NN', substr($entry, 8));
        last;
    }
}
defined($size) && $size > 0 && $size <= 16777216 && $offset + $size <= -s $path
    or die 'No Intel slice within the diagnostic size budget';
seek($input, $offset, 0) or die "Seek: $!";
open my $out, '>:raw', $output or die "$output: $!";
my $left = $size;
while ($left > 0) {
    my $got = read($input, my $data, $left > 65536 ? 65536 : $left);
    $got or die 'Read failed before the end of the Intel slice';
    print {$out} $data or die "Write: $!";
    $left -= $got;
}
close $out or die "Close: $!";
print "x86_64 slice bytes: $size; file offset: $offset\n";
PERL
    if [ "$?" -eq 0 ]; then
        /usr/bin/shasum -a 256 "$work/bluetoothd-x86_64.bin" |
            /usr/bin/awk '{ print "x86_64 slice SHA256: " $1 }' >> "$work/bluetoothd-binary.txt"
    else
        /bin/rm -f -- "$work/bluetoothd-x86_64.bin"
    fi
fi

archive="$base/EricWirelessDiagnostics-$boot_build-$(/bin/date +%Y%m%d-%H%M%S)-${work##*.}.zip"
if ! /usr/bin/zip -q -9 -X -j "$archive" "$work"/*; then
    echo "Archive could not be written."
    exit 1
fi
bytes="$(/usr/bin/stat -f %z "$archive")"
if [ "$bytes" -ge 24000000 ]; then
    /bin/rm -f -- "$archive"
    echo "Diagnostics exceeded the 24 MB attachment budget."
    exit 1
fi
/bin/chmod 600 "$archive"
if [ -n "${SUDO_UID:-}" ]; then
    /usr/sbin/chown "$SUDO_UID:${SUDO_GID:-0}" "$archive"
fi
printf 'Send privately: %s (%s bytes)\n' "$archive" "$bytes"
