/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gtest/gtest.h>

#include "score/time/utility/SharedMemTimeBaseReader.h"
#include "score/time/utility/SharedMemTimeBaseWriter.h"
#include "score/time/utility/TimeBaseReaderFactory.h"
#include "score/time/utility/TimeBaseWriterFactory.h"
#include "score/time/utility/TsyncIdMappingsHandler.h"

using namespace score::time;

namespace testing {
namespace lib_timebaseaccessorfactory_ut {

TEST(Lib_TimeBaseAccessorFactory_TestSuite, TimeBaseAccessorFactory_Create_Function_ReadWrite_Success) {
    // Set up writer
    std::unique_ptr<ITimeBaseWriter> writer;
    // A random write value to write to shared-mem
    const uint64_t write_value = 0xb0b;
    writer = TimeBaseWriterFactory::Create("/time_domain1", kIdMappingsShmemSize, true);
    // Assert non null pointer
    EXPECT_NE(writer, nullptr);
    // Writing something to the shared-mem file
    writer->GetAccessor().Open();
    bool res = writer->Write(write_value);
    EXPECT_TRUE(res);

    // Setup Reader
    std::unique_ptr<ITimeBaseReader> reader;
    uint64_t read_value;
    reader = TimeBaseReaderFactory::Create("/time_domain1", kIdMappingsShmemSize);
    // Assert non null pointer
    EXPECT_NE(reader, nullptr);
    reader->GetAccessor().Open();
    res = reader->Read(read_value);
    EXPECT_TRUE(res);
    // Assert if read and write result are equal
    EXPECT_EQ(read_value, write_value);
}

}  // namespace lib_timebaseaccessorfactory_ut
}  // namespace testing