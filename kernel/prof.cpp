#include <kernel/prof.h>

int ProfDefer::depth = 0;
Vec<ProfDefer::Info>* ProfDefer::s_vec = nullptr;
