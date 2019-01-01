#ifndef INC_3PC_ABSTRACTCRASHABLEPROCESS_H
#define INC_3PC_ABSTRACTCRASHABLEPROCESS_H

#include "AbstractProcess.h"

class AbstractCrashableProcess : public AbstractProcess {
public:

    explicit AbstractCrashableProcess(std::shared_ptr<MpiSimpleCommunicator> communicator, Tag mpiCrashTag)
        : AbstractProcess(std::move(communicator)) {
        crashSignalReceiver = std::thread([=]{ receiveCrashSignal(mpiCrashTag); });
    }

    std::shared_ptr<MpiSimpleCommunicator> getMpiCommunicator() {
        return std::static_pointer_cast<MpiSimpleCommunicator>(communicator);
    }

    virtual ~AbstractCrashableProcess() {
        crashSignalReceiver.join();
    }

protected:

    void receiveCrashSignal(Tag mpiCrashTag) {
        Logger::registerThread("Crash");
        while (not terminate) {
            auto potentialPacket = getMpiCommunicator()->receive(500, mpiCrashTag);
            if (potentialPacket.has_value()) {
                Logger::log("Received crash signal");
                crashSignalReceived = true;
            }
        }
    }

    void crashIfSignalled() {
        if (crashSignalReceived.load()) {
            logWithState("Committing suicide...");
            terminate = true;
        }
    }

    std::thread crashSignalReceiver;
    std::atomic<bool> crashSignalReceived = false;
    std::atomic<bool> terminate = false;
};

#endif //INC_3PC_ABSTRACTCRASHABLEPROCESS_H
