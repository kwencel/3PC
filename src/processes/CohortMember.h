#ifndef INC_3PC_COHORTMEMBER_H
#define INC_3PC_COHORTMEMBER_H


#include <logging/Logger.h>
#include "AbstractProcess.h"

template <typename Tag>
class CohortMember : public AbstractCrashableProcess<Tag> {
public:

    explicit CohortMember(std::shared_ptr<ITaggedCommunicator<Tag>> communicator, Tag defaultTag, Tag crashTag)
        : AbstractCrashableProcess<Tag>(std::move(communicator), defaultTag, crashTag) {
        this->state = Q;
    }

    void run() override {
        this->sleep();
        this->crashIfSignalled();
        while (not this->terminate) {
            switch (this->state) {
                case Q: {
                    this->logWithState("Entered state Q");
                    auto potentialPacket = this->getTaggedCommunicator()->receive(ROUND_TIME, this->defaultTag);
                    if (potentialPacket.has_value()) {
                        auto packet = potentialPacket.value();
                        if (packet.source == COORDINATOR_ID and packet.messageType == MessageType::CAN_COMMIT) {
                            this->logWithState("Received CAN_COMMIT request from the coordinator");
                            this->communicator->send(MessageType::COMMIT_AGREE, "Y", COORDINATOR_ID);
                            this->logWithState("Sent COMMIT_AGREE to coordinator's CAN_COMMIT request");
                            this->state = W;
                            break;
                        }
                        this->logUnexpectedPacket(packet);
                    } else {
                        this->logWithState("There was a timeout when receiving CAN_COMMIT");
                        this->communicator->send(MessageType::DO_ABORT, "", COORDINATOR_ID);
                        this->logWithState("Sent DO_ABORT to coordinator");
                        this->state = A;
                    }
                    break;
                }
                case W: {
                    this->logWithState("Entered state W");
                    auto potentialPacket = this->getTaggedCommunicator()->receive(ROUND_TIME, this->defaultTag);
                    if (potentialPacket.has_value()) {
                        auto packet = potentialPacket.value();
                        if (packet.source == COORDINATOR_ID) {
                            if (packet.messageType == MessageType::PREPARE_COMMIT) {
                                this->logWithState("Received PREPARE_COMMIT request from the coordinator");
                                this->communicator->send(MessageType::COMMIT_ACK, "", COORDINATOR_ID);
                                this->logWithState("Sent COMMIT_ACK to coordinator's PREPARE_COMMIT request");
                                this->state = P;
                                break;
                            } else if (packet.messageType == MessageType::DO_ABORT) {
                                this->logWithState("Received DO_ABORT request from the coordinator");
                                this->state = A;
                                break;
                            }
                        }
                        this->logUnexpectedPacket(packet);
                    } else {
                        this->logWithState("There was a timeout when receiving PREPARE_COMMIT or DO_ABORT");
                        this->state = A;
                    }
                    break;
                }
                case P: {
                    this->logWithState("Entered state P");
                    auto potentialPacket = this->getTaggedCommunicator()->receive(ROUND_TIME, this->defaultTag);
                    if (potentialPacket.has_value()) {
                        auto packet = potentialPacket.value();
                        if (packet.source == COORDINATOR_ID) {
                            if (packet.messageType == MessageType::DO_COMMIT) {
                                this->logWithState("Received DO_COMMIT from the coordinator");
                                this->state = C;
                                break;
                            } else if (packet.messageType == MessageType::DO_ABORT) {
                                this->logWithState("Received DO_ABORT from the coordinator");
                                this->state = A;
                                break;
                            }
                        }
                        this->logUnexpectedPacket(packet);
                    } else {
                        this->logWithState("There was a timeout when receiving DO_COMMIT or DO_ABORT");
                        this->state = C;
                    }
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
            this->sleep();
            this->crashIfSignalled();
        }
    }
};


#endif //INC_3PC_COHORTMEMBER_H
