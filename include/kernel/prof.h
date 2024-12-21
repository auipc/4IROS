#pragma once
#include <kernel/util/Vec.h>

#define PROFDEFER ProfDefer _(__LINE__, __func__);

// Bootleg 'profiler'
// The professor will see you now
// FIXME This only works for nested functions, though I'm convinced you can get the tree hierarchy working.
class ProfDefer final {
public:
	inline ProfDefer()
		: m_tsc(rdtsc())
		, m_depth(depth++)
	{
		if (!s_vec) {
			s_vec = new Vec<Info>();
		}
	}
	inline ProfDefer(int line, const char* func)
		: m_tsc(rdtsc())
		, m_line(line)
		, m_depth(depth++)
		, m_func(func)
	{
		if (!s_vec) {
			s_vec = new Vec<Info>();
		}
	}
	inline ~ProfDefer() {
		volatile uint64_t tsc_delta = rdtsc()-m_tsc;
		if (!m_depth) {
			if (m_line && m_func)
				printk("Prof. Defer (⌐■_■): Results are in! Line %d of procedure '%s' took roughly %d cycles.", m_line, m_func, tsc_delta);
			else
				printk("Prof. Defer (⌐■_■): Results are in! *Something* took roughly %d cycles.", tsc_delta);
			printk("\n");
			for (size_t i = s_vec->size(); i > 0; i--) {
				Info& info = (*s_vec)[i-1];
				// New zealanders pronounce this weird. Fuck that shitty island
				int root_percent = info.delta/(tsc_delta/100);
				int parent_percent = 0;
				if (i < s_vec->size()) {
					Info& parent = (*s_vec)[i];
					parent_percent = info.delta/(parent.delta/100);
				}
				if (!info.func)
					printk("  Anonymous sub-procedure took roughly %d cycles which is %d% of the root procedure", info.func, info.delta, root_percent);
				else
					printk("  Sub-procedure '%s' took roughly %d cycles which is %d% of the root procedure", info.func, info.delta, root_percent);
				
				if (parent_percent) {
					printk(" and %d% of the parent procedure", parent_percent);
				}
				printk(".\n");
			}
			s_vec->clear();
			depth = 0;
			return;
		}
		s_vec->push(Info{tsc_delta, m_line, m_func, m_depth});
	}
private:
	uint64_t m_tsc;
	int m_line = -1;
	int m_depth = 0;
	const char* m_func = nullptr;
	// Well obviosuly this is wrong
	static int depth;
	struct Info {
		uint64_t delta;
		int line;
		const char* func;
		int depth;
	};
	static Vec<Info>* s_vec;
};
