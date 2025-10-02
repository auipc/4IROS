#pragma once
#include <kernel/arch/amd64/kernel.h>
#include <kernel/arch/amd64/x86_64.h>
#include <kernel/mem/Paging.h>
#include <kernel/util/HashTable.h>
#include <kernel/vfs/vfs.h>
#include <stddef.h>

#define STACK_SIZE 64 * KB
#define FD_LIM 128

class Process;
struct CoWPage {
	int refs = 0;
	PageSkellington skel;
};

extern KSyscallData ksyscall_data;
class Process;
struct Blocker {
	enum {
		ProcessBlock,
		SleepBlock,
		FileIOReadBlock,
		FileIOWriteBlock
	} blocked_on;
	union {
		Process *blocked_process;
		uint64_t sleep_waiter_us;
		struct {
			Vec<FileHandle *> *blocked_set;
			size_t blocked_sz;
		} file;
	};
};

enum class ProState {
	Running,
	Blocked,
	Dead,
};

class Process {
  public:
	Process(const char *name);
	Process(Process *p);
	~Process();
	static void init();
	static void yield();
	static void sched(uintptr_t rsp, uintptr_t user_rsp = 0,
					  bool is_timer_triggered = false);
	static void reentry();
	void collapse_cow();
	static void resolve_cow_recurse(RootPageLevel *level, Process *current,
									uintptr_t fault_addr);
	static int resolve_cow(Process *current, uintptr_t fault_addr);

	static void kill_process(Process *p);
	static Process *create(void *func_ptr);
	static Process *create_user(const char *path, size_t argc = 0,
								const char **argv = NULL);
	inline void kill() { m_state = ProState::Dead; }
	inline void set_state(ProState state) { m_state = state; }

	void register_signal_handler(void *ptr);

	void send_signal(int sig);
	void sigret();

	Process *fork(SyscallReg *regs);

	inline RootPageLevel *root_page_level() { return m_pglv; }

	inline uintptr_t kern_stack_top() { return m_kern_stack_top; }

	inline ProState state() { return m_state; }

	void block_on_proc(Process *p);
	void block_on_sleep(size_t us);
	void block_on_handle_read(FileHandle *handle);
	void block_on_handle_read_mul(Vec<FileHandle *> handles);
	void block_on_handle_write(FileHandle *handle, size_t sz);
	void poll_is_blocked();

	const char *name() { return m_name; }

	inline Process *parent() { return m_parent; }
	inline void set_parent(Process *p) { m_parent = p; }

	inline void add_child(Process *p) { m_children.push(p); }

	inline size_t file_handles_size() const { return m_file_handle_counter; }

	inline void drop_page_dir() {
#if 0
		if (m_pglv_virt) {
			Paging::the()->release(*m_pglv, m_cow_tbl);
			actual_free(m_pglv_virt);
			m_pglv_virt = 0;
		}
#endif
	}

	inline void copy_handles(Process *p) {
		// clear_file_handles();
		m_file_handle_counter = p->m_file_handle_counter;
		memcpy(m_file_handles, p->m_file_handles, m_file_handle_counter);
	}

	inline void set_cwd(Vec<const char *> dir) { m_cwd = dir; }

	inline Vec<const char *> get_cwd() {
		Vec<const char *> np;
		for (size_t i = 0; i < m_cwd.size(); i++) {
			const char *nc = strdup(m_cwd[i]);
			np.push(nc);
		}
		return np;
	}

	inline bool has_signal_handler() { return m_sig_handler; }

	void del_handle(int fd);
	int push_handle(FileHandle *handle);

	int exit_code;
	uint64_t pid;
	Process *next;
	Process *prev;
	// file handle list is a massive array so lookup is constant
	// you could save a marginal amount of memory by adding an indirect pointer
	// for groups of x or whatever exercise in futility
	FileHandle *m_file_handles[FD_LIM];
	bool dont_goto_me = false;
	uintptr_t m_actual_kern_stack_top;
	uintptr_t m_kern_stack_top;
	uintptr_t m_saved_user_stack_ksyscall;
	uintptr_t m_signal_saved_actual_kern_stack_top;
	uintptr_t m_signal_saved_kern_stack_top;

  private:
	inline void clear_file_handles() {
		for (size_t i = 0; i < m_file_handle_counter; i++) {
			if (m_file_handles[i])
				delete m_file_handles[i];
			m_file_handles[i] = nullptr;
		}
		m_file_handle_counter = 0;
	}
	Vec<const char *> m_cwd;
	uint64_t m_file_handle_counter = 0;
	Process *m_parent;
	Vec<Process *> m_children;
	Blocker m_blocker;
	ProState m_state = ProState::Running;
	uintptr_t m_stack_bot;
	uintptr_t m_stack_top;
	uintptr_t m_kern_stack_bot;
	uint8_t *m_saved_fpu;
	bool was_blocked = false;
	bool m_is_kernel_task = false;
	RootPageLevel *m_pglv;
	void *m_pglv_virt;
	uintptr_t m_sig_handler = 0;
	const char *m_name;
	bool m_in_signal = false;
	HashTable<CoWPage *> *m_cow_tbl;
};

extern Process *current_process;
