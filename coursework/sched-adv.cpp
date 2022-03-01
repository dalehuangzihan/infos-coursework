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
     * Called during scheduler initialisation.
     */
    void init()
    {
        session_A_rq_list.append(&rq_realtime_A);
        session_A_rq_list.append(&rq_interactive_A);
        session_A_rq_list.append(&rq_normal_A);
        session_A_rq_list.append(&rq_daemon_A);
        syslog.messagef(LogLevel::IMPORTANT, "Initialised session A runqueues list");

        session_B_rq_list.append(&rq_realtime_B);
        session_B_rq_list.append(&rq_interactive_B);
        session_B_rq_list.append(&rq_normal_B);
        session_B_rq_list.append(&rq_daemon_B);
        syslog.messagef(LogLevel::IMPORTANT, "Initialised session B runqueues list");
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
            add_entity_to_idle_runqueue(session_B_rq_list, entity);
        } else {
            // add new tasks to Alpha session runqueues:
            add_entity_to_idle_runqueue(session_A_rq_list, entity);
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
                }
                break;

            case SchedulingEntityPriority::INTERACTIVE:
                if (rq_interactive_A.empty() && rq_interactive_B.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Interactive runqueues are empty! Entity [%s] not removed.", entity.name().c_str());
                } else {
                    rq_interactive_A.remove(&entity);
                    rq_interactive_B.remove(&entity);
                }
                break;

            case SchedulingEntityPriority::NORMAL:
                if (rq_normal_A.empty() && rq_normal_B.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Normal runqueues are empty! Entity [%s] not removed.", entity.name().c_str());
                } else {
                    rq_normal_A.remove(&entity);
                    rq_normal_B.remove(&entity);
                }
                break;

            case SchedulingEntityPriority::DAEMON:
                if (rq_daemon_A.empty() && rq_daemon_B.empty()) {
                    //do nothing
                    syslog.messagef(LogLevel::ERROR, "Daemon runqueues are empty! Entity [%s] not removed.", entity.name().c_str());
                } else {
                    rq_daemon_A.remove(&entity);
                    rq_daemon_B.remove(&entity);
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
            return search_runqueues_for_next_entity(session_A_rq_list, session_B_rq_list, is_session_A_active);
        } else {
            return search_runqueues_for_next_entity(session_B_rq_list, session_A_rq_list, is_session_A_active);
        }
    }

private:
    /**
     * Run-queues (Alpha session) for realtime, interactive, normal, daemon:
     */
    List<SchedulingEntity *> rq_realtime_A;
    List<SchedulingEntity *> rq_interactive_A;
    List<SchedulingEntity *> rq_normal_A;
    List<SchedulingEntity *> rq_daemon_A;
    List<List<SchedulingEntity *> *> session_A_rq_list;

    /**
     * Run-queues (Beta session) for realtime, interactive, normal, daemon:
     */
    List<SchedulingEntity *> rq_realtime_B;
    List<SchedulingEntity *> rq_interactive_B;
    List<SchedulingEntity *> rq_normal_B;
    List<SchedulingEntity *> rq_daemon_B;
    List<List<SchedulingEntity *> *> session_B_rq_list;

    // flag to control which session is active:
    bool is_session_A_active = true;

    /**
     * Helper function for add_to_runqueue().
     * Adds the entity to the appropriate idle runqueue according to level of priority.
     */
    static void add_entity_to_idle_runqueue(List<List<SchedulingEntity *> *>& idle_session_rq_list, SchedulingEntity& entity) {
        // based on the entity's priority, enqueue into appropriate runqueue:
        switch(entity.priority()) {
            case SchedulingEntityPriority::REALTIME:
                // enqueue to rq_realtime:
                idle_session_rq_list.at(0)->enqueue(&entity);
                break;
            case SchedulingEntityPriority::INTERACTIVE:
                // enqueue to rq_interactive:
                idle_session_rq_list.at(1)->enqueue(&entity);
                break;
            case SchedulingEntityPriority::NORMAL:
                // enqueue to rq_normal:
                idle_session_rq_list.at(2)->enqueue(&entity);
                break;
            case SchedulingEntityPriority::DAEMON:
                // enqueue to rq_daemon:
                idle_session_rq_list.at(3)->enqueue(&entity);
                break;
            default:
                // do nothing
                break;
        }
    }

    /**
     * Helper function for pick_next_entity().
     * Searches active runqueues to find next entity to pass to scheduler;
     * If active runqueues are all empty, inspect the idle runqueues;
     * If all runqueues are empty, return NULL.
     * @param active_session_rq_list is the list of active runqueues / session
     * @param idle_session_rq_list is the list of idle runqueues / session
     * @param is_session_A_active is bool flag indicating if runqueues in session A are active.
     * @return entity chosen from the active / idle runqueues.
     */
    static SchedulingEntity *search_runqueues_for_next_entity(List<List<SchedulingEntity *> *>& active_session_rq_list,
                                                                   List<List<SchedulingEntity *> *>& idle_session_rq_list,
                                                                   bool& is_session_A_active) {
        if (!active_session_rq_list.at(0)->empty()) {
            // inspect active rq_realtime:
            return get_entity_from_runqueue(active_session_rq_list.at(0), idle_session_rq_list.at(0));
        } else if (!active_session_rq_list.at(1)->empty()) {
            // inspect active rq_interactive:
            return get_entity_from_runqueue(active_session_rq_list.at(1), idle_session_rq_list.at(1));
        } else if (!active_session_rq_list.at(2)->empty()) {
            // inspect active rq_normal:
            return get_entity_from_runqueue(active_session_rq_list.at(2), idle_session_rq_list.at(2));
        } else if (!active_session_rq_list.at(3)->empty()) {
            // inspect active rq_daemon:
            return get_entity_from_runqueue(active_session_rq_list.at(3), idle_session_rq_list.at(3));
        } else {

            // all priority queues in this session are empty; toggle active session to the other session:
            is_session_A_active = !is_session_A_active;

            if (!idle_session_rq_list.at(0)->empty()) {
                // inspect idle rq_realtime:
                return get_entity_from_runqueue(idle_session_rq_list.at(0), active_session_rq_list.at(0));
            } else if (!idle_session_rq_list.at(1)->empty()) {
                // inspect idle rq_interactive:
                return get_entity_from_runqueue(idle_session_rq_list.at(1), active_session_rq_list.at(1));
            } else if (!idle_session_rq_list.at(2)->empty()) {
                // inspect idle rq_normal:
                return get_entity_from_runqueue(idle_session_rq_list.at(2), active_session_rq_list.at(2));
            } else if (!idle_session_rq_list.at(3)->empty()) {
                // inspect idle rq_daemon:
                return get_entity_from_runqueue(idle_session_rq_list.at(3), active_session_rq_list.at(3));
            } else {
                // all priority queues in both sessions are empty:
                return NULL;
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
        static SchedulingEntity *get_entity_from_runqueue(List<SchedulingEntity *>* active_runqueue, List<SchedulingEntity *>* idle_runqueue) {
            unsigned int org_active_rq_len = active_runqueue->count();
            unsigned int org_idle_rq_len = idle_runqueue->count();
            // pop entity from start of active queue:
            SchedulingEntity* entityPtr = active_runqueue->pop();
            if (entityPtr != NULL) {
                // enqueue entity to end of idle queue:
                idle_runqueue->enqueue(entityPtr);
                if (active_runqueue->count() != org_active_rq_len - 1) syslog.message(LogLevel::ERROR, "Active runqueue has wrong length after fetching next entity.");
                if (idle_runqueue->count() != org_idle_rq_len + 1) syslog.message(LogLevel::ERROR, "Idle runqueue has wrong length after fetching next entity.");
            } else {
                syslog.message(LogLevel::ERROR, "Dequeued next-Entity is NULL! Entity not re-enqueued.");
            }
            return entityPtr;
        }

};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

RegisterScheduler(O1MQPriorityScheduler);