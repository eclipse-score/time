/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "score/result/result.h"

#include "flatbuffers/flatbuffers.h"
#include "flatcfg/flatcfg_error.h"
#include "ctc.h"
#include "fb_verifier.h"

FLATCFG_SKIPCOV_BEGIN();

namespace {

/// @brief Helper class to check for fields required by SCORE in binary.
class RequiredFields final : public flatbuffers::Table {
   public:
    /// @brief Verify that we can parse required fields.
    /// @note  Required and called by flatbuffers::Verifier::VerifyBuffer(...).
    bool Verify(flatbuffers::Verifier& verifier) const {
        return VerifyTableStart(verifier) && VerifyOffset(verifier, VT_FUNCTION_CLUSTER) && verifier.EndTable();
    }

    /// @brief Convert offset from .fbs file to VTable offset.
    static constexpr flatbuffers::voffset_t schemaIdToVtableOffset(int id) noexcept {
        // helper typedef
        using type = flatbuffers::voffset_t;

        // coverity[autosar_cpp14_a4_7_1_violation:Intentional] Because the implementation is designed not to overflow.
        type offset = static_cast<type>(static_cast<type>(static_cast<type>(id) + static_cast<type>(2)) * sizeof(type));
        return offset;
    }

    // vtable offset for each required field in the table
    // use variables rather than enum so that we don't need to cast
    static const flatbuffers::voffset_t VT_FUNCTION_CLUSTER;
    static const flatbuffers::voffset_t VT_VERSION_MAJOR;
    static const flatbuffers::voffset_t VT_VERSION_MINOR;
};

// out of line definition because inline variables are a C++17 feature
constexpr flatbuffers::voffset_t RequiredFields::VT_FUNCTION_CLUSTER{schemaIdToVtableOffset(0)};
constexpr flatbuffers::voffset_t RequiredFields::VT_VERSION_MAJOR{schemaIdToVtableOffset(1)};
constexpr flatbuffers::voffset_t RequiredFields::VT_VERSION_MINOR{schemaIdToVtableOffset(2)};

}  // namespace

namespace flatcfg {
namespace utils {

score::ResultBlank
// RULECHECKER_comment(1, 1, check_max_parameters, "Four parameters is reasonable here.", true);
FbVerifier::verifyBinary(std::string_view binaryData, std::string_view functionCluster, std::int32_t versionMajor,
                         std::int32_t versionMinor) noexcept {
    // helper typedef
    using T = RequiredFields;

    // verify we can parse buffer and its fields
    flatbuffers::Verifier verifier{// RULECHECKER_comment(1, 1, check_reinterpret_cast_extended, "Input is std::string_view
                                   // (char*) but function needs unsigned char*.", true)
                                   reinterpret_cast<const unsigned char*>(binaryData.data()), binaryData.size()};
    if (!verifier.VerifyBuffer<T>()) {
        return score::MakeUnexpected(FlatCfgErrorCode::kInvalidBinaryFormat);
    }

    // now we can get root of buffer
    const T* self{flatbuffers::GetRoot<T>(binaryData.data())};

    // every type has a default value (string is "", integers are 0, etc.)
    // if a field has the default value, it's (usually) not written to the binary file
    // so we must interpret absence of a field as if the field had the default value when written

    // verify functionCluster field is present and has the correct value
    const flatbuffers::String* fcName{self->GetPointer<const flatbuffers::String*>(T::VT_FUNCTION_CLUSTER)};
    if ((fcName == nullptr && !functionCluster.empty()) ||
        (fcName != nullptr && fcName->size() != functionCluster.size())) {
        return score::MakeUnexpected(FlatCfgErrorCode::kBadName);
    }

    for (std::size_t i = 0U; i < functionCluster.size(); ++i) {
        // fcName cannot be null here because we checked size
        const flatbuffers::String& fc = *fcName;
        flatbuffers::uoffset_t index = static_cast<flatbuffers::uoffset_t>(i);
        // RULECHECKER_comment(1, 1, check_underlying_signedness_conversion, "There is no conversion here that could
        // result in the wrong value.", false)
        auto lhs = std::tolower(static_cast<unsigned char>(fc[index]));
        auto rhs = functionCluster[i];
        if (lhs != rhs) {
            return score::MakeUnexpected(FlatCfgErrorCode::kBadName);
        }
    }

    // verify versionMajor field is present and has the correct value
    int32_t vMajor{self->GetField<int32_t>(T::VT_VERSION_MAJOR, 0)};
    if (vMajor != versionMajor) {
        return score::MakeUnexpected(FlatCfgErrorCode::kBadVersionMajor);
    }

    // verify versionMinor field is present and has the correct value
    int32_t vMinor{self->GetField<int32_t>(T::VT_VERSION_MINOR, 0)};
    if (vMinor != versionMinor) {
        return score::MakeUnexpected(FlatCfgErrorCode::kBadVersionMinor);
    }

    // success
    return score::ResultBlank();
}

}  // namespace utils
}  // namespace flatcfg

FLATCFG_SKIPCOV_END();
