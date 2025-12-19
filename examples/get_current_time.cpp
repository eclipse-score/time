/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <iostream>
#include "score/time/synchronized_time_base_consumer.h"

int main() {
    std::string_view instance_specifier = "consumer";

    std::cout << "Instantiate a SynchronizedTimeBaseConsumer for instance '" 
              << instance_specifier << "'" << std::endl;
    score::time::SynchronizedTimeBaseConsumer stbc{instance_specifier};
    
    std::cout << "Current time = "
        << stbc.GetCurrentTime().time_since_epoch().count()
        << std::endl;

    return 0;
}
