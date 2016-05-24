# Signals and Slots

Multiple slot connection schemes to suit requirements of the signal:
- Synchronized: where return of emission guarantees completion of connected slots
- Asynchronous: emission spawns a detached thread for each connected function
- Asynchronous_Enqueue: Spawns a thread on slot connection which waits for emissions
and processes them in order of arrival
- Threadpooled: Emission parameters are bound to the function and enqueued, to be
executed by a pool of threads waiting for tasks - threads grow/shrink dynamically
based on business

Todo:
- Optimize queue structures, (replace with folly mpmc queue or multiple
spsc queues)
- Internal documentation
- Optimize thread pool growth/shrinking
- Thorough benchmark against other signals/slots implementations
