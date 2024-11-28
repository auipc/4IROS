#include <kernel/Debug.h>
#include <kernel/printk.h>
#include <stdint.h>

// https://github.com/llvm/llvm-project/blob/main/compiler-rt/lib/ubsan/ubsan_handlers.cpp#L40
/// Situations in which we might emit a check for the suitability of a
/// pointer or glvalue. Needs to be kept in sync with CodeGenFunction.h in
/// clang.
enum TypeCheckKind {
	/// Checking the operand of a load. Must be suitably sized and aligned.
	TCK_Load,
	/// Checking the destination of a store. Must be suitably sized and aligned.
	TCK_Store,
	/// Checking the bound value in a reference binding. Must be suitably sized
	/// and aligned, but is not required to refer to an object (until the
	/// reference is used), per core issue 453.
	TCK_ReferenceBinding,
	/// Checking the object expression in a non-static data member access. Must
	/// be an object within its lifetime.
	TCK_MemberAccess,
	/// Checking the 'this' pointer for a call to a non-static member function.
	/// Must be an object within its lifetime.
	TCK_MemberCall,
	/// Checking the 'this' pointer for a constructor call.
	TCK_ConstructorCall,
	/// Checking the operand of a static_cast to a derived pointer type. Must be
	/// null or an object within its lifetime.
	TCK_DowncastPointer,
	/// Checking the operand of a static_cast to a derived reference type. Must
	/// be an object within its lifetime.
	TCK_DowncastReference,
	/// Checking the operand of a cast to a base object. Must be suitably sized
	/// and aligned.
	TCK_Upcast,
	/// Checking the operand of a cast to a virtual base object. Must be an
	/// object within its lifetime.
	TCK_UpcastToVirtualBase,
	/// Checking the value assigned to a _Nonnull pointer. Must not be null.
	TCK_NonnullAssign,
	/// Checking the operand of a dynamic_cast or a typeid expression.  Must be
	/// null or an object within its lifetime.
	TCK_DynamicOperation
};

struct SourceLocation {
	const char *filename;
	uint32_t line;
	uint32_t column;
};

struct TypeDescriptor {
	uint16_t type_kind;
	uint16_t type_info;
	char type_name[1];
};

// https://github.com/llvm/llvm-project/blob/main/compiler-rt/lib/ubsan/ubsan_handlers.h#L19
struct TypeMismatchData {
	SourceLocation loc;
	const TypeDescriptor &type;
	uint8_t log_alignment;
	uint8_t typecheck_kind;
};

extern "C" void __ubsan_handle_type_mismatch_v1(TypeMismatchData *mismatch_data,
												uintptr_t handle) {
	(void)mismatch_data;
	(void)handle;
	// switch (mismatch_data->typecheck_kind) {

	//}
	// printk("kind: %d %s\n", mismatch_data->typecheck_kind,
	// TypeCheckKinds[mismatch_data->typecheck_kind]); printk("UBSAN: type
	// mismatch %s:%d type %d typeinf %d\n", mismatch_data->loc.filename,
	// mismatch_data->loc.line, mismatch_data->type.type_kind,
	// mismatch_data->type.type_info); printk("UBSAN: alignment %x\n",
	// 1<<mismatch_data->log_alignment); printk("Value handle %x\n", handle);
}

extern "C" void __ubsan_handle_pointer_overflow() {
	Debug::stack_trace();
	panic("UBSAN: pointer overflow\n");
}

extern "C" void __ubsan_handle_shift_out_of_bounds() {
	Debug::stack_trace();
	panic("UBSAN: shift out of bounds\n");
}

extern "C" void __ubsan_handle_mul_overflow() {
	Debug::stack_trace();
	panic("UBSAN: mul overflow\n");
}

extern "C" void __ubsan_handle_divrem_overflow() {
	Debug::stack_trace();
	panic("UBSAN: divrem overflow\n");
}

extern "C" void __ubsan_handle_add_overflow() {
	Debug::stack_trace();
	panic("UBSAN: add overflow\n");
}

extern "C" void __ubsan_handle_out_of_bounds() {
	Debug::stack_trace();
	panic("UBSAN: out of bounds\n");
}

extern "C" void __ubsan_handle_load_invalid_value() {
	Debug::stack_trace();
	panic("UBSAN: load invalid value\n");
}

extern "C" void __ubsan_handle_builtin_unreachable() {
	Debug::stack_trace();
	panic("UBSAN: builtin unreachable\n");
}

extern "C" void __ubsan_handle_nonnull_return_v1() {
	Debug::stack_trace();
	panic("UBSAN: nonnull return\n");
}

extern "C" void __ubsan_handle_sub_overflow() {
	Debug::stack_trace();
	panic("UBSAN: sub overflow\n");
}

extern "C" void __ubsan_handle_negate_overflow() {
	Debug::stack_trace();
	panic("UBSAN: negate overflow\n");
}
