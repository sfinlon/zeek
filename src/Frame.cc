// See the file "COPYING" in the main distribution directory for copyright.

#include "zeek-config.h"

#include "Frame.h"
#include "Stmt.h"
#include "Func.h"
#include "Trigger.h"

#include <string>

vector<Frame*> g_frame_stack;

Frame::Frame(int arg_size, const BroFunc* func, const val_list* fn_args)
	{
	size = arg_size;
	frame = new Val*[size];
	function = func;
	func_args = fn_args;

	next_stmt = 0;
	break_before_next_stmt = false;
	break_on_return = false;

	trigger = 0;
	call = 0;
	delayed = false;

	Clear();
	}

// Performs a shallow copy of the input frame. Does not copy elements.
// TODO: need a Ref in here?
Frame::Frame(const Frame* other)
	{
	this->size = other->size;
	this->frame = other->frame;
	this->function = other->function;
	this->func_args = other->func_args;

	this->next_stmt = 0;
	this->break_before_next_stmt = false;
	this->break_on_return = false;
	this->delayed = false;

	if ( other->trigger )
		Ref(other->trigger);

	this->trigger = other->trigger;
	this->call = other->call;
	}

Frame::~Frame()
	{
	Unref(trigger);
	Release();
	}

Val* Frame::GetElement(ID* id) const
	{
	return this->frame[id->Offset()];
	}

void Frame::AddElement(ID* id, Val* v)
	{
	this->SetElement(id, v);
	}

void Frame::Reset(int startIdx)
	{
	for ( int i = startIdx; i < size; ++i )
		{
		Unref(frame[i]);
		frame[i] = 0;
		}
	}

void Frame::Release()
	{
	for ( int i = 0; i < size; ++i )
		Unref(frame[i]);

	delete [] frame;
	}

void Frame::Describe(ODesc* d) const
	{
	if ( ! d->IsBinary() )
		d->AddSP("frame");

	if ( ! d->IsReadable() )
		{
		d->Add(size);

		for ( int i = 0; i < size; ++i )
			 {
			 d->Add(frame[i] != 0);
			 d->SP();
			 }
		}

	for ( int i = 0; i < size; ++i )
		if ( frame[i] )
			frame[i]->Describe(d);
		else if ( d->IsReadable() )
			d->Add("<nil>");
	}

void Frame::Clear()
	{
	for ( int i = 0; i < size; ++i )
		frame[i] = 0;
	}

Frame* Frame::Clone()
	{
	Frame* f = new Frame(size, function, func_args);

	for ( int i = 0; i < size; ++i )
		f->frame[i] = frame[i] ? frame[i]->Clone() : 0;

	if ( trigger )
		Ref(trigger);
	f->trigger = trigger;
	f->call = call;

	return f;
	}

Frame* Frame::SelectiveClone(id_list* selection)
	{
	Frame* f = new Frame(size, function, func_args);

	loop_over_list(*selection, i)
		{
		ID* id = (*selection)[i];
		f->frame[id->Offset()] =
			frame[id->Offset()] ? frame[id->Offset()]->Clone() : 0;
		}

		if ( trigger )
			Ref(trigger);
		f->trigger = trigger;
		f->call = call;

		return f;
	}

void Frame::SetTrigger(Trigger* arg_trigger)
	{
	ClearTrigger();

	if ( arg_trigger )
		Ref(arg_trigger);

	trigger = arg_trigger;
	}

void Frame::ClearTrigger()
	{
	Unref(trigger);
	trigger = 0;
	}

// A closure frame is just a regular frame with a differnet GetElement op.
ClosureFrame::ClosureFrame(Frame* closure, Frame* not_closure,
	std::shared_ptr<id_list> outer_ids)
: Frame(not_closure)
	{
	assert(closure);

	this->closure = closure;
	Ref(this->closure);

	if (outer_ids)
		{
		// Install the closure IDs
		id_list* tmp = outer_ids.get();
		loop_over_list(*tmp, i)
			{
			ID* id = (*tmp)[i];
			if (id)
				this->closure_elements.insert(id->Name());
			}
		}
	}

ClosureFrame::~ClosureFrame()
	{
	Unref(this->closure);
	}

Val* ClosureFrame::GetElement(ID* id) const
	{
	if (this->closure_elements.find(id->Name()) == this->closure_elements.end())
		return Frame::GetElement(id);
	return this->closure->GetElement(id);
	}

void ClosureFrame::SetElement(ID* id, Val* v)
	{
	if (this->closure_elements.find(id->Name()) == this->closure_elements.end())
		return Frame::SetElement(id->Offset(), v);
	return this->closure->SetElement(id, v);
	}

Frame* ClosureFrame::Clone()
	{
	Frame* new_closure = this->closure->Clone();
	Frame* new_regular = Frame::Clone();

	ClosureFrame* cf =  new ClosureFrame(new_closure, new_regular, nullptr);
	cf->closure_elements = this->closure_elements;
	return cf;
	}
