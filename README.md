# Signals and Slots

Multiple slot executors to suit requirements of the signal:
- Synchronized: where return of emission guarantees completion of connected slots
- Asynchronous: emission spawns a detached thread for each connected function
- Strand: Spawns a thread on slot connection which waits for emissions
and processes them in order of arrival
- Thread Pooled: Emission parameters are bound to the function and enqueued, to be
executed by a pool of threads waiting for tasks, each thread is a MPSC queue

Todo:
- Internal documentation
- Thorough benchmark against other signals/slots implementations
