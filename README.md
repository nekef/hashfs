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
