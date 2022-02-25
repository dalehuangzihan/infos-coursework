//
// Created by dalehuang on 2/24/22.
//

#include <infos/kernel/sched.h>
#include <infos/kernel/thread.h>
#include <infos/kernel/log.h>
#include <infos/util/list.h>
#include <infos/util/lock.h>

using namespace infos::kernel;
using namespace infos::util;

class O1MQPriorityScheduler : public SchedulingAlgorithm
{
public:
    /**
     * Returns the friendly name of the algorithm, for debugging and selection purposes.
     */
    const char* name() const override { return "o1mq"; }

    /**
     * Run-queues (Alpha session) for realtime, interactive, normal, daemon:
     */
    private: List<SchedulingEntity *> rq_realtime_A;
    private: List<SchedulingEntity *> rq_interactive_A;
    private: List<SchedulingEntity *> rq_normal_A;
    private: List<SchedulingEntity *> rq_daemon_A;

    /**
     * Run-queues (Beta session) for realtime, interactive, normal, daemon:
     */
    private: List<SchedulingEntity *> rq_realtime_B;
    private: List<SchedulingEntity *> rq_interactive_B;
    private: List<SchedulingEntity *> rq_normal_B;
    private: List<SchedulingEntity *> rq_daemon_B;

    // flag to control which session is active:
    private: bool is_session_A_active = true;

    /**
     * Called during scheduler initialisation.
     */
    void init()
    {
        // TODO: Implement me!
    }

    /**
     * Called when a scheduling entity becomes eligible for running.
     * Adds tasks to the currently-inactive session queues.
     * e.g. if Alpha session is active, add new tasks to the Beta session runqueues.
     * @param entity
     */
    void add_to_runqueue(SchedulingEntity& entity) override
    {
        // disable interrupts before modifying runqueue:
        UniqueIRQLock l;

        if (is_session_A_active) {
            // add new tasks to Beta session runqueues:
            // based on the entity's priority, enqueue into appropriate runqueue:
            switch(entity.priority()) {
                case SchedulingEntityPriority::REALTIME:
                    rq_realtime_B.enqueue(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] enqueued to Realtime runqueue.", entity.name().c_str());
                    break;
                case SchedulingEntityPriority::INTERACTIVE:
                    rq_interactive_B.enqueue(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] enqueued to Interactive runqueue.", entity.name().c_str());
                    break;
                case SchedulingEntityPriority::NORMAL:
                    rq_normal_B.enqueue(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] enqueued to Normal runqueue.", entity.name().c_str());
                    break;
                case SchedulingEntityPriority::DAEMON:
                    rq_daemon_B.enqueue(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] enqueued to Daemon runqueue.", entity.name().c_str());
                    break;
                default:
                    // do nothing
                    break;
            }
        } else {
            // add new tasks to Alpha session runqueues:
            // based on the entity's priority, enqueue into appropriate runqueue:
            switch(entity.priority()) {
                case SchedulingEntityPriority::REALTIME:
                    rq_realtime_A.enqueue(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] enqueued to Realtime runqueue.", entity.name().c_str());
                    break;
                case SchedulingEntityPriority::INTERACTIVE:
                    rq_interactive_A.enqueue(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] enqueued to Interactive runqueue.", entity.name().c_str());
                    break;
                case SchedulingEntityPriority::NORMAL:
                    rq_normal_A.enqueue(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] enqueued to Normal runqueue.", entity.name().c_str());
                    break;
                case SchedulingEntityPriority::DAEMON:
                    rq_daemon_A.enqueue(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] enqueued to Daemon runqueue.", entity.name().c_str());
                    break;
                default:
                    // do nothing
                    break;
            }
        }

    }

    /**
     * Called when a scheduling entity is no longer eligible for running.
     * @param entity
     */
    void remove_from_runqueue(SchedulingEntity& entity) override
    {
        // disable interrupts before modifying runqueue:
        UniqueIRQLock l;

        // searches runqueues on both Alpha and Beta sessions and removes entity from them.
        // based on the entity's priority, remove from the appropriate runqueue:
        switch(entity.priority()) {
            case SchedulingEntityPriority::REALTIME:
                if (rq_realtime_A.empty() && rq_realtime_B.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Realtime runqueues are empty! Entity [%s] not removed.", entity.name().c_str());
                } else {
                    rq_realtime_A.remove(&entity);
                    rq_realtime_B.remove(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] removed from Realtime runqueues", entity.name().c_str());
                }
                break;

            case SchedulingEntityPriority::INTERACTIVE:
                if (rq_interactive_A.empty() && rq_interactive_B.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Interactive runqueues are empty! Entity [%s] not removed.", entity.name().c_str());
                } else {
                    rq_interactive_A.remove(&entity);
                    rq_interactive_B.remove(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] removed from Interactive runqueues", entity.name().c_str());
                }
                break;

            case SchedulingEntityPriority::NORMAL:
                if (rq_normal_A.empty() && rq_normal_B.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Normal runqueues are empty! Entity [%s] not removed.", entity.name().c_str());
                } else {
                    rq_normal_A.remove(&entity);
                    rq_normal_B.remove(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] removed from Normal runqueues", entity.name().c_str());
                }
                break;

            case SchedulingEntityPriority::DAEMON:
                if (rq_daemon_A.empty() && rq_daemon_B.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Daemon runqueues are empty! Entity [%s] not removed.", entity.name().c_str());
                } else {
                    rq_daemon_A.remove(&entity);
                    rq_daemon_B.remove(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] removed from Daemon runqueues", entity.name().c_str());
                }
                break;
            default:
                // do nothing
                break;
        }

    }

    /**
     * Called every time a scheduling event occurs, to cause the next eligible entity
     * to be chosen.  The next eligible entity might actually be the same entity, if
     * e.g. its timeslice has not expired.
     *
     * Only runnable tasks can be scheduled onto a CPU!
     * For a task in a particular queue to be scheduled, all the higher priority queues must
     * be EMPTY at the point when the scheduling event occurs.
     */
    SchedulingEntity *pick_next_entity() override
    {
        // disable interrupts before modifying runqueue:
        UniqueIRQLock l;

        // deal with runqueues in order of priority:
        if (is_session_A_active) {
            if (!rq_realtime_A.empty()) {
                return getEntityFromRunqueue(rq_realtime_A, rq_realtime_B);
            } else if (!rq_interactive_A.empty()) {
                return getEntityFromRunqueue(rq_interactive_A, rq_interactive_B);
            } else if (!rq_normal_A.empty()) {
                return getEntityFromRunqueue(rq_normal_A, rq_normal_B);
            } else if (!rq_daemon_A.empty()) {
                return getEntityFromRunqueue(rq_daemon_A, rq_daemon_B);
            } else {

                // all priority queues in this session are empty; toggle active session to the other session:
                is_session_A_active = false;
                syslog.messagef(LogLevel::INFO, "Switching to Beta session");
                if (!rq_realtime_B.empty()) {
                    return getEntityFromRunqueue(rq_realtime_B, rq_realtime_A);
                } else if (!rq_interactive_B.empty()) {
                    return getEntityFromRunqueue(rq_interactive_B, rq_interactive_A);
                } else if (!rq_normal_B.empty()) {
                    return getEntityFromRunqueue(rq_normal_B, rq_normal_A);
                } else if (!rq_daemon_B.empty()) {
                    return getEntityFromRunqueue(rq_daemon_B, rq_daemon_A);
                } else {
                    // all priority queues in both sessions are empty:
                    return NULL;
                }
            }
        } else {
            if (!rq_realtime_B.empty()) {
                return getEntityFromRunqueue(rq_realtime_B, rq_realtime_A);
            } else if (!rq_interactive_B.empty()) {
                return getEntityFromRunqueue(rq_interactive_B, rq_interactive_A);
            } else if (!rq_normal_B.empty()) {
                return getEntityFromRunqueue(rq_normal_B, rq_normal_A);
            } else if (!rq_daemon_B.empty()) {
                return getEntityFromRunqueue(rq_daemon_B, rq_daemon_A);
            } else {

                // all priority queues in this session are empty; toggle active session to the other session:
                syslog.messagef(LogLevel::INFO, "Switching to Alpha session");
                is_session_A_active = true;
                if (!rq_realtime_A.empty()) {
                    return getEntityFromRunqueue(rq_realtime_A, rq_realtime_B);
                } else if (!rq_interactive_A.empty()) {
                    return getEntityFromRunqueue(rq_interactive_A, rq_interactive_B);
                } else if (!rq_normal_A.empty()) {
                    return getEntityFromRunqueue(rq_normal_A, rq_normal_B);
                } else if (!rq_daemon_A.empty()) {
                    return getEntityFromRunqueue(rq_daemon_A, rq_daemon_B);
                } else {
                    // all priority queues in both sessions are empty:
                    return NULL;
                }
            }
        }

    }

    /**
     * Helper function for pick_next_entity().
     * Pops the first still-runnable entity from the queue and returns it.
     * Enqueues the to-be-returned entity back to the end of the active_runqueue
     * @param active_runqueue is the active_runqueue that we wish to obtain the scheduling entity from.
     * @return the next scheduling entity in the active_runqueue.
     */
    static SchedulingEntity *getEntityFromRunqueue(List<SchedulingEntity *>& active_runqueue, List<SchedulingEntity *>& idle_runqueue) {
        // pop entity from start of active queue:
        SchedulingEntity* entityPtr = active_runqueue.pop();
        // enqueue entity to end of inactive queue:
        idle_runqueue.enqueue(entityPtr);
        return entityPtr;
    }

};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

RegisterScheduler(O1MQPriorityScheduler);