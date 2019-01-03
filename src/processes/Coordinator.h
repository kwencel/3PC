#ifndef INC_3PC_COORDINATOR_H
#define INC_3PC_COORDINATOR_H


#include <logging/Logger.h>
#include "AbstractCrashableProcess.h"

template <typename Tag>
class Coordinator : public AbstractCrashableProcess<Tag> {
public:

    explicit Coordinator(std::shared_ptr<ITaggedCommunicator<Tag>> communicator, Tag defaultTag, Tag crashTag)
        : AbstractCrashableProcess<Tag>(std::move(communicator), defaultTag, crashTag) {
        std::thread([&]{ processCrashInput(); }).detach();
        this->state = Q;
    }

    void run() override {
        Logger::log("Initializing 3PC");
        sleep();
        this->crashIfSignalled();
        while (not this->terminate) {
            switch (this->state) {
                case Q: {
                    this->logWithState("Entered state Q");
                    this->communicator->sendOthers(MessageType::CAN_COMMIT, "");
                    this->logWithState("Sent CAN_COMMIT to the cohort");
                    this->state = W;
                    break;
                }
                case W: {
                    this->logWithState("Entered state W");
                    auto potentialPackets = receiveFromAll(ROUND_TIME);
                    this->logWithState("Finished gathering responses for CAN_COMMIT from the cohort");
                    if (potentialPackets.has_value()) {
                        auto packets = potentialPackets.value();
                        if (checkPackets(packets, MessageType::COMMIT_AGREE, "Y")) {
                            this->logWithState("Got positive response from every cohort member for CAN_COMMIT request");
                            this->communicator->sendOthers(MessageType::PREPARE_COMMIT, "");
                            this->logWithState("Sent PREPARE_COMMIT to the cohort");
                            this->state = P;
                            break;
                        }
                        this->logWithState("Some cohort members did not agree to commit");
                    } else {
                        this->logWithState("There was a timeout - some cohort members did not sent their vote");
                    }
                    this->communicator->sendOthers(MessageType::DO_ABORT, "");
                    this->logWithState("Sent DO_ABORT to the cohort because did not get agreement from every cohort member");
                    this->state = A;
                    break;
                }
                case P: {
                    this->logWithState("Entered state P");
                    auto potentialPackets = receiveFromAll(ROUND_TIME);
                    this->logWithState("Finished gathering responses for PREPARE_COMMIT from the cohort");
                    if (potentialPackets.has_value()) {
                        if (checkPackets(potentialPackets.value(), MessageType::COMMIT_ACK, "")) {
                            this->logWithState("Got COMMIT_ACK from every cohort member");
                            this->communicator->sendOthers(MessageType::DO_COMMIT, "");
                            this->logWithState("Sent DO_COMMIT to the cohort");
                            this->state = C;
                            break;
                        }
                        this->logWithState("Some cohort members sent an unexpected message");
                    } else {
                        this->logWithState("There was a timeout - some cohort member did not acknowledge");
                    }
                    this->communicator->sendOthers(MessageType::DO_ABORT, "");
                    this->logWithState("Sent DO_ABORT to the cohort because of a missing acknowledgement");
                    this->state = A;
                    break;
                }
                case A: {
                    this->logWithState("Entered state A - aborted the transaction!");
                    this->terminate = true;
                    return;
                };
                case C: {
                    this->logWithState("Entered state C - committed the transaction!");
                    this->terminate = true;
                    return;
                }
            }
            sleep();
            this->crashIfSignalled();
        }
    }

    void sleep() override {
        std::this_thread::sleep_for(std::chrono::milliseconds(this->random.randomBetween(MIN_SLEEP_TIME_COORDINATOR, MAX_SLEEP_TIME_COORDINATOR)));
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
            auto taggedCommunicator = this->getTaggedCommunicator();
            auto remainingTimeout = timeoutMillis - duration_cast<milliseconds>(system_clock::now() - timeStarted).count();
            auto optionalPacket = taggedCommunicator->receive(remainingTimeout, this->defaultTag);
            if (optionalPacket.has_value()) {
                auto packet = optionalPacket.value();
                packets.insert(packet);
                responders.insert(packet.source);
                if (responders.size() == (unsigned) taggedCommunicator->getNumberOfProcesses() - 1) {
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
            if (processToKill == this->communicator->getProcessId()) {
                this->crashSignalReceived = true;
                Logger::log("Killing the coordinator");
            } else if (processToKill >= 0 && processToKill < this->communicator->getNumberOfProcesses()) {
                this->getTaggedCommunicator()->send(MessageType::CRASH, "", processToKill, this->crashTag);
                Logger::log(util::concat("Killing the process ", processToKill));
            } else {
                Logger::log(util::concat("Unexpected input '", processToKill, "'", " - ignoring"));
            }
        }
    }
};


#endif //INC_3PC_COORDINATOR_H
