# HashFS

A lightweight, production-grade Linux userspace filesystem (FUSE) featuring **live cryptographic data integrity verification**. 

HashFS calculates a unique FNV-1a hash for every file data block when it is written to the disk. By forcing direct I/O execution loops and completely bypassing the Linux kernel Page Cache, HashFS reads directly from the raw silicon block cells on every read operation. If any background hardware corruption or malicious sector tampering occurs, HashFS intercepts it in real-time, alerts the system, and blocks the corrupted data with an Input/Output error.

## Features
* **Live Integrity Verification:** Catches hardware-level sector manipulation on the fly.
* **Direct Hardware I/O:** Bypasses kernel buffering memory layers to enforce active block state lookups.
* **Production Resource Actions:** Full support for file creation, sequential reading, writing, and atomic file removal (`rm`) with sector zero-wiping.
* **Arch Linux Integrated:** Built and managed natively using standard system toolchains.

## Installation

### From the AUR
If you are on Arch Linux or CachyOS, you can install HashFS natively using an AUR helper:
```bash
yay -S hashfs
```
## Building From Source
Ensure you have fuse3, gcc, and pkg-config installed on your system:

```Bash
gcc -Wall mkfs.hashfs.c -o mkfs.hashfs
gcc -Wall -I/usr/include/fuse3 hashfs.c -o hashfs -lfuse3
```
## Usage Guide
1. Format a Storage Target
Initialize the custom HashFS structural architecture metadata blocks on a disk image or physical block storage node (e.g., a USB drive):

```Bash
sudo mkfs.hashfs /dev/sdX
```
(Replace sdX with your targeted storage device node name)

## 2. Mount the Filesystem
Create a clean mount directory point and launch the driver execution service daemon in the foreground to monitor operations:

```Bash
mkdir -p ~/usb_mnt
sudo hashfs /dev/sdX ~/usb_mnt -f -o allow_other
```
## 3. File Operations
Open a secondary terminal session window to interact with your secure storage mount:

```Bash
cd ~/usb_mnt
echo "Cryptographically signed data payload." > secure.txt
cat secure.txt
rm secure.txt
```
---
