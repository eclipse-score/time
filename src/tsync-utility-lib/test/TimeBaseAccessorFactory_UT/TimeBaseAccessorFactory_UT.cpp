/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gtest/gtest.h>

#include "score/time/utility/TimeBaseReaderFactory.h"
#include "score/time/utility/TimeBaseWriterFactory.h"

using namespace score::time;

namespace testing {
namespace timebaseaccessorfactory_ut {

TEST(Lib_TimeBaseAccessorFactory_TestSuite, TimeBaseAccessorFactory_Create_Function_ReadWrite_Success) {
    // Set up writer
    auto writer = TimeBaseWriterFactory::Create("/time_domain1", true);
    ASSERT_NE(writer, nullptr);

    // Writing something to the shared-mem file
    writer->GetAccessor().Open();
    const uint64_t write_value = 0xb0b;
    bool res = writer->Write(write_value);
    EXPECT_TRUE(res);

    // Setup Reader
    auto reader = TimeBaseReaderFactory::Create("/time_domain1");
    ASSERT_NE(reader, nullptr);

    reader->GetAccessor().Open();
    uint64_t read_value;
    res = reader->Read(read_value);
    EXPECT_TRUE(res);

    // Assert if read and write result are equal
    EXPECT_EQ(read_value, write_value);
}

}  // namespace timebaseaccessorfactory_ut
}  // namespace testing
