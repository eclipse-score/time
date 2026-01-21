# src/ptpd/BUILD

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

# Define a genrule to create config.h
genrule(
    name = "generate_config_h",
    srcs = ["cmake/config_linux.h.in"], # Reference the config_linux.h.in file
    outs = ["gen_config/config.h"],       # Output to a generated directory
    cmd_bash = """
        echo "Replacing BUILD_DATE"
        echo "Reading $(SRCS)"
        echo "Writing $(OUTS)"
        sed -e "s/@BUILD_DATE@/$$(date +%Y%m%d)/" $(SRCS) > $(OUTS)
    """,
    output_to_bindir = True
)

cc_library(
    name = "config_h_lib",
    hdrs = [":generate_config_h"],
    strip_include_prefix = "gen_config", # This makes #include "config.h" work
)

cc_library(
    name = "ptpd_lib",
    srcs = [
        "src/arith.c",
        "src/bmc.c",
        "src/constants.h",
        "src/ptp_primitives.h",
        "src/ptp_datatypes.h",
        "src/datatypes.h",
        "src/dep/constants_dep.h",
        "src/dep/datatypes_dep.h",
        "src/dep/ipv4_acl.h",
        "src/dep/ipv4_acl.c",
        "src/dep/msg.c",
        "src/dep/net.c",
        "src/dep/ptpd_dep.h",
        "src/dep/eventtimer.h",
        "src/dep/eventtimer.c",
        "src/ptp_timers.h",
        "src/ptp_timers.c",
        "src/dep/servo.c",
        "src/dep/iniparser/dictionary.h",
        "src/dep/iniparser/iniparser.h",
        "src/dep/iniparser/dictionary.c",
        "src/dep/iniparser/iniparser.c",
        "src/dep/configdefaults.h",
        "src/dep/configdefaults.c",
        "src/dep/daemonconfig.h",
        "src/dep/daemonconfig.c",
        "src/dep/startup.c",
        "src/dep/sys.c",
        "src/display.c",
        "src/management.c",
        "src/signaling.c",
        "src/protocol.c",
        "src/dep/ntpengine/ntp_isc_md5.c",
        "src/dep/ntpengine/ntp_isc_md5.h",
        "src/dep/ntpengine/ntpdcontrol.c",
        "src/dep/ntpengine/ntpdcontrol.h",
        "src/timingdomain.h",
        "src/timingdomain.c",
        "src/dep/alarms.h",
        "src/dep/alarms.c",
        "src/ptpd.c",
        "src/ptpd.h",
        "src/score/src/score_ptp_consumer.c",
        "src/score/src/score_ptp_provider.c",
        "src/score/src/score_ptp_common.c",
        "src/score/src/score_ptp_arith.c",
        "src/score/src/score_ptp_subtlv.c",

        "src/dep/eventtimer_posix.c",
        "src/dep/statistics.h",
        "src/dep/statistics.c",
        "src/dep/outlierfilter.h",
        "src/dep/outlierfilter.c",
    ],
    hdrs = glob([
        "src/**/*.h",
        "src/**/*.def"
    ]),
    includes = [
        "src",
        "src/def",
        "src/def/message",
        "src/def/managementTLV",
        "src/def/signalingTLV",
        "src/def/derivedData",
        "src/score",
        "src/score/src",
        "src/score/inc",
        "src/dep",
        "src/dep/ntpengine",
        "src/dep/iniparser"
    ],
    deps = [
        ":config_h_lib",
        "//src/common",
        "//src/tsync-ptp-lib",
        "//src/tsync-utility-lib:tsync_utility"
    ],
    strip_include_prefix = "",
    copts = [
        "-DHAVE_CONFIG_H",
        '-DDATADIR=\\"/share\\"',
        "-DPTPD_EXPERIMENTAL",
        "-DPTPD_STATISTICS",
        "-DPTPD_UNICAST_MAX=128",
        "-DPTPD_PTIMERS",
        "-D_GNU_SOURCE",
        "-DHAVE_PCAP_PCAP",
        "-DPTPD_PCAP"
    ],
)

cc_binary(
    name = "ptpd",
    deps = [
        ":ptpd_lib",
    ],
    linkopts = [
        "-lpthread",
        "-lrt",
        "-lm",
        "-lpcap",
    ]
)
