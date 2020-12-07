#include "os_process.h"

/*!
 *  Checks whether given process is runnable
 *
 *  \param process A pointer on the process
 *  \return The state of the process, 1 if it is ready or running, 0 otherwise
 */
bool os_isRunnable(Process const* process) {
    if (process && (process->state == OS_PS_READY || process->state == OS_PS_RUNNING)) {
        return true;
    }

    return false;
}
