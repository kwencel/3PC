#ifndef INC_3PC_ICOMMUNICATORTAGGED_H
#define INC_3PC_ICOMMUNICATORTAGGED_H

#include "ICommunicator.h"

template <typename Tag>
class ITaggedCommunicator : public ICommunicator {
public:

    virtual Packet send(MessageType messageType, const std::string& message, const std::unordered_set<ProcessId>& recipients, Tag tag) = 0;

    virtual Packet send(MessageType messageType, const std::string& message, const std::unordered_set<ProcessId>& recipients) override {
        return send(messageType, message, recipients, getDefaultTag());
    }

    virtual Packet send(MessageType messageType, const std::string& message, ProcessId recipient, Tag tag) {
        return send(messageType, message, std::unordered_set<ProcessId> {recipient}, tag);
    };

    virtual Packet sendOthers(MessageType messageType, const std::string& message, Tag tag) {
        return send(messageType, message, otherProcesses, tag);
    };

    virtual Packet receive(Tag tag) = 0;

    virtual std::optional<Packet> receive(long timeoutMillis, Tag tag) = 0;

    virtual Tag getDefaultTag() const = 0;
};

#endif //INC_3PC_ICOMMUNICATORTAGGED_H
