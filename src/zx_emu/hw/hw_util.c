#include "hw_util.h"
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "hardware/structs/systick.h"


void ext_delay_ms(uint32_t delay)
{
    usleep(delay*1000);

};

void ext_delay_us(uint32_t delay)
{
    usleep(delay);

};

struct timespec tp;

uint32_t __not_in_flash_func(get_ticks)()
{
    return (uint32_t)(0xffffff-((uint32_t)systick_hw->cvr))&0xffffff;
}
uint64_t ext_get_ns()
{
  clock_gettime(CLOCK_REALTIME, &tp);
  return tp.tv_sec*1000000000+tp.tv_nsec;


};

