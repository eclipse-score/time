/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <array>
#include "flatcfg/flatcfg.h"
#include "flatcfg/flatcfg_error.h"
#include "flatcfg/utils/ctc.h"
#include "flatcfg/utils/fb_verifier.h"

#include <glob.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string_view>

FLATCFG_SKIPCOV_BEGIN();

/// @brief Helper type to hold result of parsing file name.
struct ParseResult {
    std::string swclName;
    std::optional<std::string_view> processName;
};

/// @brief Parse mandatory swclName and optional processName from file path.
/// @error FlatCfgErrorCode::kInvalidFileName Returned if the config file's name is invalid.
score::Result<ParseResult> parseSwclProcFromFilePath(std::string_view path, std::string_view fcName) noexcept {
    using flatcfg::FlatCfgErrorCode;
    ParseResult pr;

    // remove everything before (and including) final '/'
    auto lastSlashPos = path.rfind('/');
    if (lastSlashPos != std::string_view::npos) {
        path.remove_prefix(static_cast<std::size_t>(lastSlashPos + 1U));
    }
    // check and remove '_flatcfg.bin' suffix
    const std::string_view binSuffix{"_flatcfg.bin"};
    if (path.size() < binSuffix.size() || path.substr(path.size() - binSuffix.size()) != binSuffix) {
        return score::MakeUnexpected<ParseResult>(FlatCfgErrorCode::kInvalidFileName);
    }
    path.remove_suffix(binSuffix.size());

    // check and remove fcName prefix
    if (path.size() < fcName.size() || path.substr(0U, fcName.size()) != fcName) {
        return score::MakeUnexpected<ParseResult>(FlatCfgErrorCode::kInvalidFileName);
    }
    path.remove_prefix(fcName.size());

    // check and remove swclName suffix (and leading double underscore)
    // can't use path.rfind("__"), PWI #68184
    const std::string_view doubleUnderscore{"__"};
    auto lastDoubleUnderscoreIt =
        std::find_end(path.begin(), path.end(), doubleUnderscore.begin(), doubleUnderscore.end());
    auto lastDoubleUnderscorePos = std::distance(path.begin(), lastDoubleUnderscoreIt);
    if (lastDoubleUnderscoreIt == path.end()) {
        return score::MakeUnexpected<ParseResult>(FlatCfgErrorCode::kInvalidFileName);
    }
    // coverity[autosar_cpp14_a4_7_1_violation] Will not overflow, params will never be large enough.
    pr.swclName = path.substr(static_cast<std::size_t>(lastDoubleUnderscorePos + 2));
    // coverity[autosar_cpp14_a4_7_1_violation] Will not overflow, params will never be large enough.
    path.remove_suffix(path.size() - static_cast<std::size_t>(lastDoubleUnderscorePos));

    // ensure that swclName is not empty
    if (pr.swclName.empty()) {
        return score::MakeUnexpected<ParseResult>(FlatCfgErrorCode::kInvalidFileName);
    }

    // parse processName if anything is left in the path
    if (!path.empty()) {
        // check (but DO NOT remove) leading underscore
        // don't remove so that our trailing underscore check works if path == '_'
        if (path[0U] != '_') {
            return score::MakeUnexpected<ParseResult>(FlatCfgErrorCode::kInvalidFileName);
        }

        // check that there are no double or trailing underscores
        // this also doubles as a check that processName is not empty (except for the leading underscore)
        if (path.find("__") != std::string_view::npos || path.back() == '_') {
            return score::MakeUnexpected<ParseResult>(FlatCfgErrorCode::kInvalidFileName);
        }

        // check that all characters are in [a-z0-9_]
        for (char c : path) {
            if (!std::islower(c) && !std::isdigit(c) && c != '_') {
                return score::MakeUnexpected<ParseResult>(FlatCfgErrorCode::kInvalidFileName);
            }
        }

        // save existing processName
        path.remove_prefix(1U);  // get rid of leading underscore
        pr.processName = path;
    }

    // return parsed data
    return pr;
}

namespace flatcfg {

score::Result<std::shared_ptr<void>> FlatCfg::load(const std::string& SwClusterName) noexcept {
    std::ifstream infile{};
    std::string filePath{""};

    {
        // Get the path to the configuration file for the requested SWCluster
        // clang-format off
        // coverity[autosar_cpp14_a23_0_1_violation:FALSE] #54112: An iterator shall not be implicitly converted to const_iterator.
        std::map<std::string, std::string>::const_iterator it { static_cast<std::map<std::string, std::string>::const_iterator>(m_SwClusterPathMap.find(SwClusterName)) };
        // clang-format on
        if (it == m_SwClusterPathMap.cend()) {
            return score::MakeUnexpected<std::shared_ptr<void>>(FlatCfgErrorCode::kInvalidSWCLName);
        }

        // Create copy because map iterator is locked only in this scope
        filePath = it->second;
    }

    // Open binary file as input with AtTheEnd flag, to easily get the file size
    // RULECHECKER_comment(1, 1, check_enum_usage_overloaded_operator, "Exception for BitmaskType.", false)
    infile.open(filePath.data(), std::ios::binary | std::ios::in | std::ifstream::ate);
    if (!infile.is_open()) {
        return score::MakeUnexpected<std::shared_ptr<void>>(FlatCfgErrorCode::kFileNotFound);
    }

    // Get the file size
    const size_t fileSize{static_cast<size_t>(infile.tellg())};
    if (infile.bad()) {
        infile.close();
        return score::MakeUnexpected<std::shared_ptr<void>>(FlatCfgErrorCode::kFileAccessError);
    }
    if (0U == fileSize) {
        infile.close();
        return score::MakeUnexpected<std::shared_ptr<void>>(FlatCfgErrorCode::kFileEmpty);
    }

    // Seek to beginning prior to read
    infile.seekg(0, std::ios::beg);
    if (infile.bad()) {
        infile.close();
        return score::MakeUnexpected<std::shared_ptr<void>>(FlatCfgErrorCode::kFileAccessError);
    }

    // Shared pointer is used to simplify releasing of the allocated memory for the clients
    // custom delete ("free") needed for shared pointer because memory was allocated by "malloc"
    // on failure, makeSharedNothrow calls free(ptr) and returns nullptr
    /* RULECHECKER_comment(2, 1, check_stdlib_use_alloc, "This is necessary for shared_ptr to free the allocated
     * memory.", true) */
    std::shared_ptr<char> buffer;
    try {
        // RULECHECKER_comment(1, 1, check_new_operator, "New required to allocate memory.", true)
        buffer = std::shared_ptr<char>(new char[fileSize], std::default_delete<char[]>());
    } catch (const std::bad_alloc&) {
        infile.close();
        return score::MakeUnexpected<std::shared_ptr<void>>(FlatCfgErrorCode::kMemoryAllocationFailed);
    }

    infile.read(buffer.get(), static_cast<ptrdiff_t>(fileSize));
    if (infile.bad()) {
        infile.close();
        return score::MakeUnexpected<std::shared_ptr<void>>(FlatCfgErrorCode::kFileAccessError);
    }

    // Ignore error handling for close(), we got what we need
    infile.close();

    // Verify fields required by SCORE are in binary
    score::ResultBlank verifyRes{
        utils::FbVerifier::verifyBinary(std::string_view(buffer.get(), fileSize), m_functionClusterNameLowerCase,
                                        m_functionClusterMajorVersion, m_functionClusterMinorVersion)};
    if (!verifyRes) {
        return score::MakeUnexpected<std::shared_ptr<void>>(verifyRes.error());
    }

    return buffer;
}

score::Result<std::vector<std::string>> FlatCfg::getSwClusterList() noexcept {
    // Check if config path exists
    score::Result<std::string> cfgPath{getConfigPath()};

    if (!cfgPath) {
        return score::MakeUnexpected<std::vector<std::string>>(cfgPath.error());
    }

    // Get the list of the fc configuration files in the folder
    score::Result<std::vector<std::string>> cfgFileList{getConfigFiles(*cfgPath)};
    if (!cfgFileList) {
        return score::MakeUnexpected<std::vector<std::string>>(cfgFileList.error());
    }

    // Check that only one process name used in the path
    return checkFilesAndGetSwClusterList(*cfgFileList);
}

score::Result<std::string> FlatCfg::getConfigPath() const noexcept {
    std::string configPath{};

    // Read the environment variable containing the rootFolder for the config for this application
    const char* configPathEnv{getenv(envVar_configPath.data())};
    if (configPathEnv == nullptr) {
        return score::MakeUnexpected(FlatCfgErrorCode::kInvalidConfigPath,
                (std::string{"Environment variable "} + envVar_configPath.data() + " is not defined.").c_str());
    }

    // Check if path exists
    // RULECHECKER_comment(1, 1, check_union_object, "This is the definition provided by the external library.", true)
    struct stat buffer;
    if (stat(configPathEnv, &buffer) != 0) {
        return score::MakeUnexpected(FlatCfgErrorCode::kInvalidConfigPath,
                (std::string{"Path \""} + configPathEnv + "\" not found.").c_str());
    }

    configPath = std::string{configPathEnv};

    // Append a / if it is not there
    if (configPath.back() != '/') {
        configPath += "/";
    }

    return configPath;
}

score::Result<std::vector<std::string>> FlatCfg::getConfigFiles(const std::string& configPath) const noexcept {
    std::vector<std::string> fileList{};
    std::string globPattern{configPath};
    globPattern += m_functionClusterNameLowerCase;
    globPattern += configFileGlobPattern;

    // Get all files matching the pattern (path/filepattern)
    glob_t glob_result;
    int glob_ret{glob(globPattern.data(), GLOB_TILDE, NULL, &glob_result)};
    if (glob_ret == 0) {
        for (size_t i{0U}; i < glob_result.gl_pathc; ++i) {
            // check that the path points to a regular file (rather than e.g. a directory)
            /* RULECHECKER_comment(2, 1, check_union_object, "This is the definition provided by the external library.
             * ", true) */
            struct stat path_stat;
            // *std::next(it, n) is equivalent to it[n] and doesn't trigger an SCA violation
            int stat_result{stat(*std::next(glob_result.gl_pathv, static_cast<long>(i)), &path_stat)};
            /* RULECHECKER_comment(2, 1, check_underlying_signedness_conversion, "This is the definition provided by the
             * external library.", true) */
            if (stat_result != 0 || !S_ISREG(path_stat.st_mode)) {
                continue;
            }

            // *std::next(it, n) is equivalent to it[n] and doesn't trigger an SCA violation
            fileList.push_back(std::string(*std::next(glob_result.gl_pathv, static_cast<long>(i))));
        }
        globfree(&glob_result);
    } else if (glob_ret == GLOB_NOMATCH) {  // glob failed but don't return error, return empty vector
        return {};
    } else {  // glob failed
        return score::MakeUnexpected<std::vector<std::string>>(FlatCfgErrorCode::kConfigFiles);
    }

    return fileList;
}

// coverity[exn_spec_violation:FALSE] Can't exceed swclList.max_size() due to fileList.size() check
score::Result<std::vector<std::string>> FlatCfg::checkFilesAndGetSwClusterList(const std::vector<std::string>& fileList) noexcept {
    std::vector<std::string> swclList{};

    // Check the fileList isn't bigger than swclList max size so std::length_error can't be thrown
    if (fileList.size() > swclList.max_size()) {
        return score::MakeUnexpected<std::vector<std::string>>(FlatCfgErrorCode::kConfigFiles);
    }

    // Clear the map as we are getting new list of files
    m_SwClusterPathMap.clear();

    // Iterate over all filenames in the fileList and get the process name and SWCL
    std::optional<std::string_view> lastProcName;
    for (const std::string& fileName : fileList) {
        // parse file name
        score::Result<ParseResult> prRes = parseSwclProcFromFilePath(fileName, m_functionClusterNameLowerCase);
        if (!prRes) {
            return score::MakeUnexpected<std::vector<std::string>>(prRes.error());
        }

        // check processName
        if (!lastProcName) {
            lastProcName = prRes->processName;
        }
        if (prRes->processName != lastProcName) {
            // two files with different process names (or one has a process name and one doesn't)
            return score::MakeUnexpected<std::vector<std::string>>(FlatCfgErrorCode::kMultipleProcessesInPath);
        }

        // save swcl if not seen before
        if (swclList.end() == std::find(swclList.begin(), swclList.end(), prRes->swclName)) {
            swclList.push_back(prRes->swclName);
        }
        m_SwClusterPathMap[std::move(prRes)->swclName] = fileName;
    }

    // Success
    return swclList;
}

// RULECHECKER_comment(1, 1, check_static_object_dynamic_initialization, "Trivial types do not cause issues here", true)
const std::string_view FlatCfg::configFileGlobPattern = "_*_*_flatcfg[.]bin";
// RULECHECKER_comment(1, 1, check_static_object_dynamic_initialization, "Trivial types do not cause issues here", true)
const std::string_view FlatCfg::envVar_configPath = "ECUCFG_ENV_VAR_ROOTFOLDER";

}  // namespace flatcfg

FLATCFG_SKIPCOV_END();
