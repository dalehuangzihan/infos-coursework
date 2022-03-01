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
     * Called during scheduler initialisation.
     */
    void init()
    {
        // TODO: Implement me!
        // Not required for this implementation.
    }

    /**
     * Called when a scheduling entity becomes eligible for running.
     * @param entity
     */
    void add_to_runqueue(SchedulingEntity& entity) override
    {
        // disable interrupts before modifying runqueue:
        UniqueIRQLock l;
        //based on the entity's priority, enqueue into appropriate runqueue:
        switch(entity.priority()) {
            case SchedulingEntityPriority::REALTIME:
                rq_realtime.enqueue(&entity);
                break;
            case SchedulingEntityPriority::INTERACTIVE:
                rq_interactive.enqueue(&entity);
                break;
            case SchedulingEntityPriority::NORMAL:
                rq_normal.enqueue(&entity);
                break;
            case SchedulingEntityPriority::DAEMON:
                rq_daemon.enqueue(&entity);
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
        // disable interrupts before modifying runqueue:
        UniqueIRQLock l;
        // based on the entity's priority, remove from the appropriate runqueue:
        switch(entity.priority()) {
            case SchedulingEntityPriority::REALTIME:
                if (rq_realtime.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Realtime runqueue is empty! Entity [%s] not removed.", entity.name().c_str());
                } else {
                    rq_realtime.remove(&entity);
                }
                break;

            case SchedulingEntityPriority::INTERACTIVE:
                if (rq_interactive.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Interactive runqueue is empty! Entity [%s] not removed.", entity.name().c_str());
                } else {
                    rq_interactive.remove(&entity);
                }
                break;

            case SchedulingEntityPriority::NORMAL:
                if (rq_normal.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Normal runqueue is empty! Entity [%s] not removed.", entity.name().c_str());
                } else {
                    rq_normal.remove(&entity);
                }
                break;

            case SchedulingEntityPriority::DAEMON:
                if (rq_daemon.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Daemon runqueue is empty! Entity [%s] not removed.", entity.name().c_str());
                } else {
                    rq_daemon.remove(&entity);
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
        SchedulingEntity* next_entity;
        if (!rq_realtime.empty()) {
            next_entity = get_entity_from_runqueue(&rq_realtime);
            if (next_entity != NULL) return next_entity;    // move on to next highest priority queue if entity is NULL.

        } else if (!rq_interactive.empty()) {
            next_entity = get_entity_from_runqueue(&rq_interactive);
            if (next_entity != NULL) return next_entity;

        } else if (!rq_normal.empty()) {
            next_entity = get_entity_from_runqueue(&rq_normal);
            if (next_entity != NULL) return next_entity;

        } else if (!rq_daemon.empty()) {
            next_entity = get_entity_from_runqueue(&rq_daemon);
            if (next_entity != NULL) return next_entity;

        } else {
            return NULL;
        }
    }


private:
    /**
     * Run-queuese for realtime, interactive, normal, daemon:
     */
    List<SchedulingEntity *> rq_realtime;
    List<SchedulingEntity *> rq_interactive;
    List<SchedulingEntity *> rq_normal;
    List<SchedulingEntity *> rq_daemon;

    /**
     * Helper function for pick_next_entity().
     * Pops the first still-runnable entity from the queue and returns it.
     * Enqueues the to-be-returned entity back to the end of the runqueue
     * @param runqueue is the runqueue that we wish to obtain the scheduling entity from.
     * @return the next scheduling entity in the runqueue.
     */
    static SchedulingEntity *get_entity_from_runqueue(List<SchedulingEntity *> *runqueue) {
        unsigned int org_queue_len = runqueue->count();
        // pop entity from start of queue:
        SchedulingEntity* entityPtr = runqueue->pop();
        if (entityPtr != NULL) {
            // enqueue entity back to the end of the queue:
            runqueue->enqueue(entityPtr);
            // check if entity has been successfully enqueued:
            if (runqueue->count() != org_queue_len) {
                syslog.message(LogLevel::ERROR, "Entity(s) lost from queue when fetching next entity.");
            }
        } else {
            syslog.message(LogLevel::ERROR, "Dequeued Entity is NULL! Entity not re-enqueued.");
        }
        return entityPtr;
    }

};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

RegisterScheduler(MultipleQueuePriorityScheduler);