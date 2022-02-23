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
                    syslog.messagef(LogLevel::ERROR, "Realtime runqueue is empty! Scheduling entity not removed.");
                } else {
                    rqRealtime.remove(&entity);
                }
                break;

            case SchedulingEntityPriority::INTERACTIVE:
                if (rqInteractive.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Interactive runqueue is empty! Scheduling entity not removed.");
                } else {
                    rqInteractive.remove(&entity);
                }
                break;

            case SchedulingEntityPriority::NORMAL:
                if (rqNormal.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Normal runqueue is empty! Scheduling entity not removed.");
                } else {
                    rqNormal.remove(&entity);
                }
                break;

            case SchedulingEntityPriority::DAEMON:
                if (rqDaemon.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Daemon runqueue is empty! Scheduling entity not removed.");
                } else {
                    rqDaemon.remove(&entity);
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

//        bool hasFoundRunnableEntity = false;

        // deal with runqueues in order of priority:
        if (!rqRealtime.empty()) {
            SchedulingEntity* entityPtr = getEntityFromRunqueue(&rqRealtime);
            if (entityPtr != NULL) return entityPtr;

        } else if (!rqInteractive.empty()) {
            SchedulingEntity* entityPtr = getEntityFromRunqueue(&rqInteractive);
            if (entityPtr != NULL) return entityPtr;

        } else if (!rqNormal.empty()) {
            SchedulingEntity* entityPtr = getEntityFromRunqueue(&rqNormal);
            if (entityPtr != NULL) return entityPtr;

        } else if (!rqDaemon.empty()) {
            SchedulingEntity* entityPtr = getEntityFromRunqueue(&rqDaemon);
            if (entityPtr != NULL) return entityPtr;
            
        } else {
            syslog.messagef(LogLevel::INFO, "All queues are empty.");
            return NULL;
        }

    }

    SchedulingEntity *getEntityFromRunqueue(List<SchedulingEntity *> *runqueue) {
        while(true) {
            // get entity from start of list:
            SchedulingEntity* entityPtr = rqRealtime.pop();
            if (rqRealtime.empty()) {
                // signal calling function to move on to queue in next highest priority
                return NULL;
            } else if (entityPtr->state() == SchedulingEntityState::RUNNABLE) {
                // found valid next entity to be run
                return entityPtr;
            } else {
                // move entity to end of the list:
                rqRealtime.enqueue(entityPtr);
            }
        }
    }

};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

RegisterScheduler(MultipleQueuePriorityScheduler);