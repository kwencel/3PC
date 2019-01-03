#include <communication/MpiOptimizedCommunicator.h>
#include <processes/Coordinator.h>
#include <processes/CohortMember.h>


int main(int argc, char** argv) {
    auto communicator = std::make_shared<MpiOptimizedCommunicator>(argc, argv);
    Logger::init(communicator);
    Logger::registerThread("Main ");

    if (communicator->getProcessId() == COORDINATOR_ID) {
        Coordinator<MpiTag> coordinator(communicator, MPI_DEFAULT_TAG, MPI_CRASH_TAG);
        coordinator.run();
    } else {
        CohortMember<MpiTag> cohortMember(communicator, MPI_DEFAULT_TAG, MPI_CRASH_TAG);
        cohortMember.run();
    }
}
