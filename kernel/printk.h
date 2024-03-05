#pragma once
#include <kernel/util/Singleton.h>
#include <stdint.h>

class PrintInterface {
  public:
	PrintInterface() {}
	virtual void clear_output() = 0;
	virtual void write_character(char c) = 0;
};

class VGAInterface : public PrintInterface {
  public:
	VGAInterface();
	void clear_output() override;
	void write_character(char c) override;

  private:
	uint16_t m_x;
	uint16_t m_y;
	uint16_t *m_screen;
};

PrintInterface *printk_get_interface();
void printk_use_interface(PrintInterface *interface);
void printk(const char *str, ...);
