#ifndef INC_3PC_ICOMMUNICATOR_H
#define INC_3PC_ICOMMUNICATOR_H

#include <unordered_set>
#include <util/Define.h>
#include <util/Utils.h>

using ProcessId = int;
using LamportTime = unsigned long;

struct Packet {
    LamportTime lamportTime;
    ProcessId source;
    MessageType messageType;
    std::string message;

    inline bool operator==(const Packet &other) const {
        return source == other.source && messageType == other.messageType && message == other.message;
    }

    inline bool operator<(const Packet &other) const {
        return lamportTime < other.lamportTime;
    }
};

namespace std {
    template<>
    struct hash<Packet> {
        inline std::size_t operator()(const Packet& packet) const {
            std::size_t hash = 0;
            hashCombine(hash, packet.source, packet.messageType, packet.message);
            return hash;
        }
    };
}

class ICommunicator {
public:

    virtual Packet send(MessageType messageType, const std::string& message, const std::unordered_set<ProcessId>& recipients) = 0;

    virtual Packet send(MessageType messageType, const std::string& message, ProcessId recipient) {
        return send(messageType, message, std::unordered_set<ProcessId> {recipient});
    };

    virtual Packet sendOthers(MessageType messageType, const std::string& message) {
        return send(messageType, message, otherProcesses);
    };

    virtual Packet receive() = 0;

    virtual std::optional<Packet> receive(long timeoutMillis) = 0;

    virtual ProcessId getProcessId() {
        return myProcessId;
    }

    virtual ProcessId getNumberOfProcesses() {
        return numberOfProcesses;
    }

    virtual LamportTime getCurrentLamportTime() {
        return currentLamportTime;
    }

protected:

    ProcessId myProcessId;

    ProcessId numberOfProcesses;

    std::unordered_set<ProcessId> otherProcesses;

    LamportTime currentLamportTime;
};

#endif //INC_3PC_ICOMMUNICATOR_H
