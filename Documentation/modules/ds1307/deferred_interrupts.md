# FreeRTOS Task and Deferred Interrupt Processing

This document explains the synchronization model between the hardware interrupts (ISR) and the background FreeRTOS tasks to maintain real-time constraints.

## 1. The Real-Time Constraint
In high-speed bus communications like I2C, spending too much time within an interrupt service routine (ISR) can cause clock stretching, missed packets, or system lag. Moving logic operations, copy actions, and heavy computations out of the ISR is critical.

---

## 2. Deferred ISR Processing Flow

The simulator employs a **Deferred Interrupt Processing** pattern:

```
[Hardware I2C Interrupt] 
       │
       ▼
I2C1_EV_IRQHandler() or I2C1_ER_IRQHandler()
       │
       ▼
Driver: i2c_slave_handle_ev() / i2c_slave_handle_er()
       │ (Parses registers, formats event)
       ▼
Sim Callback: prv_i2c_event_cb()
       │ 
       ├───────► Write Done Event (STOPF detected)
       │                │
       │                ▼
       │         xSemaphoreGiveFromISR(write_sem, &woken)
       │                │
       │                ▼
       │         portYIELD_FROM_ISR(woken)  <--- Forces instant context switch
       │
       ▼
[Exit ISR]
       │
       ▼
[FreeRTOS Task: prv_sim_task()] <--- Wakes up immediately due to high priority
       │
       ├───────► Take Mutex
       ├───────► Apply writes: ds1307_apply_writes()
       └───────► Give Mutex
```

### Key RTOS Elements:
1. **`xSemaphoreGiveFromISR`**:
   Signifies the completion of a transaction. Because it is called within interrupt context, it uses the `woken` parameter to determine if a higher-priority task was unblocked.
2. **`portYIELD_FROM_ISR(woken)`**:
   Ensures that if the background simulation task (which has a higher priority than the idle or app tasks) is waiting on the semaphore, the CPU switches to it immediately upon exiting the ISR, bypassing standard tick latency.
3. **`Mutex` (Resource Protection)**:
   Protects the shared `sim->logic` state. Both the 1-second timer tick (within `prv_sim_task`) and the immediate write-apply operation need atomic access to `sim->logic`.

---

## 3. The Core Task Loop

The background simulation task is structured as follows:

```c
static void prv_sim_task(void *argument)
{
    sim_ds1307_t *sim = (sim_ds1307_t *)argument;
    
    for (;;)
    {
        /* Wait up to 1 second for an I2C write transaction to complete */
        if (xSemaphoreTake(sim->write_sem, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            if (xSemaphoreTake(sim->mutex, portMAX_DELAY) == pdTRUE)
            {
                ds1307_apply_writes(&sim->logic);
                xSemaphoreGive(sim->mutex);
            }
        }
        else
        {
            /* Timeout occurred, meaning 1 second has elapsed. Tick the clock. */
            if (xSemaphoreTake(sim->mutex, portMAX_DELAY) == pdTRUE)
            {
                ds1307_tick(&sim->logic);
                xSemaphoreGive(sim->mutex);
            }
        }
    }
}
```
This loop serves a dual purpose:
- Actively waits for and processes asynchronous write events immediately.
- Functions as a software-based timer tick that advances the clock every 1000 milliseconds if no writes occur.
