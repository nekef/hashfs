# Maintainer: Nekef Chk <nekef@duck.com>
pkgname=hashfs
pkgver=1.0.0
pkgrel=1
pkgdesc="A production-grade userspace file system featuring live cryptographic data integrity verification."
arch=('x86_64')
url="https://github.com/nekef/hashfs" # Change to your actual GitHub username!
license=('GPL3')
depends=('fuse3')
makedepends=('gcc' 'pkg-config')

# Dynamically pulls down the source archive directly from your GitHub release tags
source=("${pkgname}-${pkgver}.tar.gz::${url}/archive/refs/tags/v${pkgver}.tar.gz")
sha256sums=('SKIP') # Tells makepkg to download the remote archive directly

prepare() {
    # Navigates into the source tree folder extracted by the source array tarball download
    cd "${srcdir}/${pkgname}-${pkgver}"
}

build() {
    cd "${srcdir}/${pkgname}-${pkgver}"
    # Compiles the pristine source code files directly inside the clean extraction target directory
    gcc -Wall mkfs.hashfs.c -o mkfs.hashfs
    gcc -Wall -I/usr/include/fuse3 hashfs.c -o hashfs -lfuse3
}

package() {
    cd "${srcdir}/${pkgname}-${pkgver}"
    
    install -d -m755 "${pkgdir}/usr/bin"
    install -d -m755 "${pkgdir}/usr/local/bin"

    # Ships the newly built binaries into the filesystem installation target boundaries
    install -m755 hashfs "${pkgdir}/usr/bin/hashfs"
    install -m755 mkfs.hashfs "${pkgdir}/usr/local/bin/mkfs.hashfs"
}
