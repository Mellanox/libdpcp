# DPCP

Direct Packet Control Plane (DPCP) provides an unified flexible
interface for programming IB devices using DevX.

## Licenses

DPCP is licensed as:

* [BSD-3-Clause] See LICENSE file

## Supported CPU Architectures

* [x86_64](https://en.wikipedia.org/wiki/X86-64)
* [Power8/9](https://www.ibm.com/support/knowledgecenter/en/POWER9/p9hdx/POWER9welcome.htm)
* [Arm v8](https://www.arm.com/products/silicon-ip-cpu)

## Usage

### Linux Builds

Building DPCP on Linux is typically a combination of running "configure" and "make".
Execute the following commands to install the DPCP library from within the
directory at the top of the tree:

```sh
$ ./autogen.sh
$ ./configure [--prefix=${target-installation-path}]
$ make -j
$ make install
```
> Note that the argument `--prefix` can be omitted from the `./configure` line.
> In this case the target will be installed to the location specified by the `ac_default_prefix`,
> which is defined within the [configure](configure) file.

### Build internal tests

Use the following command to build the internal tests (done incrementally):

```sh
$ make -j -C tests/gtest
```

Use the following command to run a sepcific set of tests:

```sh
$tests/gtest/gtest --gtest_filter=${pattern}
```

Use the following command to both build and run the full test suite.

```sh
$ make -j -C tests/gtest test
```

### Build RPM or DEB package

```sh
$ contrib/build_pkg.sh -s -b
```

### Windows Builds

Building DPCP on Windows requires to install WinOF-2 driver with DevX SDK:
 1. Download WinOF-2 from here: https://network.nvidia.com/products/adapter-software/ethernet/windows/winof-2/
 2. Choose custom installation and select to install "DevX SDK installer".
 3. After WinOF-2 is installed, please find the installer in `C:\Program Files\Mellanox\MLNX_WinOF2\DevX_SDK\`
 4. Install DevX SDK.
 5. Check that below environment variables are set (for example, start `cmd.exe` and run command `set`):
    ```
    DEVX_INC_PATH=C:\Program Files\Mellanox\MLNX_WinOF2_DevX_SDK\inc
    DEVX_LIB_PATH=C:\Program Files\Mellanox\MLNX_WinOF2_DevX_SDK\lib
    ```

Visual Studio 2019 is a minimal supported version.
Also Spectre-mitigated standard libraries are required:
 1. Go to "Apps & Features".
 2. Choose "Visual Studio 2019" and press button "Modify".
 3. In "Visual Studio Installer" window select tab "Individual components".
 4. In search edit box enter "Spectre".
 5. In the section "Compilers, build tools and runtimes" choose component "MSVC v142 - VS 2019 C++ x64/x86 Spectre-mitigated libs (Latest)".
 6. Press "Modify" and wait for finishing installation.

When all pre-requisites are installed, just open dpcp.sln in Visual Studio IDE and do "Build solution".

### Issue release

* Update the version number in the configure.ac file
* Update dpcp_version constant at src/api/dpcp.h file
* Update changelog sections of contrib/scripts/dpcp.spec.in and debian/changelog.in files 
* Commit changes with message in format "X.Y.Z"
* Create git TAG named "X.Y.Z"
* Create source package using script as contrib/build_pkg.sh Optionally use PRJ_RELEASE=n environment variable to specify release number.
* Update information at https://github.com/Mellanox/dpcp/releases
