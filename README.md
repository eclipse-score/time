# Time Synchronization

Time Synchronization between different applications and/or ECUs is of paramount importance when correlation of different
events across a distributed system is needed, either to track such events in time or to trigger them at an accurate point
in time.

The Time-Synchronization (TSync) module is responsible for providing users with the functionality to retrieve time
information synchronized with other entities / ECUs.

## Project Structure

This C++ code implements the *Time Synchronization* module for S-CORE.
The folder structure is:

```plain
score-time
 ├── examples                tsync-lib usage examples
 ├── src
 |    ├── common             Common functionality used by "all" packages
 │    ├── flatbuffer         Flatbuffers mod for coverage(?)
 │    ├── flatcfg            Flatcfg implementation (to be moved to a common repo)
 │    ├── ptpd               Modified variant of ptpd
 │    ├── tsync-daemon       Daemon for managing tsync shared mem
 │    ├── tsync-lib          Library to be used by time clients
 │    ├── tsync-ptp-lib      Library used by ptpd
 │    └── tsync-utility-lib  Internal utility library for shared mem access
 ├── tests
      └── Binary/integration tests
```
Common sub-structure of each component (if applicable):
* Folder `include` contains the API definition of the component (i.e. its public headers).
* Folder `src` contains source files of the implementation and private headers.
* Folder `test` contains unit tests for the implemented classes.

## 🚀 Getting Started

### 1️⃣ Clone the Repository

```sh
git clone https://github.com/eclipse-score/inc_time.git
cd inc_time
```

## 2️⃣ Build

In order to build and run `ptpd`, install `libpcap`:

```shell
sudo apt-get install libpcap-dev
```

To build TSYNC, simply run Bazel:

```shell
bazel build //...
```
### 3️⃣ Run Tests

```sh
bazel test //src/...
```
### 4️⃣ Get the local coverage report
```sh
# the coverage HTML report will be generated in the folder `cpp_coverage`
./run_coverage_local.sh
```


## 🛠 Usage

Run the timesync daemon:

```shell
export ECUCFG_ENV_VAR_ROOTFOLDER=$(pwd)/bazel-out/k8-fastbuild/bin/src/tsync-daemon/src/
bazel run //src/tsync-daemon:tsync_daemon
```

Run the ptp daemon:

```shell
sudo bazelisk run //src/ptpd -- -i eth0 -d 1 --global:foreground=Y -s --ptpengine:transport=ethernet --ptpengine:delay_mechanism=DELAY_DISABLED --ptpengine:disable_bmca=y --score:globaltimepropagationdelay=0.0 -L --ptpengine:dot1as=1 --clock:no_adjust=Y
```

## 📖 Documentation

- Feature: <https://eclipse-score.github.io/score/main/features/time/>
- Module Documentation: <https://eclipse-score.github.io/inc_time>
- Requirements: <https://eclipse-score.github.io/score/main/features/time/docs/requirements/>

### High Level Design

The basic architecture/high level design of the S-CORE time related components is given in the following figure:

<figure>
<img src="docs/architecture/deployment.drawio.svg" style="width:90%">
</figure>

* S-CORE time can handle up to 16 different time bases/time domains.
* Syncing of the "clocks" of those time bases between different devices/exeution environments is done via gPTP or AUTOSAR Tsync Protocol (see below)
* A modified ptpd2 implementation is used to achive that on the S-CORE domain:
   * It syncs a single time base with its related master clock
   * For each time base a separate ptpd instance is required
   * It stores the determined master time and the related local clock in a shared memory area related to the time base
   * It typically does *not* sync the local clock time!
   * The modifications of ptpd2 are done for
     * supporting the AUTOSAR Time Synchronization Protocol
     * interfacing to the shared memory for the local exchange time base data
* Applications use the tsync-lib to get the current time of the desired time base. The tsync-lib
   * gets the time base data from the respective shared memory ressource to
   * determine the current master clock value using this data and the current local time
* The tsync-daemon is responsible to
   * manage the shared memory ressources of the different configured time bases
   * provide the time base configuration via shared memory to the applications and the ptpd process
   * determining timed out sync data

Supported protocols:
* IEEE 802.1AS-2011 - aka gPTP (without AUTOSAR extensions)
* AUTOSAR Time Synchronization Protocol

"Plain PTPv2" (IEEE 1588) seems not be supported at the moment (setting ptpd cmdline arg `--ptpengine:dot1as=0` seems not to have any effect)

### Client App API

Client apps use the S-CORE Time API (`score::time`) to either consume or provide data of the different time bases.
It is defined in folder `src/tsync-lib/include/` and provided via the `tsync` library to the client apps.

This API is essentially a mirrored AUTOSAR Time Synchronization API as defined in chapter 8 of the Specification of Time Synchronization AUTOSAR AP.
Although, it has some differences:
* The API namespace is `score::time`.
* All `ara::core`-based types are replaced either by C++17 std-types or by S-CORE baselib types.

---
