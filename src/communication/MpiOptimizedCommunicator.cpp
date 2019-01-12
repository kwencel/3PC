#include "MpiOptimizedCommunicator.h"

Packet MpiOptimizedCommunicator::send(MessageType messageType, const std::string& message,
                                      const std::unordered_set<ProcessId>& recipients, MpiTag tag) {

    std::lock_guard<std::recursive_mutex> lock(communicationMutex);
    std::string finalMessage = encode(++currentLamportTime, messageType, message);

    for (ProcessId recipient : recipients) {
        MPI_Send(finalMessage.c_str(), static_cast<int>(finalMessage.size()), MPI_BYTE, recipient, tag, MPI_COMM_WORLD);
    }

    return Packet {
            .lamportTime = currentLamportTime,
            .source = myProcessId,
            .messageType = messageType,
            .message = message
    };
}

Packet MpiOptimizedCommunicator::receive(MpiTag tag) {
    MPI_Status status;
    int messageLength;

    MPI_Probe(MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_BYTE, &messageLength);

    ProcessId source = status.MPI_SOURCE;
    std::string message;
    message.resize(static_cast<unsigned long>(messageLength));
    MPI_Recv(message.data(), messageLength, MPI_BYTE, source, tag, MPI_COMM_WORLD, &status);

    Packet packet = getPacket(message, source);
    updateTimestamp(packet);
    return packet;
}

std::optional<Packet> MpiOptimizedCommunicator::receive(long timeoutMillis, MpiTag tag) {
    using namespace std::chrono;
    MPI_Status status;
    int hasReceivedData;
    auto timeStarted = system_clock::now();

    do {
        MPI_Iprobe(MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &hasReceivedData, &status);
    } while (not hasReceivedData &&
             duration_cast<milliseconds>(system_clock::now() - timeStarted).count() < timeoutMillis);
    if (not hasReceivedData) {
        return std::nullopt;
    }

    ProcessId source = status.MPI_SOURCE;
    int messageLength;
    MPI_Get_count(&status, MPI_BYTE, &messageLength);
    std::string message;
    message.resize(static_cast<unsigned long>(messageLength));
    MPI_Recv(message.data(), messageLength, MPI_BYTE, source, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    Packet packet = getPacket(message, source);
    updateTimestamp(packet);
    return packet;
}

std::string MpiOptimizedCommunicator::encode(LamportTime lamportTime, MessageType messageType, const std::string& message) {
    std::string finalMessage;
    const auto encodedLamportTime = static_cast<EncodedLamportTime>(lamportTime);
    const auto encodedMessageType = static_cast<EncodedMessageType>(messageType);
    finalMessage.resize(sizeof(encodedLamportTime) + sizeof(encodedMessageType) + message.size());
    *(reinterpret_cast<EncodedLamportTime*>(finalMessage.data())) = encodedLamportTime;
    *(reinterpret_cast<EncodedMessageType*>(finalMessage.data() + sizeof(encodedLamportTime))) = encodedMessageType;
    message.copy(finalMessage.data() + sizeof(encodedLamportTime) + sizeof(encodedMessageType), message.size());
    return finalMessage;
}

Packet MpiOptimizedCommunicator::getPacket(const std::string& encodedMessage, ProcessId source) {
    const auto lamportTime = static_cast<LamportTime>(*reinterpret_cast<const EncodedLamportTime*>(encodedMessage.data()));
    const auto messageType = static_cast<MessageType>(*reinterpret_cast<const EncodedMessageType*>(encodedMessage.data() + sizeof(EncodedLamportTime)));
    const auto headerSize = sizeof(EncodedLamportTime) + sizeof(EncodedMessageType);

    return Packet {
            .lamportTime = lamportTime,
            .source = source,
            .messageType = messageType,
            .message = encodedMessage.substr(headerSize)
    };
}

void MpiOptimizedCommunicator::updateTimestamp(Packet& packet) {
    {
        std::lock_guard<std::recursive_mutex> lock(communicationMutex);
        currentLamportTime = std::max(packet.lamportTime, currentLamportTime) + 1;
    }
    packet.lamportTime = currentLamportTime;
}

MpiOptimizedCommunicator::MpiOptimizedCommunicator(int argc, char** argv) : MpiSimpleCommunicator(argc, argv) { }
