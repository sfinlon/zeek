// See the file "COPYING" in the main distribution directory for copyright.

#ifndef frame_h
#define frame_h

#include <vector>
#include <unordered_set>
#include <memory>

#include "Val.h"

using namespace std;

class BroFunc;
class Trigger;
class CallExpr;
class Val;

class Frame : public BroObj {
public:
	Frame(int size, const BroFunc* func, const val_list *fn_args);
	Frame(const Frame* other);
	~Frame() override;

	Val* NthElement(int n) { return frame[n]; }
	void SetElement(int n, Val* v)
		{
		Unref(frame[n]);
		frame[n] = v;
		}
	virtual void SetElement(ID* id, Val* v)
		{
		SetElement(id->Offset(), v);
		}

	virtual Val* GetElement(ID* id) const;
	void AddElement(ID* id, Val* v);

	void Reset(int startIdx);
	void Release();

	void Describe(ODesc* d) const override;

	// For which function is this stack frame.
	const BroFunc* GetFunction() const	{ return function; }
	const val_list* GetFuncArgs() const	{ return func_args; }

	// Next statement to be executed in the context of this frame.
	void SetNextStmt(Stmt* stmt)	{ next_stmt = stmt; }
	Stmt* GetNextStmt() const	{ return next_stmt; }

	// Used to implement "next" command in debugger.
	void BreakBeforeNextStmt(bool should_break)
		{ break_before_next_stmt = should_break; }
	bool BreakBeforeNextStmt() const
		{ return break_before_next_stmt; }

	// Used to implement "finish" command in debugger.
	void BreakOnReturn(bool should_break)
		{ break_on_return = should_break; }
	bool BreakOnReturn() const	{ return break_on_return; }

	// Deep-copies values.
	virtual Frame* Clone();
	// Only deep-copies values corresponding to requested IDs.
	Frame* SelectiveClone(id_list* selection);

	// If the frame is run in the context of a trigger condition evaluation,
	// the trigger needs to be registered.
	void SetTrigger(Trigger* arg_trigger);
	void ClearTrigger();
	Trigger* GetTrigger() const		{ return trigger; }

	void SetCall(const CallExpr* arg_call)	{ call = arg_call; }
	void ClearCall()			{ call = 0; }
	const CallExpr* GetCall() const		{ return call; }

	void SetDelayed()	{ delayed = true; }
	bool HasDelayed() const	{ return delayed; }

	int n_elements() { return this->size; }

protected:
	void Clear();

	Val** frame;
	int size;

	const BroFunc* function;
	const val_list* func_args;
	Stmt* next_stmt;

	bool break_before_next_stmt;
	bool break_on_return;

	Trigger* trigger;
	const CallExpr* call;
	bool delayed;
};

/*
Class that allows for lookups in both a closure frame and a regular frame
according to a list of IDs passed into the constructor.

Facts:
	- A ClosureFrame is created from two frames: a closure and a regular frame.
	- ALL operations except GetElement operations operate on the regular frame.
	- A ClosureFrame requires a list of outside ID's captured by the closure.
	- Get operations on those IDs will be performed on the closure frame.

ClosureFrame allows functions that generate functions to be passed between
different sized frames and still properly capture their closures.
*/
class ClosureFrame : public Frame {
public:
	ClosureFrame(Frame* closure, Frame* not_closure,
		std::shared_ptr<id_list> outer_ids);
	~ClosureFrame() override;
	Val* GetElement(ID* id) const override;
	void SetElement(ID* id, Val* v) override;
	Frame* Clone() override;

private:
	Frame* closure;

	struct const_char_hasher {
		size_t operator()(const char* in) const
			{
			// http://www.cse.yorku.ca/~oz/hash.html
			size_t h = 5381;
			int c;

			while ((c = *in++))
				h = ((h << 5) + h) + c;

			return h;
			}
	};

	// NOTE: In a perfect world this would be better done with a trie or bloom
	// filter. We only need to check if things are NOT in the closure.
	std::unordered_set<const char*, const_char_hasher> closure_elements;
};

extern vector<Frame*> g_frame_stack;

#endif
