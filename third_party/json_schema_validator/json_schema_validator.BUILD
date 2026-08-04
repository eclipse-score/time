package(default_visibility = ["//visibility:public"])

cc_library(
    name = "json_schema_validator_lib",
    srcs = glob(["src/*"]),
    hdrs = ["src/nlohmann/json-schema.hpp"],
    features = ["third_party_warnings", "-treat_warnings_as_errors"],
    includes = ["src"],
    deps = ["@nlohmann_json//:json"],
)

cc_binary(
    name = "json_schema_validator",
    srcs = ["app/json-schema-validate.cpp"],
    deps = [":json_schema_validator_lib"],
)
