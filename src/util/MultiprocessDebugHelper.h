#ifndef INC_3PC_MULTIPROCESSDEBUGHELPER_H
#define INC_3PC_MULTIPROCESSDEBUGHELPER_H

/**
 * Attaches gdbserver to itself on a given port to make debugging with gdb (client) easier in distributed
 * environment like MPI.
 */
class MultiprocessDebugHelper {
public:
    /**
     * Attaches gdbserver to this program's process.
     * @param port gdbserver will listen on
     * @return gdbserver PID
     */
    static int setup(int port);

    MultiprocessDebugHelper() = delete;
    ~MultiprocessDebugHelper() = delete;

private:
    static void cleanup([[maybe_unused]] int i);

    static int gdbServerPid;
};

#endif //INC_3PC_MULTIPROCESSDEBUGHELPER_H
