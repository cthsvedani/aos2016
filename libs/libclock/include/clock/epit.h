#ifndef EPIT_H
#define EPIT_H


typedef struct {
	uint32_t REG_Control;
	uint32_t REG_Status;
	uint32_t REG_Load;
	uint32_t REG_Compare;
	uint32_t REG_Counter;
} EPIT;

typedef struct {
    EPIT *reg;
    seL4_IRQHandler cap;
    uint32_t source;
} timer;

#define EPIT_CLK_PERIPHERAL (1 << 24)
#define EPIT_CLK_MSK (3 << 24)
#define EPIT_STOP_EN (1 << 21)
#define EPIT_OM (3 << 22)
#define EPIT_WAIT_EN (1 << 19)
#define EPIT_DB_EN (1 << 18)
#define EPIT_I_OVW (1 << 17)
#define EPIT_SW_R (1 << 16)
#define EPIT_PRESCALE_CONST (3300 << 4)
#define EPIT_PRESCALE_MSK (0xFFF << 4)
#define EPIT_RLD (1 << 3)
#define EPIT_OCI_EN (1 << 2)
#define EPIT_EN_MOD (1 << 1)
#define EPIT_EN (1 << 0)

#define EPIT_TICK 0.05
#define EPIT_TICK_PER_MS 20
#define EPIT_CLOCK_TICK 0.06206
//#define EPIT_CLOCK_TICK 0.00001515
#define EPIT_CLOCK_OVERFLOW 266546000
//#define EPIT_CLOCK_OVERFLOW 65069


//void epit_init(EPIT timer);
void epit_init(EPIT *timer);
void epit_setTime(EPIT *timer, uint64_t milliseconds, int reset);
void epit_startTimer(EPIT *timer);
void epit_stopTimer(EPIT *timer);
uint64_t epit_getCurrentTimestamp();
void epit_setTimerClock(EPIT *timer);

int epit_timerRunning(EPIT *timer);
uint32_t epit_getCount(EPIT *timer);
uint32_t epit_currentCompare(EPIT *timer);
#endif
