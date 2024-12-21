#pragma once

class TimerSource {
  public:
	virtual void init() = 0;

  private:
	uint8_t m_timer_irq;
};
