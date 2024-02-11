/* Implement this class. */

import java.util.PriorityQueue;

public class MyHost extends Host {

    private volatile boolean shut = false;
    public PriorityQueue<Task> pq = new PriorityQueue<>( (o1, o2) -> Integer.compare(o1.getPriority(), o2.getPriority()));
    @Override
    synchronized public void run() {
        while (shut == false && pq.isEmpty() == false) {
            System.out.println("VACA");
            Task myTask = pq.poll();
            if (myTask != null) {
                long duration = myTask.getDuration();
                while (duration != 0) {
                    duration--;
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        throw new RuntimeException(e);
                    }
                }
            }
        }
    }

    @Override
    synchronized public void addTask(Task task) {
        // Need to add a task in queue
        // add the task in prio queue to assure we always have
        // the task with the maximum priority on the first position
        pq.add(task);
    }

    @Override
    public int getQueueSize() {
        return pq.size();
    }

    @Override
    public long getWorkLeft() {
        long duration = 0;
        for (Task t : pq) {
            duration += t.getDuration();
        }
        return duration;
    }

    @Override
    public void shutdown() {
        shut = true;
    }
}
