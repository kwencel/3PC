#ifndef INC_3PC_ABSTRACTPROCESS_H
#define INC_3PC_ABSTRACTPROCESS_H


#include <util/Random.h>
#include <util/Define.h>
#include <communication/ICommunicator.h>
#include <util/StringConcat.h>

class AbstractProcess {

public:
    explicit AbstractProcess(std::shared_ptr<ICommunicator> communicator) : communicator(std::move(communicator)) { }

    virtual void run() = 0;

    virtual void sleep() {
        std::this_thread::sleep_for(std::chrono::milliseconds(random.randomBetween(MIN_SLEEP_TIME, MAX_SLEEP_TIME)));
    }

protected:

    void logUnexpectedPacket(const Packet& p) {
        logWithState("Unexpected packet received: " + printPacket(p));
    }

    void logWithState(const std::string& message) {
        Logger::log(util::concat("[", stateString.at(state), communicator->getProcessId(), "] ", message));
    }

    static std::string printPacket(const Packet& p) {
        return util::concat("TS: ", p.lamportTime, ", source: ", p.source, ", type: ", messageTypeString.at(p.messageType), ", message: ", p.message);
    }

    std::shared_ptr<ICommunicator> communicator;

    Random random;

    State state;
};

#endif //INC_3PC_ABSTRACTPROCESS_H
