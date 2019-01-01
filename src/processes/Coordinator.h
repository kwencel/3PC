#ifndef INC_3PC_COORDINATOR_H
#define INC_3PC_COORDINATOR_H


#include <logging/Logger.h>
#include "AbstractCrashableProcess.h"

class Coordinator : public AbstractCrashableProcess {
public:

    explicit Coordinator(std::shared_ptr<MpiSimpleCommunicator> communicator)
        : AbstractCrashableProcess(std::move(communicator), MPI_CRASH_TAG) {
        std::thread([&]{ processCrashInput(); }).detach();
        state = Q;
    }

    void run() override {
        Logger::log("Initializing 3PC");
        sleep();
        crashIfSignalled();
        while (not terminate) {
            switch (state) {
                case Q: {
                    logWithState("Entered state Q");
                    communicator->sendOthers(MessageType::CAN_COMMIT, "");
                    logWithState("Sent CAN_COMMIT to the cohort");
                    state = W;
                    break;
                }
                case W: {
                    logWithState("Entered state W");
                    auto potentialPackets = receiveFromAll(ROUND_TIME);
                    logWithState("Finished gathering responses for CAN_COMMIT from the cohort");
                    if (potentialPackets.has_value()) {
                        auto packets = potentialPackets.value();
                        if (checkPackets(packets, MessageType::COMMIT_AGREE, "Y")) {
                            logWithState("Got positive response from every cohort member for CAN_COMMIT request");
                            communicator->sendOthers(MessageType::PREPARE_COMMIT, "");
                            logWithState("Sent PREPARE_COMMIT to the cohort");
                            state = P;
                            break;
                        }
                        logWithState("Some cohort members did not agree to commit");
                    } else {
                        logWithState("There was a timeout - some cohort members did not sent their vote");
                    }
                    communicator->sendOthers(MessageType::DO_ABORT, "");
                    logWithState("Sent DO_ABORT to the cohort because did not get agreement from every cohort member");
                    state = A;
                    break;
                }
                case P: {
                    logWithState("Entered state P");
                    auto potentialPackets = receiveFromAll(ROUND_TIME);
                    logWithState("Finished gathering responses for PREPARE_COMMIT from the cohort");
                    if (potentialPackets.has_value()) {
                        if (checkPackets(potentialPackets.value(), MessageType::COMMIT_ACK, "")) {
                            logWithState("Got COMMIT_ACK from every cohort member");
                            communicator->sendOthers(MessageType::DO_COMMIT, "");
                            logWithState("Sent DO_COMMIT to the cohort");
                            state = C;
                            break;
                        }
                        logWithState("Some cohort members sent an unexpected message");
                    } else {
                        logWithState("There was a timeout - some cohort member did not acknowledge");
                    }
                    communicator->sendOthers(MessageType::DO_ABORT, "");
                    logWithState("Sent DO_ABORT to the cohort because of a missing acknowledgement");
                    state = A;
                    break;
                }
                case A: {
                    logWithState("Entered state A - aborted the transaction!");
                    terminate = true;
                    return;
                };
                case C: {
                    logWithState("Entered state C - committed the transaction!");
                    terminate = true;
                    return;
                }
            }
            sleep();
            crashIfSignalled();
        }
    }

    void sleep() override {
        std::this_thread::sleep_for(std::chrono::milliseconds(random.randomBetween(MIN_SLEEP_TIME_COORDINATOR, MAX_SLEEP_TIME_COORDINATOR)));
    }

private:

    /**
     * Receives all messages until it gets one from every process or a timeout occurs.
     * @param timeoutMillis Threshold which aborts the receiving process when reached
     * @return Set of received messages if every process participated or nullopt otherwise
     */
    std::optional<std::unordered_set<Packet>> receiveFromAll(long timeoutMillis) {
        using namespace std::chrono;
        std::unordered_set<ProcessId> responders;
        std::unordered_set<Packet> packets;
        auto timeStarted = system_clock::now();
        do {
            auto communicator = getMpiCommunicator();
            auto remainingTimeout = timeoutMillis - duration_cast<milliseconds>(system_clock::now() - timeStarted).count();
            auto optionalPacket = communicator->receive(remainingTimeout, MPI_DEFAULT_TAG);
            if (optionalPacket.has_value()) {
                auto packet = optionalPacket.value();
                packets.insert(packet);
                responders.insert(packet.source);
                if (responders.size() == (unsigned) communicator->getNumberOfProcesses() - 1) {
                    return packets;
                }
            } else {
                // Round time elapsed
                return std::nullopt;
            }
        } while (true);
    }

    template <typename Container>
    bool checkPackets(Container packets, MessageType expectedType, const std::string& expectedMessage) {
        return std::all_of(packets.begin(), packets.end(), [&](Packet p) { return p.messageType == expectedType &&
                                                                                  p.message == expectedMessage; });
    }

    void processCrashInput() {
        while (true) {
            Logger::registerThread("Input");
            ProcessId processToKill;
            std::cin >> processToKill;
            if (processToKill == communicator->getProcessId()) {
                crashSignalReceived = true;
                Logger::log("Killing the coordinator");
            } else if (processToKill >= 0 && processToKill < communicator->getNumberOfProcesses()) {
                getMpiCommunicator()->send(MessageType::CRASH, "", processToKill, MPI_CRASH_TAG);
                Logger::log(util::concat("Killing the process ", processToKill));
            } else {
                Logger::log(util::concat("Unexpected input '", processToKill, "'", " - ignoring"));
            }
        }
    }
};


#endif //INC_3PC_COORDINATOR_H
