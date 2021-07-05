#ifndef INC_3PC_MPISIMPLECOMMUNICATOR_H
#define INC_3PC_MPISIMPLECOMMUNICATOR_H

#include <mpi.h>
#include <mutex>
#include "ITaggedCommunicator.h"

#define MPI_DEFAULT_TAG 0

// The following 'using' and 'define' sections always have to match
#define MPI_ENCODED_LAMPORT_TIME MPI_UINT64_T
#define MPI_ENCODED_MESSAGE_TYPE MPI_UINT8_T
#define MPI_NEXT_PACKET_LENGTH MPI_UINT32_T
using EncodedLamportTime = uint64_t;
using EncodedMessageType = uint8_t;
using EncodedNextPacketLength = uint32_t;

struct RawPacket {
    EncodedLamportTime lamportTime;
    EncodedMessageType messageType;
    EncodedNextPacketLength nextPacketLength;

    inline bool operator==(const RawPacket& other) const {
        return messageType == other.messageType && nextPacketLength == other.nextPacketLength;
    }

    inline bool operator<(const RawPacket& other) const {
        return lamportTime < other.lamportTime;
    }
};

namespace std {
    template<>
    struct hash<RawPacket> {
        inline std::size_t operator()(const RawPacket& packet) const {
            std::size_t hash = 0;
            hashCombine(hash, packet.messageType, packet.nextPacketLength);
            return hash;
        }
    };
}

using MpiTag = int;

class MpiSimpleCommunicator : public ITaggedCommunicator<MpiTag> {
public:

    Packet send(MessageType messageType, const std::string& message, const std::unordered_set<ProcessId>& recipients, MpiTag tag) override;

    Packet receive(MpiTag tag) override;

    Packet receive() override;

    std::optional<Packet> receive(long timeoutMillis, MpiTag tag) override;

    std::optional<Packet> receive(long timeoutMillis) override;

    MpiTag getDefaultTag() const override;

    LamportTime getCurrentLamportTime() override;

    MpiSimpleCommunicator(int argc, char** argv);

    virtual ~MpiSimpleCommunicator();

protected:

    static Packet toPacket(RawPacket rawPacket, ProcessId source, std::string message);

    MPI_Datatype mpiRawPacketType;
    std::recursive_mutex communicationMutex;
};

#endif //INC_3PC_MPISIMPLECOMMUNICATOR_H
