#include "MpiSimpleCommunicator.h"
#include <iostream>

Packet MpiSimpleCommunicator::send(MessageType messageType, const std::string& message,
                                   const std::unordered_set<ProcessId>& recipients, MpiTag tag) {

    std::lock_guard<std::recursive_mutex> lock(communicationMutex);

    RawPacket rawPacket {
            .lamportTime = static_cast<EncodedLamportTime>(++currentLamportTime),
            .messageType = static_cast<EncodedMessageType>(messageType),
            .nextPacketLength = static_cast<EncodedNextPacketLength>(message.size()),
    };

    for (ProcessId recipient : recipients) {
        MPI_Send(&rawPacket, 1, mpiRawPacketType, recipient, tag, MPI_COMM_WORLD);
        if (not message.empty()) {
            MPI_Send(message.c_str(), static_cast<int>(message.size()), MPI_CHAR, recipient, tag, MPI_COMM_WORLD);
        }
    }

    Packet packet {
            .lamportTime = rawPacket.lamportTime,
            .source = myProcessId,
            .messageType = messageType,
            .message = message
    };

    return packet;
}

Packet MpiSimpleCommunicator::receive(MpiTag tag) {
    MPI_Status status;
    RawPacket rawPacket;
    MPI_Recv(&rawPacket, 1, mpiRawPacketType, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
    ProcessId source = status.MPI_SOURCE;

    std::string message;
    uint32_t messageLength = rawPacket.nextPacketLength;
    if (messageLength > 0) {
        message.resize(messageLength);
        MPI_Recv(message.data(), messageLength, MPI_CHAR, source, tag, MPI_COMM_WORLD, &status);
    }
    {
        std::lock_guard<std::recursive_mutex> lock(communicationMutex);
        currentLamportTime = std::max(rawPacket.lamportTime, currentLamportTime) + 1;
    }
    return toPacket(rawPacket, source, message);
}

Packet MpiSimpleCommunicator::receive() {
    return receive(MPI_ANY_TAG);
}

std::optional<Packet> MpiSimpleCommunicator::receive(long timeoutMillis, MpiTag tag) {
    using namespace std::chrono;
    MPI_Status status;
    int hasReceivedData;
    RawPacket rawPacket;
    auto timeStarted = system_clock::now();

    {
        MPI_Request request;
        MPI_Irecv(&rawPacket, 1, mpiRawPacketType, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &request);

        do {
            MPI_Test(&request, &hasReceivedData, &status);
        } while (not hasReceivedData and duration_cast<milliseconds>(system_clock::now() - timeStarted).count() < timeoutMillis);
        if (not hasReceivedData) {
            MPI_Cancel(&request);
            MPI_Request_free(&request);
            return std::nullopt;
        }
    }

    ProcessId source = status.MPI_SOURCE;
    std::string message;
    uint32_t messageLength = rawPacket.nextPacketLength;
    message.resize(messageLength);
    if (messageLength > 0) {
        {
            MPI_Request request;
            MPI_Irecv(message.data(), messageLength, MPI_CHAR, source, tag, MPI_COMM_WORLD, &request);

            do {
                MPI_Test(&request, &hasReceivedData, &status);
            } while (not hasReceivedData and
                     duration_cast<milliseconds>(system_clock::now() - timeStarted).count() < timeoutMillis);
            if (not hasReceivedData) {
                MPI_Cancel(&request);
                MPI_Request_free(&request);
                return std::nullopt;
            }
        }
    }

    {
        std::lock_guard<std::recursive_mutex> lock(communicationMutex);
        currentLamportTime = std::max(rawPacket.lamportTime, currentLamportTime) + 1;
    }
    return toPacket(rawPacket, source, message);
}

std::optional<Packet> MpiSimpleCommunicator::receive(long timeoutMillis) {
    return receive(timeoutMillis, MPI_ANY_TAG);
}


MpiTag MpiSimpleCommunicator::getDefaultTag() const {
    return MPI_DEFAULT_TAG;
}

MpiSimpleCommunicator::MpiSimpleCommunicator(int argc, char** argv) {
    int provided = 0;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    /*************** Create a type for a custom 'RawPacket' structure ***************/
    const int blockLengths[] = {1, 1, 1};
    const int fields = sizeof(blockLengths) / sizeof(*blockLengths);
    MPI_Datatype types[] = {MPI_ENCODED_LAMPORT_TIME, MPI_ENCODED_MESSAGE_TYPE, MPI_NEXT_PACKET_LENGTH};
    MPI_Aint offsets[fields];

    offsets[0] = offsetof(RawPacket, lamportTime);
    offsets[1] = offsetof(RawPacket, messageType);
    offsets[2] = offsetof(RawPacket, nextPacketLength);

    MPI_Type_create_struct(fields, blockLengths, offsets, types, &mpiRawPacketType);
    MPI_Type_commit(&mpiRawPacketType);
    /*****************************************************************************/

    MPI_Comm_rank(MPI_COMM_WORLD, &myProcessId);
    MPI_Comm_size(MPI_COMM_WORLD, &numberOfProcesses);
    if (provided != MPI_THREAD_MULTIPLE) {
        std::cerr << "[Process " << myProcessId << "] Your MPI implementation is not thread-safe! "
                                                   "You need to take care of synchronization yourself." << std::endl;
    }

    /************ Fill 'otherProcesses' structure with their ranks ************/
    for (ProcessId id = 0; id < numberOfProcesses; ++id) {
        if (id != myProcessId) {
            otherProcesses.insert(id);
        }
    }
    /**************************************************************************/

    currentLamportTime = 0;
}

Packet MpiSimpleCommunicator::toPacket(RawPacket rawPacket, ProcessId source, std::string message) {
    return Packet {
            .lamportTime = static_cast<LamportTime>(rawPacket.lamportTime),
            .source = source,
            .messageType = static_cast<MessageType>(rawPacket.messageType),
            .message = std::move(message)
    };
}

LamportTime MpiSimpleCommunicator::getCurrentLamportTime() {
    std::lock_guard<std::recursive_mutex> lock(communicationMutex);
    return currentLamportTime;
}

MpiSimpleCommunicator::~MpiSimpleCommunicator() {
    MPI_Finalize();
}
