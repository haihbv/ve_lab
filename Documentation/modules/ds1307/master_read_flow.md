# DS1307 Simulator Master-Read Transaction Flow

This document details the step-by-step sequential flow of an I2C Master Read transaction on the simulated DS1307 RTC, showing how hardware interrupts, driver callbacks, and the triple-buffering logic cooperate to guarantee thread-safe and atomic data delivery.

---

## 1. Typical Master Read Sequence (I2C Protocol)

A standard read transaction from a Raspberry Pi (I2C Master) to the DS1307 (I2C Slave at `0x68`) consists of two sequential phases:

### Phase 1: Set Register Pointer (Write Transaction)
```
START ──► Address + W (0xD0) [ACK] ──► Reg Pointer (e.g. 0x00) [ACK] ──► STOP / REPEATED START
```

### Phase 2: Sequential Read (Read Transaction)
```
START ──► Address + R (0xD1) [ACK] ──► Data Byte 0 [ACK] ──► Data Byte 1 [ACK] ──► ... ──► Last Data Byte [NACK] ──► STOP
```

---

## 2. Layer-by-Layer Execution Timeline

Below is the execution flow across the driver, simulation layer, and pure logic layer during these two phases.

```
       [Hardware / Driver]                     [Simulation Layer]                  [Pure Logic Layer]
     (driver_i2c_slave.c/h)                    (sim_ds1307.c/h)                      (ds1307.c/h)
               │                                       │                                   │
               │                                       │                                   │
               ▼                                       ▼                                   ▼
┌──────────────────────────────┐
│  Phase 1: Set Reg Pointer    │
└──────────────────────────────┘
 1. ADDR Match (TRA=0) ───────► I2C_SLAVE_EVT_WRITE_START ───────────────────────► write_pending = 0
                                                                                   write_len = 0
 2. RXNE (Data = 0x00) ───────► I2C_SLAVE_EVT_BYTE_RECEIVED ─────────────────────► ds1307_rx(0x00, is_first=1)
                                                                                   current_reg_ptr = 0x00
                                                                                   write_pending = 1
 3. STOPF Detected ───────────► I2C_SLAVE_EVT_WRITE_DONE (write_len=0, no-op)
               │                                       │                                   │
               │                                       │                                   │
               ▼                                       ▼                                   ▼
┌──────────────────────────────┐
│  Phase 2: Master Read        │
└──────────────────────────────┘
 4. ADDR Match (TRA=1) ───────► I2C_SLAVE_EVT_READ_START ────────────────────────► ds1307_read_start()
                                                                                   Locks tx_bank = active_bank
                                                                                   Sets tx_active = 1

 5. ADDR Match continues ─────► Driver calls prv_get_tx_byte() ──────────────► ds1307_tx()
                                                                                   Fetches reg_bank[tx_bank][0x00]
                                                                                   current_reg_ptr++ (now 0x01)
                                                                                   Returns byte to i2c->DR

 6. TXE (Transmit empty) ─────► Driver calls prv_get_tx_byte() ──────────────► ds1307_tx()
                                                                                   Fetches reg_bank[tx_bank][0x01]
                                                                                   current_reg_ptr++ (now 0x02)
                                                                                   Returns byte to i2c->DR

 7. Last byte sent, ──────────► i2c_slave_handle_er() (AF Flag)
    Master replies NACK        │
                               ├─► Clear AF flag & WORKAROUND:
                               │   Re-enable ACK bit in CR1
                               │
                               └─► I2C_SLAVE_EVT_READ_DONE ──────────────────────► ds1307_read_done()
                                                                                   Sets tx_active = 0
```

---

## 3. Triple Buffering Mechanics during Read

To prevent **data tearing** (where the 1-second background simulation tick updates registers like seconds/minutes mid-read, causing a mismatch in time reading), a three-bank architecture is implemented.

### The Three States:
1. **`active_bank`**: The bank containing the most up-to-date BCD values representing the current simulated clock.
2. **`tx_bank`**: The bank currently locked by the active I2C Master read transaction.
3. **`tx_active`**: A boolean flag indicating whether a Master read is in progress.

### Transition Rules in Pure Logic (`ds1307.c`):

1. **Locking the Bank (`ds1307_read_start`)**:
   When the driver detects a Master read start condition, it immediately locks the current active bank:
   ```c
   void ds1307_read_start(ds1307_t *dev)
   {
       if (dev == NULL) return;
       dev->tx_bank = dev->active_bank;
       dev->tx_active = 1;
   }
   ```

2. **Publishing New Ticks (`prv_publish_regs`)**:
   When the background task increments the clock every 1 second, it attempts to publish to a bank that is *not* currently being read:
   ```c
   static void prv_publish_regs(ds1307_t *dev)
   {
       uint8_t next_bank = (dev->active_bank + 1) % DS1307_BANK_COUNT;
       if (next_bank == dev->tx_bank && dev->tx_active)
       {
           next_bank = (next_bank + 1) % DS1307_BANK_COUNT;
       }
       memcpy(dev->reg_bank[next_bank], dev->regs, DS1307_REG_COUNT);
       dev->active_bank = next_bank;
   }
   ```
   This ensures that even if a read transaction spans across a 1-second clock tick boundary, the Master reads a completely consistent snapshot of time without any torn bytes.

---

## 4. Pointer Auto-Increment Comparison

A vital design improvement between the legacy simulation logic and the refactored architecture lies in how register pointer incrementation is handled:

| Aspect | Legacy Simulator (`simulator_old`) | Refactored Simulator (`simulator_fw`) |
|--------|────────────────────────────────────|──────────────────────────────────────|
| **Variables** | Single global `current_reg` tracking current pointer. | Layered `current_reg_ptr` encapsulated inside pure logic `ds1307_t`. |
| **Write Transactions** | Did not increment `current_reg` as bytes were accumulated. Only updated at transaction completion. | Increments `current_reg_ptr` after each byte written to the buffer, mirroring hardware auto-increment behavior. |
| **Read Transactions** | Increments `current_reg` immediately during transmit callback. | Increments `current_reg_ptr` immediately during transmit callback. |
| **Atomicity** | Synchronized solely via raw mutexes in the main loop. | Synchronized using dual-layer (Semaphores for ISR-to-Task deferral + Mutexes for state ticks + Triple Buffering for reads). |
