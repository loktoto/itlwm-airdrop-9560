# Build details

The archive `artifacts/Intel9560-AirDrop-Sequoia-source.tar.gz` contains the patched source and prepared SDK. The checked-in source is provided for browsing. The archive includes the SDK, which upstream's ignore rules exclude from ordinary Git tracking.

For a firmware-only build, extract the source archive into a new directory, then run these commands from `Eric-AirportItlwm-source` before building as described in the README.

```sh
firmware_dir=$(mktemp -d)
cp itlwm/firmware/iwm-9000-46 "$firmware_dir/"
python3 scripts/zlib_compress_fw.py include/FwBinary.cpp "$firmware_dir"
```

The upstream build phase checks timestamps and reuses the generated source if it is newer than the firmware inputs. After the build, verify `include/FwBinary.cpp` still contains `const int fwNumber = 1;`. The 9560 firmware-only binary only supplies `iwm-9000-46` and is intended for the tested 9560 configuration.

To apply a different patch, check out upstream commit `cf0c5263eae5981e78fdb2ffb0b9406702bc7fc6`, apply one patch from `patches/`, clone MacKernelSDK at `3f750085caa17ec3a7880f11c11bf4f48cd6a164`, and run `python3 scripts/prepare_tahoe_sdk.py` before building. Each patch is relative to the clean upstream commit.
