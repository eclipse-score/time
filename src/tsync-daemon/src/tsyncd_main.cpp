/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <iostream>
#include <string_view>

#include "ArgParser.h"
#include "ConfigLoader.h"
#include "TsyncWorker.h"

// coverity[autosar_cpp14_a15_3_3_violation] No exceptions will occur.
int tsyncd_main(int argc, char* argv[]) {
    std::cout << "Hello World! This is the tsync daemon" << std::endl;

    score::time::daemon::ArgParser argParser(argc, argv);
    if (argParser.IsHelpRequested()) {
        score::time::daemon::ArgParser::PrintHelp();
        return 0;
    }

    if (argParser.IsVersionRequested()) {
        score::time::daemon::ArgParser::PrintVersion();
        return 0;
    }

    // coverity[autosar_cpp14_m3_4_1_violation] This object is declared at the correct scope.
    score::time::daemon::ConfigLoader confLoader;

    if (argParser.IsDebugEnabled()) {
        confLoader.DumpConfig();
    }

    score::time::daemon::TsyncWorker worker;
    worker.Init();
    worker.Run();
    worker.ShutDown();

    return 0;
}
