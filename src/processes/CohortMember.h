#ifndef INC_3PC_COHORTMEMBER_H
#define INC_3PC_COHORTMEMBER_H


#include <logging/Logger.h>
#include "AbstractProcess.h"

class CohortMember : public AbstractCrashableProcess {
public:

    explicit CohortMember(std::shared_ptr<MpiSimpleCommunicator> communicator)
        : AbstractCrashableProcess(std::move(communicator), MPI_CRASH_TAG) {
        state = Q;
    }

    void run() override {
        sleep();
        crashIfSignalled();
        while (not terminate) {
            switch (state) {
                case Q: {
                    logWithState("Entered state Q");
                    auto potentialPacket = getMpiCommunicator()->receive(ROUND_TIME, MPI_DEFAULT_TAG);
                    if (potentialPacket.has_value()) {
                        auto packet = potentialPacket.value();
                        if (packet.source == COORDINATOR_ID && packet.messageType == MessageType::CAN_COMMIT) {
                            logWithState("Received CAN_COMMIT request from the coordinator");
                            communicator->send(MessageType::COMMIT_AGREE, "Y", COORDINATOR_ID);
                            logWithState("Sent COMMIT_AGREE to coordinator's CAN_COMMIT request");
                            state = W;
                            break;
                        }
                        logUnexpectedPacket(packet);
                    } else {
                        logWithState("There was a timeout when receiving CAN_COMMIT");
                        communicator->send(MessageType::DO_ABORT, "", COORDINATOR_ID);
                        logWithState("Sent DO_ABORT to coordinator");
                        state = A;
                    }
                    break;
                }
                case W: {
                    logWithState("Entered state W");
                    auto potentialPacket = getMpiCommunicator()->receive(ROUND_TIME, MPI_DEFAULT_TAG);
                    if (potentialPacket.has_value()) {
                        auto packet = potentialPacket.value();
                        if (packet.source == COORDINATOR_ID) {
                            if (packet.messageType == MessageType::PREPARE_COMMIT) {
                                logWithState("Received PREPARE_COMMIT request from the coordinator");
                                communicator->send(MessageType::COMMIT_ACK, "", COORDINATOR_ID);
                                logWithState("Sent COMMIT_ACK to coordinator's PREPARE_COMMIT request");
                                state = P;
                                break;
                            } else if (packet.messageType == MessageType::DO_ABORT) {
                                logWithState("Received DO_ABORT request from the coordinator");
                                state = A;
                                break;
                            }
                        }
                        logUnexpectedPacket(packet);
                    } else {
                        logWithState("There was a timeout when receiving PREPARE_COMMIT or DO_ABORT");
                        state = A;
                    }
                    break;
                }
                case P: {
                    logWithState("Entered state P");
                    auto potentialPacket = getMpiCommunicator()->receive(ROUND_TIME, MPI_DEFAULT_TAG);
                    if (potentialPacket.has_value()) {
                        auto packet = potentialPacket.value();
                        if (packet.source == COORDINATOR_ID) {
                            if (packet.messageType == MessageType::DO_COMMIT) {
                                logWithState("Received DO_COMMIT from the coordinator");
                                state = C;
                                break;
                            } else if (packet.messageType == MessageType::DO_ABORT) {
                                logWithState("Received DO_ABORT from the coordinator");
                                state = A;
                                break;
                            }
                        }
                        logUnexpectedPacket(packet);
                    } else {
                        logWithState("There was a timeout when receiving DO_COMMIT or DO_ABORT");
                        state = C;
                    }
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
};


#endif //INC_3PC_COHORTMEMBER_H
