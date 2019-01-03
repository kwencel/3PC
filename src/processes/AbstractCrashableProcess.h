#ifndef INC_3PC_ABSTRACTCRASHABLEPROCESS_H
#define INC_3PC_ABSTRACTCRASHABLEPROCESS_H

#include "AbstractProcess.h"

template <typename Tag>
class AbstractCrashableProcess : public AbstractProcess {
public:

    explicit AbstractCrashableProcess(std::shared_ptr<ITaggedCommunicator<Tag>> communicator, Tag defaultTag, Tag crashTag)
        : AbstractProcess(std::move(communicator)), defaultTag(defaultTag), crashTag(crashTag) {
        crashSignalReceiver = std::thread([=]{ receiveCrashSignal(crashTag); });
    }

    std::shared_ptr<ITaggedCommunicator<Tag>> getTaggedCommunicator() {
        return std::static_pointer_cast<ITaggedCommunicator<Tag>>(communicator);
    }

    virtual ~AbstractCrashableProcess() {
        crashSignalReceiver.join();
    }

protected:

    void receiveCrashSignal(Tag crashTag) {
        Logger::registerThread("Crash");
        while (not terminate) {
            auto potentialPacket = getTaggedCommunicator()->receive(500, crashTag);
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
    Tag defaultTag;
    Tag crashTag;
};

#endif //INC_3PC_ABSTRACTCRASHABLEPROCESS_H
