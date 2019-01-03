#include <communication/MpiSimpleCommunicator.h>
#include <communication/MpiOptimizedCommunicator.h>
#include <processes/Coordinator.h>
#include <processes/CohortMember.h>


int main(int argc, char** argv) {
    auto communicator = std::make_shared<MpiOptimizedCommunicator>(argc, argv);
    Logger::init(communicator);
    Logger::registerThread("Main ");

    if (communicator->getProcessId() == COORDINATOR_ID) {
        Coordinator coordinator(communicator);
        coordinator.run();
    } else {
        CohortMember cohortMember(communicator);
        cohortMember.run();
    }
}
