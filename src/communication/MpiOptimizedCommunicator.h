#ifndef INC_3PC_MPIOPTIMIZEDCOMMUNICATOR_H
#define INC_3PC_MPIOPTIMIZEDCOMMUNICATOR_H

#include "MpiSimpleCommunicator.h"

class MpiOptimizedCommunicator : public MpiSimpleCommunicator {
public:

    MpiOptimizedCommunicator(int argc, char** argv);

    Packet send(MessageType messageType, const std::string& message, const std::unordered_set<ProcessId>& recipients, Tag tag) override;

    Packet receive(Tag tag) override;

    std::optional<Packet> receive(long timeoutMillis, Tag tag) override;

protected:

    static std::string encode(LamportTime lamportTime, MessageType messageType, const std::string& message);

    static Packet getPacket(const std::string& encodedMessage, ProcessId source);

    void updateTimestamp(Packet& packet);
};


#endif //INC_3PC_MPIOPTIMIZEDCOMMUNICATOR_H
