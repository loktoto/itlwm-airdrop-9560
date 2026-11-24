# itlwm-airdrop-9560

Experimental native macOS Wi-Fi and bidirectional AirDrop for Intel Wireless-AC 9560. This project builds on [Eric-AirportItlwm](https://github.com/Ersiter/Eric-AirportItlwm) and its upstream itlwm lineage.

## Hardware tested

- Acer Swift 5 SF514-53T, Intel AC 9560 PCI ID `8086:9DF0`, subsystem `8086:0034`
- macOS Sequoia 15.8.3, build `24H241`, Darwin `24.6.0`
- Native Wi-Fi, Bluetooth and iPhone AirDrop in both directions worked on the owner's machine. The owner subsequently reported the internal EFI setup was stable.
- An observed iPhone-to-Mac transfer averaged approximately 217 kB/s. Performance and operation on other hardware, macOS versions and networks need testing.

This is experimental kernel and wireless firmware code. Keep a bootable backup and test from a separate USB OpenCore configuration first.

## Downloads

| Build | Description |
| --- | --- |
| [AC 9560 firmware-only](artifacts/Intel9560-AirDrop-Sequoia-dwell-ac9560.zip) | The build used in the reported stable internal setup. Includes the 9560 firmware only. |
| [Queue](artifacts/Intel9560-AirDrop-Sequoia-queue.zip) | Earlier hardware-tested full-firmware build. |
| [Dwell](artifacts/Intel9560-AirDrop-Sequoia-dwell.zip) | Full-firmware build with opportunistic transmission during a shared window. |
| [Station order](artifacts/Intel9560-AirDrop-Sequoia-station-order.zip) | Earlier hardware-tested build. |
| [BlueToolFixup 2.7.4](artifacts/BlueToolFixup-AirDrop-2.7.4.zip) | Bluetooth companion used for AirDrop discovery. |

[Checksums](artifacts/SHA256SUMS) · [Patches](patches/) · [Complete source and SDK archive](artifacts/Intel9560-AirDrop-Sequoia-source.tar.gz) · [Bluetooth companion source](artifacts/BlueToolFixup-AirDrop-2.7.4-source.tar.gz)

The kexts have local ad hoc signatures. Verify downloads with `cd artifacts && shasum -a 256 -c SHA256SUMS`.

## OpenCore installation

1. Back up your existing EFI and prepare a bootable USB test configuration.
2. Copy the chosen `AirportItlwm.kext` and the packaged `BlueToolFixup-AirDrop.kext` into `EFI/OC/Kexts`. Add corresponding enabled entries in `Kernel > Add` with their actual folder names. Replace the previous enabled BlueToolFixup entry so only one copy loads. Retain Lilu, IntelBluetoothFirmware and IntelBTPatcher in dependency order.
3. Enable only one Intel Wi-Fi driver. Disable existing `itlwm.kext` and `ForbiddenItlwm.kext` entries when enabling this AirportItlwm. The tested OpenCore entries have `MinKernel` and `MaxKernel` set to `24.6.0`. Append `-itlawdl` to the existing `boot-args`.
4. Validate with the `ocvalidate` version matching your OpenCore release. Boot the USB, then test Wi-Fi connectivity, Bluetooth, speakers and AirDrop in both directions. Restore the backup if necessary.

The supplied Wi-Fi and Bluetooth components do not contain an EFI configuration. Preserve your own device-specific settings and identifiers.

## Build

The checked-in [source](source/Eric-AirportItlwm-source/) contains the dwell patch applied to upstream commit `cf0c5263eae5981e78fdb2ffb0b9406702bc7fc6`. The source archive includes the prepared MacKernelSDK based on commit `3f750085caa17ec3a7880f11c11bf4f48cd6a164`. On an Intel Mac with Xcode, extract the archive and run:

```sh
cd Eric-AirportItlwm-source
xcodebuild -project itlwm.xcodeproj -target AirportItlwm-Sequoia \
  -configuration Debug ARCHS=x86_64 ONLY_ACTIVE_ARCH=NO \
  SYMROOT="$PWD/build/products" OBJROOT="$PWD/build/obj" \
  CODE_SIGNING_ALLOWED=NO build
codesign --force --sign - build/products/Debug/Sequoia/AirportItlwm.kext
```

For the 9560-only build, before running the build, use the upstream firmware generator with a directory containing only `itlwm/firmware/iwm-9000-46`. Confirm the generated `include/FwBinary.cpp` contains `const int fwNumber = 1;` after building. See [build details](docs/build.md).

## Contributing and reporting

Please include the adapter PCI ID, macOS build, OpenCore version, driver build, symptoms, transfer direction and steps to reproduce. Remove serial numbers, MAC addresses, Wi-Fi names and other private details from logs before posting them publicly. Reports from additional machines and networks are welcome.

## Credits and license

Based on Eric-AirportItlwm, itself descended from itlwm and related community projects. The Wi-Fi source is GPL-2.0 as provided upstream. Intel firmware is distributed under its [separate firmware license](source/iwlwifi-firmware-license). The Bluetooth companion source and its licenses are included in its source archive. Apple, AirDrop and Intel are names of their respective owners.
