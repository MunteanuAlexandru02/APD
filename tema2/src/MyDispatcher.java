/* Implement this class. */

import java.util.Collections;
import java.util.List;

public class MyDispatcher extends Dispatcher {
    //protected int previous = -1;

    public MyDispatcher(SchedulingAlgorithm algorithm, List<Host> hosts) {
        super(algorithm, hosts);
    }

    public void roundRobinPlanning(Task task) {
        // First task to be planned -> send to the host with
        // id  0
//        if (previous == -1) {
//            hosts.get(0).addTask(task);
//            previous = 0;
//        } else {
//            hosts.get((previous + 1) % hosts.size()).addTask(task);
//            previous++;
//        }
        ;
    }

    public void leastWorkLeftPlanning(Task task) {
        if (task.getType() == TaskType.SHORT) {
            hosts.get(0).addTask(task);
        } else if (task.getType() == TaskType.MEDIUM) {
            hosts.get(1).addTask(task);
        } else if (task.getType() == TaskType.LONG) {
            hosts.get(2).addTask(task);
        }
    }

    public void shortestQueuePlanning(Task task) {
        // Check for the minimum queue size of the host
        int minimumSize = hosts.get(0).getQueueSize();
        int receiver = 0;

        for (int i = 1; i < hosts.size(); i++) {
            // Don't need to check if the sizes are equal, because
            // receiver will automatically have the minimum host id
            if (hosts.get(i).getQueueSize() < minimumSize) {
                minimumSize = hosts.get(i).getQueueSize();
                receiver = i;
            }
        }

        hosts.get(receiver).addTask(task);
    }

    public void sitaPlanning(Task task) {

        // TODO: find a way to calculate for the one that is running
        // TODO: if there is one
        MyHost myHost = (MyHost) hosts.get(0);
        int receiver = 0;
        long minimumDuration = 0;
        long duration;
        //Calculate the duration for the first host
        for (Task t : myHost.pq) {
            minimumDuration += t.getDuration();
        }

        for (int i = 1; i < hosts.size(); i++) {

            myHost = (MyHost) hosts.get(i);
            duration = myHost.getWorkLeft();

            if (duration < minimumDuration) {
                minimumDuration = duration;
                receiver = i;
            }
        }

        hosts.get(receiver).addTask(task);
    }
    @Override
    public void addTask(Task task) {
        System.out.println("Vaca in addTask");
        // Check the scheduling algorithm
        if (algorithm == SchedulingAlgorithm.ROUND_ROBIN) {
            //previous = -1;
            roundRobinPlanning(task);
        }
        else if (algorithm == SchedulingAlgorithm.LEAST_WORK_LEFT) {
            leastWorkLeftPlanning(task);
        }
        else if (algorithm == SchedulingAlgorithm.SHORTEST_QUEUE) {
            shortestQueuePlanning(task);
        }
        else if (algorithm == SchedulingAlgorithm.SIZE_INTERVAL_TASK_ASSIGNMENT) {
            sitaPlanning(task);
        }
    }
}
