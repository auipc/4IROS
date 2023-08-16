#pragma once
#include <kernel/stdint.h>

class PrintInterface {
public:
	virtual void clear_output();
	virtual void write_character(char c);
};

class VGAInterface : public PrintInterface
{
public:
	VGAInterface();
	void clear_output() override;
	void write_character(char c) override;
private:
	uint16_t m_x;
	uint16_t m_y;
	uint16_t* m_screen;
};

void printk_use_interface(PrintInterface* interface);
void printk(const char* str, ...);
