#ifndef INC_3PC_DEFINE_H
#define INC_3PC_DEFINE_H

#include <map>

#define LOGGER_NUMBER_DIGITS 7
#define ROUND_TIME 10000
#define MIN_SLEEP_TIME 6000
#define MAX_SLEEP_TIME 7000
#define MIN_SLEEP_TIME_COORDINATOR 4000
#define MAX_SLEEP_TIME_COORDINATOR 5000
#define COORDINATOR_ID 0
#define MPI_CRASH_TAG 100

using ProcessId = int;
using SubscriptionId = std::size_t;
using LamportTime = unsigned long;

enum State : unsigned char {
    Q, W, A, P ,C
};

const std::map<State, std::string>  stateString = {{State::Q, "Q"},
                                                   {State::W, "W"},
                                                   {State::A, "A"},
                                                   {State::P, "P"},
                                                   {State::C, "C"}};

inline std::ostream& operator<< (std::ostream& os, State messageType) {
    return os << stateString.at(messageType);
}

inline std::string& operator+ (std::string& str, State messageType) {
    return str.append(stateString.at(messageType));
}

namespace std {
    template<>
    struct hash<State> {
        inline int operator()(const State& state) const {
            return static_cast<std::underlying_type<State>::type>(state);
        }
    };
}

enum class MessageType : unsigned char {
    CAN_COMMIT, PREPARE_COMMIT, DO_COMMIT, DO_ABORT, COMMIT_AGREE, COMMIT_ACK, CRASH
};

const std::map<MessageType, std::string>  messageTypeString = {{MessageType::CAN_COMMIT, "CAN_COMMIT"},
                                                               {MessageType::PREPARE_COMMIT, "PREPARE_COMMIT"},
                                                               {MessageType::DO_COMMIT, "DO_COMMIT"},
                                                               {MessageType::DO_ABORT, "DO_ABORT"},
                                                               {MessageType::COMMIT_AGREE, "COMMIT_AGREE"},
                                                               {MessageType::COMMIT_ACK, "COMMIT_ACK"},
                                                               {MessageType::CRASH, "CRASH"}};

inline std::ostream& operator<< (std::ostream& os, MessageType messageType) {
    return os << messageTypeString.at(messageType);
}

inline std::string& operator+ (std::string& str, MessageType messageType) {
    return str.append(messageTypeString.at(messageType));
}

namespace std {
    template<>
    struct hash<MessageType> {
        inline int operator()(const MessageType& messageType) const {
            return static_cast<std::underlying_type<MessageType>::type>(messageType);
        }
    };
}

#endif //INC_3PC_DEFINE_H
