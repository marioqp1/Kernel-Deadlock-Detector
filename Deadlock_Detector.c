#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>

static struct task_struct *thread1, *thread2;
static DEFINE_MUTEX(mutex1);
static DEFINE_MUTEX(mutex2);
static bool stop_threads = false;

static int thread1_function(void *data) {
    while (!kthread_should_stop() && !stop_threads) {
        pr_info("Thread 1 trying to acquire mutex1\n");
        if (!mutex_trylock(&mutex1)) {
            pr_info("Thread 1 failed to acquire mutex1, retrying\n");
            msleep(1000); // Delay before retrying
            continue;
        }
        pr_info("Thread 1 acquired mutex1\n");

        pr_info("Thread 1 trying to acquire mutex2\n");
        if (!mutex_trylock(&mutex2)) {
            pr_info("Thread 1 failed to acquire mutex2, releasing mutex1 and retrying\n");
            mutex_unlock(&mutex1);
            msleep(1000); // Delay before retrying
            continue;
        }
        pr_info("Thread 1 acquired mutex2\n");

        // Critical section
        pr_info("Thread 1 in critical section\n");

        mutex_unlock(&mutex2);
        mutex_unlock(&mutex1);
    }
    return 0;
}

static int thread2_function(void *data) {
    while (!kthread_should_stop() && !stop_threads) {
        pr_info("Thread 2 trying to acquire mutex2\n");
        if (!mutex_trylock(&mutex2)) {
            pr_info("Thread 2 failed to acquire mutex2, retrying\n");
            msleep(1000); // Delay before retrying
            continue;
        }
        pr_info("Thread 2 acquired mutex2\n");

        pr_info("Thread 2 trying to acquire mutex1\n");
        if (!mutex_trylock(&mutex1)) {
            pr_info("Thread 2 failed to acquire mutex1, releasing mutex2 and retrying\n");
            mutex_unlock(&mutex2);
            msleep(1000); // Delay before retrying
            continue;
        }
        pr_info("Thread 2 acquired mutex1\n");

        // Critical section
        pr_info("Thread 2 in critical section\n");

        mutex_unlock(&mutex1);
        mutex_unlock(&mutex2);
    }
    return 0;
}

static int __init deadlock_detector_init(void) {
    pr_info("Deadlock Detector Module Loaded\n");

    thread1 = kthread_run(thread1_function, NULL, "Thread1");
    if (IS_ERR(thread1)) {
        pr_err("Failed to create thread1\n");
        return PTR_ERR(thread1);
    }

    thread2 = kthread_run(thread2_function, NULL, "Thread2");
    if (IS_ERR(thread2)) {
        pr_err("Failed to create thread2\n");
        kthread_stop(thread1);
        return PTR_ERR(thread2);
    }

    return 0;
}

static void __exit deadlock_detector_exit(void) {
    pr_info("Deadlock Detector Module Unloaded\n");

    stop_threads = true;

    if (thread1)
        kthread_stop(thread1);

    if (thread2)
        kthread_stop(thread2);
}

module_init(deadlock_detector_init);
module_exit(deadlock_detector_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel Deadlock Detector with Lockdep and Timeout Handling");
