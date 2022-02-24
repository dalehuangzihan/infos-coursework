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
    private: List<SchedulingEntity *> rqRealtimeAlpha;
    private: List<SchedulingEntity *> rqInteractiveAlpha;
    private: List<SchedulingEntity *> rqNormalAlpha;
    private: List<SchedulingEntity *> rqDaemonAlpha;

    /**
     * Run-queues (Beta session) for realtime, interactive, normal, daemon:
     */
    private: List<SchedulingEntity *> rqRealtimeBeta;
    private: List<SchedulingEntity *> rqInteractiveBeta;
    private: List<SchedulingEntity *> rqNormalBeta;
    private: List<SchedulingEntity *> rqDaemonBeta;

    // flag to control which session is active:
    private: bool isAlphaSessionActive = true;

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

        if (isAlphaSessionActive) {
            // add new tasks to Beta session runqueues:
            // based on the entity's priority, enqueue into appropriate runqueue:
            switch(entity.priority()) {
                case SchedulingEntityPriority::REALTIME:
                    rqRealtimeBeta.enqueue(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] enqueued to Realtime runqueue.", entity.name().c_str());
                    break;
                case SchedulingEntityPriority::INTERACTIVE:
                    rqInteractiveBeta.enqueue(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] enqueued to Interactive runqueue.", entity.name().c_str());
                    break;
                case SchedulingEntityPriority::NORMAL:
                    rqNormalBeta.enqueue(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] enqueued to Normal runqueue.", entity.name().c_str());
                    break;
                case SchedulingEntityPriority::DAEMON:
                    rqDaemonBeta.enqueue(&entity);
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
                    rqRealtimeAlpha.enqueue(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] enqueued to Realtime runqueue.", entity.name().c_str());
                    break;
                case SchedulingEntityPriority::INTERACTIVE:
                    rqInteractiveAlpha.enqueue(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] enqueued to Interactive runqueue.", entity.name().c_str());
                    break;
                case SchedulingEntityPriority::NORMAL:
                    rqNormalAlpha.enqueue(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] enqueued to Normal runqueue.", entity.name().c_str());
                    break;
                case SchedulingEntityPriority::DAEMON:
                    rqDaemonAlpha.enqueue(&entity);
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
                if (rqRealtimeAlpha.empty() && rqRealtimeBeta.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Realtime runqueues are empty! Entity [%s] not removed.", entity.name().c_str());
                } else {
                    rqRealtimeAlpha.remove(&entity);
                    rqRealtimeBeta.remove(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] removed from Realtime runqueues", entity.name().c_str());
                }
                break;

            case SchedulingEntityPriority::INTERACTIVE:
                if (rqInteractiveAlpha.empty() && rqInteractiveBeta.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Interactive runqueues are empty! Entity [%s] not removed.", entity.name().c_str());
                } else {
                    rqInteractiveAlpha.remove(&entity);
                    rqInteractiveBeta.remove(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] removed from Interactive runqueues", entity.name().c_str());
                }
                break;

            case SchedulingEntityPriority::NORMAL:
                if (rqNormalAlpha.empty() && rqNormalBeta.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Normal runqueues are empty! Entity [%s] not removed.", entity.name().c_str());
                } else {
                    rqNormalAlpha.remove(&entity);
                    rqNormalBeta.remove(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity [%s] removed from Normal runqueues", entity.name().c_str());
                }
                break;

            case SchedulingEntityPriority::DAEMON:
                if (rqDaemonAlpha.empty() && rqDaemonBeta.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Daemon runqueues are empty! Entity [%s] not removed.", entity.name().c_str());
                } else {
                    rqDaemonAlpha.remove(&entity);
                    rqDaemonBeta.remove(&entity);
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
        if (isAlphaSessionActive) {
            if (!rqRealtimeAlpha.empty()) {
                return getEntityFromRunqueue(&rqRealtimeAlpha, &rqRealtimeBeta);
            } else if (!rqInteractiveAlpha.empty()) {
                return getEntityFromRunqueue(&rqInteractiveAlpha, &rqInteractiveBeta);
            } else if (!rqNormalAlpha.empty()) {
                return getEntityFromRunqueue(&rqNormalAlpha, &rqNormalBeta);
            } else if (!rqDaemonAlpha.empty()) {
                return getEntityFromRunqueue(&rqDaemonAlpha, &rqDaemonBeta);
            } else {
                // all priority queues in this session are empty; toggle active session to the other session:
                isAlphaSessionActive = false;
                if (!rqRealtimeBeta.empty()) {
                    return getEntityFromRunqueue(&rqRealtimeBeta, &rqRealtimeAlpha);
                } else if (!rqInteractiveBeta.empty()) {
                    return getEntityFromRunqueue(&rqInteractiveBeta, &rqInteractiveAlpha);
                } else if (!rqNormalBeta.empty()) {
                    return getEntityFromRunqueue(&rqNormalBeta, &rqNormalAlpha);
                } else if (!rqDaemonBeta.empty()) {
                    return getEntityFromRunqueue(&rqDaemonBeta, &rqDaemonAlpha);
                } else {
                    // all priority queues in this session are empty; toggle active session to the other session:
                    isAlphaSessionActive = false;
                    return NULL;
                }
            }
        } else {
            if (!rqRealtimeBeta.empty()) {
                return getEntityFromRunqueue(&rqRealtimeBeta, &rqRealtimeAlpha);
            } else if (!rqInteractiveBeta.empty()) {
                return getEntityFromRunqueue(&rqInteractiveBeta, &rqInteractiveAlpha);
            } else if (!rqNormalBeta.empty()) {
                return getEntityFromRunqueue(&rqNormalBeta, &rqNormalAlpha);
            } else if (!rqDaemonBeta.empty()) {
                return getEntityFromRunqueue(&rqDaemonBeta, &rqDaemonAlpha);
            } else {
                // all priority queues in this session are empty; toggle active session to the other session:
                isAlphaSessionActive = false;
                if (!rqRealtimeAlpha.empty()) {
                    return getEntityFromRunqueue(&rqRealtimeAlpha, &rqRealtimeBeta);
                } else if (!rqInteractiveAlpha.empty()) {
                    return getEntityFromRunqueue(&rqInteractiveAlpha, &rqInteractiveBeta);
                } else if (!rqNormalAlpha.empty()) {
                    return getEntityFromRunqueue(&rqNormalAlpha, &rqNormalBeta);
                } else if (!rqDaemonAlpha.empty()) {
                    return getEntityFromRunqueue(&rqDaemonAlpha, &rqDaemonBeta);
                } else {
                    return NULL;
                }
            }
        }

    }

    /**
     * Helper function for pick_next_entity().
     * Pops the first still-runnable entity from the queue and returns it.
     * Enqueues the to-be-returned entity back to the end of the activeRunqueue
     * @param activeRunqueue is the activeRunqueue that we wish to obtain the scheduling entity from.
     * @return the next scheduling entity in the activeRunqueue.
     */
    static SchedulingEntity *getEntityFromRunqueue(List<SchedulingEntity *> *activeRunqueue, List<SchedulingEntity *> *idleRunqueue) {
        // pop entity from start of active queue:
        SchedulingEntity* entityPtr = activeRunqueue->pop();
        // enqueue entity to end of inactive queue:
        idleRunqueue->enqueue(entityPtr);
        return entityPtr;
    }

};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

RegisterScheduler(O1MQPriorityScheduler);