# src/tsync-utility-lib/BUILD

load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "tsync_utility",
    srcs = glob([
        "**/*.cpp",
        "**/*.h",
    ]),
    hdrs = [
        "score/time/utility/ITimeBaseAccessor.h",
        "score/time/utility/ITimeBaseReader.h",
        "score/time/utility/ITimeBaseWriter.h",
        "score/time/utility/SysCalls.h",
        "score/time/utility/TimeBaseReaderFactory.h",
        "score/time/utility/TimeBaseWriterFactory.h",
        "score/time/utility/TsyncConfigTypes.h",
        "score/time/utility/TsyncIdMappingsHandler.h",
        "score/time/utility/TsyncNamedSemaphore.h",
        "score/time/utility/TsyncSharedUtils.h",
    ],
    strip_include_prefix = ".",
    # copts = ["-Isrc/tsync-utility-lib"],
    deps = [
        "//src/tsync-lib:tsync_if",
        "//src/common",
        "@score_baselibs//score/language/futurecpp",
    ],
    linkstatic = True,
    visibility = [
        "//src:__subpackages__",
        "//tests:__subpackages__",
    ],
)
