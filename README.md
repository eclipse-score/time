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
 │    │    ├── include         API definitions of tsync-lib (i.e. public headers)
 │    │    └── src             Implementation (source files and privateheaders)
 │    ├── tsync-ptp-lib      Library used by ptpd
 │    └── tsync-utility-lib  Internal utility library for shared mem access
 ├── tests
      ├── Unit Tests
      └── Binary Tests
```

* Folder `include` contains the API definitions of TSYNC and the public headers.
* Folder `src` contains source files of the TSYNC implementation and private headers.
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
bazel test //tests/...
```

## 🛠 Usage

Run the timesync daemon:

```shell
export ECUCFG_ENV_VAR_ROOTFOLDER=$(pwd)/bazel-out/k8-fastbuild/bin/src/tsync-daemon/src/score/time/daemon/
bazel run //src/tsync-daemon:tsync_daemon
```

Run the ptp daemon:

```shell
sudo bazel run //src/ptpd -- -i <ethnernet-if> -d 1 --global:foreground=Y -S --ptpengine:transport=ethernet --ptpengine:delay_mechanism=DELAY_DISABLED --ptpengine:disable_bmca=y --score:globaltimepropagationdelay=0.0 -L --ptpengine:dot1as=1 --clock:no_adjust=Y
```

## 📖 Documentation

- Feature: <https://eclipse-score.github.io/score/main/features/time/>
- Module Documentation: <https://eclipse-score.github.io/inc_time>
- Requirements: <https://eclipse-score.github.io/score/main/features/time/docs/requirements/>

---
