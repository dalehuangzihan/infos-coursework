/*
 * The Priority Task Scheduler
 * SKELETON IMPLEMENTATION TO BE FILLED IN FOR TASK 1
 */

#include <infos/kernel/sched.h>
#include <infos/kernel/thread.h>
#include <infos/kernel/log.h>
#include <infos/util/list.h>
#include <infos/util/lock.h>

using namespace infos::kernel;
using namespace infos::util;

/**
 * A Multiple Queue priority scheduling algorithm
 */
class MultipleQueuePriorityScheduler : public SchedulingAlgorithm
{
public:
    /**
     * Returns the friendly name of the algorithm, for debugging and selection purposes.
     */
    const char* name() const override { return "mq"; }

    /**
     * Run-queuese for realtime, interactive, normal, daemon:
     */
    private: List<SchedulingEntity *> rqRealtime;
    private: List<SchedulingEntity *> rqInteractive;
    private: List<SchedulingEntity *> rqNormal;
    private: List<SchedulingEntity *> rqDaemon;

    /**
     * Called during scheduler initialisation.
     */
    void init()
    {
        // TODO: Implement me!
    }

    /**
     * Called when a scheduling entity becomes eligible for running.
     * @param entity
     */
    void add_to_runqueue(SchedulingEntity& entity) override
    {
        // TODO: Implement me!

        // disable interrupts before modifying runqueue:
        UniqueIRQLock l;
        //based on the entity's priority, enqueue into appropriate runqueue:
        switch(entity.priority()) {
            case SchedulingEntityPriority::REALTIME:
                rqRealtime.enqueue(&entity);
                break;
            case SchedulingEntityPriority::INTERACTIVE:
                rqInteractive.enqueue(&entity);
                break;
            case SchedulingEntityPriority::NORMAL:
                rqNormal.enqueue(&entity);
                break;
            case SchedulingEntityPriority::DAEMON:
                rqDaemon.enqueue(&entity);
                break;
            default:
                // do nothing
                break;
        } 
    }

    /**
     * Called when a scheduling entity is no longer eligible for running.
     * @param entity
     */
    void remove_from_runqueue(SchedulingEntity& entity) override
    {
        // TODO: Implement me!
        
        // disable interrupts before modifying runqueue:
        UniqueIRQLock l;
        // based on the entity's priority, remove from the appropriate runqueue:
        switch(entity.priority()) {
            case SchedulingEntityPriority::REALTIME:
                if (rqRealtime.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Realtime runqueue is empty! Entity not removed.");
                } else {
                    rqRealtime.remove(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity removed from Realtime runqueue");
                }
                break;

            case SchedulingEntityPriority::INTERACTIVE:
                if (rqInteractive.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Interactive runqueue is empty! Entity not removed.");
                } else {
                    rqInteractive.remove(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity removed from Interactive runqueue");
                }
                break;

            case SchedulingEntityPriority::NORMAL:
                if (rqNormal.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Normal runqueue is empty! Entity not removed.");
                } else {
                    rqNormal.remove(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity removed from Normal runqueue");
                }
                break;

            case SchedulingEntityPriority::DAEMON:
                if (rqDaemon.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Daemon runqueue is empty! Entity not removed.");
                } else {
                    rqDaemon.remove(&entity);
                    syslog.messagef(LogLevel::INFO, "Entity removed from Daemon runqueue");
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
        // TODO: Implement me!

        // disable interrupts before modifying runqueue:
        UniqueIRQLock l;
        // deal with runqueues in order of priority:
        if (!rqRealtime.empty()) {
            return getEntityFromRunqueue(&rqRealtime);
        } else if (!rqInteractive.empty()) {
            return getEntityFromRunqueue(&rqInteractive);
        } else if (!rqNormal.empty()) {
            return getEntityFromRunqueue(&rqNormal);
        } else if (!rqDaemon.empty()) {
            return getEntityFromRunqueue(&rqDaemon);
        } else {
            return NULL;
        }
    }

    /**
     * Gets the first still-runnable entity from the queue and returns it.
     * @param runqueue is the runqueue that we wish to obtain the scheduling entity from.
     * @return the next scheduling entity in the runqueue.
     */
    static SchedulingEntity *getEntityFromRunqueue(List<SchedulingEntity *> *runqueue) {
        // pop entity from start of list and enqueue it to the end:
        SchedulingEntity* entityPtr = runqueue->pop();
        runqueue->enqueue(entityPtr);
        return entityPtr;
    }

};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

RegisterScheduler(MultipleQueuePriorityScheduler);